# Feature: Quick Clear / Preload on Craft Equipment (New Battle)

**Status:** Implemented (Jun 2026).

## Outcome (as shipped)

The New-Battle craft equipment screen ([CraftEquipmentState.cpp](../src/Basescape/CraftEquipmentState.cpp))
gained a **Fill** button alongside the existing **Unload Craft** (clear) button:

- **Unload Craft** (kept as-is) empties the craft.
- **Fill** (`btnFillClick`, New Battle only) stocks the craft with a generous spread of every usable
  item (recoverable, non-corpse, inventory items): **40** of each ammo / grenade / proximity grenade /
  flare, **10** of everything else. Then opening the inventory lets you distribute them.
- The New-Battle bottom row was re-laid-out to fit both buttons plus Inventory and OK: the filter
  combo is narrower in New Battle (80 vs 140), then Inventory (62) · Unload Craft (78) · Fill (30) ·
  OK (30).
- New `STR_DX_CRAFT_FILL` string in `Language/DX/`.

## Resolved decisions

1. **Preload amount:** 40 per ammo/consumable (ammo, grenade, proximity grenade, flare), 10 for all
   other gear. (Larger than the New-Battle initial 1/2 stock, per the request.)
2. **Buttons:** kept the existing "Unload Craft" as the clear; added a separate compact "Fill".
3. **Scope:** New Battle only.

## Summary

Give the craft equipment screen quick buttons to **clear** all craft items and to **preload "some of
everything"**, so New Battle loadout setup is fast (empty the auto-stock, or refill it without
restarting New Battle).

## OXCE / OXCE-Plus + DX audit (Jun 2026)

- **Clear already exists.** In New Battle, `CraftEquipmentState` shows an "Unload Craft" button
  (`btnClearClick`) that moves all items off the craft and clears the craft's item list
  ([CraftEquipmentState.cpp](../src/Basescape/CraftEquipmentState.cpp)). So "clear" is done; the
  delta is a **preload** action (and possibly a clearer button pairing).
- **Preload precedent.** `NewBattleState::initSave` stocks the New-Battle craft + base storage with
  `howMany` of each item — `2` for `BT_AMMO`, `1` otherwise — limited to recoverable, non-corpse,
  inventory items ([NewBattleState.cpp:574](../src/Menu/NewBattleState.cpp#L574)). The preload button
  replays exactly this so it matches the New-Battle starting stock.
- **No equivalent upstream.** OXCE has no "fill the craft with one of everything" button.
- **Layout constraint.** The New-Battle bottom row already holds: filter combo (x16), Inventory
  (x128), Unload Craft (x194), OK (x274). A preload button needs the row re-laid-out.

## Proposed design

- Add a **Preload** button (New Battle only) that, for each recoverable non-corpse inventory item,
  adds `howMany` (2 ammo / 1 else) to the craft, mirroring `initSave`. Implement as a small helper
  (or factor `initSave`'s loop) so the two stay in sync.
- Keep the existing **Unload Craft** as the clear action; re-lay-out the New-Battle bottom row to fit
  both it and the new Preload button alongside the combo + Inventory + OK (compact labels / narrower
  combo as needed).
- New Battle only — the real geoscape craft equipment is bound by base storage and a real economy, so
  a free "fill" there would be a cheat.

Key file: [CraftEquipmentState.cpp](../src/Basescape/CraftEquipmentState.cpp) (button + handler +
New-Battle row layout). New `STR_*` label(s) in `Language/DX/`.

## Open questions

1. **Preload amount:** match New-Battle initial stock (1 each / 2 ammo), or a larger fixed amount?
2. **Button pairing/labels:** keep "Unload Craft" as the clear and add a separate "Preload" button, or
   replace with a compact **Clear** + **Fill** pair (frees space, but renames the existing button)?
3. **Scope:** New Battle only (recommended), or also offer it in the geoscape craft screen bound by
   available stock?
