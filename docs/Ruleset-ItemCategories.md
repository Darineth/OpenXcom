# Ruleset: `itemCategories:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`RuleItemCategory`](../src/Mod/RuleItemCategory.h) · **List key:** `type` · **Loader:**
[`RuleItemCategory::load`](../src/Mod/RuleItemCategory.cpp)

An item category is a **label that items opt into** (via their `categories:` list in
[`items:`](Ruleset-Items.md)). Categories drive the filter dropdowns on the base screens
(Purchase, Sell, Craft Equipment), are matched by the UFOpaedia/inventory text search, and can
supply a **preferred slot order** for auto-equip. The category entry itself is tiny — mostly
ordering and visibility metadata.

```yaml
itemCategories:
  - type: STR_CONCEALABLE          # unique id; also the filter's display string key
    listOrder: 150
    invOrder:                      # auto-equip tries these sections first for items in this category
      - STR_BELT
      - STR_RIGHT_LEG
      - STR_LEFT_LEG

items:
  - type: STR_PISTOL
    categories: [ STR_CONCEALABLE ]
```

Entries **merge** across mods/files by `type`, support `refNode:` inheritance and `delete: true`
(see the [index](Ruleset.md#how-rulesets-load-the-60-second-version)).

## Fields

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | — | Unique id; also the display-name string key shown in filter dropdowns. |
| `refNode` | map | — | Inherit all fields from another (anonymous) node first. |
| `replaceBy` | string | — | After all mods load, every item carrying this category has it swapped for the named category (a rename/merge mechanism). |
| `hidden` | bool | false | Hide this category from the Craft Equipment screen's category filter. |
| `listOrder` | int | auto (+100 per entry) | Sort position among categories in filter lists. |
| `invOrder` | list of section ids | — | Ordered [`invs:`](Ruleset-Invs.md) section ids that auto-equip / ctrl-click-equip tries first for items of this category. |

## Semantics worth knowing

- **`replaceBy` is a post-load pass**, not an alias: after every mod is loaded,
  [`Mod`](../src/Mod/Mod.cpp) collects all `replaceBy` pairs and rewrites each item's
  `categories:` list (`RuleItem::updateCategories`). The replaced category entry still exists but
  no item references it anymore.
- **`invOrder` feeds placement, not display.** When auto-equip or ctrl-click-equip places an item,
  the engine asks the item for its **first category that has a non-empty `invOrder`**
  ([`RuleItem::getFirstCategoryWithInvOrder`](../src/Mod/RuleItem.cpp)) and tries those sections in
  order before the generic candidate scan (ground sections in the list are skipped). Used both in
  the inventory screen ([`Inventory`](../src/Battlescape/Inventory.cpp)) and battlescape pickup
  ([`BattleUnit`](../src/Savegame/BattleUnit.cpp)). An item's own `defaultInventorySlot` is tried
  before its category order.
- The text search on inventory/base screens also matches an item's **translated category names**,
  for items with a visible UFOpaedia article.
- Slot-side placement filters ([DX] `battleType` on the section — see
  [Ruleset-Invs.md](Ruleset-Invs.md)) still apply to every section `invOrder` suggests.

## See also

- [`items:`](Ruleset-Items.md) — where items declare `categories:` and `defaultInventorySlot`
- [`invs:`](Ruleset-Invs.md) — the inventory sections named by `invOrder`
