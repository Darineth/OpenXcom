# Ruleset: `inventoryLayouts:` **[DX]**

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`RuleInventoryLayout`](../src/Mod/RuleInventoryLayout.h) · **List key:** `id` ·
**Loader:** [`RuleInventoryLayout::load`](../src/Mod/RuleInventoryLayout.cpp)

A **DX-only** root. An inventory layout is a named, **ordered set of
[`invs:`](Ruleset-Invs.md) section ids** that an armor grants its wearer via the armor's
`inventoryLayout` field ([`armors:`](Ruleset-Armors.md)). In stock OXCE every unit shares the one
global section set; with layouts, different armors can expose different slots. The inventory
screen, item placement, auto-equip, quick-move and the alien inventory all iterate only the active
unit's layout — sections not in it are not drawn or usable.

```yaml
invs:
  - id: STR_SATCHEL              # a normal global section, just not in STR_STANDARD_INV
    x: 192
    y: 37
    type: 0
    slots: [ [0,0], [1,0], [2,0], [0,1], [1,1], [2,1] ]
    costs: { STR_RIGHT_HAND: 8, STR_BELT: 12, STR_GROUND: 10 }

inventoryLayouts:
  - id: STR_LAYOUT_LIGHT
    invs: [ STR_RIGHT_HAND, STR_LEFT_HAND, STR_BELT, STR_SATCHEL, STR_GROUND ]

armors:
  - type: STR_STEALTH_SUIT_UC
    inventoryLayout: STR_LAYOUT_LIGHT
```

Entries **merge** across mods/files by `id`, support `refNode:` inheritance and `delete: true`
(see the [index](Ruleset.md#how-rulesets-load-the-60-second-version)).

## Fields

| Key | Type | Default | Meaning |
|---|---|---|---|
| `id` | string | — | Unique layout id, referenced by `Armor.inventoryLayout`. |
| `refNode` | map | — | Inherit from another (anonymous) node first; a child's `invs:` **replaces** the inherited list wholesale (omit `invs:` to keep the parent's). |
| `invs` | list of section ids | — | Ordered global [`invs:`](Ruleset-Invs.md) section ids making up the layout. |
| `listOrder` | int | auto (+10 per entry) | Sort/list weight. |

A legacy `sections:` block is rejected with a load error telling you to use `invs:`.

## Resolution rules (from `afterLoad`/`resolveHands`)

- **Sections are reusable globals.** There are no inline section definitions — every id must name
  a global `invs:` section (unknown id = load error), and the same section object may appear in
  any number of layouts. To make a section exclusive to some layouts, simply don't list it in the
  others (or in `STR_STANDARD_INV`).
- **Duplicates are an error** — a section may appear only once per layout.
- **A ground section is guaranteed:** if the list has no `type: 2` section, the global
  `STR_GROUND` is appended automatically, so items can always be dropped.
- **At most one hand per side.** Each layout resolves its right/left hand from the sections'
  [`hand:` property](Ruleset-Invs.md#dx-hands-by-property-not-by-id); two same-side hands in one
  layout is a load error (the same hand section in *different* layouts is fine). A layout may also
  omit a hand entirely (one-handed units) — displaced ammo then best-fits elsewhere or drops.
- **The first `INV_UTILITY` section** becomes the layout's "utility slot"
  (`BattleUnit::getUtilitySlot()`); extra utility sections still work as normal slots but only the
  first is the per-unit handle (a warning is logged).

## The default layout: `STR_STANDARD_INV`

The base game data (`bin/standard/xcom1/inventories.rul`, `xcom2` likewise) defines the vanilla
nine-slot set as the `STR_STANDARD_INV` layout. Any armor with no `inventoryLayout` uses it — so
adding a section to *everyone* means adding it to `STR_STANDARD_INV`, while adding it to *some*
units means listing it in their armors' layouts only. If a total conversion defines no
`STR_STANDARD_INV` at all, the engine synthesizes an implicit layout from the **full global
`invs:` set** (in map iteration order), so layout-less mods behave exactly as stock OXCE.

## Stranding protection

Placement that would target a section outside the unit's layout (loadout templates, saved
equipment layouts made under a different armor) leaves the item on the ground instead (templates
warn with `STR_DX_TEMPLATE_SLOT_NOT_IN_LAYOUT`); battlescape saves whose slots became invalid drop
the items to the unit's tile. See
[DX-Features.md](../DX-Features.md#configurable-inventory-layouts).

## See also

- [`invs:`](Ruleset-Invs.md) — the sections themselves (geometry, costs, [DX] typed-slot fields)
- [`armors:`](Ruleset-Armors.md) — the `inventoryLayout` assignment (keying is armor-only)
- [DX-Features.md](../DX-Features.md#configurable-inventory-layouts) — feature-level description
