# Ruleset: `crafts:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`RuleCraft`](../src/Mod/RuleCraft.h) · **List key:** `type` · **Loader:**
[`RuleCraft::load`](../src/Mod/RuleCraft.cpp)

A craft type defines a **player vehicle**: purchase/rent economics, geoscape flight stats,
dogfight stats, weapon hardpoints, troop/HWP capacity, pilot requirements, and the battlescape
map + spawn layout used on missions. Craft carry [`craftWeapons:`](Ruleset-CraftWeapons.md) and
fight [`ufos:`](Ruleset-Ufos.md).

```yaml
crafts:
  - type: STR_INTERCEPTOR
    sprite: 3
    fuelMax: 1000                 # craft-stats block, inline at top level
    damageMax: 100
    speedMax: 2100
    accel: 3
    weapons: 2
    costBuy: 600000
    costRent: 600000
    refuelRate: 50
    repairRate: 1
    transferTime: 96
    score: 250
```

Entries **merge** across mods/files by `type`, support `refNode:` inheritance and `delete: true`
(see the [index](Ruleset.md#how-rulesets-load-the-60-second-version)).

## Identity & acquisition

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | — | Unique id; also the craft's display-name string key. |
| `refNode` | map | — | Inherit all fields from another (anonymous) node first. |
| `requires` | list of research | — | [Research](Ruleset-Research.md) needed before the craft appears (purchase/manufacture). |
| `requiresBuyBaseFunc` | list of tags | — | Base-function tags ([facilities](Ruleset-Facilities.md)) the buying base must provide. |
| `requiresBuyCountry` | string country | — | The named [country](Ruleset-Countries.md) must not have signed an alien pact for the craft to be purchasable. |
| `costBuy` | int $ | 0 | Purchase cost (0 = not purchasable). |
| `costRent` | int $ | 0 | Monthly rental/maintenance cost. |
| `costSell` | int $ | 0 | Sale value (rented craft should use 0). |
| `monthlyBuyLimit` | int | 0 | Maximum purchases per month (0 = unlimited). |
| `monthlyBuyLimitMessage` | string | — | String key of the message shown when the monthly limit blocks a purchase. |
| `transferTime` | int hours | 24 | Delivery time when purchased/transferred. |
| `score` | int | 0 | Points lost when this craft is destroyed. |
| `listOrder` | int | auto (+100) | Sort position in lists. |
| `forceShowInMonthlyCosts` | bool | false | Always list this craft type in Monthly Costs, even with none owned. |

## The craft-stats block

These keys (`RuleCraftStats`) load **inline at the entry's top level**. The same block appears as
the bonus `stats:` of [craft weapons](Ruleset-CraftWeapons.md) (additive when equipped) and, with
two extra keys, at the top level of [`ufos:`](Ruleset-Ufos.md).

| Key | Type | Default | Meaning |
|---|---|---|---|
| `fuelMax` | int | 0 | Fuel capacity (units, or refuel-items' worth when `refuelItem` is set). |
| `damageMax` | int | 0 | Hull hit points. |
| `speedMax` | int knots | 0 | Top geoscape speed. |
| `accel` | int | 0 | Acceleration for takeoff/intercept speed changes. |
| `radarRange` | int nmi | 672 | Onboard radar detection range. |
| `radarChance` | int % | 100 | Onboard radar detection chance. |
| `sightRange` | int nmi | 1696 | Range for spotting alien bases while patrolling. |
| `hitBonus` | int % | 0 | Dogfight accuracy bonus. |
| `avoidBonus` | int % | 0 | Dogfight dodge bonus (reduces enemy hit chance). |
| `avoidBonus2` | int % | 0 | Second dodge term (applies when standing off / evasive maneuvers). |
| `powerBonus` | int % | 0 | Percentage bonus to equipped weapons' damage. |
| `armor` | int | 0 | Flat damage subtracted from each hit taken. |
| `shieldCapacity` | int | 0 | Energy shield points. |
| `shieldRecharge` | int | 0 | Shield points recharged per 5s while in a dogfight. |
| `shieldRechargeInGeoscape` | int | 0 | Shield recharge per hour while flying the geoscape. |
| `shieldBleedThrough` | int % | 0 | Portion of shield-overflow damage that reaches the hull. |
| `soldiers` | int | 0 | Default unit capacity (soldiers + vehicles, small and large). |
| `vehicles` | int | 0 | Default capacity for vehicles and 2×2 soldiers. |
| `maxItems` | int | 999999 | Cap on equipment item count carried. |
| `maxStorageSpace` | double | 99999.0 | Cap on equipment storage size carried. |

Craft-weapon `stats:` blocks add to these (so a weapon module can grant, say, `soldiers: 4`).

## Capacity fine-tuning

All of these default to −1 = "no separate limit". `maxUnitsLimit`/`maxHWPUnitsLimit` are the hard
caps *including* weapon-module bonuses; −1 falls back to the base `soldiers`/`vehicles` values.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `maxUnitsLimit` | int | −1 → `soldiers` | Absolute unit-capacity cap even with capacity-boosting weapon modules. |
| `maxHWPUnitsLimit` | int | −1 → `vehicles` | Absolute vehicle/2×2 cap even with weapon modules. |
| `maxSmallSoldiers` / `maxLargeSoldiers` | int | −1 | Cap on 1×1 / 2×2 soldiers specifically. |
| `maxSmallVehicles` / `maxLargeVehicles` | int | −1 | Cap on 1×1 / 2×2 vehicles (HWPs) specifically. |
| `maxSmallUnits` / `maxLargeUnits` | int | −1 | Cap on all 1×1 / 2×2 units. |
| `maxSoldiers` / `maxVehicles` | int | −1 | Cap on all soldiers / all vehicles. |

## Pilots

| Key | Type | Default | Meaning |
|---|---|---|---|
| `pilots` | int | 0 | Number of pilots required to take off (0 = craft needs no pilots). |
| `pilotMinStatsRequired` | [UnitStats](Ruleset-UnitStats.md) map | all 0 | Minimum stats a soldier needs to count as a pilot. |
| `pilotSoldierBonusesRequired` | list | — | [Soldier bonuses](Ruleset-SoldierBonuses.md) a soldier must have to pilot this craft. |

## Weapons

| Key | Type | Default | Meaning |
|---|---|---|---|
| `weapons` | int 0–4 | 0 | Number of weapon hardpoints. |
| `weaponTypes` | list (per slot) | all 0 | Weapon-type filter per slot: a scalar allows that one type, a list allows several — matched against each craft weapon's `weaponType`. |
| `weaponStrings` | list of strings | `STR_WEAPON_ONE`/`_TWO` | Per-slot label string keys in the craft screen. |
| `fixedWeapons` | list of craftWeapons | — | Craft weapon permanently installed in each slot (not removable). |

## Fuel, repair & shields

| Key | Type | Default | Meaning |
|---|---|---|---|
| `refuelItem` | string item | — | [Item](Ruleset-Items.md) consumed to refuel (e.g. Elerium); absent = fuel is free and burn rate scales with speed. |
| `refuelRate` | int | 1 | Fuel added per 30 minutes while refueling. |
| `repairRate` | int | 1 | Damage repaired per hour at base. |
| `shieldRechargedAtBase` | int | 1000 | Shield points recharged per hour while at base. |
| `notifyWhenRefueled` | bool | false | Show a popup when refueling completes. |

## Geoscape behavior

| Key | Type | Default | Meaning |
|---|---|---|---|
| `sprite` | int | −1 | Sprite index for the Basescape/Equip screens (`BASEBITS.PCK` +33, `INTICON.PCK` +11/+0; mod offset above 4). |
| `skinSprites` | list of ints | — | Alternative sprite indices for skins 1..N (skin 0 uses `sprite`). |
| `maxSkinIndex` | int | 0 | Highest selectable skin index. |
| `marker` | int | −1 | Globe marker sprite (−1 = engine default; mod offset above 8). |
| `autoPatrol` | bool | false | Craft may use the auto-patrol feature. |
| `undetectable` | bool | false | Invisible to hunter-killers and alien bases. |
| `patrolWithoutFuel` | bool | false | Patrolling consumes no fuel. |
| `missilePower` | int | 0 | > 0 turns the craft into a single-use player missile dealing this damage (craft is lost on hit). |
| `maxAltitude` | int | −1 | −1 = normal aircraft; ≥ 0 marks a **water-only** craft (TFTD-style sub) that can only engage over water, with this max dogfight altitude (0–4). |
| `defaultAltitude` | string | `STR_VERY_LOW` | Altitude string displayed by default (display only). |
| `spacecraft` | bool | false | Can fly the final (Cydonia/T'leth) mission. |
| `allowLanding` | bool | true | May land at mission sites (false = intercept-only). |
| `keepCraftAfterFailedMission` | bool | false | The craft is not lost when its battle is aborted/lost (e.g. paratrooper insertions). |
| `selectSound` | sound id/list (GEO.CAT) | — | Sound when selecting the craft on the globe (random pick from a list). |
| `takeoffSound` | sound id/list (GEO.CAT) | — | Sound on takeoff from base (random pick from a list). |

## Battlescape

| Key | Type | Default | Meaning |
|---|---|---|---|
| `battlescapeTerrainData` | map | — | Inline [terrain](Ruleset-Terrains.md) definition (`name`, `mapDataSets`, `mapBlocks`) — the craft's own map placed on the mission map. Required for New Battle support. |
| `deployment` | list of `[x, y, z, facing]` | — | Ordered soldier spawn positions inside the craft map. |
| `useAllStartTiles` | bool | false | Let units spawn on any start tile of the craft map, not just the `deployment` list. |
| `craftInventoryTile` | `[x, y, z]` | — | Tile where left-behind equipment is piled during the pre-battle inventory. |
| `mapVisible` | bool | true | Whether the craft map is revealed at battle start. |
| `customPreview` | string deployment | `STR_CRAFT_DEPLOYMENT_PREVIEW` | [alienDeployment](Ruleset-AlienDeployments.md) used for the craft deployment-preview feature. |

## Groups & boarding restrictions

| Key | Type | Default | Meaning |
|---|---|---|---|
| `groups` | list of ints | — | Craft group ids (matched by map-script craft filters). |
| `allowedSoldierGroups` | list of ints | — | Only [soldier types](Ruleset-Soldiers.md) in these groups may board. |
| `allowedArmorGroups` | list of ints | — | Only [armors](Ruleset-Armors.md) with these `group` ids may board. |
| `limitArmorGroups` | map group → int | — | Per-armor-group headcount limits aboard. |
| `onlyOneSoldierGroupAllowed` | bool | false | All boarded soldiers must share a single soldier group. |

## Scripting

Craft expose Y-Script hooks (`scripts:` sub-node, `craftScripts`) and custom `tags:`. See
[Ruleset-Scripting.md](Ruleset-Scripting.md).

## See also

- [`craftWeapons:`](Ruleset-CraftWeapons.md) — the weapons/modules filling the hardpoints
- [`ufos:`](Ruleset-Ufos.md) — the opposition (shares the craft-stats block)
- [`terrains:`](Ruleset-Terrains.md) — the inline craft map definition
- [`facilities:`](Ruleset-Facilities.md) — hangars (craft capacity) and `requiresBuyBaseFunc` providers
