# Phase 3: Combat Infrastructure — Index

**Status:** Planning — not started. Jun 2026.

---

## Overview

Phase 3 provides the foundational async combat machinery that all multi-projectile and
concurrent-explosion features depend on. None of these are user-visible standalone features;
they are the underpinnings that later phases (burst fire, shotgun pellets, dual-fire,
advanced damage) hang off.

Individual feature docs (created as each item is designed/started):

| Item | Design doc | Status |
|------|------------|--------|
| Async Projectile System | [Feature-AsyncProjectileSystem.md](Feature-AsyncProjectileSystem.md) | Planning |
| Async Explosion System | — | Not started |
| Advanced Damage Models | — | Not started |

| Area | Present in OXCE-Plus | Notes |
|------|----------------------|-------|
| Multi-explosion visuals | ✓ | `Map::_explosions` is already a `std::list<Explosion*>`; multiple animated sprites coexist |
| Projectile trajectory pre-calc | ✓ | `Projectile::calculateTrajectory()` pre-computes all voxels before flight begins |
| Shotgun extra pellets (computation) | ✓ | `ProjectileFlyBState::think()` computes extra pellet trajectories for `getShotgunPellets()` |
| Shotgun extra pellets (rendering) | ✗ | Pellets call `skipTrajectory()` immediately — never rendered in flight; only impact VFX shown |
| Range-based power reduction | ✓ | `RuleItem::getPowerRangeReduction(range)` — linear range penalty on the item rule |
| Blast-radius-internal falloff | ✗ | `TileEngine::explode()` applies no distance weighting within the radius; all tiles hit at full blast-center power minus range reduction |
| Armor degradation | ✗ | Armor values are fixed per unit (`currentArmor[]` initialised from `Armor` rule and never written back after hits) |
| Game singleton accessor | ✗ | `Game*` is passed down the call stack; no `Game::getGame()` static accessor exists |

Conclusion: OXCE-Plus has useful building blocks (the `_explosions` list, pre-computed
trajectories, pellet hit calculation) but lacks the async flight model, the blast-radius
falloff, armor wear, and the static accessor. All four items below are genuine DX deltas.

---

## Deliverable 1 — Async Projectile System

### Problem

`Map` owns a single `Projectile* _projectile`. Only one projectile can be in-flight at any
time. `ProjectileFlyBState::think()` moves it one step, then stops. This blocks:

- Showing shotgun pellets flying simultaneously (they currently skip to endpoint).
- Burst fire where rounds are visually staggered but overlapping in flight.
- Dual-fire weapons firing from two hands at the same time.

### Design

**Replace the single slot with a collection.**

In `Map.h`, change:
```cpp
Projectile *_projectile;            // current
bool _followProjectile;
bool _projectileInFOV;
```
to:
```cpp
std::vector<Projectile*> _projectiles;   // NEW: all in-flight projectiles
bool _followProjectile;
bool _projectileInFOV;
```

Public API:
```cpp
// Replaces setProjectile / getProjectile
void addProjectile(Projectile* p);
void removeProjectile(Projectile* p);    // deletes p, removes from vector
const std::vector<Projectile*>& getProjectiles() const;
bool hasProjectiles() const;

// Backward-compat shim for callers that only deal with one projectile.
// Returns _projectiles[0] or nullptr if empty.
Projectile* getProjectile() const;       // KEEP, delegates to [0]
void setProjectile(Projectile* p);       // KEEP, clears vector then calls addProjectile(p)
```

This lets all existing single-projectile callers continue to compile unmodified during
migration. The shims can be removed once all call sites are updated.

**`Map::drawTerrain` rendering update:**

The bullet-drawing loop currently checks `if (_projectile != 0)`. Change it to iterate
`_projectiles`:

```cpp
for (Projectile* proj : _projectiles)
{
    // draw each projectile sprite along its trajectory segment
}
```

Camera-following: the camera currently follows `_projectile`. With multiple projectiles,
follow the first one registered (index 0). If `_followProjectile` is true and `_projectiles`
is non-empty, center on `_projectiles[0]->getPosition()`.

**`ProjectileFlyBState` redesign:**

The state currently manages one projectile at a time. Redesign to manage N:

