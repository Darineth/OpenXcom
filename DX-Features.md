# OpenXcom DX — Features

This document tracks features added in OpenXcom DX on top of OXCE-Plus. Entries will be added
here as features are implemented.

## Combat Log

A floating, centered event log across the top of the Battlescape that reports combat events
as they happen, color-coded by outcome (good/neutral/warning/bad from the player's
perspective). Reported events: new turn; weapon fire and throws (with shot mode, e.g. "fires
Rifle (Snap Shot)"); melee attacks; reaction shots/swings (tagged as such); hits, with
research-gated damage detail (exact damage and fatal wounds for your units and researched
enemies, a vague light/heavy band for un-researched enemies); kills and knockouts; panic and
berserk; and a player-only warning when a shot empties a weapon. Entries fade out over time and
the list is capped, so it self-empties when combat is quiet. Unit and weapon names respect the
player's knowledge — own soldiers and researched enemies are named, while unseen/un-researched
hostiles show as "Hostile"/"Unknown" and their gear as "an unknown weapon". The log is
transient (never serialized) and distinct from the on-demand OXCE hit log (Ctrl-H).

- Toggle: advanced option **Combat log** (`combatLogEnabled`, default on), under Battlescape.
- Theming: the `combatLog` element in the `battlescape` interface ruleset sets the panel's
  position and size; `combatLogNeutral` / `combatLogGood` / `combatLogWarning` / `combatLogBad`
  set the four outcome colors.

## Action Menu Revamp

The battlescape action popup (Throw / Snap / Aimed / Auto / Hit / …) is more compact and more
informative. Rows use a small font in ~25px boxes, and each row now shows its configurable
hotkey at the left. Multi-shot modes show a shot count (e.g. `x3`) and shotgun ammo shows pellets
(`x3 (9 pellets)`). Affordability is flagged at a glance: an action you can't perform — not
enough TU, or no usable ammo — turns the whole row (text + border) red with a "No TU" / "No Ammo"
tag, while an action with *some* ammo but not enough for a full burst turns amber. Flagged rows
stay clickable (the usual warning still fires).

- A sixth configurable action hotkey (`keyBattleActionItem6`, default `6`) was added so the
  six-slot popup and the skill menu are fully keyboard-reachable.
- Theming: `actionMenuDisabled` (red) and `actionMenuWarning` (amber) elements in the
  `battlescape` interface ruleset color the unaffordable / partial-ammo states.
- DX options now live under their own **DX** tab in Options → Advanced and Options → Controls
  (a dedicated `OPTION_DX` owner), separate from the OXC/OXCE tabs.

## On-Map Overlays

A family of small, always-on visual cues drawn directly onto the Battlescape map, each with its
own toggle so they can be enabled independently. *(design:
[plans/Feature-MapOverlays.md](plans/Feature-MapOverlays.md))*

- **Hovered unit name** — a knowledge-aware, faction-colored floating name label over the unit
  under the cursor. Toggle: **Hovered unit name** (`hoveredUnitNameEnabled`, default on).
- **Primed-grenade indicator** — a pulsing marker hovering over each player-thrown primed
  grenade lying on a discovered tile (a brightness-pulsed red disc for normal grenades, an
  animated red "wifi ping" for proximity grenades). Enemy grenades are never shown. Toggle:
  **Primed grenade indicator** (`grenadeIndicatorEnabled`, default on).
- **Unit status indicators** — status glyphs hovering over each of the player's own *living*
  units, one per active condition: **bleeding** (fatal wounds), **on fire**, **shock** (losing
  HP each turn), and a **near-knockout** stun warning when accumulated stun is close to dropping
  the unit. All active conditions stack side-by-side above the head. Uses the same glyphs the
  engine already shows over unconscious bodies. Toggle: **Unit status indicators**
  (`unitStatusIndicatorEnabled`, default on).

  The four OXCE status indicators (`Floor{Wound,Burn,Shock,Stun}Indicator`) are mod-supplied and
  absent from base data, so they used to render nothing out of the box. DX now builds a small
  **procedural fallback** for all four in `Map::init()` (red blood drop / flame / lightning bolt /
  "Zzz" sleep glyph, palette-safe and shape-distinct), preferred only when no mod art is present — so both the
  living-unit overlay and the long-dormant unconscious-body indicators now work by default. A mod
  can still override any of them with an `extraSprites` `Floor*Indicator` (`singleImage: true`).
- **Motion-detector readings** — a pulsing amber target reticle painted on the floor of each
  enemy/neutral unit detected this turn by a motion scanner (a path-preview-style floor decal),
  brighter the more the unit moved. It shows passively (no key held) and is drawn under the unit/
  wall sprites, so a visible enemy occludes its own marker and the cue is left for fog tiles —
  replacing OXCE's hard-to-see Alt-held bobbing arrow. Detection is unchanged from OXCE — a unit
  only lights up once actually scanned, so the overlay grants no intel a scanner wouldn't. Toggle:
  **Motion detector readings on map** (`motionDetectorOverlayEnabled`, default on).

## Fog of War

Base OpenXcom draws every *discovered* tile at full brightness, with no on-map sign of what a
soldier can actually see right now. DX **dims discovered tiles that no player unit currently has in
line of sight**, so remembered terrain reads as out of live observation while in-sight terrain stays
bright (undiscovered tiles remain black). The dim layers on top of the existing light/night-vision
shading and updates live — tiles re-fog as soon as soldiers move or turn. *(design:
[plans/Feature-FogOfWar.md](plans/Feature-FogOfWar.md))*

Implemented as a renderer change keyed off the engine's existing per-tile player-LOS count
(`Tile::getVisible()`); also fixes a pre-existing double-increment in
`TileEngine::calculateTilesInFOV` that kept that count from returning to zero (so tiles never
re-fogged). Toggle: **Fog of war (dim unseen tiles)** (`fogOfWarEnabled`, default on).

## Maximize Info Screens (all screens)

OXCE's **Maximize Info Screens** option (`maximizeInfoScreens`) drops a screen to 320×200 so its
UI fills the window instead of sitting small in a corner at a high base resolution — but stock
OXCE only honored it for 6 Battlescape popups. DX makes it apply to **every** menu, detail, and
dialog screen (main menu, options, all of Basescape, Ufopaedia, briefings/debriefings, geoscape
dialogs, …). The only screens left at the gameplay resolution are the primary views themselves —
the Geoscape, the Battlescape, and dogfights — plus the self-managing loading/cutscene/intro
screens.

This is driven centrally rather than per-screen: each `State` declares a `ScaleContext`
(`UI` / `Geoscape` / `Battlescape` / `SelfManaged`), and `Game` applies the matching base
resolution once whenever the top of the state stack changes (only resetting the display when the
resolution actually changes, so the common case is free). When the option is **off**, behavior is
unchanged from before — info screens that overlay the Battlescape (the unit info / scanner /
minimap / medikit / inventory popups) keep the battlescape scale, so there's no open/close display
thrash. No new option; this extends the existing `maximizeInfoScreens` toggle. *(design:
[plans/Feature-MaximizeAllScreens.md](plans/Feature-MaximizeAllScreens.md))*

## Craft Stat Display

The Basescape craft info screen now shows the craft's core performance numbers alongside its
status readouts. The stats are laid out in two compact columns so they fit cleanly at 320×200:
damage capacity, maximum speed, and acceleration on the left; current damage, shield, and fuel on
the right. This mirrors the numbers already shown in the Ufopaedia craft article, but makes them
available directly from Craft Info where base management happens.

## Inventory UI Improvements

The inventory ground area now supports mouse-wheel scrolling anywhere over the ground slots
(`WHEEL UP` for backward, `WHEEL DOWN` for forward), alongside the existing ground-scroll button
and keyboard controls. Scrolling moves one column at a time (not page-by-page), and the view
starts fully scrolled left when a unit is selected. Multi-slot items that extend past the left
edge of the viewport are rendered with their visible portion showing.

## Soldier Info Layout Rework

The Soldier Info screen now follows the Legacy DX-style two-row action layout (excluding Level/EXP):
top row `<<`, `OK`, `>>`, `DIARY`, `ARMOR`, `SACK`; second row with a craft button and
`INVENTORY`. The craft line is now a clickable assign/unassign toggle for the base's first craft
slot (using the same placement validation rules as craft soldier assignment), and `INVENTORY`
opens base inventory setup with the current soldier pre-selected.

## Debriefing Soldier Results

The Battlescape debriefing now keeps each soldier in the mission-results table and shows a compact
outcome code for them: `KIA`, `MIA`, `WND`, or `OK`. Wounded soldiers also show recovery time in
days, while the existing per-soldier stat-gain breakdown remains on the same view. Bad outcomes and
the recovery-day value are highlighted in the list's secondary color.
