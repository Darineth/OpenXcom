# Feature - Async Projectile System

**Status:** Done — Jun 2026. See **Implementation updates (post-plan)** below for changes that
diverged from the original infrastructure plan (shim removal, camera centroid, per-projectile
impact, timer-based firing, shotgun pellets flying as real projectiles).

---

## Motivation

`Map` owns a single `Projectile* _projectile`. Only one projectile can be in-flight at any
time. `ProjectileFlyBState::think()` moves it one step per tick, then stops. This hard
blocks several planned combat features:

- **Shotgun pellet flight** — pellets currently call `skipTrajectory()` immediately and are
  never rendered in-flight; only the impact VFX shows.
- **Burst fire with visual overlap** — rounds fire sequentially; you can't see two rounds in
  the air at once.
- **Dual-fire** — two simultaneous independent trajectories from the same unit.

This feature replaces the single-pointer slot with a vector and updates all related code so
that multiple projectiles can fly simultaneously. **No new firing modes are added here** —
that comes in Phase 5. This is pure infrastructure.

---

## Audit: OXCE-Plus baseline

| Area | Status | Notes |
|------|--------|-------|
| Single `Projectile*` slot on `Map` | Present | Blocks concurrent flight |
| `Projectile::calculateTrajectory()` pre-computes full path | Present | No change needed |
| Shotgun extra-pellet hit computation | Present | `getShotgunPellets()` loop in `ProjectileFlyBState::think()` |
| Shotgun pellet rendering | **Absent** | Each extra pellet calls `skipTrajectory()` immediately |
| Multi-projectile rendering loop in `Map::drawTerrain` | **Absent** | Single `_projectile` check |

---

## Design

### 1. `Map` — replace the single slot with a vector

**`Map.h` changes:**

```cpp
// Remove:
Projectile *_projectile;

// Add:
std::vector<Projectile*> _projectiles;
```

`_projectileInFOV` and `_followProjectile` are unchanged.

**New public API (`Map.h`):**

```cpp
void addProjectile(Projectile* p);
void removeProjectile(Projectile* p);   // removes from vector AND deletes p
bool hasProjectiles() const;
const std::vector<Projectile*>& getProjectiles() const;
```

**Backward-compat shims (keep in `Map.h`/`Map.cpp`, mark `[[deprecated]]` to find call
sites over time):**

```cpp
// Returns _projectiles[0] or nullptr.
Projectile* getProjectile() const;

// Clears _projectiles (deleting any existing), then calls addProjectile(p).
// Passing nullptr is equivalent to clearing the collection (existing callers use
// setProjectile(0) to signal "done").
void setProjectile(Projectile* p);
```

**`Map.cpp` — `setProjectile` implementation:**

```cpp
void Map::setProjectile(Projectile* projectile)
{
    for (Projectile* p : _projectiles) delete p;
    _projectiles.clear();
    if (projectile)
    {
        _projectiles.push_back(projectile);
        if (Options::battleSmoothCamera) _launch = true;
    }
}
```

**`Map.cpp` — `addProjectile` implementation:**

```cpp
void Map::addProjectile(Projectile* p)
{
    _projectiles.push_back(p);
    if (Options::battleSmoothCamera) _launch = true;
}

void Map::removeProjectile(Projectile* p)
{
    _projectiles.erase(std::find(_projectiles.begin(), _projectiles.end(), p));
    delete p;
}
```

---

### 2. `Map::drawTerrain` — bounding box and rendering

There are two separate roles `_projectile` plays in `drawTerrain` today:

**a) Bounding-box computation and camera follow** (~line 1005–1100)

Currently:
```cpp
if (_projectile && _explosions.empty())
{
    int part = _projectile->getItem() ? 0 : BULLET_SPRITES-1;
    for (int i = 0; i <= part; ++i)
    {
        // update bulletLow*/bulletHigh* from _projectile->getPosition(1-i)
    }
    // camera follow using _projectile->getPosition()
}
```

