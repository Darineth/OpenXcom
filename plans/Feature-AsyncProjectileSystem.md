# Feature - Async Projectile System

**Status:** Planning — not started. Jun 2026.

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
