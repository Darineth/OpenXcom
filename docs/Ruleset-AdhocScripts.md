# Ruleset: `adhocScripts:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`RuleMissionScript`](../src/Mod/RuleMissionScript.h) · **List key:** `type` · **Loader:**
[`RuleMissionScript::load`](../src/Mod/RuleMissionScript.cpp)

Ad-hoc scripts are [`missionScripts:`](Ruleset-MissionScripts.md) that do **not** run on the monthly
schedule. They are stored in a separate list and are only consulted when a
[geoscape event](Ruleset-Events.md) with `adhocMissionScriptTags:` pops up: the event calls
`determineAlienMissions(false, event)`, which filters the `adhocScripts:` list by tag and then runs
the *exact same* eligibility and generation logic as a monthly mission script
([`GeoscapeState::determineAlienMissions` / `processCommand`](../src/Geoscape/GeoscapeState.cpp)).
This is how an event ("a UFO was spotted over Tokyo") can immediately spawn the corresponding
[alien mission](Ruleset-AlienMissions.md).

```yaml
adhocScripts:
  - type: adhocTerrorFromEvent
    adhocMissionScriptTags: [ terrorRumour ]   # matched against the event's tags
    executionOdds: 100
    missionWeights:
      0: { STR_ALIEN_TERROR: 100 }
    regionWeights:
      0: { STR_NORTH_AMERICA: 100 }

events:
  - name: STR_EVENT_TERROR_RUMOUR
    adhocMissionScriptTags: [ terrorRumour ]
```

Entries **merge** across mods/files by `type`, support `refNode:` inheritance and `delete: true`
(see the [index](Ruleset.md#how-rulesets-load-the-60-second-version)).

## Fields

`adhocScripts:` uses the **same rule class and therefore the same field set** as
[`missionScripts:`](Ruleset-MissionScripts.md#identity--scheduling) — see that page for the full
tables (`firstMonth`/`lastMonth`, `executionOdds`, `varName`/`maxRuns`/`avoidRepeats`,
`startDelay`/`randomDelay`, `label`/`conditionals`, all the `*Triggers`, the counter gates,
`missionWeights`/`regionWeights`/`raceWeights`, `targetBaseOdds`, `useTable`). The one field that is
*only* meaningful here:

| Key | Type | Default | Meaning |
|---|---|---|---|
| `adhocMissionScriptTags` | list of strings | — | Tags matched against the triggering [event's](Ruleset-Events.md) `adhocMissionScriptTags:`; a script with no matching tag is skipped. |

## How it differs from a monthly mission script

- **Trigger:** only via an event popup, never at the month boundary. `arcScripts:` and
  `eventScripts:` are *not* run in this pass — only the ad-hoc mission commands.
- **Gates still apply:** month range, difficulty, score/funds, all `*Triggers`, counters, `maxRuns`
  and `executionOdds` are checked exactly as for a monthly script, using the month in which the
  event popped up.
- **`label`/`conditionals`** are evaluated within this single ad-hoc pass only (the condition table
  is rebuilt per call), so they can only reference other ad-hoc scripts fired by the same event.
- **Site-type validation** is shared with `missionScripts:`: a command may not mix mission-site
  (`objective: 3`) and non-site missions, and an unknown mission type throws at mod load.

> **Note — DX fixes an upstream tag-matching bug.** In stock OXCE the inner loop of the tag match had
> an *unconditional* `break`, so each of the event's tags was compared against the ad-hoc script's
> **`tag[0]` and nothing else** — every later tag on a script was structurally unreachable. DX guards
> the `break`, so matching now works as the tag lists imply: **any event tag may match any script
> tag.** A script with a single tag is unaffected; a script with several tags now fires for events it
> previously ignored. Full write-up: [DX-OXCE-Fixes.md](../DX-OXCE-Fixes.md).

## See also

- [`events:`](Ruleset-Events.md) — the trigger side (`adhocMissionScriptTags:`)
- [`missionScripts:`](Ruleset-MissionScripts.md) — the full field reference
- [`alienMissions:`](Ruleset-AlienMissions.md) — what actually gets started
