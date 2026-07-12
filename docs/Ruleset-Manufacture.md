# Ruleset: `manufacture:` and `manufactureShortcut:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`RuleManufacture`](../src/Mod/RuleManufacture.h) · **List key:** `name` ·
**Loader:** [`RuleManufacture::load`](../src/Mod/RuleManufacture.cpp)

A manufacture entry is one **workshop project**: engineers spend hours and money, consume input
items, and produce output items, [craft](Ruleset-Crafts.md), or even people. Projects are gated by
[research](Ruleset-Research.md) and by [base functions](Ruleset-Facilities.md#base-functions-providebasefunc--requiresbasefunc--forbiddenbasefunc),
and they need workshop space from [`facilities:`](Ruleset-Facilities.md).

```yaml
manufacture:
  - name: STR_LASER_PISTOL       # by default this also names the produced item
    category: STR_WEAPON
    requires:
      - STR_LASER_PISTOL         # the research topic
    space: 4
    time: 300                    # engineer-hours per unit
    cost: 8000                   # $ per unit
    requiredItems:
      STR_ALIEN_ALLOYS: 1
    listOrder: 1400
```

Entries **merge** across mods/files by `name`, support `refNode:` inheritance and `delete: true`
(see the [index](Ruleset.md#how-rulesets-load-the-60-second-version)).

## Core

| Key | Type | Default | Meaning |
|---|---|---|---|
| `name` | string | — | Unique id; also the display-name string key, and the default produced item. |
| `refNode` | map | — | Inherit all fields from another (anonymous) node first. |
| `category` | string | — | Grouping shown in the manufacture list (`STR_WEAPON`, `STR_CRAFT`, …); **`STR_CRAFT` switches the project to craft production**. |
| `requires` | list of research | — | [Research](Ruleset-Research.md) topics that must be discovered before the project appears. |
| `requiresBaseFunc` | list of tags | — | [Base-function](Ruleset-Facilities.md#base-functions-providebasefunc--requiresbasefunc--forbiddenbasefunc) tags the base must provide. |
| `space` | int | 0 | Workshop space (engineer slots) the project occupies. |
| `time` | int | 0 | Engineer-hours to produce one unit. **Must be > 0** — the mod fails to load otherwise. |
| `cost` | int $ | 0 | Money spent per unit produced. |
| `points` | int | 0 | Score awarded per unit produced. |
| `refund` | bool | false | Refund money and input items of a **cancelled** project's partially-finished unit. |
| `listOrder` | int | auto | Sort position in the manufacture list. |

## Inputs

| Key | Type | Default | Meaning |
|---|---|---|---|
| `requiredItems` | map name → int | — | Items **or craft** consumed per unit produced — each key is resolved as an [item](Ruleset-Items.md) first, then as a [craft](Ruleset-Crafts.md) type (unknown names are a load error). |

Consuming a craft is how "refit/upgrade craft X into craft Y" projects are built: put the old craft
type in `requiredItems` and the new one in `producedItems` with `category: STR_CRAFT`.

## Outputs

| Key | Type | Default | Meaning |
|---|---|---|---|
| `producedItems` | map name → int | `{ <name>: 1 }` | Items produced per unit. With `category: STR_CRAFT` this must be exactly **one craft type with count 1**. |
| `randomProducedItems` | list of `[weight, {item: count}]` | — | Weighted alternative outputs: one set is rolled per unit produced (in addition to `producedItems`). Disables the auto-sell feature. |
| `spawnedPersonType` | string | — | Person produced instead of items (`STR_SCIENTIST`, `STR_ENGINEER`, or a [soldier](Ruleset-Soldiers.md) type). Disables auto-sell. |
| `spawnedPersonName` | string | — | String key used to name the spawned person. |
| `spawnedSoldier` | map | — | Inline soldier template (nationality, stats, rank…) applied to the spawned soldier. |
| `transferTimes` | list of 3 ints | `[0, 24, 0]` | Hours before the output arrives in the base, as `[items, personnel, craft]` (0 = appears immediately; personnel are clamped to ≥ 1 hour). |
| `events` | weighted map event → int | — | Weighted pool from which one [geoscape event](Ruleset-Events.md) is rolled per unit produced. |

## `manufactureShortcut:`

**Engine class:** [`RuleManufactureShortcut`](../src/Mod/RuleManufactureShortcut.h) · **List key:**
`name` · **Loader:** [`RuleManufactureShortcut::load`](../src/Mod/RuleManufactureShortcut.cpp)

A shortcut is not a project — it is a **recipe for generating one**. At the end of mod loading
([`Mod::afterLoadHelper`](../src/Mod/Mod.cpp)) each shortcut clones the `startFrom:` project under
the shortcut's own `name`, then repeatedly *inlines* any `requiredItems` entry that is itself a
manufacture project (see [`RuleManufacture::breakDown`](../src/Mod/RuleManufacture.cpp)). The result
is a single "build it all from raw materials" variant that sits next to the original in the list, so
the player doesn't have to queue the sub-components by hand.

```yaml
manufactureShortcut:
  - name: STR_HEAVY_PLASMA_FROM_SCRATCH
    startFrom: STR_HEAVY_PLASMA          # existing manufacture project
    breakDownItems:
      - STR_PLASMA_GUN_MECHANISM         # ...also a manufacture project: inline it
      - STR_ELERIUM_CELL
```

| Key | Type | Default | Meaning |
|---|---|---|---|
| `name` | string | — | Unique name of the **new** manufacture project to create (must not collide with an existing one). |
| `refNode` | map | — | Inherit all fields from another (anonymous) node first. |
| `startFrom` | string manufacture | — | The existing project used as the template. |
| `breakDownItems` | list of names | — | Required items that are themselves manufacture projects and should be replaced by their own inputs (applied iteratively until nothing is left to expand). |
| `breakDownRequires` | bool | false | Also merge the sub-projects' `requires:` research into the new project's requirements. |
| `breakDownRequiresBaseFunc` | bool | true | Also merge the sub-projects' `requiresBaseFunc:` tags into the new project's base-function requirements. |

What `breakDown` folds together while expanding a sub-project consumed `count` times: `time` and
`cost` gain `count ×` the sub-project's values, `space` becomes the **max** of the two (you never
build the parts in parallel), and the sub-project's own `requiredItems` are added (×`count`) to the
new recipe. Names listed in `breakDownItems` must exist **both** as an [item](Ruleset-Items.md) and
as a manufacture project.

## See also

- [`research:`](Ruleset-Research.md) — the `requires:` gates
- [`facilities:`](Ruleset-Facilities.md) — `workshops:` capacity and the base-function tags
- [`items:`](Ruleset-Items.md) — inputs and outputs
- [`crafts:`](Ruleset-Crafts.md) / [`craftWeapons:`](Ruleset-CraftWeapons.md) — `category: STR_CRAFT`
  production and craft-weapon launcher/clip items
- [`events:`](Ruleset-Events.md) — the `events:` pool rolled on completion