- `createNewProjectile()` stays mostly intact for single shots. For shotgun/burst modes
  (see Phase 5), it will be called multiple times or in a loop before the `think()` cycle
  begins — each call does `addProjectile()`.
- `think()` rewrites its projectile-stepping loop:

```cpp
// For each in-flight projectile owned by this state, advance one step.
// Collect impacts. After moving all, resolve impacts in order.
std::vector<ProjectileImpact> impacts;
for (auto* proj : _parent->getMap()->getProjectiles())
{
    // only advance projectiles this state owns (by a state-owned set)
    if (!_ownedProjectiles.count(proj)) continue;

    if (!proj->move())
    {
        impacts.push_back({ proj, proj->getLastPositions(...) });
    }
}
// Remove impacted projectiles from the map.
for (auto& imp : impacts)
    _parent->getMap()->removeProjectile(imp.proj);

// Resolve each impact (push ExplosionBState, etc.).
for (auto& imp : impacts)
    resolveImpact(imp);

// If no more owned projectiles remain and no more shots to fire → popState.
```

`_ownedProjectiles` is a `std::unordered_set<Projectile*>` added to
`ProjectileFlyBState` so the state only advances its own projectiles. This is important
once dual-fire (two simultaneous `ProjectileFlyBState` instances) is supported.

**Shotgun visual pellets (Phase 5 prereq wired here):**

Currently pellets call `proj->skipTrajectory()` immediately. Change: if
`Options::battleShowShotgunPellets` is true (new DX option, default true), add each pellet
to the map without skipping. Each pellet is a member of `_ownedProjectiles` and advances
in the same `think()` tick as the first pellet. When pellet N hits, its impact is resolved
in the same pass.

For the action-menu revamp's "show pellet count" this is purely display; the underlying
`getShotgunPellets()` already exists.

### Files changed

| File | Change |
|------|--------|
| `src/Battlescape/Map.h` | Replace `_projectile` with `_projectiles`; add `addProjectile`/`removeProjectile`/`getProjectiles`/`hasProjectiles`; keep shim getters |
| `src/Battlescape/Map.cpp` | Update `drawTerrain` bullet loop; update camera follow; update `isBlocked` / any other projectile checks |
| `src/Battlescape/ProjectileFlyBState.h` | Add `_ownedProjectiles` set |
| `src/Battlescape/ProjectileFlyBState.cpp` | Rewrite `think()` multi-step loop; update `createNewProjectile`; update `cancel()` |
| `src/Battlescape/BattlescapeState.cpp` | Any `getProjectile()` calls that test null → use `hasProjectiles()` or `getProjectile()` shim |

### Backward-compatibility notes

- Existing behavior for single shots: one projectile enters `_projectiles`, behavior
  identical to today.
- Waypoint-guided missiles: still one projectile; `nextWaypoint` sequencing unchanged.
- `cancel()` / `skipTrajectory()`: iterates `_ownedProjectiles` and calls
  `skipTrajectory()` on each.

---

## Deliverable 2 — Async Explosion System

### Problem

`ExplosionBState` is sequential in the `_states` queue. When burst fire's three rounds each
hit different targets, `ExplosionBState` for round 1 plays fully, then round 2, then round
3. This means:

- Each explosion plays its camera-center, blast sound, and damage check serially.
- The player sees explosions back-to-back instead of simultaneously.
- Chain explosions from terrain already handle their own `statePushFront` cascade —
  that is fine and should not change.

The visual layer (`Map::_explosions`) is already async — all sprites in the list animate
together per frame. The bottleneck is the *state machine* sequencing.

### Design

**Option chosen: explosion grouping.**

Rather than restructuring `handleState()` (invasive, high risk), batch simultaneous
explosions that result from the same parent action into a single `ExplosionBState`. The
batched state owns a list of `(center, power, damageType, radius, ...)` descriptors, pushes
all their visual `Explosion` sprites at once, and runs one shared animation wait. Once the
shared animation ends, `explode()` loops over all descriptors and calls
`TileEngine::hit()` / `TileEngine::explode()` for each in sequence.

