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
