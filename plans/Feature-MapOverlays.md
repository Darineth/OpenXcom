# Feature - On-Map Overlays

**Status:** 🚧 In progress. This pass implements the **hovered unit name** overlay. The other
three overlays bundled under the checklist item — primed-grenade indicator, motion-detector
readings, bleeding indicators — are deferred to later passes (see *Deferred* below).

## Overview

A family of small, always-on visual cues drawn directly onto the Battlescape map (as opposed to
the floating [Combat Log](Feature-CombatLog.md), which is a text feed). The umbrella checklist
item is **On-map overlays** in Phase 1. The legacy fork shipped four:

1. **Hovered unit name** — a floating name label over the unit currently under the mouse cursor.
2. Primed-grenade indicator — a pulsing icon over live grenades lying on visible tiles.
3. Motion-detector readings — motion blips painted in-world.
4. Bleeding indicators — a wound icon over visible units with fatal wounds.

Each is an independent cue with its own option toggle, so they can be shipped one at a time.

---

## 1. Hovered unit name (this pass)

### What it does

When the mouse hovers a tile that holds a unit the player can see, the unit's name is drawn as a
small high-contrast label just above that tile. The label is **knowledge-aware**: our own
soldiers and civilians read their plain name; a hostile reads its specific type only if
researched, its race if the race/corpse is researched, and otherwise the generic "Hostile" — the
exact same gating the Combat Log uses. The label is colored by faction (our units green, hostiles
red, civilians/neutrals yellow) to match the existing on-map visual language (the cursor box is
already yellow over a visible unit).

The label only appears for units the player can actually see (`getVisible()`, or any unit in
debug mode), so it never leaks the position or identity of an unseen enemy.

### How it works

`Map::drawTerrain` already walks every tile each frame and, for the tile under the mouse, draws
the selection cursor (the "Draw cursor front" block, gated on `_selectorX/_selectorY` matching the
tile and `_camera->getViewLevel()` matching the level). The hovered-name draw is a sibling block
right after it, keyed on the **exact** selector tile (`_selectorX == itX && _selectorY == itY`)
on the current view level, skipped when the mouse is over the HUD icons. It reuses the per-tile
`unit` pointer (`tile->getUnit()`) already resolved in the loop — so the name appears over exactly
the unit the cursor box highlights.

Rendering reuses the same `Text`-widget-blitted-into-the-locked-map-surface technique the UFO
Extender accuracy readout uses (`_txtAccuracy`): a dedicated `Text *_txtUnitName` widget, small
high-contrast font, `ALIGN_CENTER`, drawn once into its own surface then `blitNShade`'d onto the
map surface above the tile.

### Key code

| File | Role |
|------|------|
| `src/Battlescape/Map.h` / `Map.cpp` | New `Text *_txtUnitName` member (built beside `_txtAccuracy`, freed in dtor); hovered-name draw block in `drawTerrain` after the cursor-front block |
| `src/Savegame/SavedBattleGame.cpp` | Reuses the existing public `getCombatLogName(unit)` for knowledge-aware naming — no new naming code |
| `src/Engine/Options.inc.h` / `Options.cpp` | `hoveredUnitNameEnabled` option (default on), registered in `createAdvancedOptionsDX` next to `combatLogEnabled` |
| `bin/common/Language/DX/en-US.yml` | `STR_HOVERED_UNIT_NAME` option label |

### Naming reuse

`SavedBattleGame::getCombatLogName(const BattleUnit*)` is already a public, knowledge-aware
display-name resolver (own units plain; hostiles gated on research of type → race → corpse →
"Hostile"). The hovered-name overlay calls it directly rather than duplicating the logic, so the
two features always agree on what an enemy is called.

### Color

Faction-based, using the same palette-relative `Pathfinding` block offsets the accuracy text uses
(`Palette::blockOffset(Pathfinding::{green,red,yellow} - 1) - 1`), which resolve correctly in both
the UFO and TFTD palettes:

- `FACTION_PLAYER` → green
- `FACTION_HOSTILE` → red
- otherwise (civilian / neutral) → yellow

---

## Deferred (later passes)

- **Primed-grenade indicator** — iterate `tile->getInventory()` for items with
  `getFuseTimer() >= 0`, blit the pulsing `SCANG.DAT` frame-6 primer (the same sprite/animation
  `Inventory::drawPrimers` uses) over the tile. Reuses the `_animFrame` pulse.
- **Bleeding indicators** — for a visible `tile->getUnit()` with `getFatalWounds() > 0`, blit the
  existing `FloorWoundIndicator` surface (already loaded in the `Map` ctor and already used for
  unconscious bodies on the ground).
- **Motion-detector readings in-world** — paint `DETBLOB.DAT` blips for units with
  `getMotionPoints() > 0`, frame `motionPoints / 5`, mirroring `ScannerView`. The heaviest of the
  four: needs a design decision on whether/how it gates on the player actually carrying a motion
  scanner, plus balance review. Tracked but not scoped yet.