Introduce `struct ExplosionDescriptor` (internal to `ExplosionBState.h`):
```cpp
struct ExplosionDescriptor
{
    LastPositions center;
    BattleActionAttack attack;
    Tile* tile;
    bool lowerWeapon;
    int range;
};
```

`ExplosionBState` gains:
```cpp
std::vector<ExplosionDescriptor> _group;
```

When `ProjectileFlyBState::think()` collects multiple impacts in a single frame, instead of
pushing N separate `ExplosionBState` instances with `statePushFront`, it builds one
`ExplosionBState` with all descriptors loaded into `_group`. The single state plays all
their visual animations in parallel (they're all added to `Map::_explosions` in `init()`).

`init()` iterates `_group`, spawns visual Explosion sprites for each descriptor, plays the
loudest sound, and centers the camera on the centroid of all impact positions.

`explode()` iterates `_group` and calls `TileEngine::hit()` / `TileEngine::explode()` per
descriptor. Casualties, terrain chain explosions, etc., remain per-descriptor.

**Single-descriptor path:** the existing constructor is kept for all current callers
(terrain chain explosions, grenade blasts, single-shot impacts). It loads one descriptor
into `_group`. No behavioral change.

**Per-group timer:** `ExplosionBState` already drives animation via the shared
`Map::_explosions` list and polls `getExplosions()->empty()`. The group approach doesn't
need per-group private timers because all sprites are in the same list and animate together.

### Files changed

| File | Change |
|------|--------|
| `src/Battlescape/ExplosionBState.h` | Add `ExplosionDescriptor`, `_group` member; add multi-descriptor constructor |
| `src/Battlescape/ExplosionBState.cpp` | Update `init()` to spawn all sprites; update `explode()` to loop descriptors |
| `src/Battlescape/ProjectileFlyBState.cpp` | Impact collection: build `ExplosionBState` with `_group` when multiple impacts in same frame |

---

## Deliverable 3 — Advanced Damage Models

### 3a. Armor Degradation

**Motivation:** Sustained fire wears down armor over a battle, rewarding focus fire and making
late-battle units more vulnerable.

**Ruleset field** (`Armor` YAML):
```yaml
degradationRate: 0.05   # fraction of net damage applied to armor value; 0 = off (default)
```
`Armor` gains `float _degradationRate` (default 0.0, load/save).

**Mechanic:** After `TileEngine::hit()` calculates net damage (post-armor), reduce the
unit's `currentArmor` on the hit side by `netDamage * degradationRate` (floored at 0, never
below 0). This is already stored per-side as `int _currentArmor[SIDE_MAX]`.

```cpp
// In TileEngine::hit(), after net damage is calculated:
int degradation = (int)(netDamage * itemRule->getArmorDegradationRate());
// or from weapon rule / armor rule depending on which degrades
unit->reduceArmor(side, degradation);
```

`BattleUnit::reduceArmor(UnitSide side, int amount)` is a new helper (clamp to 0).

**Save compatibility:** `_currentArmor` is already serialized in `BattleUnit::save()`. The
degradation happens to these existing values, so old saves load cleanly (full armor until
first hit).

### 3b. `blastDropoff` Falloff Within AoE Radius

**Motivation:** Currently, all tiles within an AoE explosion take identical damage
(full power minus range-reduction). Explosions feel like binary circles. `blastDropoff`
makes the center of an explosion deadlier than the periphery.

**Ruleset field** (`RuleItem` YAML):
```yaml
blastDropoff: 1.0   # multiplier per tile of distance from center; 1.0 = linear (default); 0 = flat (vanilla)
```

Range: 0.0–1.0. At 1.0, damage at distance D tiles = `power * (1 - D/radius)` (linear
falloff, center=100%, edge=0%). At 0.0, no falloff (current behavior, backward compat
default). Values in between interpolate.

**Implementation:** In `TileEngine::explode()`, when computing power for each tile in the
blast:
```cpp
// existing: power -= item->getPowerRangeReduction(range_to_tile);
// add: if blastDropoff > 0
float dropoff = item->getBlastDropoff();
if (dropoff > 0.0f)
{
    float d = distanceTileToCenter;
    float r = (float)radius;
    power = (int)(power * (1.0f - dropoff * (d / r)));
    power = std::max(0, power);
}
```