Change to iterate `_projectiles`. The bounding box expands to cover **all** in-flight
projectiles. Camera follow stays on `_projectiles[0]` (the first one registered):

```cpp
if (!_projectiles.empty() && _explosions.empty())
{
    for (Projectile* proj : _projectiles)
    {
        int part = proj->getItem() ? 0 : BULLET_SPRITES-1;
        for (int i = 0; i <= part; ++i)
        {
            // update bulletLow*/bulletHigh* from proj->getPosition(1-i)
        }
    }
    // camera follow: use _projectiles[0]->getPosition()
    _camera->convertVoxelToScreen(_projectiles[0]->getPosition(), &bulletPositionScreen);
    // ... smooth/jump camera logic unchanged, just referencing _projectiles[0] ...
}
```

**b) `_projectileInFOV` update** (~line 574–580)

Currently checks `if (_projectile)` and tests one tile. Change to check any projectile in
`_projectiles`:

```cpp
_projectileInFOV = _save->getDebugMode();
for (Projectile* proj : _projectiles)
{
    Tile* t = _save->getTile(proj->getPosition(0).toTile());
    if (t && t->isDiscovered(O_FLOOR))
    {
        _projectileInFOV = true;
        break;
    }
}
```

**c) Per-tile bullet sprite rendering** (~line 1389–1480)

Currently:
```cpp
if (_projectile && _projectileInFOV)
{
    // draw _projectile sprite on this tile
}
```

Change to iterate all projectiles:

```cpp
if (_projectileInFOV)
{
    for (Projectile* proj : _projectiles)
    {
        BattleItem* item = proj->getItem();
        if (item)
        {
            // thrown item: draw shadow + item sprite (same logic, just using proj)
        }
        else
        {
            // bullet: check bounds and draw particles (same logic, using proj)
        }
    }
}
```

The `bulletLow*/bulletHigh*` range check in the bullet branch still works because those
bounds now cover all projectiles. Each projectile's particles are only drawn when their
individual voxel positions fall on the current tile (`itX/itY/itZ`), so there's no
cross-contamination between projectiles.

---

### 3. `ProjectileFlyBState` — multi-projectile think loop

The current `think()` structure:

```
if (no projectile on map):
    if should fire another → createNewProjectile()
    else → popState()
else:
    move projectile one step
    if it hit → push ExplosionBState, delete projectile
```

The rewrite keeps the same skeleton but operates on the collection:

```cpp
void ProjectileFlyBState::think()
{
    _parent->getSave()->getBattleState()->clearMouseScrollingState();

    if (!_parent->getMap()->hasProjectiles())
    {
        // Nothing in flight — same "fire next or finish" logic as today.
        bool hasFloor = _action.actor->haveNoFloorBelow() == false;
        bool unitCanFly = _action.actor->getMovementType() == MT_FLY;

        if (_action.weapon->haveNextShotsForAction(_action.type, _action.autoShotCounter)
            && !_action.actor->isOut()
            && _ammo->getAmmoQuantity() != 0
            && (hasFloor || unitCanFly))
        {
            createNewProjectile();   // adds to Map via addProjectile()
            // ... cameraPosition restore, same as today ...
        }
        else
        {
            // ... reaction fire check, abortTurn, setupCursor, convertInfected, popState
            // — unchanged from today.
        }
    }
    else
    {
        // Advance all in-flight projectiles one step. Collect impacts.
        struct Impact { Projectile* proj; int impactType; };
        std::vector<Impact> impacts;

        // Snapshot the list first: removeProjectile() mutates _projectiles.
        std::vector<Projectile*> snapshot(_parent->getMap()->getProjectiles());
        for (Projectile* proj : snapshot)
        {
            BattleActionAttack attack = BattleActionAttack::GetAferShoot(_action, _ammo);

            // Shotgun: skip to endpoint immediately (existing behavior for extra pellets).
            // Phase 5 will change this to animate them; for now, preserve current behavior.
            if (_action.type != BA_THROW && _ammo && _ammo->getRules()->getShotgunPellets() != 0)
                proj->skipTrajectory();

            if (!proj->move())
                impacts.push_back({ proj, _projectileImpact });
        }

        // Resolve each impact in order.
        for (auto& imp : impacts)
        {
            // Capture data needed for resolution BEFORE removing the projectile.
            resolveImpact(imp.proj);
            _parent->getMap()->removeProjectile(imp.proj);   // deletes imp.proj
        }
    }
}
```

