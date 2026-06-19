# Feature - On-Map Overlays

**Status:** ✅ All four bundled overlays implemented: **hovered unit name**, **primed-grenade
indicator**, **unit status indicators** (bleeding + fire/shock/stun), and **motion-detector
readings**.

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

## 3. Unit status indicators (this pass)

### What it does

Every one of the **player's own living units** gets a hovering status marker above its head for each
active ongoing-harm condition, so the player can spot at a glance who needs attention. Four
conditions, each its own glyph:

- **Bleeding** — `getFatalWounds() > 0` (a wound, losing HP each turn until treated).
- **On fire** — `getFire() > 0`.
- **Shock** — `hasNegativeHealthRegen()` (losing HP per turn from a damage-over-time effect).
- **Near knockout** — accumulated stun is within a quarter of current health of dropping the unit
  (`getStunlevel() * 4 >= getHealth() * 3`), an early warning before the soldier passes out.

All conditions that apply to a unit are shown **at once**, stacked side-by-side. Only the player's
own units are marked (the player always knows the state of their own soldiers); enemy/neutral units
are never shown. The stun *catch-all* still only applies to **unconscious bodies** on the floor — a
healthy conscious unit is not marked.

### How it works

This **reuses the existing status indicators** rather than introducing new visuals. The same glyphs
the engine already paints over *unconscious* bodies on the ground (burn/wound/shock/stun) are now
also blitted hovering over *conscious* player units, gated per condition. Markers honour the per-unit
`disableIndicators` script flag (`indicatorsAreEnabled()`) exactly like the floor overlay does.

The draw is a unit-iteration block in `Map::drawTerrain`, right after the selected-unit selection
arrow. It mirrors that arrow's positioning math — `calculateWalkingOffset`, big-unit / kneel / float
height adjustments, the `getArrowBobForFrame` hover bob. The active glyphs are gathered into a small
array (priority order burn → wound → shock → stun) and laid out as a centered row above the head,
lifted by one arrow-height so the row clears the centered selection arrow when the unit is also the
selected one. Units above the current view level are skipped.

### Source for the glyph — and the procedural fallback

The four OXCE status indicators (`FloorWoundIndicator` / `FloorBurnIndicator` /
`FloorShockIndicator` / `FloorStunIndicator`) are all **mod-supplied** — loaded in the `Map` ctor
with `required = false`, so `getSurface` returns `nullptr` when no mod provides them. No mod bundled
with DX defines them, which means the *unconscious-body* floor overlay has historically rendered
nothing out of the box, and a wound-only bleeding overlay would too.

To make the overlays work with no assets (the same philosophy as the procedural grenade disc), DX now
builds a **procedural fallback** for *all four* indicators in `Map::init()`: small 11×11 icons drawn
from pixel-index grids with an auto-painted black halo (the proxy-ping outline technique), owned by
`Map` and freed in the dtor. They use only the palette-safe red (block 2) and yellow (block 1) ramps
plus black, and are distinguished by shape — wound = red blood drop, burn = flame, shock = lightning
bolt, stun = a "Zzz" sleep glyph. At each draw site the engine prefers mod art when present and falls back to the
procedural icon otherwise (mod art keeps its tile-aligned blit; the small fallback is centered over
the tile). A side effect: the long-dormant unconscious-body indicators now render by default too.

### Key code

| File | Role |
|------|------|
| `src/Battlescape/Map.h` / `Map.cpp` | Four `_*IndicatorFallback` surfaces built in `init()`, freed in dtor; unconscious-body floor block and the living-unit hover block both pick mod-art-or-fallback |
| `src/Engine/Options.inc.h` / `Options.cpp` | `unitStatusIndicatorEnabled` option (default on), registered in `createAdvancedOptionsDX` |
| `bin/common/Language/DX/en-US.yml` | `STR_UNIT_STATUS_INDICATOR` option label |

A mod can still override any of the four by supplying the matching `Floor*Indicator` surface via
`extraSprites` (`singleImage: true`), which takes precedence over the procedural fallback.

---

## 4. Motion-detector readings (this pass)

### What it does

Each enemy/neutral unit the player **detected this turn with a motion scanner** gets a **pulsing amber
target reticle** painted on its floor — a floor decal in the same style as the path-preview markers. It
pulses (alarm) and is **brighter the more the unit moved** (its motion points, `motionPoints/5`
clamped 0–5, folded into the blit shade), preserving the scanner's near→far sense. This replaces
OXCE's hard-to-see, Alt-held bobbing arrow for scanned units with a clear, passive, ground-level
overlay (the user's request: "displayed more like the path-preview stuff … make it a floor overlay
like the pathfinding renderer").

### Gating (unchanged from OXCE — no new free intel)

Detection still rides entirely on OXCE's existing mechanism: a unit's `getScannedTurn()` is set to
the current turn **only when the player actually uses a motion scanner** (`ScannerView::draw`, the
scanner popup). This overlay just *displays* those already-detected units better — it does **not**
make detection passive, so it grants no intel the scanner wouldn't. (Making detection itself passive
— e.g. an always-on field scan, or gating on merely *carrying* a scanner — remains an open balance
decision, deliberately not taken here.) The blips persist for the rest of the turn after a scan and
clear at the next turn, since motion points reset then.

### How it works

The decal is drawn **inside the per-tile loop** of `Map::drawTerrain`, right after `tile->getUnit()`
is resolved and *before* that tile's walls/objects/unit are blitted — so it reads as ground and the
unit sprite sits over its center while the diamond's edges trace the tile around the unit's feet
(visible without being disruptive). For a tile whose occupant is a non-player, non-out unit with
`getScannedTurn() == turn` and `getMotionPoints() > 0` (anchor tile only, for big units), it
`blitRaw`s the **`Pathfinding` set's frame 10** (the target reticle) at the tile floor
(`screenPosition.y + terrainLevel`), recolored via the `newBaseColor` path to **amber** (block 1,
palette-safe in both UFO and TFTD) — exactly how the path preview recolors its markers. The blit
`shade` is `(5 − intensity) + Pulsate[animFrame%8]`, so it pulses and brightens with motion.

It sits on the unit's **actual tile and floor** (it no longer forces `z = viewLevel` like the old
arrow), so it appears only on levels currently drawn. Gated behind `motionDetectorOverlayEnabled`
(default on). The old Alt-gated motion arrow is gone; the custom-marker arrows that shared its block
stay on Alt. *(Color and pulse are easy knobs; the per-frame `DETBLOB` blip was tried first but read
poorly behind units — the tile-diamond decal is far more legible.)*

### Key code

| File | Role |
|------|------|
| `src/Battlescape/Map.cpp` | DETBLOB floor-blip drawn in the per-tile loop (under unit/wall sprites, so it's occluded in front); replaces the motion half of the old Alt-arrow block; custom-marker arrows split into their own Alt block |
| `src/Engine/Options.inc.h` / `Options.cpp` | `motionDetectorOverlayEnabled` option (default on), registered in `createAdvancedOptionsDX` |
| `bin/common/Language/DX/en-US.yml` | `STR_MOTION_DETECTOR_OVERLAY` option label |

### Possible follow-ups

- **Passive detection** — make units accrue/reveal motion without opening the popup (the pinned
  balance decision); e.g. gate on carrying a `BT_SCANNER` and auto-scan within its range each turn.
- **Direction tick** — `DETBLOB` frames 7–14 are facing arrows; could add the unit's facing.
