# Ruleset: `missionScripts:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`RuleMissionScript`](../src/Mod/RuleMissionScript.h) · **List key:** `type` · **Loader:**
[`RuleMissionScript::load`](../src/Mod/RuleMissionScript.cpp)

A mission script is one **command** in the monthly alien strategy. At the start of every game month
the engine walks the whole `missionScripts:` list in load order, keeps the commands whose gates
(month, difficulty, score, funds, research/item/facility/… triggers) are satisfied, rolls
`executionOdds` for each survivor, and for each winner picks a **region**, an
[`alienMissions:`](Ruleset-AlienMissions.md) type and an [`alienRaces:`](Ruleset-AlienRaces.md)
race, then starts that mission
([`GeoscapeState::determineAlienMissions` / `processCommand`](../src/Geoscape/GeoscapeState.cpp)).
The same rule class also backs [`adhocScripts:`](Ruleset-AdhocScripts.md), which are run on demand
by an [event](Ruleset-Events.md) instead of monthly.

```yaml
missionScripts:
  - type: recurringTerror
    firstMonth: 3
    executionOdds: 60
    varName: terror            # required, because maxRuns/avoidRepeats are used
    avoidRepeats: 4            # don't hit the same city again within 4 terror missions
    missionWeights:
      0: { STR_ALIEN_TERROR: 100 }
      8: { STR_ALIEN_TERROR: 70, STR_ALIEN_RETALIATION: 30 }
    researchTriggers:
      STR_ALIEN_ORIGINS: true
```