**`resolveImpact(Projectile* proj)`** is a new private helper that contains the existing
impact-resolution logic extracted from `think()`. It receives the in-flight `Projectile*`,
reads the data it needs (`getPosition`, `getLastPositions`, `getDistance`), then does the
same ExplosionBState push / item drop / waypoint cascade as today. Because the projectile is
still alive when `resolveImpact` runs (we only call `removeProjectile` after), all existing
accesses are valid.

> **Note on `_projectileImpact`:** `_projectileImpact` is currently set inside
> `createNewProjectile()` (via `calculateTrajectory` return value) and read in `think()`.
> With multiple projectiles, each has its own impact classification. For Phase 3 (single
> projectile at a time), `_projectileImpact` remains on the state and is set by the single
> `createNewProjectile()` call — no change. For Phase 5 (multi-pellet), we will need a
> per-projectile impact type, likely stored in a `std::unordered_map<Projectile*, int>` on
> the state.

**`cancel()`** — currently calls `skipTrajectory()` on `getProjectile()`. Change to
iterate `getProjectiles()`:

```cpp
void ProjectileFlyBState::cancel()
{
    for (Projectile* proj : _parent->getMap()->getProjectiles())
        proj->skipTrajectory();
    // ... existing camera re-center and auto-end check ...
}
```

---

### 4. Other callers of `getProjectile()`

A full-codebase grep for `getProjectile()` shows all call sites are in
`ProjectileFlyBState.cpp` and `Map.cpp` (handled above). The shim keeps them compiling;
no changes needed in `BattlescapeState.cpp` or elsewhere.

---

## Files changed

| File | Change summary |
|------|----------------|
| `src/Battlescape/Map.h` | Remove `_projectile`; add `_projectiles`; add `addProjectile`/`removeProjectile`/`hasProjectiles`/`getProjectiles`; keep `getProjectile()`/`setProjectile()` shims |
| `src/Battlescape/Map.cpp` | Implement new API; update bounding-box block; update `_projectileInFOV` check; update per-tile bullet rendering loop; update constructor init |
| `src/Battlescape/ProjectileFlyBState.h` | Declare `resolveImpact(Projectile*)` |
| `src/Battlescape/ProjectileFlyBState.cpp` | Extract `resolveImpact()`; rewrite `think()` body; update `cancel()` |

---

## Behavioral contract: no change for single shots

The shims and the single-entry vector mean every existing code path (single shot, autoshot
sequence, waypoint missiles, thrown items, shotgun) produces identical behavior to today.
The only structural difference is that `_projectile` is now `_projectiles[0]`, and the
impact resolution goes through the new `resolveImpact()` helper rather than inline code.

---

## Risks

| # | Risk | Mitigation |
|---|------|-----------|
| 1 | Iterator invalidation if `removeProjectile` is called while iterating | Snapshot the vector before the advance loop (see design above) |
| 2 | `setProjectile(nullptr)` clearing behavior changes | Implement shim to clear + delete existing; tested by all single-shot paths |
| 3 | Camera follow jitter with multiple simultaneous projectiles (future) | Follow `_projectiles[0]`; revisit in Phase 5 when pellet flight is enabled |
| 4 | `_projectileImpact` is per-state, not per-projectile | Documented limitation; Phase 5 will add per-projectile map |

---

## Implementation updates (post-plan)

