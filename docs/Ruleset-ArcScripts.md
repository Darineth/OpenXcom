# Ruleset: `arcScripts:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`RuleArcScript`](../src/Mod/RuleArcScript.h) · **List key:** `type` · **Loader:**
[`RuleArcScript::load`](../src/Mod/RuleArcScript.cpp)

An arc script is the campaign's **story-progression dial**. Once per month — *before* the
[`missionScripts:`](Ruleset-MissionScripts.md) run — the engine walks the `arcScripts:` list, keeps
the commands whose gates pass, rolls `executionOdds`, and for each winner **unlocks one research
topic** (an "arc"): the first not-yet-unlocked entry of `sequentialArcs`, and then, if `maxArcs`
still allows, one random entry of `randomArcs`
([`GeoscapeState::determineAlienMissions`](../src/Geoscape/GeoscapeState.cpp)). Unlocking is done
via `addFinishedResearch` and pops the topic's UFOpaedia article, so the arc is really just a
[research](Ruleset-Research.md) topic that mission/event scripts can then use as a
`researchTriggers` gate.

```yaml
arcScripts:
  - type: mainStoryArc
    firstMonth: 2
    executionOdds: 50
    maxArcs: 3                 # at most 3 of this command's arcs will ever be enabled
    sequentialArcs:
      - STR_ARC_CHAPTER_1
      - STR_ARC_CHAPTER_2
      - STR_ARC_CHAPTER_3
    researchTriggers:
      STR_ALIEN_ORIGINS: true
```

Entries **merge** across mods/files by `type`, support `refNode:` inheritance and `delete: true`
(see the [index](Ruleset.md#how-rulesets-load-the-60-second-version)).

## What gets unlocked

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | — | Unique id of the command (internal only). |
| `sequentialArcs` | list of research | — | [Research](Ruleset-Research.md) topics unlocked strictly in list order — the first one not yet discovered is granted. |
| `randomArcs` | weights (name→int) | — | Weighted pool of research topics; one still-undiscovered entry is granted at random (weight 0 deletes an inherited option). |
| `maxArcs` | int | −1 | Cap on how many of this command's own arcs may ever be enabled (already-researched ones count, no matter who unlocked them); −1 = no cap. |

A single execution grants at most one sequential arc **and** at most one random arc, and each grant
re-checks `maxArcs` first.

## Scheduling & eligibility gates

| Key | Type | Default | Meaning |
|---|---|---|---|
| `firstMonth` | int | 0 | First game month in which the command may run. |
| `lastMonth` | int | −1 | Last month in which it may run; −1 = forever. |
| `executionOdds` | int % | 100 | Percentage chance the command executes in a month where it is otherwise eligible. |
| `minDifficulty` | int 0–4 | 0 | Lowest difficulty level at which the command runs. |
| `maxDifficulty` | int 0–4 | 4 | Highest difficulty level at which the command runs. |
| `minScore` | int | INT_MIN | Minimum *last month's* total score; ignored during the very first month. |
| `maxScore` | int | INT_MAX | Maximum last month's total score; ignored during the very first month. |
| `minFunds` | int64 | INT64_MIN | Minimum current funds; ignored during the very first month. |
| `maxFunds` | int64 | INT64_MAX | Maximum current funds; ignored during the very first month. |
| `researchTriggers` | map research→bool | — | Each research topic must be discovered (`true`) / not discovered (`false`). |
| `itemTriggers` | map item→bool | — | Each [item](Ruleset-Items.md) must have been obtained at least once (`true`) / never (`false`). |
| `facilityTriggers` | map facility→bool | — | Each [facility](Ruleset-Facilities.md) must be built (`true`) / not built (`false`) somewhere. |
| `soldierTypeTriggers` | map soldier→bool | — | Each [soldier type](Ruleset-Soldiers.md) must have been hired (`true`) / never hired (`false`). |
| `xcomBaseInRegionTriggers` | map region→bool | — | An X-COM base must exist (`true`) / not exist (`false`) in each listed [region](Ruleset-Regions.md). |
| `xcomBaseInCountryTriggers` | map country→bool | — | An X-COM base must exist (`true`) / not exist (`false`) in each listed [country](Ruleset-Countries.md). |
| `pactCountryTriggers` | map country→bool | — | Each listed country must have signed an alien pact (`true`) / not signed (`false`). |
| `missionVarName` | string | — | Counter source for `counterMin`/`counterMax`: the number of missions run under that mission-script `varName`. |
| `missionMarkerName` | string | — | Second counter source: the savegame id counter of that name (e.g. `STR_TERROR_SITE`). |
| `counterMin` | int | 0¹ | If > 0, the counter(s) named above must be at least this high. |
| `counterMax` | int | −1¹ | If not −1, the counter(s) named above must not exceed this; both configured counters are checked. |

¹ Upstream OXCE never initialized `_counterMin`/`_counterMax`; **DX initializes them to 0 / −1**
(matching [`eventScripts:`](Ruleset-EventScripts.md)) — see the note at the bottom of this page.

## See also

- [`research:`](Ruleset-Research.md) — the topics an arc unlocks (and whose `lookup:` article pops up)
- [`missionScripts:`](Ruleset-MissionScripts.md) — run right after arc scripts, and can gate on the freshly unlocked arcs
- [`eventScripts:`](Ruleset-EventScripts.md) / [`events:`](Ruleset-Events.md) — the other half of the monthly scripting pass

## Note: `counterMin` / `counterMax` defaults

Upstream, these two members were **never initialized**. DX initializes them to `0` / `-1` (the "no
constraint" sentinels the checks test for), matching `eventScripts:`. See
[DX-OXCE-Fixes.md](../DX-OXCE-Fixes.md).
