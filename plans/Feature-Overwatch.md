# Feature: Overwatch — set-and-hold reaction fire (cone-based)

**Status:** **Implemented (Jul 2026)**, builds clean (0 warnings). Phase 7 (Tactical Unit Systems).
The biggest new Phase 7 mechanic. Builds on the shipped **Reaction Scoring Split** / **Movement-mode
Evasion** ([Feature-ReactionScoringSplit.md](Feature-ReactionScoringSplit.md)). Shipped: the five
`overwatch*` RuleItem fields + `BA_OVERWATCH`; `BattleUnit` overwatch state (persistent, save/load,
lazy weapon resolve); `TileEngine::isInOverwatchCone`; the action-menu item + aim + one-reserved-shot
arming; the cone-tile markers (aim + reselect), now **LOS-clarified** — yellow where the watcher has
line of fire, red for terrain-blocked dead zones (per-tile LOF cached per watcher position); the
trigger folded into `checkReactionFire` (cone-exclusive, reserved-free-then-TU shots, full-reaction
score); persistence with turn-start reservation expiry; and auto/toggle cancellation. **Deferred:** a
dedicated on-map per-unit overwatch indicator, and AI use of overwatch (see Follow-ons).

## Resolved decisions (as built)

1. **Cone shape:** per-weapon **full cone angle** (total width) that widens the cone with range
   (min-range carves a near dead zone).
2. **Persistence:** overwatch **persists across turns** until cancelled. Arming reserves **one free
   shot** for the upcoming enemy turn (its TU paid up front); that reservation **expires at the
   owner's next turn**, and from then on the mode is free to keep or cancel and its shots simply spend
   TU.
3. **Shots:** the reserved first shot fires **free**; every further overwatch shot (this turn and on
   later turns) **spends the unit's TU** like a normal reaction. No fixed shot count.
4. **Cone-exclusive reaction:** a unit on overwatch takes reaction fire **only** inside its cone — it
   does **not** take ordinary reaction fire at things outside the cone.
5. **Reaction score:** an overwatch shot uses the unit's **full reactions stat** × `overwatchModifier`
   (not the TU-depleted score), so a committed watcher reacts reliably.
6. **Cancellation:** overwatch auto-cancels when the unit is commanded to do anything else (move, a
   non-reaction shot, or an **explicit turn order that changes facing** — right-clicking the already-
   faced tile, e.g. the open-door gesture, keeps it; engine-driven turns from reaction fire/panic never
   cancel it), and can be toggled off by re-selecting **Overwatch** from the menu.
7. **Aim:** the player **clicks a target tile** to set the cone direction (`unit→tile`).
8. **Markers:** cone trigger-tiles drawn **while aiming and when a unit already on overwatch is
   reselected**, using a **tile-level marker** (the Pathfinding target-reticle sprite, like the path
   preview) so the area reads as a filled region rather than scattered dots.

## Audit

- **Not present in current DX/OXCE-Plus** — a grep for `overwatch`/`BA_OVERWATCH` across `src/` is
  empty. Genuine new work.
- **Legacy DX had it, radius-based** (Legacy-DX-Features.md §2): a `BA_OVERWATCH` action puts a unit
  into a held-fire state that reacts during the enemy turn. Verified legacy pieces:
  `BattleUnit::_overwatch`, `_overwatchTarget`, `_overwatchWeaponSlot`, `_overwatchShotsAvailable`;
  `BattleAction::overwatch`; `getReactionScore(bool checkOverwatch)`; per-weapon RuleItem
  `overwatchModifier` (100), **`overwatchRadius` (2)**, `overwatchRange` (20), `overwatchShot`
  (`"snap"`). Overwatch was exempt from normal reaction fire's "only spot to protect from the spotted
  unit" restriction. Shots-available derived from the TU spent (`max(actionTU, 90% baseTU)`).
- **Reaction machinery to reuse** (already in the engine): `TileEngine::checkReactionFire` →
  `getSpottingUnits`/`getReactor`/`determineReactionType`/`tryReaction`, and the DX split where the
  *offensive* score is `getReactionScore` (evasion is separate). Overwatch applies the weapon's
  overwatch modifier to that offensive score.
- **Visualization to reuse:** `Map` tile markers (`Tile::setMarkerColor`, path preview) and the DX
  targeting-preview dot machinery ([Feature-TargetingVisualization.md](Feature-TargetingVisualization.md)).

## DX deltas vs legacy

1. **Cone, not radius.** Overwatch watches a **directional cone** from the unit toward an aimed tile,
   with a configurable **max range**, **cone angle**, and optional **minimum range** (near dead zone),
   instead of a circular radius around a point.
2. **Trigger-tile markers.** While setting up overwatch, draw markers on **every tile the cone covers**
   (i.e. every tile an enemy could stand on to trigger the shot), so the player sees exactly what's
   watched.

## Cone model (proposed)

A tile **T** is *watched* (can trigger overwatch) when all hold:
- `overwatchMinRange ≤ distance(unit, T) ≤ overwatchRange`,
- the angle between `unit→T` and `unit→aimTile` is `≤ overwatchConeAngle / 2` (the field is the full
  cone width), so the cone **widens with range** from the muzzle; the min-range carves out a near wedge,
- a **line of fire** exists to T (not wall-blocked) — reusing the LOF/voxel test the aim preview uses.