Entries **merge** across mods/files by `type`, support `refNode:` inheritance and `delete: true`
(see the [index](Ruleset.md#how-rulesets-load-the-60-second-version)).

## Identity & scheduling

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | — | Unique id of the command (only used for referencing/errors, never shown). |
| `varName` | string | — | Savegame variable under which this command's runs and hit locations are tracked (shared between commands that use the same name); **mandatory** if `maxRuns` or `avoidRepeats` is set, otherwise the mod fails to load. |
| `firstMonth` | int | 0 | First game month in which the command may run (month 0 = the first month). |
| `lastMonth` | int | −1 | Last month in which it may run; −1 = forever. |
| `executionOdds` | int % | 100 | Percentage chance the command executes in a month where it is otherwise eligible. |
| `maxRuns` | int | −1 | Maximum number of times commands sharing this `varName` may run in the whole campaign; −1 = unlimited. |
| `avoidRepeats` | int | 0 | How many recently hit mission-site locations (per `varName`) to remember and avoid re-using. |
| `startDelay` | int minutes | 0 | Overrides the mission's first-wave spawn delay from the [`alienMissions:`](Ruleset-AlienMissions.md) definition. |
| `randomDelay` | int minutes | 0 | Random extra delay added on top of `startDelay` (`startDelay + rand(0..randomDelay)`). |
| `label` | int | 0 | Non-zero id other commands can reference in their `conditionals`; must be unique among the commands eligible in the same month, or the engine throws. |
| `conditionals` | list of ints | — | Only run if the referenced labels already ran this month with the expected result: `+N` = command with label N succeeded, `−N` = it failed or never ran (a `0` entry makes the command never run). |
| `adhocMissionScriptTags` | list of strings | — | Only meaningful in [`adhocScripts:`](Ruleset-AdhocScripts.md); ignored for monthly mission scripts. |

## Eligibility gates

All of these must pass before `executionOdds` is even rolled.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `minDifficulty` | int 0–4 | 0 | Lowest difficulty level at which the command runs. |
| `maxDifficulty` | int 0–4 | 4 | Highest difficulty level at which the command runs. |
| `minScore` | int | INT_MIN | Minimum *last month's* total score; ignored during the very first month. |
| `maxScore` | int | INT_MAX | Maximum last month's total score; ignored during the very first month. |
| `minFunds` | int64 | INT64_MIN | Minimum current funds; ignored during the very first month. |
| `maxFunds` | int64 | INT64_MAX | Maximum current funds; ignored during the very first month. |
| `researchTriggers` | map research→bool | — | Each [research](Ruleset-Research.md) topic must be discovered (`true`) / not discovered (`false`). |
| `itemTriggers` | map item→bool | — | Each [item](Ruleset-Items.md) must have been obtained at least once (`true`) / never (`false`). |
| `facilityTriggers` | map facility→bool | — | Each [facility](Ruleset-Facilities.md) must be built (`true`) / not built (`false`) somewhere. |
| `soldierTypeTriggers` | map soldier→bool | — | Each [soldier type](Ruleset-Soldiers.md) must have been hired (`true`) / never hired (`false`). |
| `xcomBaseInRegionTriggers` | map region→bool | — | An X-COM base must exist (`true`) / not exist (`false`) in each listed [region](Ruleset-Regions.md). |
| `xcomBaseInCountryTriggers` | map country→bool | — | An X-COM base must exist (`true`) / not exist (`false`) in each listed [country](Ruleset-Countries.md). |
| `pactCountryTriggers` | map country→bool | — | Each listed country must have signed an alien pact (`true`) / not signed (`false`). |
| `missionVarName` | string | — | Counter source for `counterMin`/`counterMax`: the number of missions run under that `varName`. |
| `missionMarkerName` | string | — | Second counter source: the savegame id counter of that name (e.g. `STR_TERROR_SITE`), i.e. how many such markers were ever created. |
| `counterMin` | int | 0¹ | If > 0, the counter(s) named above must be at least this high. |
| `counterMax` | int | −1¹ | If not −1, the counter(s) named above must not exceed this; both configured counters are checked. |

¹ `RuleMissionScript`'s constructor does **not** initialize `_counterMin`/`_counterMax` (unlike
[`eventScripts:`](Ruleset-EventScripts.md), which defaults them to 0 / −1). Always set both
explicitly when you use `missionVarName`/`missionMarkerName` gating.

## What gets generated

| Key | Type | Default | Meaning |
|---|---|---|---|
| `missionWeights` | map month→weights | — | Weighted [`alienMissions:`](Ruleset-AlienMissions.md) choice; the entry with the highest month ≤ the current month wins (weight 0 deletes an inherited option). Omit to let the region's own strategy table pick the mission. |
| `regionWeights` | map month→weights | — | Weighted [`regions:`](Ruleset-Regions.md) choice, same month semantics. Omit to let the global strategy table pick the region. |
| `raceWeights` | map month→weights | — | Weighted [`alienRaces:`](Ruleset-AlienRaces.md) choice, same month semantics. Omit to use the mission's own `raceWeights`. |
| `targetBaseOdds` | int % | 0 | Chance this run deliberately targets a region that contains an X-COM base. |
| `useTable` | bool | true | On success, remove the generated mission from the region's strategy table so the random picker won't offer it again until the table resets. |

**Mission-site commands are special.** After loading, the engine inspects all mission types a
command can generate: if they are `objective: 3` (mission site) missions it flags the command as a
*site type* ([`Mod.cpp`](../src/Mod/Mod.cpp)) and **throws** if a single command mixes site and
non-site missions. A site-type command picks the mission type *first*, then a region and a concrete
mission area (city) that `avoidRepeats` has not blacklisted; a normal command picks the region
first.

## See also

- [`alienMissions:`](Ruleset-AlienMissions.md) — what this schedules (waves, objective, scoring)
- [`adhocScripts:`](Ruleset-AdhocScripts.md) — same fields, triggered by an event instead of monthly
- [`arcScripts:`](Ruleset-ArcScripts.md) — runs just *before* mission scripts each month, unlocking research these scripts can trigger on
- [`eventScripts:`](Ruleset-EventScripts.md) / [`events:`](Ruleset-Events.md) — run just *after* mission scripts
- [`regions:`](Ruleset-Regions.md) · [`alienRaces:`](Ruleset-AlienRaces.md) · [`ufoTrajectories:`](Ruleset-UfoTrajectories.md)

## Note: `counterMin` / `counterMax` defaults

Upstream, these two members were **never initialized** — a script that used `missionVarName:` /
`missionMarkerName:` but left a counter unset had its eligibility decided by uninitialized memory.
DX initializes them to `0` / `-1` (the "no constraint" sentinels the checks already test for), matching
`eventScripts:`, which always did. See [DX-OXCE-Fixes.md](../DX-OXCE-Fixes.md). The same fix applies to
[`arcScripts:`](Ruleset-ArcScripts.md).
