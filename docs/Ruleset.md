# OpenXcom DX — Ruleset Reference

This is the index of the ruleset documentation: every **top-level node** ("root") a `.rul` file may
contain, grouped by domain, with a link to its detailed reference doc. The list was extracted from
the actual loader ([Mod::loadFile](../src/Mod/Mod.cpp)), so it is the authoritative set for **this**
engine (OpenXcom DX on OXCE-Plus) — including DX-only roots, which are tagged **[DX]**.

Every root below is documented; the **Doc** column links to its reference page.

## How rulesets load (the 60-second version)

- A mod is a folder; every `*.rul` file in it (any subdirectory) is YAML with some subset of the
  roots below. Files and mods **merge**: later mods layer over earlier ones, and a rule with the
  same key (`type:`/`name:`/`id:`) *updates* the existing entry rather than replacing the list.
- Most roots are **lists of rules** keyed by `type:` (or `name:`/`id:` — noted per root). Adding
  `delete: true` on an entry removes an inherited rule.
- `refNode:` inside an entry copies another node's fields as defaults before the entry's own
  fields apply — the standard way to make item/armor families (see any per-root doc).
- A handful of roots are **singletons** (one map, not a list): global tuning knobs where later
  definitions override field-by-field.
- Strings shown to the player are `STR_*` keys resolved through the language files /
  `extraStrings:` — rules never contain display text directly.

## Units, soldiers & armor

| Root | Key | Engine class | What it defines | Doc |
|---|---|---|---|---|
| `armors:` | `type` | `Armor` | Armor suits / battle-unit bodies: protection, movement, vision, sprites | ● [Ruleset-Armors.md](Ruleset-Armors.md) |
| `units:` | `type` | `Unit` | Non-soldier battle units (aliens, civilians, HWPs) | ● [Ruleset-Units.md](Ruleset-Units.md) |
| `soldiers:` | `type` | `RuleSoldier` | Recruitable soldier types | ● [Ruleset-Soldiers.md](Ruleset-Soldiers.md) |
| `skills:` | `type` | `RuleSkill` | Active soldier skills (hotkey abilities) | ● [Ruleset-Skills.md](Ruleset-Skills.md) |
| `soldierBonuses:` | `name` | `RuleSoldierBonus` | Stat bonus layers granted by commendations/transformations | ● [Ruleset-SoldierBonuses.md](Ruleset-SoldierBonuses.md) |
| `soldierTransformation:` | `name` | `RuleSoldierTransformation` | Soldier conversions/augmentations | ● [Ruleset-SoldierTransformation.md](Ruleset-SoldierTransformation.md) |
| `commendations:` | `type` | `RuleCommendations` | Medals and their award criteria | ● [Ruleset-Commendations.md](Ruleset-Commendations.md) |
| `alienRaces:` | `id` | `AlienRace` | Alien race member lists (rank → unit type) | ● [Ruleset-AlienRaces.md](Ruleset-AlienRaces.md) |
| `roles:` **[DX]** | `name` | `RuleRole` | Seed soldier roles | ● [Ruleset-Roles.md](Ruleset-Roles.md) |
| `roleIcons:` **[DX]** | `name` | `RuleRoleIcon` | Named role icon surfaces | ● [Ruleset-Roles.md](Ruleset-Roles.md) |

## Items & inventory

| Root | Key | Engine class | What it defines | Doc |
|---|---|---|---|---|
| `items:` | `type` | `RuleItem` | Items: weapons, ammo, grenades, medikits, equipment, corpses | ● [Ruleset-Items.md](Ruleset-Items.md) |
| `itemCategories:` | `type` | `RuleItemCategory` | Item categories for base-screen filters | ● [Ruleset-ItemCategories.md](Ruleset-ItemCategories.md) |
| `weaponSets:` | `type` | `RuleWeaponSet` | Reusable weapon lists | ● [Ruleset-WeaponSets.md](Ruleset-WeaponSets.md) |
| `invs:` | `id` | `RuleInventory` | Inventory sections (slots): geometry, move costs, filters | ● [Ruleset-Invs.md](Ruleset-Invs.md) |
| `inventoryLayouts:` **[DX]** | `id` | `RuleInventoryLayout` | Per-armor inventory slot sets | ● [Ruleset-InventoryLayouts.md](Ruleset-InventoryLayouts.md) |

