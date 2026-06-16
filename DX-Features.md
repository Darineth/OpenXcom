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
