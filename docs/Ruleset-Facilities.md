# Ruleset: `facilities:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`RuleBaseFacility`](../src/Mod/RuleBaseFacility.h) · **List key:** `type` ·
**Loader:** [`RuleBaseFacility::load`](../src/Mod/RuleBaseFacility.cpp)

A facility is one building in a base's 6×6 grid. It defines the build economics, the **capacities**
it contributes to the base (stores, quarters, labs, workshops, prison, hangars…), the **base
functions** it provides to other rules, its geoscape abilities (radar, defense, mind/grav shield),
and the battlescape map block used when the base is attacked.

```yaml
facilities:
  - type: STR_LABORATORY
    spriteShape: 2
    spriteFacility: 17
    buildCost: 750000
    buildTime: 26
    monthlyCost: 30000
    labs: 50
    provideBaseFunc: [RESEARCH]
    mapName: XBASE_04
    listOrder: 500
```

Entries **merge** across mods/files by `type`, support `refNode:` inheritance and `delete: true`
(see the [index](Ruleset.md#how-rulesets-load-the-60-second-version)).

## Identity, size & requirements

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | — | Unique id; also the facility's display-name string key. |
| `refNode` | map | — | Inherit all fields from another (anonymous) node first. |
| `ufopediaType` | string | `type` | UFOpaedia article opened for this facility. |
| `requires` | list of research | — | [Research](Ruleset-Research.md) topics needed before this facility can be built. |
| `size` | int | 1 | Backwards-compatible shorthand: sets both `sizeX` and `sizeY`. |
| `sizeX` / `sizeY` | int | 1 | Footprint in base grid squares (1 = the classic small facility). |
| `lift` | bool | false | This is the access lift — every base gets one and all facilities must connect to it. |
| `maxAllowedPerBase` | int | 0 | Maximum copies per base (0 = unlimited). |
| `fakeUnderwater` | int | −1 | Base-type filter: −1 = any base, 0 = surface bases only, 1 = fake-underwater bases only. |
| `listOrder` | int | auto | Sort position in the build list. |

## Cost & construction

| Key | Type | Default | Meaning |
|---|---|---|---|
| `buildCost` | int $ | 0 | Money to build. |
| `refundValue` | int $ | 0 | Money returned when the facility is dismantled. |
| `buildCostItems` | map item → `{build, refund}` | — | [Items](Ruleset-Items.md) consumed to build (`build`) and returned on dismantle (`refund`); an entry with both ≤ 0 is dropped. |
| `buildTime` | int days | 0 | Construction time. |
| `monthlyCost` | int $ | 0 | Upkeep charged each month once built. |

## Capacities

Every facility in a base contributes its capacities additively; the Basescape screens compare the
sum against what is in use.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `storage` | int | 0 | General stores space added. |
| `personnel` | int | 0 | Living quarters (soldiers + scientists + engineers). |
| `aliens` | int | 0 | Live-alien containment slots (of the `prisonType` below). |
| `prisonType` | int | 0 | Which containment "flavor" the `aliens` slots belong to (0 = classic alien containment; items declare which prison type they need). |
| `crafts` | int | 0 | Hangar slots for [craft](Ruleset-Crafts.md). |
| `labs` | int | 0 | Scientist slots for [research](Ruleset-Research.md) projects. |
| `workshops` | int | 0 | Engineer slots for [manufacture](Ruleset-Manufacture.md) projects. |
| `psiLabs` | int | 0 | Soldiers that can be enrolled in monthly psi training. |
| `trainingRooms` | int | 0 | Soldiers that can be enrolled in monthly physical training. |

Recovery/medical support at the base:

| Key | Type | Default | Meaning |
|---|---|---|---|
| `healthRecoveryPerDay` | int | 0 | Extra HP healed per day for soldiers recovering at this base. |
| `manaRecoveryPerDay` | int | 0 | Extra mana recovered per day. |
| `sickBayAbsoluteBonus` | float | 0.0 | Flat extra wound-recovery progress per day. |
| `sickBayRelativeBonus` | float | 0.0 | Extra wound-recovery progress per day as a percentage of the soldier's max HP. |

## Base functions (`provideBaseFunc` / `requiresBaseFunc` / `forbiddenBaseFunc`)

The base-function system is OXCE's generic "does this base have X?" mechanism, and it replaces
hard-coded facility checks throughout the engine. A base function is just a **mod-defined tag name**
(`RESEARCH`, `WORKSHOP`, `PSILAB`, `ALIENCONT`, anything you invent). Names are interned into a
bitset ([`Mod::loadBaseFunction`](../src/Mod/Mod.cpp)), so they cost nothing at runtime and any rule
that has a `requiresBaseFunc:` key is satisfied by the *union* of everything the base provides.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `provideBaseFunc` | list of tags | — | Function tags this facility grants to its base once fully built. |
| `requiresBaseFunc` | list of tags | — | Function tags that must already be provided by the base before this facility may be built. |
| `forbiddenBaseFunc` | list of tags | — | Function tags this facility **blocks** — it cannot coexist with providers of these tags. |

All three accept the list-modifier helper syntax used across OXCE rulesets: a plain list *replaces*
the inherited set, while `provideBaseFunc: {add: [X], remove: [Y]}`-style add/remove helpers edit it.

Consumers of the same tag vocabulary (all take a `requiresBaseFunc:` list):

- [`research:`](Ruleset-Research.md) — a topic can only be researched at a base with those functions
- [`manufacture:`](Ruleset-Manufacture.md) — likewise for production, plus `manufactureShortcut:`
  can roll the requirements of sub-projects up into the shortcut
- [`crafts:`](Ruleset-Crafts.md) — `requiresBuyBaseFunc:` for purchasing
- [`items:`](Ruleset-Items.md), soldier transformations, and other rules with base gates

Note that [`countries:`](Ruleset-Countries.md) and [`regions:`](Ruleset-Regions.md) can also provide
and forbid base functions to bases inside them (see [`Base::setupBaseFunctions`](../src/Savegame/Base.cpp)),
so a base's function set is not purely its facility list.

## Geoscape abilities

| Key | Type | Default | Meaning |
|---|---|---|---|
| `radarRange` | int nmi | 0 | UFO detection radius. |
| `radarChance` | int % | 0 | Chance per detection tick that a UFO in range is spotted. |
| `sightRange` | int nmi | 0 | Radius in which alien bases can be discovered. |
| `sightChance` | int % | 0 | Chance to discover an alien base in `sightRange`. |
| `hyper` | bool | false | Hyperwave decoder: reveals full UFO details (mission, race). |
| `mind` | bool | false | Mind shield: hides the base from alien detection. |
| `mindPower` | int | 1 | Strength of the mind shield (stacked shields multiply the concealment). |
| `grav` | bool | false | Grav shield: base defenses fire an extra round. |
| `missileAttraction` | int weight | 100 | Relative weight for being picked as the facility a UFO destroys when it damages the base (0 = never; the lift is never picked). |

## Base defense weapon

| Key | Type | Default | Meaning |
|---|---|---|---|
| `defense` | int | 0 | Damage per shot against an attacking UFO (0 = not a defense facility). |
| `hitRatio` | int % | 0 | Chance to hit the UFO. |
| `unifiedDamageFormula` | bool | false | Roll damage through the `ammoItem`'s damage type instead of the flat vanilla formula (requires `ammoItem`). |
| `shieldDamageModifier` | int % | 100 | Effectiveness against UFO shields. |
| `ammoItem` | string item | — | [Item](Ruleset-Items.md) consumed as ammunition. |
| `ammoMax` | int | 0 | Rounds the facility stores (0 = draws directly from base stores each shot). |
| `ammoNeeded` | int | 1 | Rounds spent per shot (≤ 0 = free/never fires on ammo). |
| `rearmRate` | int | 1 | Rounds restocked per hour. |
| `fireSound` | sound id (GEO.CAT) | 0 | Sound when the defense fires. |
| `hitSound` | sound id (GEO.CAT) | 0 | Sound when the defense hits. |

## Appearance & Basescape UI

| Key | Type | Default | Meaning |
|---|---|---|---|
| `spriteShape` | int | −1 | `BASEBITS.PCK` sprite of the building's outer shape. |
| `spriteFacility` | int | −1 | `BASEBITS.PCK` sprite of the contents drawn inside the shape. |
| `spriteEnabled` | bool | false | Draw `spriteFacility` over the shape for facilities larger than 1×1 (small facilities always do). |
| `connectorsDisabled` | bool | false | Don't draw/require corridor connectors to this facility. |
| `placeSound` | sound id (GEO.CAT) | −1 | Sound played when the facility is placed. |
| `rightClickActionType` | int | 0 | Screen opened by right-clicking the facility: 0 none, 1 prison, 2 manufacture, 3 research, 4 training, 5 psi training, 6 soldiers, 7 sell. |

## Upgrading, replacing & destruction

| Key | Type | Default | Meaning |
|---|---|---|---|
| `canBeBuiltOver` | bool | false | Any other facility may be built on top of this one (unrestricted upgrade target). |
| `buildOverFacilities` | list of facilities | — | The specific facilities *this* facility is allowed to replace (used when the old one doesn't set `canBeBuiltOver`). |
| `upgradeOnly` | bool | false | This facility may only be built over an existing one, never on empty ground. |
| `leavesBehindOnSell` | list of facilities | — | Facility/facilities left in place when this one is dismantled — either one of the **same size**, or several **1×1** ones. |
| `removalTime` | int days | 0 | Build time for those leave-behind facilities (−1 = use their own `buildTime`, 0 = instant). |
| `destroyedFacility` | string facility | — | The ruined version this facility turns into when destroyed (must have the same size). |

## Battlescape (base defense mission)

| Key | Type | Default | Meaning |
|---|---|---|---|
| `mapName` | string | — | **Required.** Map block used for this facility when the base is attacked. |
| `verticalLevels` | list | — | Multi-level map assembly for the facility's block (same `type`/`levelGroups`/`levelBlocks`/`levelTerrain` shape as [map scripts](Ruleset-MapScripts.md)). |
| `storageTiles` | list of `[x, y, z]` | — | Tiles where stored items are piled during a base-defense mission (empty = the vanilla checkerboard; a single `[-1,-1,-1]` disables storage placement). Positions are validated against the facility's `10*sizeX` × `10*sizeY` area. |

## See also

- [`research:`](Ruleset-Research.md) — `requires:` gates, and `requiresBaseFunc:` consumers
- [`manufacture:`](Ruleset-Manufacture.md) — workshop space and base-function requirements
- [`items:`](Ruleset-Items.md) — `buildCostItems`, defense `ammoItem`, prison-type consumers
- [`crafts:`](Ruleset-Crafts.md) — hangar (`crafts:`) capacity and `requiresBuyBaseFunc:`
- [`terrains:`](Ruleset-Terrains.md) / [`mapScripts:`](Ruleset-MapScripts.md) — the base-defense map
