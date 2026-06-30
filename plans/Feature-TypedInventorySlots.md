# Feature: Typed Inventory Slots (filtering / combat-lock / move-cost / stat-gating)

**Status:** Implemented (Jun 2026). Builds clean (Release/Win32). Three `RuleInventory` fields
shipped (`battleType`, `allowCombatSwap`, `countStats`); the planned fourth, `allowGenericItems`, was
**dropped** (see "Dropped: `allowGenericItems`" below). The `costs` allow-list uses the lenient Q2(a)
semantics (explicit `-1` forbids). Enforcement also covers the ctrl-click quick-equip and
ctrl-click-to-ground paths, not just manual drops.

## Resolved decisions (Jun 2026)

- **Q1 — `countStats` default = `true`.** Preserves the shipped *Item Stats Modifiers* behavior (all
  equipped items grant their bonuses); `countStats: false` marks holster/display slots that grant
  nothing.
- **Q2 — `costs` gating = explicit `-1` forbids.** Keep the lenient `DEFAULT_MOVE_COST` fallback for
  unlisted slot-pairs (custom layouts keep working); only an **explicit `-1`** cost forbids that
  in-combat move (`STR_INVALID_TRANSFER`).
- **Q3 — Scope = four `RuleInventory` fields only.** `INV_UTILITY` utility slots remain a separate
  follow-on checklist item.

Checklist item: *Phase 4 — Typed slots / filtering / move-cost — `battleType`, `allowCombatSwap`,
`costs`, `countStats`.* This is the unlock for the follow-on **Utility equipment slots**
(`INV_UTILITY`) item, which is intentionally kept out of scope here.

## Summary

Turn an inventory section (`RuleInventory`) from a generic pocket into a **typed, role-specific
socket** by adding per-slot rules:

1. **`battleType`** — restrict a slot to one item battle type (e.g. an armor-only or ammo-only slot).
2. **`allowCombatSwap`** — make a slot **loadout-only**: items can be arranged before a mission but
   are locked in/out once combat is underway.
3. **`costs`** as an in-combat **transfer allow-list** (the move-cost map already exists; this adds
   the gating semantics).
4. **`countStats`** — gate which slots' items contribute their `stats`/`statModifiers` bonuses to the
   wearer, so "display/holster" slots can grant no bonus.

This re-ports the legacy DX typed-slot behavior (documented in `Legacy-DX-Features.md` §10) onto the
current OXCE-Plus base, reconciling it with features OXCE/DX already ship.

## OXCE / OXCE-Plus + DX audit (Jun 2026)

What the **current base already provides** (so we only build the true delta):

