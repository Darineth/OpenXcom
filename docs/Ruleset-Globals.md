# Ruleset: global singletons (the catch-all)

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`Mod`](../src/Mod/Mod.h) · **Node type:** singletons (one map/scalar each, not
keyed lists) · **Loader:** [`Mod::loadFile`](../src/Mod/Mod.cpp) — the block after the keyed rule
lists. Also: [`RuleConverter::load`](../src/Mod/RuleConverter.cpp),
[`ModInfo::load`](../src/Engine/ModInfo.cpp), [`GameTime::load`](../src/Savegame/GameTime.cpp).

This page collects the **small global tuning nodes**: the ones that are a handful of keys each and
don't warrant their own page. Bigger singletons live elsewhere —
[`startingBase:`](Ruleset-StartingBase.md), [`constants:`](Ruleset-Constants.md),
[`extended:`](Ruleset-Scripting.md), the [`ai:` node](Ruleset-AI.md), the
[DX globals](Ruleset-DX-Globals.md).

Singletons **merge field-by-field across mods**: a later mod setting one key leaves the rest of the
node alone (the exception is list-valued keys, which are replaced wholesale).

```yaml
startingTime:
  second: 0
  minute: 0
  hour: 12
  weekday: 6
  day: 1
  month: 1
  year: 1999

difficultyCoefficient: [0, 1, 2, 3, 4]
aimAndArmorMultipliers: [0.5, 1.0, 1.0, 1.0, 1.0]
statGrowthMultipliers:
  tu: 4
  reactions: 6
  firing: 6

lighting:
  enhanced: 7          # occlude fire (1) + item (2) + unit (4) light
```

---

## Campaign start

| Key | Type | Default | Meaning |
|---|---|---|---|
| `startingTime` | map | 12:00, Fri 1 Jan 1999 | The campaign's start clock. Sub-keys: `second`, `minute`, `hour`, `weekday` (1 = Sunday … 7 = Saturday), `day`, `month`, `year`. |
| `startingDifficulty` | int 0–4 | 0 | Which difficulty is pre-selected on the New Game screen (0 = Beginner … 4 = Superhuman). |
| `initialFunding` | int (thousands) | 0 | Target total in the game's `$1000` unit: per-country monthly funding is scaled up so the total matches (never reducing a country below its rolled value), and starting cash is set to that total. |

The base you start with is [`startingBase:`](Ruleset-StartingBase.md).

---

## Campaign rules & research gates

| Key | Type | Default | Meaning |
|---|---|---|---|
| `psiUnlockResearch` | research name | — | [Research](Ruleset-Research.md) that unlocks psi training (empty = the classic Psi Lab requirement alone). |
| `newBaseUnlockResearch` | research name | — | Research required before new X-COM bases can be built. |
| `fakeUnderwaterBaseUnlockResearch` | research name | — | Research required before bases can be built on `fakeUnderwater` globe textures. |
| `destroyedFacility` | facility name | — | [Facility](Ruleset-Facilities.md) that replaces facilities destroyed during base defense (instead of empty ground). |
| `alienFuel` | `[item, quantity]` | — | The item recovered from UFO power sources and how much per source (`xcom1` sets `[STR_ELERIUM_115, 50]`, `xcom2` `[STR_ZRBITE, 50]`). |
| `fontName` | filename | `Font.dat` (from the standard mods) | The font data file the mod loads. |

---

## `mana:` — the mana resource