## Battlescape content

| Root | Key | Engine class | What it defines | Doc |
|---|---|---|---|---|
| `terrains:` | `name` | `RuleTerrain` | Tilesets + map blocks | ● [Ruleset-Terrains.md](Ruleset-Terrains.md) |
| `mapScripts:` | `type` | `MapScript` | Map-assembly command scripts | ● [Ruleset-MapScripts.md](Ruleset-MapScripts.md) |
| `alienDeployments:` | `type` | `AlienDeployment` | Mission unit rosters and battle setup | ● [Ruleset-AlienDeployments.md](Ruleset-AlienDeployments.md) |
| `startingConditions:` | `type` | `RuleStartingCondition` | Pre-battle constraints (allowed armor/craft/items) | ● [Ruleset-StartingConditions.md](Ruleset-StartingConditions.md) |
| `enviroEffects:` | `type` | `RuleEnviroEffects` | Per-terrain environmental effects | ● [Ruleset-EnviroEffects.md](Ruleset-EnviroEffects.md) |
| `MCDPatches:` | `type` | `MCDPatch` | Terrain tile (MCD) property patches | ● [Ruleset-MCDPatches.md](Ruleset-MCDPatches.md) |

## Geoscape & campaign

| Root | Key | Engine class | What it defines | Doc |
|---|---|---|---|---|
| `countries:` | `type` | `RuleCountry` | Funding nations | ● [Ruleset-Countries.md](Ruleset-Countries.md) |
| `regions:` | `type` | `RuleRegion` | Geoscape regions: mission zones, cities | ● [Ruleset-Regions.md](Ruleset-Regions.md) |
| `extraGlobeLabels:` | `type` | `RuleCountry` | Extra globe text labels | ● [Ruleset-Countries.md](Ruleset-Countries.md) |
| `globe:` | singleton | `RuleGlobe` | Globe geometry: polygons, textures | ● [Ruleset-Globe.md](Ruleset-Globe.md) |
| `ufos:` | `type` | `RuleUfo` | UFO types | ● [Ruleset-Ufos.md](Ruleset-Ufos.md) |
| `ufoTrajectories:` | `id` | `UfoTrajectory` | UFO flight patterns | ● [Ruleset-UfoTrajectories.md](Ruleset-UfoTrajectories.md) |
| `alienMissions:` | `type` | `RuleAlienMission` | Alien mission types: waves, races, scoring | ● [Ruleset-AlienMissions.md](Ruleset-AlienMissions.md) |
| `missionScripts:` | `type` | `RuleMissionScript` | Monthly mission generation | ● [Ruleset-MissionScripts.md](Ruleset-MissionScripts.md) |
| `arcScripts:` | `type` | `RuleArcScript` | Story arc sequencing | ● [Ruleset-ArcScripts.md](Ruleset-ArcScripts.md) |
| `eventScripts:` | `type` | `RuleEventScript` | Geoscape event generation | ● [Ruleset-EventScripts.md](Ruleset-EventScripts.md) |
| `events:` | `name` | `RuleEvent` | Individual geoscape events | ● [Ruleset-Events.md](Ruleset-Events.md) |
| `adhocScripts:` | `type` | `RuleMissionScript` | Ad-hoc mission generation hooks | ● [Ruleset-AdhocScripts.md](Ruleset-AdhocScripts.md) |

## Basescape & economy

| Root | Key | Engine class | What it defines | Doc |
|---|---|---|---|---|
| `facilities:` | `type` | `RuleBaseFacility` | Base facilities | ● [Ruleset-Facilities.md](Ruleset-Facilities.md) |
| `research:` | `name` | `RuleResearch` | The tech tree | ● [Ruleset-Research.md](Ruleset-Research.md) |
| `manufacture:` | `name` | `RuleManufacture` | Workshop projects | ● [Ruleset-Manufacture.md](Ruleset-Manufacture.md) |
| `manufactureShortcut:` | `name` | `RuleManufactureShortcut` | Manufacture variants that skip owned inputs | ● [Ruleset-Manufacture.md](Ruleset-Manufacture.md) |
| `crafts:` | `type` | `RuleCraft` | Player craft | ● [Ruleset-Crafts.md](Ruleset-Crafts.md) |
| `craftWeapons:` | `type` | `RuleCraftWeapon` | Craft weapons | ● [Ruleset-CraftWeapons.md](Ruleset-CraftWeapons.md) |

