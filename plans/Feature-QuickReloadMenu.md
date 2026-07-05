# Feature: Quick Reload — visible action-menu item

**Status:** **Implemented (Jul 2026)**, builds clean (0 warnings). Phase 6 (Ammo & Reloading). The
*core* quick-reload already ships in OXCE; this is the DX "make it visible" delta only. Implemented
as designed: a `BA_RELOAD` action-menu row (hotkey R) that calls the shared `BattleUnit::reloadWeapon`
the R key now also routes through; partial-mag swap remains deferred.

## Audit — OXCE already has the core

- **`BattleUnit::reloadAmmo()`** ([BattleUnit.cpp:3934](../src/Savegame/BattleUnit.cpp#L3934)),
  triggered by the **R** key (`keyBattleReload`, default `SDLK_r`) →
  `BattlescapeState::btnReloadClick` ([BattlescapeState.cpp:1998](../src/Battlescape/BattlescapeState.cpp#L1998)).
  It scans both hands for a weapon that needs ammo, finds the **cheapest** compatible magazine in the
  unit's inventory, spends the TU, loads it, plays the reload sound, and refreshes the soldier info.
  It already honors the DX `battleWeightBasedReloadCost` term (same cost path).
- **Limitations vs the legacy DX intent:**
  1. **Empty slots only** — it skips a weapon whose slot already holds a (partial) magazine
     (`haveAllAmmo()` short-circuit + `!getAmmoForSlot(slot)`), so it never *swaps* a partial mag.
  2. **Hotkey-only** — no visible button or menu entry, which runs against DX's "prefer visible,
     discoverable UI over hidden hotkeys" philosophy ([[dx-surface-hidden-features]]).

## Scope (decided)

Do the **visible UI trigger** only — surface quick-reload as an item in the weapon **action menu**
(where players already look for a weapon's actions and see TU costs). **Not** doing the partial-mag
swap (deferred; the empty-slot behavior matches OXCE's R key). The R hotkey stays.

## Design

- New action type `BA_RELOAD = 22` (appended to `BattleActionType` to keep serialized values stable).
- **`ActionMenuState`** (firearm path): add a **Reload** row (`STR_RELOAD`, already localized) when the
  opened weapon is a `BT_FIREARM` that `isWeaponWithAmmo() && !haveAllAmmo()` — i.e. it uses external
  clips and has an empty ammo slot. Hotkey = `Options::keyBattleReload` (so **R** works with the menu
  open too, consistent with the global bind). Added *first* so it sits at the very **bottom** of the
  menu (the last item); menu rows stack later-added-higher. Hotkey labels are capitalized (`R`, not
  SDL's lowercase `r`) via a small `hotkeyLabel` helper applied to every action row.
  - Like `BA_DUALFIRE`, it gets a **compact row** in `addItem` (name + TU cost; no accuracy/shots).
  - The cost is the cheapest compatible-ammo reload cost (`BattleUnit::getReloadCost`); the row is
    flagged red **"No Ammo"** when no compatible clip is carried, or **"No TU"** when unaffordable —
    matching how the fire-mode rows flag themselves, so the option is discoverable even when it can't
    currently be used.
- **`ActionMenuState::handleAction`**: a `BA_RELOAD` branch calls the shared reload-and-report entry
  point `BattlescapeState::quickReload(actor, weapon)` and pops. It's exempt from `handleAction`'s
  research / `canUseWeapon` gates (like `BA_THROW`), which would otherwise reject the unloaded weapon
  we're trying to reload.
- **`BattleUnit`**: factor the single-weapon reload out of `reloadAmmo()`:
  - `bool reloadWeapon(BattleItem* weapon)` — cheapest-ammo reload of one weapon (spends TU, loads,
    sets the reload sound). `reloadAmmo()` becomes `reloadWeapon(right) || reloadWeapon(left)`, so the
    R key and the menu share identical behavior.
  - `int getReloadCost(BattleItem* weapon)` — the display cost: min reload TU over carried compatible
    ammo, or `-1` if none.
  - Both `reloadWeapon` and `getReloadCost` delegate the search + cost to a shared private
    `findReloadAmmo`, so the shown cost and the charged cost cannot drift.
- **`BattlescapeState::quickReload(unit, weapon = nullptr)`**: the single reload-and-report entry
  point — reloads (`reloadWeapon(weapon)` for a specific weapon, or `reloadAmmo()` for the whole unit
  when `weapon` is null) and, on success, plays the reload sound + refreshes the soldier info. Both
  `btnReloadClick` (R key, null weapon) and the action-menu branch (specific weapon) are now one-liners
  onto it, so reload behaviour and feedback are fully shared.

## Touch points

- `src/Mod/RuleItem.h` — `BA_RELOAD = 22`.
- `src/Savegame/BattleUnit.h/.cpp` — `reloadWeapon`, `getReloadCost`, `reloadAmmo` refactor.
- `src/Battlescape/ActionMenuState.cpp` — add the item, its compact row in `addItem`, and the
  `handleAction` branch.
- No new language string (`STR_RELOAD` already exists and is localized).
- Docs: `DX-Features.md`, this doc, `DX-Roadmap.md` (tick Quick Reload with the "OXCE core + DX
  visible trigger" note).

## Testing plan

- dx-test: hold a firearm with an empty/partly-used clip and open its action menu → a **Reload** row
  shows with the TU cost (weight-based, since dx-test forces the option on). Selecting it (or pressing
  R with the menu open) loads the cheapest clip, plays the sound, and the ammo readout updates.
- With no spare clip carried, the row shows red **"No Ammo"**. With a full weapon, the row is absent.
- The global **R** key still reloads as before (shared `reloadWeapon` path).
- Build clean with `-Wall -Wextra`.

## Deferred

- **Partial-magazine swap** (eject a half-spent mag, load a fresh one) — the other gap vs OXCE; not
  requested now. Would extend `reloadWeapon` to consider non-empty slots and route the ejected clip
  back to inventory/ground.
