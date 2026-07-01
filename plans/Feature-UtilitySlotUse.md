# Feature: Use Utility-Slot Items in Battle (the utility-slot consumer)

**Status:** Implemented (Jun 2026). Builds clean (Release/Win32). Builds on the shipped *Utility
Equipment Slots* plumbing ([Feature-UtilityEquipmentSlots.md](Feature-UtilityEquipmentSlots.md)) —
this is the first **consumer** of `BattleUnit::getUtilityItem()`. Shipped as **hotkey-only**
(`keyBattleUseUtility`, default `Z`); any utility item's actions are offered (the slot's
`battleType` already gates placement). No HUD button (deferred).

## Goal

Let the item sitting in a unit's `INV_UTILITY` slot be **used/activated in battle without moving
it to a hand** — e.g. a medikit in the utility slot heals in place. This realizes the
"usable but not swappable" idea: with the slot `allowCombatSwap: false`, the item can't be moved
in/out during combat, yet its action (BA_USE/medikit, BA_PRIME, BA_THROW, scanner, etc.) is still
available.

## OXCE / DX audit (Jun 2026) — what already works

A source audit found the activation machinery is **already slot-agnostic**, so the feature is small:

- **`ActionMenuState` reads only `_action->weapon->getRules()`**
  ([ActionMenuState.cpp:59-200](../src/Battlescape/ActionMenuState.cpp#L59)) — it enumerates
  `BA_USE`/`BA_PRIME`/`BA_THROW`/shots/melee purely from the item's `RuleItem`, never from its
  inventory slot or hand status. Whatever `BattleItem` is in `_currentAction.weapon` gets its
  actions.
- **`handleItemClick(BattleItem*)`** ([BattlescapeState.cpp:2546](../src/Battlescape/BattlescapeState.cpp#L2546))
  is the universal entry: it sets `_currentAction.weapon = item` and pushes `ActionMenuState`.
- **The medikit path doesn't require a hand** — `BA_USE` on a `BT_MEDIKIT` operates on
  `_action->weapon` + target, spends TU, opens `MedikitState` for manual heal; the item's slot is
  irrelevant.
- **Existing precedent for "use an item not in a hand":** the OXCE **special-weapon button**
  (`btnSpecialClick`, [BattlescapeState.cpp:1883](../src/Battlescape/BattlescapeState.cpp#L1883),
  key `keyBattleUseSpecial`) and the **hand-item hotkeys** (`btnLeftHandItemClick` /
  `btnRightHandItemClick`, keys `keyBattleUseLeftHand`/`RightHand`) all just fetch a `BattleItem`
  and call `handleItemClick()`. The special-weapon path fetches a non-inventory armor weapon and
  works seamlessly.
- **`getUtilityItem()` already mirrors `getRightHandWeapon()`** — returns a `BattleItem*` or null,
  fully null-safe ([BattleUnit.cpp](../src/Savegame/BattleUnit.cpp)). No new accessor needed.

**Conclusion:** the only missing piece is a **trigger** that calls
`handleItemClick(getSelectedUnit()->getUtilityItem())`. No changes to `ActionMenuState`,
`MedikitState`, or `RuleItem`.

## Design

### Trigger: a dedicated battlescape hotkey (Path A)

Add `keyBattleUseUtility`, mirroring `keyBattleUseSpecial` exactly:

1. **`src/Engine/Options.inc.h`** — declare `keyBattleUseUtility` in the `keyBattle*` block.
2. **`src/Engine/Options.cpp`** — register it:
   ```cpp
   _info.push_back(OptionInfo(OPTION_OXCE, "keyBattleUseUtility", &keyBattleUseUtility,
       SDLK_r, "STR_USE_UTILITY_ITEM", "STR_BATTLESCAPE"));
   ```
   (Default key TBD — see Open Questions. `STR_USE_UTILITY_ITEM` appears in the adjustable-controls
   list under the Battlescape group, so it's rebindable in Options.)
3. **`src/Battlescape/BattlescapeState`** — new handler mirroring `btnSpecialClick`:
   ```cpp
   void BattlescapeState::btnUtilityItemClick(Action *action)
   {
       if (playableUnitSelected())
       {
           if (_battleGame->getCurrentAction()->targeting) { _battleGame->cancelCurrentAction(); return; }
           BattleItem *item = _save->getSelectedUnit()->getUtilityItem();
           if (!item) return;
           _map->draw();
           bool middleClick = _game->isMiddleClick(action, true);
           handleItemClick(item, middleClick);
       }
   }
   ```
   Wire the key in the ctor. Because there's no HUD widget bound to it, attach the
   `onKeyboardPress` to an always-present surface (e.g. reuse the pattern used for keys that have
   no dedicated button, or bind on `_btnStats`/the map like other keyboard-only handlers). Confirm
   the exact host widget during implementation.
4. **`src/Battlescape/ActionMenuState.cpp:323`** — add `keyBattleUseUtility` to the set of keys
   that close the action menu when pressed again (so the utility key toggles like the hand keys).
5. **`bin/common/Language/DX/en-US.yml`** — `STR_USE_UTILITY_ITEM: "Use Utility Item"`.

### Why not extend a hand hotkey (Path B, rejected)

`btnRightHandItemClick` could fall back to `getUtilityItem()` when the hand is empty, but that
mixes hand and utility semantics, is undiscoverable, and would surprise players whose hand is
simply empty. A dedicated key is clearer and matches how special weapons got their own key.

### Optional follow-up: HUD button

The hand items and special weapon each have a clickable HUD widget in addition to a hotkey; the
utility slot would have only a hotkey. A dedicated on-screen button (showing the utility item's
icon, like the special-weapon button) is more discoverable but needs battlescape HUD layout +
sprite work. Deferred unless wanted now — see Open Questions.

## Interaction with `allowCombatSwap: false` and throw-gating

In-place actions (BA_USE/medikit, BA_PRIME, scanner, etc.) **don't move the item between slots**,
so a combat-locked utility slot still permits *use*, just not *rearrangement* — the intended
"usable but not swappable" behavior.

**BA_THROW is the exception and is gated** ([ActionMenuState.cpp](../src/Battlescape/ActionMenuState.cpp)
throw block in the ctor): throwing *removes* the item from its slot, which would bypass the
combat-swap lock. So the throw action is withheld from the action menu when the item's slot either
(a) is combat-locked (`allowCombatSwap: false`) — throwing out of a locked slot is a "move out" the
lock forbids — or (b) is an `INV_UTILITY` slot — utility gear is meant to be used in place, not
thrown. A normal swappable, non-utility hand/belt/ground item throws as usual.

## Files touched

- `src/Engine/Options.inc.h`, `src/Engine/Options.cpp` — new key option.
- `src/Battlescape/BattlescapeState.h/.cpp` — `btnUtilityItemClick` + key wiring.
- `src/Battlescape/ActionMenuState.cpp` — utility key also closes the menu; **throw-gating** for
  combat-locked and utility slots.
- `bin/common/Language/DX/en-US.yml` — `STR_USE_UTILITY_ITEM`.
- No save-format change; no `RuleItem`/`MedikitState` change.

## Resolved decisions (Jun 2026)

1. **Default keybind = `SDLK_z`** (user preference). `Z` is nominally `keyInvAutoEquip`
   (`STR_AUTO_EQUIP`), but that shortcut lives on the **inventory** screen while the utility-use
   key lives on the **battlescape** State, so they never collide at runtime (same context split as
   the other inventory-only `c/v/x/z` keys). Registered as `OPTION_DX` in `createControlsDX`, so it
   appears rebindable under the Battlescape controls group.
2. **Hotkey only** — no HUD button this pass (deferred; would need battlescape HUD layout + a
   sprite showing the utility item's icon).
3. **Any usable item** — the handler just routes `getUtilityItem()` into `handleItemClick`; the
   action menu offers whatever actions the item's `RuleItem` supports. The slot's `battleType`
   filter is the gate on what can be placed there.

### Implementation notes

- The keyboard handler is hosted on `_btnStats->onKeyboardPress(...)` (the same catch-all widget
  the reload / personal-lighting / night-vision keyboard-only shortcuts use), since the utility
  slot has no dedicated HUD widget.
- `keyBattleUseUtility` was added to the `ActionMenuState` "close on second press" key set (with
  `keyCancel` / left-hand / right-hand), so tapping `U` again dismisses the menu.
- `btnUtilityItemClick` mirrors `btnSpecialClick`: cancel-if-targeting, fetch the item, `_map->draw()`,
  `handleItemClick`, consume the event.

## Out of scope

- HUD button/icon (unless chosen in Q2).
- Auto-activate-without-menu (the PSI-button pattern) — the action menu is the consistent path.
- Utility slots for AI units (this is a player-input trigger; AI utility use is a separate concern).
