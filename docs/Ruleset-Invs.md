# Ruleset: `invs:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`RuleInventory`](../src/Mod/RuleInventory.h) · **List key:** `id` · **Loader:**
[`RuleInventory::load`](../src/Mod/RuleInventory.cpp)

An `invs:` entry is one **inventory section** ("slot"): a screen region on the 320×200 inventory
screen where items can sit — a slot grid (belt, backpack), a hand, the ground strip, or a **[DX]**
single-item utility/equip box. Sections are **global objects**; which sections a given unit
actually has is decided by its armor's **[DX]** [inventory layout](Ruleset-InventoryLayouts.md)
(armors without one use the `STR_STANDARD_INV` layout).

```yaml
invs:
  - id: STR_DX_HOLSTER          # unique id; also the section's display string key
    x: 192                      # screen position (pixels)
    y: 104
    type: 0                     # 0 = slot grid
    battleType: 1               # [DX] firearms only (BT_FIREARM)
    slots: [ [0, 0], [0, 1] ]   # a 1-wide, 2-tall grid
    costs:
      STR_RIGHT_HAND: 8
      STR_LEFT_HAND: 1          # quick-draw: 1 TU to the left hand
      STR_GROUND: 6
```

Entries **merge** across mods/files by `id`, support `refNode:` inheritance and `delete: true`
(see the [index](Ruleset.md#how-rulesets-load-the-60-second-version)).

## Fields

| Key | Type | Default | Meaning |
|---|---|---|---|
| `id` | string | — | Unique id; also the section-name string key. `STR_GROUND` is the global ground section; `STR_RIGHT_HAND`/`STR_LEFT_HAND` get legacy handedness (see `hand`). |
| `refNode` | map | — | Inherit all fields from another (anonymous) node first. |
| `x` / `y` | int px | 0 | Top-left screen position of the section on the inventory screen. |
| `type` | int 0–4 | 0 | Section kind: 0 slot grid, 1 hand, 2 ground, **[DX]** 3 utility, **[DX]** 4 equip (see below). |
| `slots` | list of `[x, y]` | — | The grid cells of a type-0 section, in 16×16-px slot-cell coordinates relative to `x`/`y`. |
| `width` / `height` **[DX]** | int cells | 0 → 2×2 | Bounding-box size of a utility/equip section (overrides the built-in 2×2 default); ignored for other types. |
| `costs` | map section-id → TU | — | TU to move an item **from this section into** the named one; unlisted pair = 8 (`DEFAULT_MOVE_COST`), **[DX]** any negative value (conventionally `-1`) forbids that move in combat. |
| `hand` **[DX]** | `right` / `left` / `none` | see below | Declares the section a real hand of that side (fires, reacts, auto-equips); any other value is a load error. |
| `battleType` **[DX]** | int | 0 (any) | Slot-side item filter: only items of this [battle type](Ruleset-Items.md) may occupy the section (0 = unrestricted). |
| `allowCombatSwap` **[DX]** | bool | true | When false, items can't be moved into or out of the section once combat is underway (pre-battle equip unaffected). |
| `countStats` **[DX]** | bool | true | When false, items in the section don't contribute their `stats`/`statModifiers` to the wearer. |
| `armorSide` **[DX]** | `front`/`left`/`right`/`rear`/`under` | unset | Declares the section an armor hardpoint reinforcing that one facing; any other value is a load error. |
| `listOrder` | int | auto (+10 per entry) | Sort position (e.g. section iteration order in the implicit default layout). |

## Section types & geometry

All geometry is in **slot cells** of 16×16 pixels (`SLOT_W`/`SLOT_H`).

- **`type: 0` — slot grid (`INV_SLOT`).** Holds multiple items; the `slots:` list enumerates the
  cells. An item fits only if every cell of its `invWidth × invHeight` footprint is in the list
  (the grid may be any shape, like the vanilla belt's cut-out).
- **`type: 1` — hand (`INV_HAND`).** Single occupant, drawn as a fixed 2×3-cell box. A hand accepts
  an item of **any size** (a 2×3 rifle "fits" a hand). Wielding, firing, reactions and reload logic
  key off handedness (`hand:`), not the id.
- **`type: 2` — ground (`INV_GROUND`).** Not unit-attached; the grid is computed, not authored:
  it spans from `x`/`y` to the screen edge (`(320−x)/16` columns × `(200−y)/16` rows) and scrolls
  in screen-width pages. Exactly one ground section named `STR_GROUND` must exist — it is the
  engine's drop-target fallback everywhere.
- **`type: 3` — utility (`INV_UTILITY`) [DX].** Single-occupant box like a hand, but the item is
  **never wielded** (no firing/reactions from it) and, unlike a hand, the item is **size-checked**
  against the box (`width`/`height`, default 2×2). A unit's first utility section is its "utility
  slot" (`BattleUnit::getUtilityItem`), usable in place via the battlescape Use-Utility hotkey.
- **`type: 4` — equip (`INV_EQUIP`) [DX].** Same single-occupant, size-checked behavior as
  utility; a reserved sibling type for equippable gear (no dedicated per-unit accessor yet).

## Move costs

`costs:` is **directional**: moving an item from section A to section B charges
`A.costs[B]` TU (moving within the same section is free). Pairs both ways must be authored if both
directions should have real prices — the stock sections cross-list every pair. A pair with no
entry falls back to 8 TU (this keeps partially-specified custom-layout sections working), and
**[DX]** any negative value makes that move illegal while in combat (`STR_INVALID_TRANSFER`);
pre-battle equipping ignores costs entirely.

## [DX] Typed-slot rules

The three filter fields turn a section into a typed socket
(see [DX-Features.md](../DX-Features.md#typed-inventory-slots)):

- **`battleType`** is enforced whenever an item is placed as the section's occupant (manual drop,
  ctrl-click, auto-equip candidate scan) — rejection shows `STR_INVALID_ITEM_SLOT`. Values are the
  item `battleType` ordinals: 1 firearm, 2 ammo, 3 melee, 4 grenade, 5 proximity grenade,
  6 medikit, 7 scanner, 8 mind probe, 9 psi-amp, 10 flare, 11 corpse, 12 armor plate,
  13 equipment. It composes with the
  item-side `supportedInventorySections:` on [`items:`](Ruleset-Items.md) (both must pass), and
  does **not** block reloading a weapon already in the slot.
- **`allowCombatSwap: false`** only bites in combat: a locked item can be picked up and put back
  in the *same* section (so it can be unloaded/used), but not moved to a different section, and
  nothing can be moved in (`STR_NOT_COMBAT_SWAPPABLE`); Throw is withheld for locked items.
- **`countStats: false`** excludes the section's items from the wearer's item stat bonuses
  (`stats`/`statModifiers` on items) — for holster/stowage sections.

## [DX] Armor hardpoints — `armorSide`

Normally an item with [directional armor](Ruleset-Items.md) adds each of its
`frontArmor`/`sideArmor`/`rearArmor`/`underArmor` values to the matching facing of its wearer.
A section that declares `armorSide` overrides that: the item plates **only** the named facing,
using the item's own value for that side (so `armorSide: left` applies `sideArmor`).

This exists so one plate item type can serve every facing of a
[modular vehicle](../plans/Feature-ModularVehicles.md) — a chassis defines
`STR_ARMOR_FRONT`/`_LEFT`/`_RIGHT`/`_REAR` sections and the same
`STR_HWP_ARMOR_PLATE` bolts into any of them — instead of the mod shipping four near-identical
per-facing plate items. Give the plate different per-side numbers and it becomes an asymmetric
plate whose contribution depends on where it is mounted.

Pair it with `battleType: 12` (`BT_ARMOR_PLATE`) so only plates fit, and `allowCombatSwap: false`
so armor can't be re-bolted mid-firefight. Sections with several cells let plates **stack**.

## [DX] Hands by property, not by id

`hand: right`/`hand: left` makes any section a real hand — auto-equip, firing, reactions, the
active-hand toggle and held-item sprites all follow the flag. If `hand:` is absent, the historical
ids still apply: `STR_RIGHT_HAND` → right, `STR_LEFT_HAND` → left, anything else → not a hand — so
stock rulesets are unchanged. A [layout](Ruleset-InventoryLayouts.md) may contain at most one hand
per side (validated per layout, not globally).

## See also

- [`inventoryLayouts:`](Ruleset-InventoryLayouts.md) **[DX]** — which sections a unit actually gets
- [`items:`](Ruleset-Items.md) — item footprints (`invWidth`/`invHeight`), `battleType`,
  `supportedInventorySections:`, `defaultInventorySlot`
- [`itemCategories:`](Ruleset-ItemCategories.md) — category-driven auto-equip section order
- [DX-Features.md](../DX-Features.md) — typed slots, utility/equip slots, configurable hands
