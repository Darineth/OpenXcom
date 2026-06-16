# Feature - On-Map Overlays

**Status:** 🚧 In progress. The **hovered unit name** and **primed-grenade indicator** overlays
are implemented. The remaining two bundled under the checklist item — motion-detector readings
and bleeding indicators — are deferred to later passes (see *Deferred* below).

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

---

## 2. Primed-grenade indicator (this pass)

### What it does

Every **primed grenade the player threw** that is now **lying on a discovered tile** gets a small
pulsing marker hovering just above it, so the player can see at a glance where their live ordnance
is before it goes off. Two distinct icons by type:

- **Normal primed grenade** (`BT_GRENADE`) → a **red filled disc** (static, brightness-pulsed).
- **Proximity grenade** (`BT_PROXIMITYGRENADE`) → an **animated red "wifi ping"**: a core flanked
  by a `( . )` bracket-arc pair that jumps from near to far, reading as a wave broadcasting outward
  — an active detection field. Same red as the disc (distinguished by shape + animation, not
  color), and it pulses in brightness on top of the broadcast cycle.

Both markers carry a **black outline** (like the selection arrow) so they stay readable against any
terrain.

Only the player's own grenades are marked — a grenade thrown by an enemy is never shown (no free
intel). "Player-thrown" is read from the grenade's *previous owner* (a thrown item's previous
owner is the thrower, set when `moveToOwner(nullptr)` drops it on throw): the marker shows only
when `getPreviousOwner()->getFaction() == FACTION_PLAYER`. The tile must be discovered, so the
marker never x-rays through unexplored walls. "Primed" is the same `getFuseTimer() >= 0` test the
inventory primer uses.

### How it works

The markers are built **procedurally** in `Map::init()` — `Surface`s drawn from pixel-index math,
the technique the existing selection `_arrow` uses — so the feature ships no art assets and works
out of the box. Both markers use the **block-2 red ramp** (disc: fill `34`, highlight `32`; ping: core `32`, near
arc `34`, far arc `38`) with a **black (`15`) outline**, with all red indices ≤ 43 so the `+0..+4`
brightness pulse never spills out of the red block (and `15` clamps back to black under the pulse).

- The **disc** is a single 9×9 surface; its outline cells are black.
- The **ping** is `PROXY_PING_FRAMES` (2) separate 11×11 surfaces drawn from explicit grid arrays
  (`0` clear / `1` arc / `2` core): frame 0 is the core + a near `( . )` arc pair, frame 1 the core
  + a far pair. A build-time pass paints a black halo on every transparent cell that borders a lit
  cell (8-neighbourhood), giving the thin arcs the same outline the selection arrow has.
  `drawTerrain` toggles the frames with `_proxyPing[(_animFrame / 2) % PROXY_PING_FRAMES]`.
- Both are blitted with the inventory primer's `Pulsate[_animFrame % 8]` shade for a brightness
  pulse — so the ping both broadcasts (frame cycle) and pulses (shade), the disc just pulses.

The draw is a per-tile block in `Map::drawTerrain`, right after the on-ground-item block: for each
item in `tile->getInventory()` that passes the primed / grenade-type / player-thrower tests, it
blits the type-appropriate marker centered over the tile and lifted one icon-height above the
floor item (`screenPosition.y + getTerrainLevel() - height`), then stops (one marker per tile).

### Key code

| File | Role |
|------|------|
| `src/Battlescape/Map.h` / `Map.cpp` | New `_grenadeIndicator` surface + `_proxyPing[PROXY_PING_FRAMES]` animation frames, built in `init()`, freed in dtor; scan-and-blit block in `drawTerrain` |
| `src/Engine/Options.inc.h` / `Options.cpp` | `grenadeIndicatorEnabled` option (default on), registered in `createAdvancedOptionsDX` |
| `bin/common/Language/DX/en-US.yml` | `STR_GRENADE_INDICATOR` option label |

### Future enhancement

The procedural icons could be overridden by mod-supplied named surfaces (à la `reactionIndicator`)
once the ownership split (Mod-owned vs Map-owned) is handled cleanly. Not done in this pass.

---

## Deferred (later passes)

- **Bleeding indicators** — for a visible `tile->getUnit()` with `getFatalWounds() > 0`, blit the
  existing `FloorWoundIndicator` surface (already loaded in the `Map` ctor and already used for
  unconscious bodies on the ground).
- **Motion-detector readings in-world** — paint `DETBLOB.DAT` blips for units with
  `getMotionPoints() > 0`, frame `motionPoints / 5`, mirroring `ScannerView`. The heaviest of the
  four: needs a design decision on whether/how it gates on the player actually carrying a motion
  scanner, plus balance review. Tracked but not scoped yet.
