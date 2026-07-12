# Ruleset: `ufopaedia:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`ArticleDefinition`](../src/Mod/ArticleDefinition.h) · **List key:** `id` · **Loader:**
[`ArticleDefinition::load`](../src/Mod/ArticleDefinition.cpp) (+ the `type_id` dispatch in
[`Mod::loadFile`](../src/Mod/Mod.cpp))

A UFOpaedia entry is one **article**: a screen in the in-game encyclopedia. Every article picks a
**layout** with `type_id:` — that number decides which `ArticleDefinition` subclass parses the node,
which `ArticleState*` screen renders it, and therefore which extra fields are meaningful.

```yaml
ufopaedia:
  - id: STR_SKYRANGER          # unique id; also the default title string key
    type_id: 1                 # 1 = craft layout
    section: STR_XCOM_CRAFT_ARMAMENT
    image_id: UP004.SPK
    rect_stats:  { x: 160, y: 5,  width: 160, height: 60 }
    rect_text:   { x: 5,   y: 40, width: 140, height: 100 }
    text: STR_SKYRANGER_UFOPEDIA
  - id: STR_ALIEN_RESEARCH
    type_id: 8                 # 8 = plain text
    section: STR_ALIEN_RESEARCH_TOPICS
    requires: [ STR_ALIEN_ORIGINS ]
    text: STR_ALIEN_RESEARCH_UFOPEDIA
```

Entries **merge** by `id`. `type_id` is only read when the article is *created*: the first ruleset to
define an `id` fixes its layout, and later files that reuse the same `id` just overwrite fields.
An entry without `type_id` and without a pre-existing `id` is **skipped with a load error**.
`delete:` removes an inherited article.

## Article types (`type_id`)

From [`UfopaediaTypeId`](../src/Mod/ArticleDefinition.h). "Renders" names the screen class in
[`src/Ufopaedia/`](../src/Ufopaedia).

| `type_id` | Enum | Layout / what it displays |
|---|---|---|
| 1 | `UFOPAEDIA_TYPE_CRAFT` | Player craft: big `image_id` picture, a craft stats block and the description, each in its own rect. |
| 2 | `UFOPAEDIA_TYPE_CRAFT_WEAPON` | Craft weapon: `image_id` picture plus a fixed craft-weapon stats block (damage, range, accuracy, rearm). |
| 3 | `UFOPAEDIA_TYPE_VEHICLE` | HWP/tank: text description, unit stats block, and the stats of its built-in `weapon`. |
| 4 | `UFOPAEDIA_TYPE_ITEM` | Item: BIGOBS sprite, item stats, ammo list, description. `weapon` names the weapon an ammo item belongs to. |
| 5 | `UFOPAEDIA_TYPE_ARMOR` | Armor: paperdoll/`image_id` picture plus the armor's protection values per side and damage modifiers. |
| 6 | `UFOPAEDIA_TYPE_BASE_FACILITY` | Base facility: BASEBITS sprite, build cost/time/maintenance stats, description. |
| 7 | `UFOPAEDIA_TYPE_TEXTIMAGE` | Free-form: full-screen `image_id` background with a text column over it. |
| 8 | `UFOPAEDIA_TYPE_TEXT` | Free-form: title + body text only, no image. |
| 9 | `UFOPAEDIA_TYPE_UFO` | UFO: INTERWIN.DAT picture plus the UFO's stats. |
| 10 | `UFOPAEDIA_TYPE_TFTD` | The TFTD-style layout: image at top, text below, no stats. |
| 11 | `UFOPAEDIA_TYPE_TFTD_CRAFT` | TFTD layout + craft stats. |
| 12 | `UFOPAEDIA_TYPE_TFTD_CRAFT_WEAPON` | TFTD layout + craft-weapon stats. |
| 13 | `UFOPAEDIA_TYPE_TFTD_VEHICLE` | TFTD layout + HWP stats (uses `weapon`). |
| 14 | `UFOPAEDIA_TYPE_TFTD_ITEM` | TFTD layout + item stats (uses `weapon` for ammo). |
| 15 | `UFOPAEDIA_TYPE_TFTD_ARMOR` | TFTD layout + armor stats. |
| 16 | `UFOPAEDIA_TYPE_TFTD_BASE_FACILITY` | TFTD layout + facility stats. |
| 17 | `UFOPAEDIA_TYPE_TFTD_USO` | TFTD layout + USO/UFO stats. |
| 18 | `UFOPAEDIA_TYPE_SOLDIER` | Soldier type: picture, plus the min/max/cap table of the soldier's starting stats. |
| 19 | `UFOPAEDIA_TYPE_UNIT` | Alien/civilian unit: picture, a stats block, an armor block and the description. |

