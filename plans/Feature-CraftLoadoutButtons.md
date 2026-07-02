# Feature: Visible Craft Loadout Save/Load Buttons

**Status:** Implemented (Jun 2026).

## Summary

Surface the craft equipment **loadout template** save/load — which OXCE-Plus ships only as hidden
hotkeys — as real on-screen buttons on the craft equipment screen
([CraftEquipmentState](../src/Basescape/CraftEquipmentState.cpp)). The backing feature (10 named
per-craft loadout slots, save/load states) is fully present; this is purely a **UI-discoverability**
add: wire buttons to the existing handlers and fit them into the bottom button row. Keep the hotkeys.

This follows the DX design principle of preferring **visible, discoverable UI over hidden,
hotkey-only features** (OXCE's default framing). See `DX-Roadmap.md` → New Features.

## OXCE / OXCE-Plus + DX audit (Jun 2026)

The whole mechanism already exists — only the on-screen entry point is missing:

- **Handlers exist.** `CraftEquipmentState::btnSaveClick` / `btnLoadClick` open
  `CraftEquipmentSaveState` / `CraftEquipmentLoadState`, which manage **10 named craft loadout slots**
  (`MAX_CRAFT_LOADOUT_TEMPLATES`), each an `ItemContainer` of item type → qty (including HWPs),
  persisted to the save (`globalCraftLoadout0..9` + names).
- **Bound only to hotkeys.** In `init()` the handlers are attached solely as keyboard presses on
  `_btnOk` ([CraftEquipmentState.cpp:129-130](../src/Basescape/CraftEquipmentState.cpp#L129-L130)):
  ```cpp
  _btnOk->onKeyboardPress((ActionHandler)&CraftEquipmentState::btnLoadClick, Options::keyCraftLoadoutLoad);   // default F9
  _btnOk->onKeyboardPress((ActionHandler)&CraftEquipmentState::btnSaveClick, Options::keyCraftLoadoutSave);   // default F5
  ```
  There is **no button** for either action — a player who doesn't inspect the options/hotkeys never
  discovers the feature.
- **Precedent in DX.** The Quick-Stock feature already added compact bottom-row buttons here (Fill at
  30px) and re-laid-out the row, so the pattern (add button → wire to handler → rebalance the row) is
  established. See [Feature-CraftEquipmentQuickStock.md](Feature-CraftEquipmentQuickStock.md).

## Layout constraint (the real work)

The single bottom button row (y=176, h=16) is already crowded. Current occupants:

| Element | x | width | Visible when |
|---|---|---|---|
| Filter combo (`_cbxFilterBy`) | 16 | 80 (NewBattle) / 140 | always |
| Inventory (`_btnInventory`) | 98 / 164 | 62 (NewBattle) / 102 | craft has crew |
| Unload Craft (`_btnClear`) | 162 | 78 | New Battle only |
| Fill (`_btnFill`) | 242 | 30 | New Battle only |
| OK (`_btnOk`) | 274 / 164 | 30 / 140 | always |

In **New Battle** the row is essentially full (combo 16–96 · Inventory 98–160 · Clear 162–240 ·
Fill 242–272 · OK 274–304). In the **real geoscape** screen, Clear/Fill are hidden, so there is more
slack (combo 16–156 · Inventory 164–266 · OK 274–304), but still no wide gap.

The list (`_lstEquipment`, y40 h128 → ends y168) leaves room for only the one button row above the
window's 200px height, so a clean second row isn't free.

## Outcome (as shipped, Jun 2026)

Two compact 30px buttons — **Save** and **Load** (loadout template) — added to the bottom row of
[CraftEquipmentState](../src/Basescape/CraftEquipmentState.cpp), wired to the existing `btnSaveClick` /
`btnLoadClick`. The F5/F9 hotkeys stay bound on `_btnOk` (additive, not a replacement). Both buttons
show in **every** context (real geoscape *and* New Battle).

**Save enabled in New Battle.** The old `btnSaveClick` gate `if (!_isNewBattle)` was removed: New
Battle has a live in-memory `SavedGame`, and `_globalCraftLoadout[]` is a `SavedGame` member, so save
and load both work there (snapshot a loadout, clear/experiment, reload, copy across crafts).
`btnLoadClick` was already un-gated. This keeps Save/Load symmetric and matches DX's direction of
enabling more in New Battle.

**New Battle template persistence.** Templates (both the craft loadouts and the soldier equipment
layouts) only round-tripped through full *campaign* saves via `SavedGame::save`/`load`; New Battle's
own `.cfg` never stored them and only ever seeded them from the mod's starting base, so anything saved
in New Battle was discarded on exit. Fixed by factoring the template-writing block out of
`SavedGame::save` into a new `SavedGame::saveTemplates(YamlNodeWriter)` (mirrors the existing
`loadTemplates`), then having `NewBattleState::save` write a `globalTemplates:` node into the `.cfg`
and `NewBattleState::load` restore from it (falling back to starting-base defaults only when the
`.cfg` has none, so user templates aren't overwritten or duplicated). No save-format change to
campaign saves. Touches [SavedGame.h](../src/Savegame/SavedGame.h),
[SavedGame.cpp](../src/Savegame/SavedGame.cpp), [NewBattleState.cpp](../src/Menu/NewBattleState.cpp).

**Layout.** Save/Load/OK sit at fixed right-aligned slots (x218 / x250 / x282, 30px each) in all
contexts; only the left portion flexes. The row's right edge extends to ~x312 (from x304). No element
is abbreviated: the "Unload" button (its `STR_UNLOAD_CRAFT` string already renders as just "Unload")
keeps its text and only narrows.

- **New Battle** (7 elements) — combo 66 (x16) · Inventory 56 (x84) · Unload 42 (x142) · Fill 30
  (x186) · Save 30 (x218) · Load 30 (x250) · OK 30 (x282):
  ```
     ┌───────────────┐ ┌────────────┐ ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐
     │  Filter By ▼  │ │ INVENTORY  │ │UNLD │ │FILL │ │SAVE │ │LOAD │ │ OK  │
     └───────────────┘ └────────────┘ └─────┘ └─────┘ └─────┘ └─────┘ └─────┘
  ```
- **Geoscape** (Unload + Fill hidden) — combo 110/200 (x16) · Inventory 88 (x128) · Save · Load · OK:
  ```
     ┌────────────────────────┐ ┌──────────────────┐ ┌─────┐ ┌─────┐ ┌─────┐
     │       Filter By ▼      │ │     INVENTORY     │ │SAVE │ │LOAD │ │ OK  │
     └────────────────────────┘ └──────────────────┘ └─────┘ └─────┘ └─────┘
  ```

**Strings.** `STR_DX_CRAFT_LOADOUT_SAVE` = "Save", `STR_DX_CRAFT_LOADOUT_LOAD` = "Load" in
[Language/DX/en-US.yml](../bin/common/Language/DX/en-US.yml).

Touches: [CraftEquipmentState.h](../src/Basescape/CraftEquipmentState.h) (two `TextButton*`),
[CraftEquipmentState.cpp](../src/Basescape/CraftEquipmentState.cpp) (button geometry + `add` + `init`
wiring; un-gated `btnSaveClick`), and the DX language file. No save-format change.

## Resolved decisions

1. **Button footprint:** compact fixed-width Save/Load (30px, Fill-style), fit by narrowing the Unload
   button and Inventory/combo slightly and extending the row to the ~x312 edge — no label abbreviated.
2. **Unload button:** kept its existing text ("Unload", from `STR_UNLOAD_CRAFT`); only its width
   changed. No new "Clear" string.
3. **Grouping:** Save/Load adjacent as a pair, after the stock actions (Unload, Fill), before OK.
4. **Scope:** shown in both New Battle and the real geoscape screen; Save works in New Battle too
   (un-gated), effective within the session.
