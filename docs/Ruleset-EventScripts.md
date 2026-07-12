# Ruleset: `eventScripts:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`RuleEventScript`](../src/Mod/RuleEventScript.h) · **List key:** `type` · **Loader:**
[`RuleEventScript::load`](../src/Mod/RuleEventScript.cpp)

An event script schedules [`events:`](Ruleset-Events.md) the way
[`missionScripts:`](Ruleset-MissionScripts.md) schedule alien missions. Once per month — *after* the
mission scripts — the engine walks the `eventScripts:` list, keeps the commands whose gates pass,
rolls `executionOdds`, and each winner spawns up to **three** events: one from
`oneTimeSequentialEvents`, one from `oneTimeRandomEvents`, and one from the repeatable
`eventWeights` pool ([`GeoscapeState::determineAlienMissions`](../src/Geoscape/GeoscapeState.cpp)).
Spawning only starts the event's countdown — the popup happens later, after the event's own
`timer` (see [`events:`](Ruleset-Events.md)).

```yaml
eventScripts:
  - type: earlyRumours
    firstMonth: 1
    lastMonth: 6
    executionOdds: 40
    oneTimeSequentialEvents:
      - STR_EVENT_FIRST_CONTACT   # spawned once, in list order
    eventWeights:
      0: { STR_EVENT_UFO_SIGHTING: 60, STR_EVENT_CATTLE_MUTILATION: 40 }
    researchTriggers:
      STR_ALIEN_ORIGINS: false
```

Entries **merge** across mods/files by `type`, support `refNode:` inheritance and `delete: true`
(see the [index](Ruleset.md#how-rulesets-load-the-60-second-version)).

## What gets spawned

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | — | Unique id of the command (internal only). |
| `oneTimeSequentialEvents` | list of events | — | [Events](Ruleset-Events.md) spawned strictly in list order, each at most once ever — the first one never generated before is taken. |
| `oneTimeRandomEvents` | weights (name→int) | — | Weighted pool of events, each spawnable at most once ever; one still-unused entry is picked (weight 0 deletes an inherited option). |
| `eventWeights` | map month→weights | — | Weighted pool of **repeatable** events; the entry with the highest month ≤ the current month wins. |

All three lists are independent: a single execution can spawn one event from each.

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
| `researchTriggers` | map research→bool | — | Each [research](Ruleset-Research.md) topic must be discovered (`true`) / not discovered (`false`). |
| `itemTriggers` | map item→bool | — | Each [item](Ruleset-Items.md) must have been obtained at least once (`true`) / never (`false`). |
| `facilityTriggers` | map facility→bool | — | Each [facility](Ruleset-Facilities.md) must be built (`true`) / not built (`false`) somewhere. |
| `soldierTypeTriggers` | map soldier→bool | — | Each [soldier type](Ruleset-Soldiers.md) must have been hired (`true`) / never hired (`false`). |
| `xcomBaseInRegionTriggers` | map region→bool | — | An X-COM base must exist (`true`) / not exist (`false`) in each listed [region](Ruleset-Regions.md). |
| `xcomBaseInCountryTriggers` | map country→bool | — | An X-COM base must exist (`true`) / not exist (`false`) in each listed [country](Ruleset-Countries.md). |
| `pactCountryTriggers` | map country→bool | — | Each listed country must have signed an alien pact (`true`) / not signed (`false`). |
| `missionVarName` | string | — | Counter source for `counterMin`/`counterMax`: the number of missions run under that mission-script `varName`. |
| `missionMarkerName` | string | — | Second counter source: the savegame id counter of that name (e.g. `STR_TERROR_SITE`). |
| `counterMin` | int | 0 | If > 0, the counter(s) named above must be at least this high. |
| `counterMax` | int | −1 | If not −1, the counter(s) named above must not exceed this; both configured counters are checked. |
| `missionMinRuns` / `missionMaxRuns` | int | — | **Deprecated** aliases of `counterMin`/`counterMax` (read first, then overridden by them). |
| `affectsGameProgression` | bool | false | Purely informational: marks the script as story-relevant so the Tech Tree Viewer shows it. |

## See also

- [`events:`](Ruleset-Events.md) — the events this spawns (their payload, delay and interrupt research)
- [`missionScripts:`](Ruleset-MissionScripts.md) — run just before event scripts each month
- [`arcScripts:`](Ruleset-ArcScripts.md) — run first, unlocking the research these can gate on
- [`adhocScripts:`](Ruleset-AdhocScripts.md) — alien missions that an event can fire off when it pops up
