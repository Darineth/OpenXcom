# Feature: Edit Soldier Inventories from New Battle

**Status:** Implemented (Jun 2026).

## Outcome (as shipped)

The inventory machinery already existed and worked; it was only gated off for New Battle. Un-gated at
both entry points (and verified the build/persistence path works under the `monthsPassed == -1` save):

- **Equip Craft → Equipment → Inventory** ([CraftEquipmentState.cpp](../src/Basescape/CraftEquipmentState.cpp)):
  the Inventory button is now visible in New Battle (`setVisible(craftHasACrew)`), and the bottom row
  was re-laid-out for New Battle so the **Unload Craft** and **Inventory** buttons coexist (narrower
  filter combo, `Inventory` x128 w64, `Clear` x194 w78). Uses the craft's items.
- **Soldier Info → Inventory** ([SoldierInfoState.cpp](../src/Basescape/SoldierInfoState.cpp)): the
  Inventory button was split out of the `showNastyButtons` group (which keeps its `months > -1` gate
  for Sack/Craft/Transformations) so it shows in New Battle too. Uses `runInventory(0)` over the base
  soldiers; New Battle stocks base storage, so items are available.

In both paths, exiting the inventory saves each soldier's `EquipmentLayout`, and starting the battle
re-applies it, so edited loadouts carry into the fight. No changes were needed to `runInventory`, the
inventory screen, or the start-battle path.

## Summary

Let the player open the per-soldier **inventory** screen from the New Battle setup and arrange
loadouts (drag weapons/ammo/items onto soldiers) **without starting the battle**, then start the
battle with those loadouts. Today the inventory step is reachable for real geoscape missions but is
explicitly disabled in New Battle mode.

## OXCE / OXCE-Plus audit (Jun 2026)

The user's hypothesis is correct: New Battle is backed by a real `SavedGame`, and the inventory
mechanism already exists — it is just gated off for New Battle.

- **New Battle builds a save.** `NewBattleState::initSave()` creates a `SavedGame` (with
  `monthsPassed == -1`), a `Base`, a `Craft`, and soldiers, and calls `_game->setSavedGame(save)`
  ([NewBattleState.cpp:505](../src/Menu/NewBattleState.cpp#L505)). `monthsPassed == -1` is the
  engine's "new battle" sentinel.
- **The craft-management chain is already reachable.** `NewBattleState::btnEquipClick` pushes
  `CraftInfoState` ([NewBattleState.cpp:741](../src/Menu/NewBattleState.cpp#L741)) → Equipment →
  `CraftEquipmentState`.
- **The inventory build mechanism already exists** in `CraftEquipmentState::btnInventoryClick`:
  create a `SavedBattleGame`, `BattlescapeGenerator::runInventory(craft)`, then push
  `InventoryState(false, 0, _base)` (base/equip mode — no battle). This is exactly the pre-mission
  inventory used from the geoscape.
- **It is explicitly disabled for New Battle.** The Inventory button is
  `setVisible(craftHasACrew && !_isNewBattle)`
  ([CraftEquipmentState.cpp:134](../src/Basescape/CraftEquipmentState.cpp#L134)), where
  `_isNewBattle = (getMonthsPassed() == -1)`. `btnInventoryClick` and the surrounding item-management
  logic are also wrapped in many `!_isNewBattle` guards (the `oxceAlternateCraftEquipmentManagement`
  "move base items to craft" trick is skipped for New Battle, since New Battle has no base storage
  model — items are added straight to the craft with unlimited availability + a Clear button).

Conclusion: this is **not** a from-scratch feature. The inventory screen, the `runInventory` build,
and the base/equip `InventoryState` all work; the task is to **un-gate** the Inventory button for New
Battle and make the build + persistence behave with the New-Battle save model.

## Proposed approach

1. **Show the Inventory button in New Battle** — drop the `!_isNewBattle` from the visibility check
   (keep the `craftHasACrew` requirement).
2. **Make `btnInventoryClick` work in New Battle** — it already constructs a `SavedBattleGame` +
   `runInventory(craft)` + `InventoryState`; the `oxceAlternateCraftEquipmentManagement` block is
   already `!_isNewBattle` (correctly skipped, since New Battle items live on the craft, not in base
   storage). Verify `runInventory` produces the soldiers + craft items as the inventory's ground
   contents under a `monthsPassed == -1` save.
3. **Persistence** — leaving the inventory must write each soldier's loadout back so that pressing OK
   in New Battle starts the battle with it. The geoscape flow saves the soldiers' `EquipmentLayout`
   (`InventoryState::saveEquipmentLayout`) on exit and `BattlescapeGenerator` re-applies it at mission
   start. Confirm New Battle's `btnOkClick` path (`BattlescapeGenerator`) consumes the same layout, and
   that returning from the inventory leaves `CraftEquipmentState` consistent (its `init()` reconciles
   craft items after the inventory in the non-New-Battle path; the New-Battle path must not strip the
   edited loadout).
4. **Item availability** — see open question; default is to distribute the items already on the craft
   (same as geoscape), which the New Battle equipment screen lets you add freely.

Key files: [CraftEquipmentState.cpp](../src/Basescape/CraftEquipmentState.cpp) (the `!_isNewBattle`
guards around the inventory button + post-inventory `init()` reconciliation), and verification of
[BattlescapeGenerator] `runInventory` and the New-Battle `btnOkClick` start path.

## Resolved decisions

1. **Item availability: craft items only** (parity with the geoscape pre-mission inventory). The New
   Battle equipment screen already lets you add any items to the craft freely first; the inventory
   then distributes those craft items onto soldiers.
2. **Persistence: hold for the session** (until OK starts the battle), matching how the New Battle
   config currently behaves. (Revisit later if saving into the New-Battle template is wanted.)
3. **`oxceAlternateCraftEquipmentManagement`: keep disabled in New Battle** — no base storage to
   shuffle; the inventory uses the craft items directly.

## Risks / things to verify

- Why was it originally gated off? Likely just because New Battle's item model differs (no base
  storage) and the alternate-management trick assumed base storage. Need to confirm `runInventory`
  and the post-inventory `CraftEquipmentState::init()` reconciliation don't misbehave under
  `monthsPassed == -1`.
- Ensure no double-free / state leak from creating a `SavedBattleGame` for the inventory and then
  starting (or cancelling) the actual battle afterwards.
- Soldier loadout must actually reach the started battle (equipment layout applied by
  `BattlescapeGenerator` at New-Battle start).

## Out of scope

- No redesign of the New Battle screen layout.
- No changes to how the geoscape pre-mission inventory works.