The infrastructure shipped as planned, then evolved further during the same work. The final
state differs from the plan above in these ways:

### Shims removed

The `getProjectile()`/`setProjectile()` backward-compat shims were deleted once all call sites
were migrated. The live API is `addProjectile`, `removeProjectile` (removes + deletes),
`hasProjectiles()`, and `getProjectiles()`. `BattlescapeState.cpp`'s two former
`getProjectile()` null-checks now use `!_map->hasProjectiles()`.

### Camera follows the centroid

Instead of following `_projectiles[0]`, `Map::drawTerrain()` now accumulates every projectile's
voxel position and follows the average (centroid) of all visible bullets, so the view stays
centered on the swarm rather than snapping to whichever round happened to register first.

### Per-projectile impact type

`_projectileImpact` on the state was insufficient once multiple rounds can be airborne at once
(each resolves at a different time against different geometry). `Projectile` now stores its own
`int _impact` (`setImpact()`/`getImpact()`), set at fire time in `createNewProjectile()` and read
back per-projectile in the `think()` advance loop. This replaces the "Phase 5 will add a
per-projectile map" mitigation for Risk #4.

### Timer-based firing (replaces impact-gated firing)

The original `think()` only fired the next shot once the map had **no** projectiles. Multi-shot
actions now fire on a timer instead, so several rounds can overlap in flight:

- New RuleItem attribute **`fireInterval`** (milliseconds, default `150`) controls the cadence
  between consecutive burst/spray shots. It is converted to think-cycles via
  `(fireInterval * 60 + 500) / 1000` (the state ticks at ~60 Hz), clamped to a minimum of 1.
- `ProjectileFlyBState` gained an `int _shotCooldown` counter. `createNewProjectile()` resets it
  to the per-weapon interval; `think()` decrements it each cycle and only launches the next shot
  when it reaches zero (and the action still has shots/ammo and the firer has footing).
- `think()` advances **all** in-flight projectiles each cycle (iterating a snapshot copy because
  `removeProjectile()` mutates the live vector), resolving each impact with that projectile's own
  `getImpact()`.
- BA_LAUNCH (blaster waypoint cascade) and BA_THROW still yield a single shot, so the timer only
  affects genuine multi-shot actions (auto/spray); the blaster cascade continues to chain via
  `statePushNext` in the impact block.

### Shotgun pellets fly as real projectiles

The old shotgun handling fired one projectile, then at impact synchronously **traced** the
remaining pellets (`skipTrajectory()` to their endpoints) and applied their hits in a single
burst inside `think()`. That whole block is gone. Now:

- `createNewProjectile()` launches the lead pellet as before, then immediately spawns the
  remaining `shotgunPellets - 1` pellets as real `Projectile` objects, each with its own spread
  trajectory, and `addProjectile()`s them so they fly concurrently. The spread math is unchanged:
  `shotgunBehaviorType == 1` uses `(1 - spread/100) * choke/100` and re-centers the spread on the
  lead pellet's actual impact voxel; otherwise it uses the diminishing
  `firingAccuracy/100 - i*5*spread/100` formula. Pellets whose trajectory returns `V_EMPTY` (no
  line of fire) are discarded.
- To re-center behaviorType-1 spread at fire time, `Projectile` gained
  `getImpactPosition(int offset)` which reads the endpoint of the just-computed trajectory
  (`getPositionFromEnd(_trajectory, offset)`), replacing the old impact-time `getPosition(-2)`.
- The `think()` advance loop no longer special-cases shotguns with `skipTrajectory()`; every
  pellet flies and resolves its own impact through the normal explosion/hit path using its own
  travelled distance for range-based power falloff (the former `shotgun ? 0 : ...` range special
  case is removed).
- The `rememberXP()`/`nerfXP()` per-shot experience cap was removed. It only existed to bound the
  experience from the old synchronous extra-pellet burst; with each pellet now a regular
  projectile resolving asynchronously, a landed pellet awards firing experience like any other
  hit.

