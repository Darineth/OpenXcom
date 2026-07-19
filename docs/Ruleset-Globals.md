# Ruleset: global singletons (the catch-all)

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`Mod`](../src/Mod/Mod.h) · **Node type:** singletons (one map/scalar each, not
keyed lists) · **Loader:** [`Mod::loadFile`](../src/Mod/Mod.cpp) — the block after the keyed rule
lists. Also: [`RuleConverter::load`](../src/Mod/RuleConverter.cpp),
[`ModInfo::load`](../src/Engine/ModInfo.cpp), [`GameTime::load`](../src/Savegame/GameTime.cpp).

This page collects the **small global tuning nodes**: the ones that are a handful of keys each and
don't warrant their own page. Bigger singletons live elsewhere —
[`startingBase:`](Ruleset-StartingBase.md), [`constants:`](Ruleset-Constants.md),
[`extended:`](Ruleset-Scripting.md), the [DX globals](Ruleset-DX-Globals.md).

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

## Personnel hiring gates

| Key | Type | Default | Meaning |
|---|---|---|---|
| `hireScientistsRequiresBaseFunc` | list of base functions | — | Base must provide these [facility functions](Ruleset-Facilities.md) before scientists can be hired at it. |
| `hireEngineersRequiresBaseFunc` | list of base functions | — | Same, for engineers. |
| `hireScientistsUnlockResearch` | research name | — | [Research](Ruleset-Research.md) that must be done before scientists can be hired anywhere. |
| `hireEngineersUnlockResearch` | research name | — | Same, for engineers. |

---

## Dogfight & pilots

| Key | Type | Default | Meaning |
|---|---|---|---|
| `ufoTractorBeamSizeModifiers` | list of 5 ints % | `[400, 200, 100, 50, 25]` | Tractor-beam effectiveness by **UFO size** (very small → very large): the craft-weapon's `tractorBeamPower` is scaled by this percent. |
| `pilotBraveryThresholds` | list of 3 ints | `[90, 80, 30]` | Bravery cut-offs (on the pilots' **average** bravery) grading how aggressively a craft closes in a dogfight: ≥ 1st = double approach speed, ≥ 2nd = +50%, ≥ 3rd = normal, below = half speed ([`Craft.cpp`](../src/Savegame/Craft.cpp)). |

---

## Presentation

| Key | Type | Default | Meaning |
|---|---|---|---|
| `hiddenMovementBackgrounds` | list of image names | — | Pool of background images for the "hidden movement" screen; one is picked at random per battle. |

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

- [Ruleset-Constants.md](Ruleset-Constants.md) — the `constants:` node
- [Ruleset-StartingBase.md](Ruleset-StartingBase.md) — the starting base template
- [Ruleset-DX-Globals.md](Ruleset-DX-Globals.md) — the **[DX]** singletons
- [Ruleset-Scripting.md](Ruleset-Scripting.md) — the `extended:` node
