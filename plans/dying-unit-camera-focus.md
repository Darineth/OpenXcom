# Dying-unit camera focus

**Status:** Implemented (pending in-game verification).

## Motivation

When a unit dies, the tactical camera does nothing special: the death pirouette and fall play
wherever the camera happens to be sitting. If the killing shot was fired from off-screen — or the
victim simply stood outside the viewport — the player never sees the kill, only the aftermath
(a corpse appearing on the next camera move, or a "has been killed" infobox with no visual).

Worse, while a projectile is still in flight the camera actively chases the bullet
(`Map::drawTerrain`), so the bullet keeps priority even if something is dying.

Deaths are the most important beat in a firefight. They should get the camera.

## Existing OXCE / OXCE-Plus behavior (audit)

Confirmed by reading the engine, not just `Extended.txt`:

- **Projectile following** lives in `Map::drawTerrain` (`src/Battlescape/Map.cpp`). The camera
  chases the average projectile voxel when
  `_explosions.empty() && _projectileInFOV && _followProjectile`.
  - `_followProjectile` is an existing opt-out: `ProjectileFlyBState::init` clears it when Alt is
    held or the weapon's action config sets `followProjectiles: false`, and restores it when the
    shooting finishes. So "suppress bullet chasing for a moment" is already an established pattern.
  - The `_explosions.empty()` guard is a prior DX change: explosions already outrank bullets.
- **Deaths** never touch the camera. `UnitDieBState` sets `Map::setUnitDying(true)` (player-faction
  units only) purely so `Map::drawTerrain` keeps drawing the map during hidden movement — it is not
  a camera concept and is cleared at `_extraFrame == 2`.
- `Camera::isOnScreen(pos, unitWalking, unitSize, boundary)` already exists and is exactly the
  "is this unit visible in the viewport" test used by `UnitWalkBState` and `ProjectileFlyBState`.

So there is nothing upstream to reuse or extend here — the delta is genuinely new.

## Behavior

New advanced option **`battleFocusDyingUnits`** (DX, Battlescape, default **on**).

When a `UnitDieBState` reaches the front of the action queue:

1. Skip entirely if the option is off, we're before/outside a real battle, or the unit has no tile.
2. Skip if the unit is not visible to the player (`BattleUnit::getVisible()` — which is
   unconditionally true for the player's own units and for `alwaysVisible` armors, and reflects
   actual spotting for hostiles/civilians). This keeps unseen alien deaths from panning the camera
   across the map and giving away positions.
3. Skip if the unit is **already on screen** (`Camera::isOnScreen`). This is the deliberate design
   choice: no camera jerk during normal, already-framed fights — the camera only moves when it
   would otherwise miss the death.
4. Otherwise `centerOnPosition(unit position)` and raise a new `Map::_deathFocus` flag.

While `_deathFocus` is set, projectile chasing in `Map::drawTerrain` is suppressed, so a death that
overlaps an in-flight projectile keeps the camera. The flag is cleared in `UnitDieBState::deinit()`
(runs when the state is popped, including on the early-out paths).

`_deathFocus` is intentionally a **separate flag from `_unitDying`**. `_unitDying` gates map drawing
during hidden movement and is player-faction-only; widening it would leak alien deaths onto the
hidden-movement screen.

## Implementation

| File | Change |
|---|---|
| `src/Engine/Options.inc.h` | declare `battleFocusDyingUnits` |
| `src/Engine/Options.cpp` | register in `createAdvancedOptionsDX()`, default `true` |
| `src/Battlescape/Map.h` | `_deathFocus` member + `setDeathFocus()` / `getDeathFocus()` |
| `src/Battlescape/Map.cpp` | init `_deathFocus(false)`; add `!_deathFocus` to the projectile-follow condition |
| `src/Battlescape/UnitDieBState.h/.cpp` | focus logic in `init()`, clear in new `deinit()` override |
| `bin/common/Language/DX/en-US.yml` | `STR_FOCUS_DYING_UNITS` + tooltip |

No new source files, so no CMake/vcxproj registration needed. No ruleset keys added, so no
`docs/Ruleset-*.md` change.

## Notes / open questions

- In practice deaths are enqueued via `statePushNext` from `BattlescapeGame::checkForCasualties`,
  which runs after the projectile has resolved — so the die state usually starts with no projectile
  in flight. The `_deathFocus` guard is cheap insurance for the cases where they do overlap
  (DX dual-wield staggered fire, chained explosions).
- Multiple simultaneous deaths each get their own `UnitDieBState` and thus each re-center in turn.
  That is the intended behavior (the player is walked through the casualties), but if it reads as
  too busy in play, an obvious follow-up is to only focus the *first* death of a burst.
