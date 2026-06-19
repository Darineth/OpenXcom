# Feature - Fog-of-War View

**Status:** 📝 Planned. Design only; not yet implemented.

## Overview

Base OpenXcom draws every **discovered** tile at full brightness and gives no on-map indication of
which tiles a soldier can actually see *right now*. You can't tell remembered terrain from terrain
under live observation, so the tactical "what can I currently see" read is missing.

This feature **dims tiles that are discovered but not currently in any player unit's line of sight**,
giving three visible states:

1. **Currently in sight** — full brightness (unchanged from stock).
2. **Explored but out of current sight** — *dimmed* (the new state). You still see the remembered
   map layout, but can tell it isn't under live observation (and units standing there are already
   hidden by the existing visibility rules — the actual fog of war).
3. **Never discovered** — black (unchanged).

This is the marquee remaining Phase-1 item and a prerequisite for **Clairvoyance** (Phase 8), which
reveals fogged tiles.

## What already exists (OXCE-Plus base)

The audit found the visibility tracking is **entirely present** — only the rendering is missing:

- **Per-tile player-LOS count.** `Tile::_visible` (a `Sint16` reference count, runtime-only, not
  saved) with `setVisible(±1)` / `getVisible()` (`src/Savegame/Tile.*`). It counts how many player
  units currently see the tile.
- **Maintained automatically and player-only.** `TileEngine::calculateTilesInFOV`
  (`src/Battlescape/TileEngine.cpp:1542`) builds each unit's `BattleUnit::_visibleTiles`, calling
  `tile->setVisible(+1)` on add and `-1` on clear. It **early-returns for any non-player unit**
  (`TileEngine.cpp:1555`: `if (unit->getFaction() != FACTION_PLAYER ... ) return;`), so `getVisible()`
  reflects **player LOS only** — exactly what fog-of-war needs. It's recomputed on all the usual
  triggers (movement, turn, doors, terrain/fire changes, spawns, turn start `recalculateFOV`).
- **Discovery flag.** `Tile::isDiscovered(part)` / `setDiscovered` — the persisted "explored ever"
  state that already gates black vs. drawn.
- **Central shade function.** `Map::reShade(Tile*)` (`src/Battlescape/Map.cpp`) computes a tile's
  brightness from its light layers and night-vision, and is what the renderer already calls for
  discovered tiles.

Current renderer (`Map::drawTerrain`, the discovered branch):

```cpp
if (tile->isDiscovered(O_FLOOR))
{
    tileShade = reShade(tile);   // discovered: light/night-vision brightness, full-bright otherwise
    obstacleShade = tileShade;
    ...
}
else
{
    tileShade = 16;              // undiscovered: black
}
```

There is **no existing dimming** for discovered-but-unseen tiles, and no prior/partial fog feature.

## Approach

Add one step to the discovered-tile branch: after `tileShade = reShade(tile)`, if the fog option is
on and `tile->getVisible() == 0`, dim the shade:

```cpp
tileShade = (3 * tileShade) / 4 + 4;   // gentle dim
```

This maps the shade range `[0..16]` → `[4..16]`: a bright in-LOS tile (shade ~0) becomes a slightly
dimmer ~4, while already-dark tiles stay dark (16 → 16). (The legacy `(5·s)/8 + 6` dimmed harder,
to ~6; softened after seeing it in-game.) A fogged tile is never brighter than the same tile in LOS. The result layers cleanly **on top of**
night-vision (`reShade` runs first, so a night-brightened-but-unobserved tile still reads as fogged)
and leaves the undiscovered-black and in-sight-bright cases untouched.

`tileShade` shades the whole tile's content (floor, walls, objects, ground items), so remembered
terrain dims as a unit. Player units' own tiles always have `getVisible() > 0` (a unit sees its own
tile), so they never fog; hidden enemies on fogged tiles are already not drawn by the existing
visibility rules, so the dim never reveals them.

### Gating

A DX option **`fogOfWarEnabled`** (default **on**), registered in `createAdvancedOptionsDX` under
Battlescape, so players who prefer the stock full-bright map can turn it off.

## Key code

| File | Role |
|------|------|
| `src/Battlescape/Map.cpp` | In `drawTerrain`'s discovered-tile branch, dim `tileShade` when `Options::fogOfWarEnabled && tile->getVisible() == 0` |
| `src/Engine/Options.inc.h` / `Options.cpp` | `fogOfWarEnabled` option (default on), registered in `createAdvancedOptionsDX` |
| `bin/common/Language/DX/en-US.yml` | `STR_FOG_OF_WAR` option label |

No changes needed to `Tile`, `BattleUnit`, or `TileEngine` — the data and its upkeep already exist.

## Open questions / scope boundaries

- **Dim amount/curve.** The legacy `5·s/8 + 6` is the starting point; tune after seeing it in-game
  (a stronger dim reads more clearly but can make remembered terrain hard to plan around).
- **Event animation gating (out of scope).** The legacy fork also suppressed forcing the camera /
  animating projectiles & explosions that occur on out-of-FOV tiles (`_projectileInFOV` /
  `_explosionInFOV` gated on visibility). That's a separate behavior; this pass is the tile-dimming
  render only. Revisit separately if wanted.
- **Verify at turn boundaries.** `_visible` is decremented/incremented during FOV recalcs; confirm
  there's no one-frame stale-fog flicker on turn start (the base already does a full
  `recalculateFOV` at the start of the player's turn, so this is expected to be fine).

## Bug fix required during implementation

Initially tiles **stayed lit after leaving LOS** — they never re-fogged on move/turn. Root cause was
a pre-existing **double-increment** of the per-tile visible count in
`TileEngine::calculateTilesInFOV`: it called `unit->addToVisibleTiles(tile)` (which already does
`tile->setVisible(+1)`, and is explicitly guarded against double-adding) *and then* a redundant
`tile->setVisible(+1)`. Each recalc added **+2** while `clearVisibleTiles()` only subtracts **−1**, so
`Tile::_visible` drifted upward and never returned to 0 — the tile read as "seen" forever. Removing
the redundant second increment ([TileEngine.cpp:1644](../src/Battlescape/TileEngine.cpp#L1644)) makes
add/clear balanced so `getVisible()` correctly returns to 0 when no player unit sees a tile. (This
also makes the `sneakyAI` tile-visibility check more accurate, which relied on the same count.)