Types 10–17 all parse as `ArticleDefinitionTFTD` — they share one field set and differ only in which
stats table the screen draws. Despite the name they work in UFO mods too; they are simply the
"image + narrow text column + optional stats" layout.

## Common fields (all types)

| Key | Type | Default | Meaning |
|---|---|---|---|
| `id` | string | — | Unique article id; for typed articles it must match the rule it documents (item/armor/craft/facility/`type`). |
| `type_id` | int | — | The layout, from the table above. Required when the article is first defined. |
| `section` | string | — | Which UFOpaedia section button lists the article; `STR_NOT_AVAILABLE` hides it from the index (still reachable from links). |
| `requires` | list of research | — | Article is only visible once **all** these [research](Ruleset-Research.md) topics are done. |
| `disabledBy` | list of research | — | Article becomes hidden once **any** of these topics is researched (supersedes/retires an article). |
| `title` | string | `id` | Title string key of page 1. |
| `text` | string | — | Body-text string key of page 1. |
| `ammoSlot` | int | 0 | Which ammo slot page 1 shows stats for (weapon articles with several ammo slots). |
| `pages` | list | — | Multi-page article: each entry may set `title`, `text`, `ammoSlot`; unset fields inherit page 1's. |
| `listOrder` | int | auto (+100 per article) | Sort position inside the section. |
| `hiddenCommendation` | bool | false | Hides the article until the player's soldiers have actually earned the [commendation](Ruleset-Commendations.md). |

**Custom palette:** any article whose `image_id` contains the substring `_CPAL` is flagged
`customPalette` and the screen takes its palette from that image instead of the standard
`PAL_BATTLEPEDIA` — this is how [`customPalettes:`](Ruleset-CustomPalettes.md) full-color article art
is wired up. There is no explicit key for it.

## Per-type extra fields

| Key | Type | Default | Meaning | Types |
|---|---|---|---|---|
| `image_id` | string | — | The article's picture (a surface/sprite name from the mod, e.g. `UP004.SPK`). | 1, 2, 3, 5, 7, 10–17, 18, 19 |
| `rect_stats` | rect | 0,0,0,0 | Position/size of the stats block. | 1, 18, 19 |
| `rect_text` | rect | 0,0,0,0 | Position/size of the description block. | 1, 7, 18, 19 |
| `rect_armor` | rect | 0,0,0,0 | Position/size of the armor block. | 19 |
| `text_width` | int | 0 (157 for TFTD types) | Width in pixels of the text column. | 7, 10–17 |
| `align_bottom` | bool | false | Anchor the text block to the bottom of its rect instead of the top. | 7 |
| `weapon` | string item | — | The weapon this ammo/vehicle article should show weapon stats for. | 3, 4, 10–17 |
| `unit_mode` | int | 0 | 0 = show unit as hostile (with per-difficulty columns), 1 = show as neutral/allied, 2 = show both. | 19 |
| `psi_skill_mode` | int | 0 | How psi skill is listed: 0 = hidden (cap only), 1 = processed (max…max×1.5), 2 = raw min/max. | 18 |

A **rect** is a map of `x`, `y`, `width`, `height` (ints, each defaulting to 0) — see
[`read(..., ArticleDefinitionRect*)`](../src/Mod/ArticleDefinition.cpp).

## Multi-page articles

```yaml
  - id: STR_HEAVY_CANNON
    type_id: 4
    section: STR_WEAPONS_AND_EQUIPMENT
    text: STR_HEAVY_CANNON_UFOPEDIA      # page 1 (inherited by every page below)
    pages:
      - { ammoSlot: 0 }                  # page 1: stats for the first ammo slot
      - { ammoSlot: 1, text: STR_HEAVY_CANNON_UFOPEDIA_2 }
```

Pages start as copies of the page-1 fields, so you only list what differs. `getNumberOfPages()`
drives the prev/next buttons.

## See also

- [`items:`](Ruleset-Items.md) / [`armors:`](Ruleset-Armors.md) / [`crafts:`](Ruleset-Crafts.md) — the rules these articles document
- [`interfaces:`](Ruleset-Interfaces.md) — the `articleItem`/`articleArmor`/… entries that color these screens
- [`customPalettes:`](Ruleset-CustomPalettes.md) — the `_CPAL` full-color article art path
- [`extraStrings:`](Ruleset-ExtraStrings.md) — where the `title:`/`text:` string keys are defined