### Lazy impact recalculation against live terrain

Each projectile's full trajectory **and** impact voxel are computed once at fire time. With
several rounds airborne at once, an earlier impact can destroy the obstacle a later round was
going to hit, so that later round would wrongly detonate on a wall that no longer exists. To fix
this:

- `Projectile::recalculateImpact()` re-runs the deterministic voxel line trace from the original
  origin toward the (already accuracy-deviated) `_targetVoxel` against the **current** map. Because
  the trace is deterministic, the already-travelled prefix is identical; only the far end can
  change. If the fresh path reaches no further than the round already is, the obstruction still
  stands and the precomputed impact is kept. If the path now extends past the old impact point
  (the obstacle was removed), it adopts the longer trajectory, updates `_impact`/`_distanceMax`,
  and reports that the round should keep flying.
- In `ProjectileFlyBState::think()`, when a round reaches the end of its path it calls
  `recalculateImpact()` first (straight shots only — skipped for `BA_THROW` and arcing/parabola
  shots, which don't use the straight-line tracer). If the path extended, it `continue`s and the
  round resolves on a later pass instead of exploding on a now-gone wall. This also naturally
  handles a target unit killed by an earlier round in the same volley (the corpse no longer
  blocks the tile).

### Concurrent (non-blocking) impact explosions

Previously every impact pushed an `ExplosionBState` to the **front** of the BattlescapeGame state
queue, so the whole battle (including every other round still in flight) froze for the duration of
that explosion's animation. With overlapping rounds this made the volley stutter on every hit. Now
firearm/AoE impacts run their explosions **concurrently** alongside the still-firing volley:

- `BattlescapeGame` gained a second list, `_concurrentStates`, ticked every `handleState()` cycle
  *in addition to* the main queue front (over a snapshot, since a concurrent state may remove
  itself or enqueue follow-ups). New API: `statePushConcurrent()` (push + `init()` now),
  `popConcurrentState()` (deinit + remove + deferred-delete), and `hasConcurrentStates()`.
  `isBusy()` and the `think()` idle/turn-end guard now also account for concurrent states so the
  AI/turn end waits for explosions to finish, and the destructor frees them.
- Concurrency is a property of *how* a state is enqueued, so it lives on the `BattleState` base
  (`_concurrent` + `setConcurrent()`/`isConcurrent()`) and is set by `statePushConcurrent()` itself
  — callers never toggle it separately. The base also provides `finishState()`, which routes a
  state's completion to `popConcurrentState(this)` when concurrent and `popState()` otherwise, so
  any future BState can opt into concurrent execution without bespoke wiring.
- `ExplosionBState` reads that base flag. A concurrent explosion:
  - owns its sprites (`_myExplosions`) instead of iterating the shared `Map` explosion list, so
    several explosions can animate at once without disturbing each other (the non-concurrent path
    uses the same per-state ownership, identical behavior when only one explosion exists);
  - paces its own frames by **wall clock** (`SDL_GetTicks()` vs `_animInterval`, the intended
    explosion speed) while the game timer runs at the fast projectile cadence (1000/60), so it
    animates at the correct speed without slowing the bullets;
  - self-terminates through the base `finishState()` (which picks `popConcurrentState(this)` vs
    `popState()`), and routes any chained terrain explosion through `statePushConcurrent` too.
    Explosion **damage** is still applied immediately in `init()` (preserving the
    lazy-recalculation behavior above); only the animation and end-of-animation casualty
    resolution run concurrently.
- `ProjectileFlyBState::think()` now pushes the normal firearm/AoE impact explosion via
  `statePushConcurrent` and does **not** set `impactResolvedThisPass`, so the state keeps advancing
  other rounds and firing/finishing in the same pass. `BA_THROW` (single throw / hot grenade) and
  the `BA_LAUNCH` waypoint cascade still push to the front and yield the pass, since those are
  single-shot handoffs rather than part of an overlapping volley.