`RuleItem` gains `float _blastDropoff` (default 0.0 = vanilla behavior), with load/save.

### 3c. Visual and Sound Scaling by Blast Radius

**Motivation:** The current explosion VFX count scales by `powerForAnimation / 5` —
indirectly correlated to radius, but not driven by radius directly. High-radius, low-power
weapons (stun bombs, smoke, incendiary spreads) can look anemic. High-power, small-radius
weapons (shaped charges) can look over-the-top.

**Ruleset field** (`RuleItem` YAML):
```yaml
powerForAnimation: -1   # already exists; -1 = derive from power
# NEW:
radiusForAnimation: -1  # if set, overrides the radius used to scale VFX count; -1 = use explosion radius
```

`RuleItem` gains `int _radiusForAnimation` (default -1). In `ExplosionBState::init()`,
use `radiusForAnimation` (if ≥ 0) in place of `powerForAnimation / 5` when computing
explosion sprite count, so modders can tune the visual intensity independently from power.

Sound: add `explosionSound` and `explosionSoundBig` to `RuleItem` YAML (with `Mod::SMALL_EXPLOSION`
and `Mod::LARGE_EXPLOSION` as defaults). `ExplosionBState` plays `explosionSoundBig` when
`powerForAnimation > 80`, `explosionSound` otherwise — modders can override per item.

---

## Deliverable 4 — Engine Plumbing: `Game::getGame()`

### Motivation

The Phase 8 Effects framework needs to call into `Game` (to look up sounds, surfaces, and
the `Mod`) from contexts that don't hold a `BattlescapeGame*` — for example from
`BattleUnit` or `Tile` callbacks triggered deep in `TileEngine`. The current call chain is:

```
BattlescapeGame → getBattleState() → getGame()
```

That chain is available inside `BattleState` subclasses but not in `TileEngine`,
`BattleUnit`, or the future `BattleEffect` objects.

### Design

Add a static singleton accessor to `Game`:

```cpp
// Game.h
static Game* getGame() { return _instance; }

// Game.cpp
Game* Game::_instance = nullptr;

// Game constructor
Game::Game(const std::string& title)
{
    _instance = this;
    // ... existing init ...
}
```

`Game` is a singleton in practice (one instance in `main()`). The static accessor makes
this explicit and removes the need to thread `Game*` through every call stack.

**Safety:** assert in the accessor in debug builds that `_instance != nullptr`.

**Files changed:**

| File | Change |
|------|--------|
| `src/Engine/Game.h` | Add `static Game* _instance;` declaration; add `static Game* getGame()` |
| `src/Engine/Game.cpp` | Define `Game* Game::_instance = nullptr;`; assign `_instance = this` in constructor |

---

## Implementation Order

```
1. Game::getGame()                 (5 min, trivial, unblocks Phase 8 planning)
2. Map projectile collection       (core change; most effort)
3. ProjectileFlyBState multi-proj  (depends on Map change)
4. ExplosionBState grouping        (depends on ProjectileFlyBState multi-proj)
5. Armor degradation               (independent; small)
6. blastDropoff                    (independent; small)
7. radiusForAnimation / sound      (independent; cosmetic)
```

Items 5–7 are independent of 1–4 and can be done in any order or in parallel.

---

## Risks & Open Questions

| # | Risk | Mitigation |
|---|------|-----------|
| 1 | `Map::getProjectile()` shim might mask bugs where callers assume single-projectile | Audit all call sites before removing shims; use `assert(_projectiles.size() <= 1)` in the shim until multi-proj is fully exercised |
| 2 | Camera-follow with multiple projectiles may look chaotic | Follow projectile[0] (first to land); add option `cameraFollowClosestProjectile` if needed |
| 3 | Impact ordering non-determinism if pellets land in the same frame | Sort impacts by `Projectile*` address (stable FIFO) |
| 4 | `ExplosionBState` grouping changes single-explosion behavior | Default path = single descriptor in `_group`; behavior identical to today |
| 5 | Armor degradation balance | `degradationRate: 0.0` default leaves existing mods/saves unchanged |
| 6 | `blastDropoff` interaction with `getPowerRangeReduction` | Apply range reduction first, then dropoff; document order in YAML spec comment |
