# Feature: Live Trajectory Preview

**Status:** Implemented. Ships the ideal-path preview (direct fire + throws/arcing shots) with a
tracer-sprite render and the `battleTrajectoryPreview` toggle (default on). Launch/waypoint
(`BA_LAUNCH`) preview and aim-cone spread visualization remain deferred.

## What shipped

- `TileEngine::resolveFireTargetVoxel()` — the direct-fire aim-voxel priority resolution
  (unit-center → object → walls → floor → tile center) extracted from `ProjectileFlyBState::init()`
  and shared by both the real shot and the preview, so the drawn line hits the same voxel that will
  be fired. Returns false (and sets the target voxel invalid) when there is no line of fire to a unit.
- `Projectile::calculatePreviewTrajectory()` — traces the ideal straight line-of-fire (no accuracy
  deviation) and stores it. Like a real shot it **extends the ray past the aim point out to maximum
  range** (mirroring `applyAccuracy`'s `extendLine`) and traces until the first obstacle, so a shot
  into empty air still draws a path to the map edge instead of nothing. `Projectile::calculateThrow()`
  gained an `ignoreAccuracy` argument for the ideal parabola. `Projectile::getTrajectory()` exposes
  the stored voxel path for drawing.
- `Map::_targetingProjectile` + `updateTargetingPreview()` / `clearTargetingPreview()` /
  `drawTargetingPreview()` — a dedicated preview projectile kept out of the in-flight `_projectiles`
  collection, rebuilt only when the aim target/action/actor changes, drawn as spaced tracer sprites
  in a top pass (so throw arcs above the view level aren't hidden under higher floors). A single
  fixed standard tracer is used for every preview regardless of weapon or throw, and is
  **mod-configurable** via the `constants` ruleset key `trajectoryPreviewSprite`
  (`Mod::TRAJECTORY_PREVIEW_SPRITE`, a Projectiles frame index, default `35` = the rifle-type base
  bullet; loaded via `loadSpriteOffset` so it still gets extraSprites offset remapping, and read from
  `_projectileSet` so it stays depth-correct for TFTD). Standard xcom1 sets it to `35`, xcom2 to
  `36`. The impact marker uses the engine's hit sprite (`HIT.PCK` frame 0, used by both UFO and TFTD
  hit rendering). Cleared when leaving an aim cursor.
- Option `battleTrajectoryPreview` (DX, `STR_TRAJECTORY_PREVIEW`, default on).

## Original plan follows.

**Status (original):** Planned. Roadmap item *Live Trajectory Preview* (Phase 5 UI).

## Motivation

While a fire/throw action is being targeted, DX should draw the **predicted physical path** of the
shot on the battlescape — a line-of-fire for direct fire and a parabolic arc for throws/arcing
weapons — so the player can see exactly where the round will go (and what it will actually hit)
before committing TUs. Base OpenXcom and OXCE draw only the aim cursor; the path is invisible until
the bullet flies.

This is the **live trajectory preview** the legacy OpenXcom+ fork shipped (see
`Legacy-DX-Features.md` §1, "Targeting display"). It complements the on-cursor accuracy readout
(`<acc>% @ <dist>`) that DX already inherits from OXCE.

### Decisions (confirmed with user)

- **Ideal path, now.** The preview traces the **ideal** trajectory (accuracy forced perfect — the
  *intended* line, not a scattered one). This makes it deterministic and stable as the cursor moves,
  and — crucially — **independent of the aim-cone model** (`Feature-AimConeTrajectory.md`). The
  roadmap's "needs aim-cone" note is conservative; the *line* itself does not. When the aim-cone
  later lands, the preview can optionally visualize cone spread, but the ideal line ships first.
- **Scope:** direct fire (aim/snap/auto/burst) **and** throws/arcing shots. **Launch/waypoint
  (`BA_LAUNCH`) is deferred** to a later pass.
- **Rendering:** repeated **tracer sprites** from `Projectiles.PCK` along the route with an
  **impact sprite** at the end (matching the live-bullet draw path). Not a plain colored line.

## OXCE / OXCE-Plus audit

- **No trajectory preview exists** in DX or upstream OXCE. A grep for `_targetingProjectile`,
  `calculateTrajectoryFromVector`, `calculateLineFromVector` finds nothing in `src/` — these are
  legacy names, not dormant code. So this is a genuine (re)implementation.
- **What DX already has (reuse):**
  - **Async projectile collection** — `Map::_projectiles` (`std::vector<Projectile*>`) with
    `addProjectile`/`removeProjectile`/`getProjectiles`/`hasProjectiles`
    ([Map.h:96,161-168](../src/Battlescape/Map.h#L96)). The preview must stay **separate** from this
    live collection so `ProjectileFlyBState` never animates/moves/reaps it.
  - **`Projectile` computes and stores a full voxel `_trajectory`** and can draw bullet sprites
    along it: `getPosition(offset)`, `getParticle(i)`, `getPositionFromEnd()`, `getImpactPosition()`
    ([Projectile.h:44-99](../src/Battlescape/Projectile.h#L44)). The straight trace is
    `calculateTrajectory(accuracy[, originVoxel])`; the arc is `calculateThrow(accuracy)`.
    Both call `applyAccuracy` internally to deviate the target — the preview must **skip** that.
  - **On-cursor accuracy readout** — `Map::_txtAccuracy` + `_showInfoOnCursor`
    ([Map.cpp:1674-1865](../src/Battlescape/Map.cpp#L1674)); already resolves the current action,
    weapon, target tile, distance and LOS. This is the natural sibling location and shows the pattern
    for pulling `getCurrentAction()`, the actor, and the selector tile.
  - **Live bullet draw path** — [Map.cpp:1410-1479](../src/Battlescape/Map.cpp#L1410) blits
    `_projectileSet->getFrame(proj->getParticle(i))` at `proj->getPosition(1-i)`, per-tile, with
    shadow + visibility checks. The preview draw reuses this idiom.
  - **Target-voxel resolution** — `ProjectileFlyBState::init()`
    ([ProjectileFlyBState.cpp:290-391](../src/Battlescape/ProjectileFlyBState.cpp#L290)) picks the
    aim voxel via `TileEngine::canTargetUnit`/`canTargetTile` before building the `Projectile`. For
    the preview to match the real shot, this resolution should be shared (see approach below).

## Target design

### Ownership & lifecycle

Add `Projectile *_targetingProjectile = nullptr;` to `Map`, owned and deleted by `Map`, kept out of
`_projectiles`. A helper `Map::updateTargetingPreview()`:

1. Runs only when a fire/throw action is being targeted: `_cursorType == CT_AIM || CT_THROW`, there
   is a current action (`_save->getBattleGame()->getCurrentAction()`) with an actor and weapon, and
   the selector is on a valid tile. Otherwise it clears the preview (delete + null).
2. Computes the target tile from the selector (`Position(_selectorX, _selectorY,
   _camera->getViewLevel())`, mirroring the accuracy readout).
3. **Caches** the last (actor, target tile, action type) it built for, and rebuilds only when that
   changes — the voxel trace is not free and `draw()` runs every frame.
4. Builds the preview `Projectile` and traces the **ideal** path (below).

`updateTargetingPreview()` is called from `Map::draw()`/`drawTerrain()` before the projectile-draw
pass; the cleanup path (targeting ended) also fires when `setCursorType` leaves an aim mode.

### Building the ideal trajectory

The preview must (a) pick the same target voxel the real shot would, and (b) trace **without**
accuracy deviation.

- **(a) Shared target-voxel resolution.** Extract the `canTargetUnit`/`canTargetTile` selection
  block from `ProjectileFlyBState::init()` into a reusable helper (e.g.
  `TileEngine::resolveTargetVoxel(action, origin, &targetVoxel)` or a static on `ProjectileFlyBState`)
  and call it from both the real fire path and the preview. This keeps the preview line honest —
  it hits the same voxel the shot aims at. *(Fallback if extraction proves invasive: aim at tile
  center `action.target.toVoxel() + TileEngine::voxelTileCenter`, matching the accuracy readout —
  visually close but can diverge slightly from the real aim voxel.)*
- **(b) Ideal (undeviated) trace.** Add an ignore-accuracy path so `applyAccuracy`'s deviation is
  skipped:
  - **Direct fire:** `Projectile::calculatePreviewTrajectory()` — trace `originVoxel → targetVoxel`
    straight via `TileEngine::calculateLineVoxel(..., storeTrajectory=true, ...)`, no `applyAccuracy`.
    (Equivalent to `calculateTrajectory` minus the deviation stage.)
  - **Throw/arcing:** reuse `calculateThrow` but pass a perfect accuracy / add an
    `ignoreAccuracy` parameter so the arc is the intended parabola (the `applyAccuracy(...,
    keepRange=true)` deviation is skipped). Curvature still comes from `validateThrow`.

The preview `Projectile` stores its `_trajectory` but is **never `move()`d**.

### Rendering

In `drawTerrain()`'s per-tile pass, after live projectiles, if `_targetingProjectile` exists:

- Walk its `_trajectory` and, at a fixed **stride** (every N voxels, so it reads as a dotted tracer
  line rather than a solid smear), blit a tracer frame from `_projectileSet`
  (`getParticle`/`_bulletSprite`) at the projectile screen position for voxels whose tile matches the
  tile currently being drawn (`voxelPos/16 == itX,itY`, `voxelPos.z/24 == itZ`) and that are
  voxel-visible — reusing the live-bullet visibility idiom.
- Blit an **impact sprite** at `getImpactPosition()` (end of trajectory).
- **Throwing arc Z-handling:** a lobbed arc rises above the current view level; draw arc points that
  are above the current Z-level so the path isn't hidden under higher floors (legacy behavior —
  clamp/adjust the draw Z or draw on the top layer for arc voxels above `viewLevel`).

Tracer/impact frame indices: reuse the weapon's bullet sprite where sensible, or a fixed preview
frame from `Projectiles.PCK`. Pick during implementation against the actual sprite set.

## Implementation approach (delta)

1. **`Map` state + lifecycle.** Add `_targetingProjectile`, `updateTargetingPreview()`, cache fields
   (last actor/target/action-type), and cleanup in `setCursorType`/destructor. Call
   `updateTargetingPreview()` from `draw()`.
2. **Shared target-voxel resolver.** Extract `canTargetUnit`/`canTargetTile` selection from
   `ProjectileFlyBState::init()` into a shared helper; call from both fire and preview paths.
   (Or use tile-center fallback — decide at implementation time; prefer the shared helper.)
3. **Ideal-trace API on `Projectile`.** Add `calculatePreviewTrajectory()` (straight, no deviation)
   and an `ignoreAccuracy` path for `calculateThrow`. No `move()` on the preview.
4. **Draw pass in `drawTerrain()`.** Tracer-stride + impact-sprite rendering, with arc-above-Z
   handling for throws. Reuse the existing bullet-draw visibility checks.
5. **Option gate.** Add a DX option to toggle the preview (default on). Follows the DX "visible UI"
   preference; also lets players who dislike it opt out. (Confirm option name/placement.)
6. **Docs.** Update `DX-Features.md` (new entry), tick the roadmap checkbox, set this doc's status.

## Open questions

- **Option name/default — resolved.** New DX battlescape option, **default ON**, so players can turn
  the preview off (proposed `battleTrajectoryPreview`; final name confirmed at implementation).
- **Sprite choice.** Which `Projectiles.PCK` frame(s) for the tracer dots and the impact marker, and
  the tracer stride (every N voxels).
- **Target-voxel resolution — resolved.** Extract the `canTargetUnit`/`canTargetTile` selection from
  `ProjectileFlyBState::init()` into a **shared `TileEngine` helper** called by both the real fire
  path and the preview, so the preview line never drifts from the real shot. (Re-test the fire path
  after the refactor.)
- **Interaction with aim-cone (future).** Once `Feature-AimConeTrajectory.md` lands, optionally show
  cone spread (e.g. a faint fan) — out of scope for this first pass.
- **Performance.** Rebuild only on target change (cached); confirm the voxel trace per cursor move is
  cheap enough at large ranges.