## Presentation, UI & assets

| Root | Key | Engine class | What it defines | Doc |
|---|---|---|---|---|
| `ufopaedia:` | `id` | `ArticleDefinition` | UFOpaedia articles | ● [Ruleset-Ufopaedia.md](Ruleset-Ufopaedia.md) |
| `extraSprites:` | `type` | `ExtraSprites` | Image assets / surface sets | ● [Ruleset-ExtraSprites.md](Ruleset-ExtraSprites.md) |
| `extraSounds:` | `type` | `ExtraSounds` | Sound assets | ● [Ruleset-ExtraSounds.md](Ruleset-ExtraSounds.md) |
| `extraStrings:` | `type` (language) | `ExtraStrings` | Translation strings | ● [Ruleset-ExtraStrings.md](Ruleset-ExtraStrings.md) |
| `statStrings:` | — | `StatString` | Stat-derived soldier name suffixes | ● [Ruleset-StatStrings.md](Ruleset-StatStrings.md) |
| `interfaces:` | `type` | `RuleInterface` | Per-screen UI element positions/colors | ● [Ruleset-Interfaces.md](Ruleset-Interfaces.md) |
| `cutscenes:` | `type` | `RuleVideo` | Cutscenes | ● [Ruleset-Cutscenes.md](Ruleset-Cutscenes.md) |
| `musics:` | `type` | `RuleMusic` | Music tracks | ● [Ruleset-Musics.md](Ruleset-Musics.md) |
| `soundDefs:` * | `type` | `SoundDefinition` | CAT sound remapping (TFTD) | ● [Ruleset-SoundDefs.md](Ruleset-SoundDefs.md) |
| `customPalettes:` | `type` | `CustomPalettes` | Palettes | ● [Ruleset-CustomPalettes.md](Ruleset-CustomPalettes.md) |
| `transparencyLUTs:` * | — | (Mod tables) | Tint/opacity lookup tables | ● [Ruleset-TransparencyLUTs.md](Ruleset-TransparencyLUTs.md) |
| `unitResponseSounds:` | `name` | (Mod tables) | Unit voice response banks | ● [Ruleset-UnitResponseSounds.md](Ruleset-UnitResponseSounds.md) |

\* `soundDefs:` and `transparencyLUTs:` are **not** read from ordinary `.rul` files: the engine parses
them only from the single file a mod names in `resourceConfig:` in its `metadata.yml` (xcom2 uses
`vars.rul`; xcom1 declares none).

## Global tuning (singleton nodes)

These are one-map nodes, not keyed lists. The catch-all doc covers the small ones; larger ones get
their own page.