- **Item-side slot restriction already exists.** `RuleItem.supportedInventorySections` +
  `RuleItem::canBePlacedIntoInventorySection()` ([RuleItem.cpp:1132](../src/Mod/RuleItem.cpp#L1132))
  already restrict which sections an item type may occupy, enforced at placement
  ([Inventory.cpp:1081](../src/Battlescape/Inventory.cpp#L1081) and
  [:1905](../src/Battlescape/Inventory.cpp#L1905), and in the ctrl-click auto-place candidate scan at
  [:1961](../src/Battlescape/Inventory.cpp#L1961)). The new slot-side **`battleType`** is the
  *complement* of this — restricting from the slot side so an armor/ammo slot works without editing
  every item's `supportedInventorySections`. The two compose (both must pass).
- **`costs` map already exists** on `RuleInventory` (per-destination move TU). `getCost` currently
  returns a `DEFAULT_MOVE_COST = 8` fallback for unlisted pairs (a DX hardening for custom layouts
  that don't fully specify costs — [RuleInventory.cpp:246](../src/Mod/RuleInventory.cpp#L246)). It is
  **not** used as an allow-list today. The move-cost is charged at
  [Inventory.cpp:1242](../src/Battlescape/Inventory.cpp#L1242) via `BattleItem::getMoveToCost`.
- **Item stat bonuses already apply from every slot.** `BattleUnit::computeEffectiveBaseStats`
  ([BattleUnit.cpp:4475](../src/Savegame/BattleUnit.cpp#L4475)) sums `getStats()`/`getStatModifiers()`
  over **all** inventory items that have a slot (DX's shipped *Item Stats Modifiers*, §9). There is
  no per-slot gating yet — `countStats` adds it.
- **In-combat vs pre-battle is already distinguishable.** `InventoryState` carries `_tu`
  ([InventoryState.h:59](../src/Battlescape/InventoryState.h#L59)): `false` in the pre-battle equip /
  base inventory screen (no TU charged), `true` once the battle is live. `_tu == true` is the natural
  "combat is underway" signal for `allowCombatSwap` and the `costs` allow-list.
- **Enum has no utility/equip types.** `enum InventoryType { INV_SLOT, INV_HAND, INV_GROUND }`
  ([RuleInventory.h:33](../src/Mod/RuleInventory.h#L33)). `INV_UTILITY`/`INV_EQUIP` are a separate
  follow-on feature.
- **No upstream equivalent** for slot-side `battleType` / `allowCombatSwap` / `countStats` — these are
  genuine DX deltas.

## Proposed design

### New `RuleInventory` fields (parsed in `load`)

| Field | Type | Default | Meaning |
|---|---|---|---|
| `battleType` | `BattleType` | `BT_NONE` | If set, only items whose `battleType` matches may be placed. `BT_NONE` = anything fits. |
| `allowCombatSwap` | bool | `true` | When `false`, items can't be moved **into or out of** this slot while combat is underway (`_tu`). |
| `countStats` | bool | **see Q1** | Whether items in this slot contribute `stats`/`statModifiers` to the wearer. |

`costs` is already parsed; only its *enforcement* semantics change (see Q2).

### Enforcement points

- **`battleType`**: extend the two placement gates ([Inventory.cpp:1081](../src/Battlescape/Inventory.cpp#L1081),
  [:1905](../src/Battlescape/Inventory.cpp#L1905)) and the auto-place candidate scan
  ([:1961](../src/Battlescape/Inventory.cpp#L1961)) with a slot-`battleType` check. New warning
  `STR_INVALID_ITEM_SLOT` (DX language file). Put the check in a small `RuleInventory` helper
  (`canAcceptBattleType(const RuleItem*)`) so all callers share it.
- **`allowCombatSwap`**: in the same placement gates, when `_tu` (combat) and either the source or
  destination slot has `allowCombatSwap == false`, reject with `STR_NOT_COMBAT_SWAPPABLE`. Also tint
  combat-locked slot labels differently (optional polish; legacy did this).
- **`costs` allow-list** (per Q2): if a destination's cost resolves to "forbidden", reject the
  in-combat move with `STR_INVALID_TRANSFER`.
- **`countStats`**: in `computeEffectiveBaseStats`, only add an item's stats/modifiers when
  `item->getSlot()->getCountStats()` (per Q1 default).

### Files touched

- [src/Mod/RuleInventory.h/.cpp](../src/Mod/RuleInventory.cpp) — fields, getters, `load`, helper,
  script consts.
- [src/Battlescape/Inventory.cpp](../src/Battlescape/Inventory.cpp) — placement + cost enforcement.
- [src/Savegame/BattleUnit.cpp](../src/Savegame/BattleUnit.cpp) — `countStats` gating in
  `computeEffectiveBaseStats`.
- `bin/common/Language/DX/en-US.yml` — `STR_INVALID_ITEM_SLOT`, `STR_NOT_COMBAT_SWAPPABLE`,
  `STR_INVALID_TRANSFER`.
- No save-format change (all rule-side; runtime state unchanged).

## Open questions (RESOLVED — see "Resolved decisions" at the top; kept for rationale)

**Q1 — `countStats` default.** Legacy DX defaulted it **false** (stat-granting was opt-in per slot).
But DX already shipped *Item Stats Modifiers* where **every** equipped item grants its stats. A
`false` default would silently disable that. Options: (a) default **true** — preserve current
behavior, use `countStats: false` to make holster/display slots; (b) default **false** — match
legacy, requires marking the standard slots `countStats: true`.

**Q2 — `costs` allow-list semantics.** Legacy: an unlisted destination returns −1 and is **rejected
in combat** (`STR_INVALID_TRANSFER`); our current code instead falls back to `DEFAULT_MOVE_COST = 8`
for unlisted pairs (needed so custom inventory layouts that don't fully specify `costs` still work).
Options: (a) keep lenient fallback, only an **explicit `-1`** in `costs` forbids the move; (b) adopt
strict legacy (unlisted = forbidden in combat) — but that breaks partially-specified custom layouts.

**Q3 — Scope.** Confirm this feature is the **four `RuleInventory` fields only**, with `INV_UTILITY`
(utility equipment slots) left to its own follow-on checklist item.

## Dropped: `allowGenericItems`

Legacy DX had a fourth slot field, `allowGenericItems`, which we initially parsed-but-kept-dormant
for forward-compat. **We removed it** rather than ship a dead key, because its intended purpose was
never clear. What we do know: the original legacy gating (which was **commented out** in the live
build) worked item-side — it first checked whether the item declared specific slots it was allowed
in, and **only if it didn't** did it fall back to consulting this slot flag. In other words
`allowGenericItems` governed whether a slot would accept an item that had *no* slot restriction of
its own (a "generic" item). That whole path was superseded by today's `supportedInventorySections`
(item side) + `battleType` (slot side) filters, so the flag had no live role. If a real need for
"reject items that don't explicitly opt into this slot" resurfaces, reintroduce it as an explicit,
enforced field then.

## Out of scope

- `INV_UTILITY` / `INV_EQUIP` slot types and per-unit utility-slot plumbing (separate feature).
- Sided/directional armor slots (uses `battleType: BT_ARMOR` once this lands; armor application itself
  already shipped — see `Feature-DirectionalArmorOnItems.md`).
