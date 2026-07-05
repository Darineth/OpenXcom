# Feature: Weight-based reload/unload TU cost

**Status:** **Implemented (Jul 2026)**, builds clean (0 warnings). Phase 6 (Ammo & Reloading).
Re-derived from the legacy DX fork (Legacy-DX-Features.md §4, "Adjust Reload Costs"). Standalone
`battleWeightBasedReloadCost` option. Faithful to the legacy intent, which *replaced* the flat 15
base reload cost with a weight-based one (rather than stacking on top): the weight term
`getReloadWeightCost()` = `weight*2` replaces the slot-path base term (`extendedItemReloadCost`),
and the weapon's `tuLoad`/`tuUnload` **default drops from 15/8 to 5** while the option is on (an
explicit ruleset value still wins). So a default weapon reloads for `weight*2 + 5`. Applied at all
reload/unload/AI sites via the shared getters.

## Resolved design decisions

1. **Formula:** the weight term is `magazineWeight * 2` (2 TU per unit of magazine weight),
   `BattleItem::getReloadWeightCost()`. The old flat `+5` overhead is *not* baked into the weight
   term — instead it becomes the reduced base default (see below), matching the legacy intent that the
   weight cost **replaces** the flat base rather than adding to it.
2. **Base default:** while the option is on, an *unset* `tuLoad`/`tuUnload` defaults to **5** (down
   from the stock 15/8); a weapon that sets `tuLoad`/`tuUnload` explicitly keeps its own value as the
   basis. Implemented by re-sentinelling `_tuLoad`/`_tuUnload` to `-1` ("unset") and applying the
   default in `RuleItem::getTULoad`/`getTUUnload` (which are option-aware). So a default weapon
   reload = `5 + weight*2`; a weapon with `tuLoad: 20` reloads for `20 + weight*2`.
3. **Model:** the weight term **replaces the "base" handling cost** — the OXCE `extendedItemReloadCost`
   inventory-slot move term (`getMoveToCost`) where that applies — so when the DX option is on it
   takes precedence and the two don't stack. Incidental inventory moves (moving the weapon body into a
   hand) are unchanged.
4. **Option:** a standalone DX battlescape option `battleWeightBasedReloadCost` (**default off**),
   independent of `extendedItemReloadCost`.

## Motivation

Reloading a heavy magazine should cost more time than snapping in a light one. Today reload/unload
TU is a flat per-weapon number (`tuLoad`/`tuUnload`, defaults 15/8) plus — with OXCE's
`extendedItemReloadCost` — a flat per-inventory-slot move cost. Nothing scales with how heavy the
magazine actually is, so a 3-weight pistol clip and a 12-weight autocannon drum reload identically.
DX adds the magazine's **weight** as a term in the reload/unload cost.

## Audit — current OXCE-Plus reload cost model (the delta)

Reload/unload TU is assembled at four sites, all from the same two ingredients:

- **Flat rule cost:** `RuleItem::getTULoad(slot)` / `getTUUnload(slot)` (defaults 15 / 8), stored per
  ammo slot.
- **Slot-path cost (OXCE `extendedItemReloadCost`):** `BattleItem::getMoveToCost(slot)` →
  `RuleInventory::getCost(fromSlot, toSlot)`, a **flat per-slot-pair** TU value from the inventory
  ruleset `costs:` map, scaled by the item's `inventoryMoveCost.basePercent` (default 100%).
  **Weight is not part of this** — moving a feather and an anvil between the same two slots costs the
  same.

Sites:
1. Inventory drag-load ([Inventory.cpp:1172-1222](../src/Battlescape/Inventory.cpp#L1172)):
   `getTULoad` + (extended) move-ammo-to-offhand + (quick-swap/Shift) `getTUUnload` + drop-old-to-ground.
2. Inventory unload ([Inventory.cpp:1501](../src/Battlescape/Inventory.cpp#L1501)): `getTUUnload(slot)`.
3. Battlescape auto-reload ([BattlescapeGame.cpp:2926-2928](../src/Battlescape/BattlescapeGame.cpp#L2926)):
   (extended) `getMoveToCost` + `getTULoad`.
4. AI reload estimate ([BattleUnit.cpp:3961-3962](../src/Savegame/BattleUnit.cpp#L3961)):
   (extended) `getMoveToCost` + `getTULoad`.

**Conclusion:** the slot-path half of the legacy "Adjust Reload Costs" already ships as
`extendedItemReloadCost`. The DX work is purely **adding a weight term** at these sites. The legacy
Quick Reload *action* is a separate Phase 6 item; this cost model will apply to it automatically once
that action exists (it goes through the same reload plumbing), so there is no hard dependency —
weight cost lands on the existing load/unload paths now.

## Design (proposed — pending Open questions)

- New DX battlescape option `battleWeightBasedReloadCost` (**default off**), following the DX option
  convention (`OptionInfo(OPTION_DX, ...)`, declared in `Options.inc.h`, registered in `Options.cpp`,
  label strings in `Language/DX/en-US.yml`). When off, reload cost is byte-for-byte current behavior.
- When on, add the **magazine's weight** to load and unload TU at all four sites above. "Magazine
  weight" = the ammo `BattleItem`'s weight (`getRules()->getWeight()`; a bare clip's own weight —
  ammo weight in OpenXcom is per-clip, not per-round).
- The weight term is **added on top of** the existing `tuLoad`/`tuUnload` (+ slot path), not a
  replacement — least disruptive, and mods can lower `tuLoad`/`tuUnload` if they want weight to
  dominate. *(Alternative — replace the flat cost — is an Open question.)*
- Scaling: a global tunable so the weight→TU mapping can be balanced without touching every item —
  e.g. `weightCost = round(ammoWeight * battleReloadWeightCostPercent / 100)`, default 100 (1 TU per
  weight unit, matching the legacy literal "+ Magazine Weight"). *(Whether to expose the percentage
  is an Open question.)*

## Touch points

- `Options.inc.h` / `Options.cpp` — declare + register `battleWeightBasedReloadCost` (and optional
  `battleReloadWeightCostPercent`).
- A small shared helper for "weight surcharge for this ammo item" so the four sites stay in sync
  (candidate: a free function or `BattleItem::getReloadWeightCost()`).
- The four cost sites above (load ×2 paths, unload, battlescape reload, AI estimate). Keep the AI
  estimate in sync so the AI doesn't mis-budget reloads.
- `Language/DX/en-US.yml` — `STR_*` option label + description.
- Docs: `DX-Features.md` entry, this doc's status, `DX-Roadmap.md` checkbox.

## Testing plan

- dx-test: with the option on, confirm heavier ammo (e.g. a high-`weight` clip) costs visibly more
  TU to load/unload than a light one, and that the pre-battle inventory TU tooltip reflects it.
- Confirm option **off** = unchanged costs (regression).
- Confirm the battlescape mid-turn reload and the AI both see the surcharge (AI still reloads sanely).
- Build clean with `-Wall -Wextra`.

## Open questions (need answers before coding)

1. **Weight→TU mapping:** raw ammo weight added 1:1 (legacy-literal), or scaled by a tunable global
   percentage (moddable balance knob)?
2. **On top of, or replacing, the flat `tuLoad`/`tuUnload`?** Adding on top is least disruptive;
   replacing matches the legacy "cost = Weight + slot path" phrasing more literally.
3. **Option shape:** standalone `battleWeightBasedReloadCost` (independent toggle, DX convention), or
   fold the weight term into the existing `extendedItemReloadCost` so slot-path + weight travel
   together?