The player picks the cone by aiming a tile (like choosing a shot target); direction = `unit→aimTile`.

## Per-weapon rules (RuleItem) — proposed

| Field | Default | Meaning |
|---|---|---|
| `overwatchRange` | **0** | Max watched distance (tiles). **0 = overwatch unavailable** (opt-in): a weapon enables it by setting `> 0`, and the menu option is hidden otherwise. |
| `overwatchMinRange` | 0 | Near dead zone; 0 = watch from adjacent out. |
| `overwatchConeAngle` | 40 | **Full** cone angle in degrees (the total width of the watched wedge). |
| `overwatchShot` | `snap` | Fire mode used for the overwatch shot (snap/burst/auto/aimed). |
| `overwatchModifier` | 100 | Percent applied to the offensive reaction score for overwatch. |

**Mod-configurable defaults.** These per-weapon defaults are themselves overridable mod-wide via a
top-level `overwatchDefaults:` node (same keys), resolved getter-side against the static
`RuleItem::overwatchDefaults` (a weapon's own field wins). So a mod can, e.g., raise the default
`overwatchRange` to give every firearm overwatch, or change the default shot mode, without editing
each item.

**Hotkey.** The overwatch action-menu item is bound to `keyBattleActionItem8` (default **O**).

## State & flow (proposed)

- **New `BA_OVERWATCH` action** (next free `BattleActionType`, = 23) shown for firearms that can
  reaction-fire. Selecting it enters an **aim mode**; the player clicks a tile to set the cone
  direction, the cone-tile markers draw live, and confirming spends the TU and enters overwatch.
- **`BattleUnit` state:** `_overwatch` (active), `_overwatchTarget` (aim tile), `_overwatchWeapon`
  /slot, `_overwatchShotsAvailable`; cone params read from the weapon. All persisted in the save.
- **Cost / shots:** entering costs the overwatch reservation TU; `_overwatchShotsAvailable` = shots
  affordable from that reserve (legacy `max(actionTU, 90% baseTU)` basis). Each trigger decrements it;
  overwatch ends when it hits 0.
- **Trigger (enemy turn):** in `checkReactionFire`, an overwatching unit also fires if the acting
  enemy's tile is **within its cone** (+ LOF + shots left), exempt from the normal spot-to-protect
  restriction. Uses `overwatchShot` and the overwatch-modified reaction score.
- **Persistence:** overwatch is armed for the **upcoming enemy turn** and cleared at the **start of
  the owner's next turn** (also cleared early if shots run out or the player cancels). No cross-turn
  TU upkeep — it's a fresh per-round commitment.
- **Markers:** cone tiles drawn while aiming, **and** when a unit already on overwatch is reselected
  (coverage review). Reuses the `Map` tile-marker overlay with a dedicated colour.

## Touch points (anticipated)

- `src/Mod/RuleItem.*` — the five `overwatch*` fields.
- `src/Mod/RuleItem.h` — `BA_OVERWATCH` enum value.
- `src/Savegame/BattleUnit.*` — overwatch state + save/load; `getReactionScore` overwatch modifier.
- `src/Battlescape/ActionMenuState.*` — the `BA_OVERWATCH` menu item + aim entry.
- `src/Battlescape/BattlescapeGame.*` — action dispatch, TU/shot accounting, turn upkeep/clear.
- `src/Battlescape/TileEngine.*` — cone test folded into `checkReactionFire`.
- `src/Battlescape/Map.*` — cone-tile marker overlay while aiming/reviewing.
- `bin/common/Language/DX/en-US.yml` — `STR_OVERWATCH` etc.; `bin/standard/dx-test/dx-test.rul` — a
  test weapon with a tuned cone.
- Docs: `DX-Features.md`, this doc, `DX-Roadmap.md`.

## Suggested build order

1. **Rules + state (no behaviour):** `overwatch*` RuleItem fields, `BA_OVERWATCH`, `BattleUnit`
   overwatch members + save/load, and the cone-membership test (`isTileInOverwatchCone`) as a pure
   helper — unit-testable in isolation.
2. **Entering + markers:** the `BA_OVERWATCH` action-menu item, aim mode, TU/shot reservation, and the
   `Map` cone-tile overlay (drives the "does the geometry look right" check first).
3. **Triggering:** fold the cone test into `checkReactionFire` (exempt from spot-to-protect), shot
   accounting/decrement, and the per-turn clear.
4. **Polish:** overwatch-unit on-map indicator, `dx-test` weapon, docs.

## Follow-ons / notes

- **On-map per-unit indicator** — a glyph over units currently on overwatch (fits the floor
  status-indicator system, needs a sprite). Deferred; the cone markers already show coverage when the
  overwatch unit is selected.
- **Per-tile line-of-fire on the markers** — the cone markers are geometry-only for now (they can show
  through walls); the *trigger* already requires LOF (`canTargetUnit`/`visible`), so only the preview
  is optimistic. Add a cached LOF filter to the marker computation.
- **Move/act clear** — overwatch clears at the owner's next turn (and when shots run out). Because
  entering reserves most of the unit's TU, mid-turn re-use is already limited; an explicit clear on a
  post-overwatch move could be added.
- **AI use of overwatch** is out of scope for the first cut (AI keeps normal reaction fire); can be
  added later.
- **Vertical (z) cone** — first cut treats the cone in the horizontal plane; a full 3D cone can come
  later if needed.
