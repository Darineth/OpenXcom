# OpenXcom DX — Features

This document tracks features added in OpenXcom DX on top of OXCE-Plus. Entries will be added
here as features are implemented.

## Combat Log

A floating, centered event log across the top of the Battlescape that reports combat events
as they happen (currently: new turn, unit killed, unit knocked out), color-coded by outcome
(good/neutral/warning/bad from the player's perspective). Entries fade out over time and the
list is capped, so it self-empties when combat is quiet. Unit names respect the player's
knowledge — own soldiers and currently-visible enemies are named, unseen units show as
"Unknown" — and are gendered. The log is transient (never serialized) and distinct from the
on-demand OXCE hit log (Ctrl-H).

- Toggle: advanced option **Combat log** (`combatLogEnabled`, default on), under Battlescape.
- Theming: the `combatLog` element in the `battlescape` interface ruleset sets the panel's
  position and size; `combatLogNeutral` / `combatLogGood` / `combatLogWarning` / `combatLogBad`
  set the four outcome colors.