A singleton sub-map enabling and tuning the OXCE **mana** stat (a mod-defined resource — psionic
energy, fatigue, …). The [DX `health:` node](Ruleset-DX-Globals.md#health) has the same
`woundThreshold`/`replenishAfterMission` pair for health.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `mana: enabled` | bool | false | Master switch for the whole mana feature. |
| `mana: battleUI` | bool | false | Show the mana bar in the battlescape unit stats. |
| `mana: unlockResearch` | research name | — | Research that reveals/activates mana handling. |
| `mana: trainingPrimary` | bool | false | Mana improves in training like a primary skill (e.g. firing). |
| `mana: trainingSecondary` | bool | false | Mana improves in training like a secondary skill (e.g. strength). |
| `mana: woundThreshold` | int | 200 | How much **missing** mana acts like fatal wounds and blocks deployment on a craft. |
| `mana: replenishAfterMission` | bool | true | Fully refill mana after each mission. |

---

## `gameOver:` & defeat

| Key | Type | Default | Meaning |
|---|---|---|---|
| `gameOver: loseMoney` | cutscene | `loseGame` | [Cutscene](Ruleset-Cutscenes.md) played when losing by economy (two months in debt). |
| `gameOver: loseRating` | cutscene | `loseGame` | Cutscene played when losing by two months of terrible ratings. |
| `gameOver: loseDefeat` | cutscene | `loseGame` | Cutscene played when losing the last base. |
| `defeatScore` | int | 0 | Shifts the monthly defeat threshold: a month scoring at or below `defeatScore + 100 × difficultyCoefficient` earns a warning, and a second such month loses the game. |
| `defeatFunds` | int $ | 0 | Month-end funds at or below this earn the "balance the books" warning; a second month in a row loses the game. |

---

## Random name pools

Names are assembled by picking one entry from each list. Each key is a **list of `STR_*` keys**
(resolved through the language files). A later mod's plain list **replaces** the inherited one;
the `!add` / `!remove` YAML tags append to / remove from it instead.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `baseNamesFirst` | list of strings | — | First word of a randomly suggested base name. |
| `baseNamesMiddle` | list of strings | — | Optional middle word. |
| `baseNamesLast` | list of strings | — | Optional last word. |
| `operationNamesFirst` | list of strings | — | First word of a random mission ("operation") name. |
| `operationNamesLast` | list of strings | — | Second word of a random operation name. |

---

## Score → rating labels

Both are **maps of `score → STR_ key`**; the engine picks the entry with the **highest threshold that
is ≤ the score**. Defining either one **replaces** the built-in `STR_RATING_TERRIBLE` … `EXCELLENT`
ladder entirely.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `missionRatings` | map int → string | — | Post-battle debriefing rating for the mission score ([`DebriefingState`](../src/Battlescape/DebriefingState.cpp)). |
| `monthlyRatings` | map int → string | — | Monthly-report rating for the month's total score ([`MonthlyReportState`](../src/Geoscape/MonthlyReportState.cpp)). |

```yaml
monthlyRatings:
  -9999: STR_RATING_TERRIBLE
  0: STR_RATING_OK
  500: STR_RATING_EXCELLENT
```

---

## Difficulty scaling

All the array-valued keys below are **5 entries, one per difficulty** (Beginner, Experienced,
Veteran, Genius, Superhuman). Missing entries keep the engine default.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `difficultyCoefficient` | list of 5 ints | `[0, 1, 2, 3, 4]` | The classic "MultiplierA": alien stat growth per difficulty, and the general difficulty scalar the geoscape uses (e.g. in the monthly rating threshold). |
| `aimAndArmorMultipliers` | list of 5 floats | `[0.5, 1, 1, 1, 1]` | Sets **both** `aimMultipliers` and `armorMultipliers` at once ("MultiplierB"). |
| `aimMultipliers` | list of 5 floats | from above | Multiplier on **alien firing accuracy** per difficulty. |
| `armorMultipliers` | list of 5 floats | from above | Multiplier on **alien armor values** per difficulty. |
| `armorMultipliersAbs` | list of 5 ints | all 0 | **Flat** bonus added to alien armor values per difficulty (applied on top of `armorMultipliers`). |
| `statGrowthMultipliers` | [UnitStats](Ruleset-UnitStats.md) map | see `xcom1/difficulty.rul` | Per-stat growth increment applied to aliens as `difficultyCoefficient` steps up. Written **once**, applied to all difficulties. |
| `statGrowthMultipliersAbs` | list of 5 UnitStats maps | all 0 | Per-difficulty **flat** stat additions for aliens — one UnitStats map per difficulty (unlike `statGrowthMultipliers`, this one is indexed). |
| `sellPriceCoefficient` | list of 5 ints % | all 100 | Sell price scaling per difficulty. |
| `buyPriceCoefficient` | list of 5 ints % | all 100 | Buy price scaling per difficulty. |
| `difficultyBasedRetaliationDelay` | list of 5 ints | all 0 | Extra delay (days) before alien retaliation missions launch, per difficulty. |
| `difficultyDemigod` | bool | false | "Demigod" mode: alien deployments spawn their **maximum** random quantity instead of rolling, and units force-spawn near a friend rather than failing placement. |

### `difficultyCoefficientOverrides:`

A sub-map of per-difficulty overrides for individual geoscape systems. Each key is a list indexed by
difficulty; if the list is shorter than the difficulty index, the engine falls back to its built-in
formula.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `monthlyRatingThresholds` | list of ints | — | Per-difficulty score threshold used to grade the monthly report (**negative values only**; a non-negative entry is ignored). |
| `ufoFiringRateCoefficients` | list of ints | — | Per-difficulty UFO rate of fire in dogfights. |
| `ufoEscapeCountdownCoefficients` | list of ints | — | Per-difficulty countdown before a UFO breaks off a dogfight. |
| `retaliationTriggerOdds` | list of ints | — | Per-difficulty odds that shooting a UFO down triggers retaliation. |
| `retaliationBaseRegionOdds` | list of ints | — | Per-difficulty odds that retaliation targets the region of the player's base. |
| `aliensFacingCraftOdds` | list of ints | — | Per-difficulty odds that aliens start the battle already facing the landed craft. |

---

## Lighting

| Key | Type | Default | Meaning |
|---|---|---|---|
| `lighting: enhanced` | int bitmask | 0 | Occluded ("enhanced") lighting: `1` = fire light, `2` = item light, `4` = unit/personal light. With `7`, all three stop at walls instead of shining through them. |
| `lighting: maxStatic` | int tiles | 16 | Max radius considered for **static** (terrain/item) light sources. |
| `lighting: maxDynamic` | int tiles | 24 | Max radius considered for **dynamic** (unit/fire) light sources. |

**[DX] note:** DX's directional cone lights (flashlights) are always occluded regardless of this
bitmask — see [Light Equipment](../DX-Features.md#light-equipment-directional-cone-light--sneak-light-gate).
The sneak light gate lives in [`sneakDefaults:`](Ruleset-DX-Globals.md#sneakdefaults).

---

## Personnel & economy

| Key | Type | Default | Meaning |
|---|---|---|---|
| `costHireScientist` | int $ | 0 | One-time hiring fee for a scientist (0 = use `costScientist`). |
| `costHireEngineer` | int $ | 0 | One-time hiring fee for an engineer. |
| `costScientist` | int $ | 0 | Monthly salary of a scientist. |
| `costEngineer` | int $ | 0 | Monthly salary of an engineer. |
| `timePersonnel` | int hours | 0 | Transfer/delivery time for newly hired personnel. |
| `hireByCountryOdds` | int % | 0 | Chance a new recruit's nationality is rolled from funding **countries** (weighted by funding). |
| `hireByRegionOdds` | int % | 0 | Chance it is rolled from [regions](Ruleset-Regions.md) instead (checked after countries). |
| `transferCosts: globalCostMult` | int | 1 | Multiplier applied to **all** transfer costs. |
| `transferCosts: globalCostDiv` | int | 1 | Divisor applied to all transfer costs (with the multiplier: `cost × mult / div`). |
| `hireScientistsRequiresBaseFunc` | list of base functions | — | Base must provide these [facility functions](Ruleset-Facilities.md) before scientists can be hired at it. |
| `hireEngineersRequiresBaseFunc` | list of base functions | — | Same, for engineers. |
| `hireScientistsUnlockResearch` | research name | — | [Research](Ruleset-Research.md) that must be done before scientists can be hired anywhere. |
| `hireEngineersUnlockResearch` | research name | — | Same, for engineers. |

### Promotions & flags

| Key | Type | Default | Meaning |
|---|---|---|---|
| `soldiersPerSergeant` | int | 5 | Soldiers needed per Sergeant promotion slot. |
| `soldiersPerCaptain` | int | 11 | Soldiers needed per Captain slot. |
| `soldiersPerColonel` | int | 23 | Soldiers needed per Colonel slot. |
| `soldiersPerCommander` | int | 30 | Soldiers needed for the (single) Commander slot. |
| `flagByKills` | list of ints | — | Kill-count thresholds; when set, a soldier's rank flag sprite is picked by kills crossing each threshold instead of by rank. |

---

## Geoscape & alien strategy

| Key | Type | Default | Meaning |
|---|---|---|---|
| `chanceToStopRetaliation` | int % | 0 | Chance that destroying an attacking force stops the retaliation mission for good. |
| `chanceToDetectAlienBaseEachMonth` | int % | 20 | Monthly chance X-COM operatives reveal an undiscovered alien base. |
| `lessAliensDuringBaseDefense` | bool | false | A UFO damaged on the way in deploys fewer aliens in the resulting base defense. |
| `allowCountriesToCancelAlienPact` | bool | false | Countries rejoin X-COM funding after their infiltration base is destroyed. |
| `buildInfiltrationBaseCloseToTheCountry` | bool | false | Infiltration bases spawn near the infiltrated country instead of anywhere in the region. |
| `infiltrateRandomCountryInTheRegion` | bool | false | Infiltration picks a random country of the region rather than ruleset order. |
| `allowAlienBasesOnWrongTextures` | bool | true | As a last resort, alien bases may be placed on globe textures with no suitable terrain. |
| `shortRadarRange` | int | 0 → auto | The largest radar range still counted as "short" for the base info screen (0 = derived from the facilities). |
| `buildTimeReductionScaling` | int % | 100 | Scaling of the facility build-time reduction when adjacent facilities speed up construction. |
| `baseDefenseMapFromLocation` | int | 0 | 1 = generate the base-defense battle terrain from the base's globe texture instead of the facility rules. |
| `alienItemLevels` | list of lists | — | The alien equipment-level table: one row per campaign month (last row repeats), each a weighted list of `itemLevel` values (0–2 in the standard mods) rolled per spawned alien. |
| `alienFuel` | — | — | See [Campaign rules](#campaign-rules--research-gates). |

---

## Battlescape tuning

| Key | Type | Default | Meaning |
|---|---|---|---|
| `maxViewDistance` | int tiles | 20 | Engine-wide max view distance (what armor `visibilityAtDay: 0` falls back to; see [armors](Ruleset-Armors.md#vision--stealth)). |
| `maxDarknessToSeeUnits` | int shade | 9 | Tile shade at or below which units are visible at night range. |
| `maxLookVariant` | int | 0 | Highest `lookVariant` used when generating soldiers. |
| `tooMuchSmokeThreshold` | int | 10 | Smoke density above which a tile blocks vision entirely. |
| `customTrainingFactor` | int % | 100 | Speed of stat gains in the martial-arts training facility. |
| `kneelBonusGlobal` | int % | 115 | Default accuracy bonus while kneeling (items can override). |
| `oneHandedPenaltyGlobal` | int % | 80 | Default accuracy multiplier for firing a two-handed weapon one-handed. |
| `enableCloseQuartersCombat` | int | 0 | 1 = enable the close-quarters-combat (CQC) melee-interrupt mechanic. |
| `closeQuartersAccuracyGlobal` | int % | 100 | Default CQC success chance. |
| `closeQuartersTuCostGlobal` | int TU | 12 | Default TU cost of a CQC attempt. |
| `closeQuartersEnergyCostGlobal` | int | 8 | Default energy cost of a CQC attempt. |
| `closeQuartersSneakUpGlobal` | int % | 0 | Chance to avoid CQC when attacking from behind (0 = off). |
| `noLOSAccuracyPenaltyGlobal` | int % | −1 | Default accuracy multiplier when firing without line of sight (−1 = no penalty; items can override). |
| `explodeInventoryGlobal` | int | 0 | Default for items' `explodeInventory` (see [items](Ruleset-Items.md)): 0 no, 1 except in hands, 2 always. |
| `surrenderMode` | int | 0 | 0 = no surrender; 1 = remaining enemies surrender when panicking **now**; 2 = also if they panicked earlier; 3 = if empty-handed and they panicked earlier. Requires the units' `canSurrender`/hands check. |
| `bughuntMinTurn` | int | 999 | First turn "bug hunt" mode (reveal stragglers) can kick in (999 = effectively off; [deployments](Ruleset-AlienDeployments.md) can override). |
| `bughuntMaxEnemies` | int | 2 | Bug hunt requires at most this many live enemies. |
| `bughuntRank` | int | 0 | Enemies of this rank or higher ("VIPs") prevent bug hunt mode. |
| `bughuntLowMorale` | int | 40 | Enemies below this morale count toward bug hunt eligibility. |
| `bughuntTimeUnitsLeft` | int % | 60 | Enemies with more than this % TU left at turn end block bug hunt. |
| `tuRecoveryWakeUpNewTurn` | int % | 100 | TU (percent of max) granted to a unit that wakes from stun at the start of a turn. |

---

## Dogfight & pilots

| Key | Type | Default | Meaning |
|---|---|---|---|
| `ufoTractorBeamSizeModifiers` | list of 5 ints % | `[400, 200, 100, 50, 25]` | Tractor-beam effectiveness by **UFO size** (very small → very large): the craft-weapon's `tractorBeamPower` is scaled by this percent. |
| `pilotBraveryThresholds` | list of 3 ints | `[90, 80, 30]` | Bravery cut-offs (on the pilots' **average** bravery) grading how aggressively a craft closes in a dogfight: ≥ 1st = double approach speed, ≥ 2nd = +50%, ≥ 3rd = normal, below = half speed ([`Craft.cpp`](../src/Savegame/Craft.cpp)). |
| `pilotAccuracyZeroPoint` | int | 55 | Pilot firing accuracy that gives no dogfight aim bonus/penalty. |
| `pilotAccuracyRange` | int | 40 | How strongly accuracy above/below the zero point shifts dogfight aim (percent of the distance to the zero point). |
| `pilotReactionsZeroPoint` | int | 55 | Pilot reactions value that gives no dodge bonus/penalty. |
| `pilotReactionsRange` | int | 60 | How strongly reactions shift the craft's dodge in a dogfight. |
| `ufoGlancingHitThreshold` | int | 0 | UFO damage below this fraction of a hit counts as a glancing hit. |
| `ufoBeamWidthParameter` | int | 1000 | Scales how wide a UFO's beam weapon is drawn, based on its power. |
| `escortRange` | int | 20 | Distance within which craft escort each other (and HK escorts their charge). |
| `drawEnemyRadarCircles` | int | 1 | Radar circles around detected hunter-killers/alien bases: 0 = never, 1 = only when hyper-detected, 2 = always. |
| `escortsJoinFightAgainstHK` | bool | true | Escorting craft automatically join a dogfight against a hunter-killer. |
| `hunterKillerFastRetarget` | bool | true | Hunter-killers may retarget every 5 in-game seconds on the slow timers. |
| `crewEmergencyEvacuationSurvivalChance` | int % | 100 | Chance each crew member survives when the craft is destroyed in a dogfight (with evacuation rules active). |
| `pilotsEmergencyEvacuationSurvivalChance` | int % | 100 | Same, for pilots. |
| `showUfoPreviewInBaseDefense` | bool | false | Show the attacking UFO's preview in the base-defense screen. |

---

## UI, pedia & presentation

| Key | Type | Default | Meaning |
|---|---|---|---|
| `hiddenMovementBackgrounds` | list of image names | — | Pool of background images for the "hidden movement" screen; one is picked at random per battle. |
| `enableNewResearchSorting` | bool | false | Let the player sort the New Research list. |
| `displayCustomCategories` | int | 0 | [Item categories](Ruleset-ItemCategories.md) in Buy/Sell/Transfer filters: 0 = vanilla only, 1 = custom only, 2 = both. |
| `shareAmmoCategories` | bool | false | Weapons "inherit" the categories of their ammo (also affects [starting-condition](Ruleset-StartingConditions.md) item checks). |
| `showDogfightDistanceInKm` | bool | false | Dogfight UI shows distance in km instead of the raw unit. |
| `showFullNameInAlienInventory` | bool | false | Alien inventory shows the full unit name (e.g. Sectoid Leader) instead of just the race; [units](Ruleset-Units.md) can override per unit. |
| `alienInventoryOffsetX` | int px | 80 | Horizontal offset of the alien-inventory paperdoll and hand slots. |
| `alienInventoryOffsetBigUnit` | int px | 32 | Extra hand-slot offset for 2×2 units. |
| `hidePediaInfoButton` | bool | false | Hide the UFOpaedia INFO button where it would appear. |
| `extraNerdyPediaInfoType` | int | 0 | Show extra item stats (accuracy modifier, power bonus) in pedia articles: 0 = off; higher values enable the extra block. |
| `pediaReplaceCraftFuelWithRangeType` | int | −1 | Replace the craft article's fuel stat with a range: −1 = off, otherwise selects the range type shown. |
| `generateMissingPediaArticles` **[DX]** | bool | true | Auto-generate a stand-in UFOpaedia article for any item, armor, craft, craft weapon, base facility, soldier type, alien unit type or UFO that has no authored `ufopaedia` entry, so middle-clicking it still opens its stats block. Gating follows the rule's own `requires:`; where a rule has none (`Unit` and `RuleUfo` have no such field at all), DX derives the gate from the vanilla convention that an article requires a research topic named after its subject — so a generated alien-unit page sits behind that unit's interrogation topic. For alien units and UFOs a derivable gate is **mandatory**: with no matching research topic, no article is generated, so nothing enemy-side is ever exposed ungated. Set `false` to restore the vanilla behavior, where middle-clicking a rule with no article does nothing. |
| `listGeneratedPediaArticles` **[DX]** | bool | false | Whether articles created by `generateMissingPediaArticles` also appear in the browsable UFOpaedia index, and in the article prev/next navigation. Off by default so a mod's authored table of contents and its page order are unchanged; generated articles remain openable via middle-click either way. No effect when `generateMissingPediaArticles` is false. |
| `performanceBonusFactor` | float | 0.0 | Council performance bonus: monthly score × this factor is added to funding. |

---

## Scoring & misc rules

| Key | Type | Default | Meaning |
|---|---|---|---|
| `giveScoreAlsoForResearchedArtifacts` | bool | false | Recovering already-researched artifacts still scores points. |
| `statisticalBulletConservation` | bool | false | Instead of rounding partial clips at debriefing, keep ammo statistically (a 40% clip has a 40% chance to survive). |
| `stunningImprovesMorale` | bool | false | Stunning an enemy grants the same morale boost as a kill. |

---

## Global sounds

| Key | Type | Default | Meaning |
|---|---|---|---|
| `selectBaseSound` | sound id(s) | — | Sound (index into `BATTLE.CAT`) played when selecting a base (referenced from [Ruleset-Constants.md](Ruleset-Constants.md)). |
| `startDogfightSound` | sound id(s) | — | Sound played when a dogfight starts. |
| `disableUnderwaterSounds` | bool | false | TFTD: don't switch to the underwater sound set on underwater missions. |

The per-unit voice banks (`enableUnitResponseSounds`, `unitResponseSoundsFrequency`,
`unitResponseSounds:`) are documented in
[Ruleset-UnitResponseSounds.md](Ruleset-UnitResponseSounds.md).

---

## Options a mod forces or suggests

Both are **maps of option name → value** (values are the option's string form, as they appear in
`options.cfg`). See [`Options`](../src/Engine/Options.h) for the option names.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `fixedUserOptions` | map string → string | — | Options the mod **forces**: applied on load and locked (the player cannot change them while the mod is active). |
| `recommendedUserOptions` | map string → string | — | Options the mod **suggests**: applied once, the first time the mod is activated; the player can change them afterwards. |

A short blocklist of options is exempt and silently dropped: `fixedUserOptions` drops `oxceLinks`,
`oxceUpdateCheck`, `maximizeInfoScreens`, `oxceModValidationLevel`, `oxceAutoNightVisionThreshold`
and `oxceAlternateCraftEquipmentManagement`, while `recommendedUserOptions` drops only
`maximizeInfoScreens` and `oxceModValidationLevel` — user-preference/UI options the engine refuses
to let a mod dictate.

---

## `converter:` — save-import id mapping

Used only by the **original-savegame importer** (File → Convert an original X-COM save). It maps the
binary DAT files' numeric ids onto ruleset ids, so a mod that renumbers content can still import
original saves. See [`RuleConverter`](../src/Mod/RuleConverter.h) and
[`bin/standard/xcom1/converter.rul`](../bin/standard/xcom1/converter.rul).

| Key | Type | Meaning |
|---|---|---|
| `offsets` | map name → int | Byte offsets into the original `.DAT` structures (e.g. `BASE.DAT_FACILITIES: 0x16`). |
| `markers` | list | Geoscape target marker id mapping. |
| `countries` / `regions` / `facilities` / `items` / `crafts` / `ufos` / `craftWeapons` / `missions` / `armor` / `alienRaces` / `alienRanks` / `research` / `manufacture` / `ufopaedia` | lists of rule ids | Ordered lists: index = the original game's numeric id, value = the ruleset id it becomes. |
| `crews` | list | Original crew-slot → unit type mapping. |

---

## Mod metadata (`metadata.yml`)

Every mod folder needs a `metadata.yml`. Its keys sit at the **document root** (there is no wrapping
node — [`ModInfo::load`](../src/Engine/ModInfo.cpp) reads the root map directly), and they are what
the Mods list in Options shows.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `id` | string | folder name | Unique mod id (what other mods reference as a `master`). |
| `name` | string | folder name | Display name in the mod list. |
| `description` | string | "No description." | Blurb shown in the mod list. |
| `version` | string | `1.0` | Mod version; compared numerically/segment-wise against `requiredMasterModVersion`. |
| `versionDisplay` | string | = `version` | Version string shown to the player (when it differs from the comparable one). |
| `author` | string | "unknown author" | Credited author. |
| `isMaster` | bool | false | This is a **master** (total-conversion base like `xcom1`/`xcom2`), not a submod. |
| `master` | string | `xcom1` | Which master this submod attaches to. `*` (or empty) = works with any master / standalone. Masters default to no master. |
| `requiredMasterModVersion` | string | — | Minimum `version` of the master mod required. Ignored (with a warning) if the mod has no master. |
| `requiredExtendedVersion` | string | — | Minimum engine version required. Setting it also implies `requiredExtendedEngine: Extended`. |
| `requiredExtendedEngine` | string | — | Which engine the mod needs (`Extended` = OXCE). DX accepts `Extended` — it is backward compatible with OXCE mods. |
| `loadResources` | list of dirs | — | External resource directories to mount (e.g. `UFO`, `TFTD`). **Top-level masters only.** |
| `resourceConfig` | filename | — | Resource-config file for the mod's VFS layer. |
| `reservedSpace` | int 1–100 | 1 | How many sprite/sound index blocks this mod reserves (clamped to 1–100). Raise it if the mod adds a very large number of `extraSprites`/`extraSounds`. |

```yaml
# metadata.yml
name: "My Mod"
version: 1.2
description: "Does a thing."
author: someone
id: my_mod
master: xcom1
requiredExtendedVersion: 7.0
```

## See also

- [Ruleset-AI.md](Ruleset-AI.md) — the `ai:` node (tactical AI tuning)
- [Ruleset-Constants.md](Ruleset-Constants.md) — the `constants:` node
- [Ruleset-StartingBase.md](Ruleset-StartingBase.md) — the starting base template
- [Ruleset-DX-Globals.md](Ruleset-DX-Globals.md) — the **[DX]** singletons
- [Ruleset-Scripting.md](Ruleset-Scripting.md) — the `extended:` node