| Root | What it tunes | Doc |
|---|---|---|
| `startingBase:` (+ `startingBaseBeginner:` … `startingBaseSuperhuman:`) | The initial base layout/inventory, optionally per difficulty | ● [Ruleset-StartingBase.md](Ruleset-StartingBase.md) |
| `startingTime:`, `startingDifficulty:` | Campaign start clock/difficulty | ● [Ruleset-Globals.md](Ruleset-Globals.md) |
| `baseNamesFirst/Middle/Last:`, `operationNamesFirst/Last:` | Random base/operation name pools | ● [Ruleset-Globals.md](Ruleset-Globals.md) |
| `missionRatings:`, `monthlyRatings:` | Score → rating label tables | ● [Ruleset-Globals.md](Ruleset-Globals.md) |
| `difficultyCoefficient:` & friends, `aimAndArmorMultipliers:`, `statGrowthMultipliers(Abs):` | Difficulty scaling knobs | ● [Ruleset-Globals.md](Ruleset-Globals.md) |
| `constants:` | Battlescape/geoscape constants (sounds, animation frames, timing, blast limits…) | ● [Ruleset-Constants.md](Ruleset-Constants.md) |
| `lighting:` | OXCE enhanced-lighting mode (`enhanced` bitmask, `maxStatic`/`maxDynamic`) | ● [Ruleset-Globals.md](Ruleset-Globals.md) |
| `extended:` | Y-Script tags and global script config | ● [Ruleset-Scripting.md](Ruleset-Scripting.md) |
| `converter:` | Save-compat id remapping | ● [Ruleset-Globals.md](Ruleset-Globals.md) |
| `fixedUserOptions:`, `recommendedUserOptions:` | Options a mod forces/suggests | ● [Ruleset-Globals.md](Ruleset-Globals.md) |
| `metadata.yml` | Mod identity/version/master (the file's document root, not a `mod:` node) | ● [Ruleset-Globals.md](Ruleset-Globals.md) |

### DX-added singletons **[DX]**

| Root | What it tunes | Doc |
|---|---|---|
| `damageTypes:` | Edits the built-in damage types' `RuleDamageType` fields globally (early pre-pass) | ● [Ruleset-DamageTypes.md](Ruleset-DamageTypes.md) |
| `health:` * | Proportional wound recovery + Field Surgery gate | ● [Ruleset-DX-Globals.md](Ruleset-DX-Globals.md) |
| `moveCostDefaults:` | Mod-wide default armor move costs (what `moveCost:`-less armors fall back to) | ● [Ruleset-DX-Globals.md](Ruleset-DX-Globals.md) |
| `evasionDefaults:` | Sprint/sneak defensive-evasion reshaping defaults | ● [Ruleset-DX-Globals.md](Ruleset-DX-Globals.md) |
| `sneakDefaults:` | Sneak-mode gates (`maxLight` — no creeping while glowing) | ● [Ruleset-DX-Globals.md](Ruleset-DX-Globals.md) |
| `overwatchDefaults:` | Mod-wide overwatch tuning that weapons fall back to | ● [Ruleset-DX-Globals.md](Ruleset-DX-Globals.md) |
| `bleedoutDefaults:` | Bleedout thresholds/buffer wounds/mission lockout | ● [Ruleset-DX-Globals.md](Ruleset-DX-Globals.md) |
| `soldierArmorBaseColors:` | The palette color list the per-role armor recolor picks from | ● [Ruleset-Roles.md](Ruleset-Roles.md) |

\* `health:` itself is a stock OXCE node (`woundThreshold`, `replenishAfterMission`); DX extends it
with the proportional-recovery / Field Surgery keys.

## Shared building blocks

Structures reused across many rule types get their own page, and the per-root docs link to them
instead of re-explaining:

| Chunk | Used by | Doc |
|---|---|---|
| **Stat bonus formula** (`RuleStatBonus`) | armor recovery/psiDefence/meleeDodge, item damage/accuracy bonuses, soldier bonuses, skills | ● [Ruleset-StatBonus.md](Ruleset-StatBonus.md) |
| **Unit stats block** (`UnitStats`) | soldiers, units, armor `stats:`/`statModifiers:`, items | ● [Ruleset-UnitStats.md](Ruleset-UnitStats.md) |
| **Damage type** (`RuleDamageType` / `damageAlter:`) | items, [DX] global `damageTypes:` | ● [Ruleset-DamageTypes.md](Ruleset-DamageTypes.md) |
| **Use cost/flat blocks** (`RuleItemUseCost`) | items (`costAimed`/`flatSnap`/…), skills | ● [Ruleset-UseCost.md](Ruleset-UseCost.md) |
| **Move cost pairs** (`[time%, energy%]`) | armors `moveCost:`, [DX] `moveCostDefaults:` | ● [Ruleset-Armors.md](Ruleset-Armors.md#movement) |
| **Y-Script hooks & tags** | armors, items, units, skills, `extended:` | ● [Ruleset-Scripting.md](Ruleset-Scripting.md) |

---

*Upstream bugs:* where DX has fixed — or knowingly left alone — a bug in inherited OXCE behavior, the
affected page says so and links to [DX-OXCE-Fixes.md](../DX-OXCE-Fixes.md), the register of those
deltas.

*Conventions used throughout these docs:* field tables give **Key · Type · Default · Meaning**;
**[DX]** marks fields/roots added by OpenXcom DX (absent in stock OXCE); engine source is linked as
the ultimate authority. See also [DX-Features.md](../DX-Features.md) for feature-level descriptions
and [Extended.txt](../Extended.txt) for the historical OXCE changelog.
