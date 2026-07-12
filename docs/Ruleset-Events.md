# Ruleset: `events:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`RuleEvent`](../src/Mod/RuleEvent.h) · **List key:** `name` · **Loader:**
[`RuleEvent::load`](../src/Mod/RuleEvent.cpp)

A geoscape event is a **popup with consequences**: a windowed message (title, text, background,
optional music and closing cutscene) that hands out or takes away score, funds, items, craft,
soldiers and research, and can kick off ad-hoc alien missions. Events are scheduled by
[`eventScripts:`](Ruleset-EventScripts.md) (and can also be spawned by
[`research:`](Ruleset-Research.md) via `spawnedEvent:`); the payload is applied when the popup is
built ([`GeoscapeEventState`](../src/Geoscape/GeoscapeEventState.cpp)).

```yaml
events:
  - name: STR_EVENT_UFO_SIGHTING       # also the popup title's language key
    description: STR_EVENT_UFO_SIGHTING_DESC
    background: BACK13.SCR
    regionList: [ STR_NORTH_AMERICA, STR_EUROPE ]   # title/description get {0} = region name
    city: true
    points: -20
    timer: 4320                        # ~3 days after being spawned
    timerRandom: 2880
    everyMultiItemList:
      STR_UFO_POWER_SOURCE: 1
```

Entries **merge** across mods/files by `name`, support `refNode:` inheritance and `delete: true`
(see the [index](Ruleset.md#how-rulesets-load-the-60-second-version)).

## Identity, presentation & timing

| Key | Type | Default | Meaning |
|---|---|---|---|
| `name` | string | — | Unique id; also the language key used as the popup **title**. |
| `description` | string | — | Language key for the popup body text. |
| `alignBottom` | bool | false | Bottom-align the body text instead of top-aligning it. |
| `background` | string | `BACK13.SCR` | Background image of the popup window. |
| `music` | string | — | [Music track](Ruleset-Musics.md) played while the popup is up. |
| `cutscene` | string | — | [Cutscene](Ruleset-Cutscenes.md) played after the player closes the popup (its `winGame`/`loseGame` flags end the campaign). |
| `timer` | int minutes | 30 | Base delay between the event being *spawned* by an event script and it *popping up*. |
| `timerRandom` | int minutes | 0 | Random extra delay: actual wait = `(timer + rand(0..timerRandom))` rounded **down** to a whole 30 minutes, with a hard floor of 60 minutes. |
| `interruptResearch` | research id | — | If this [research](Ruleset-Research.md) topic is discovered before the countdown ends, the event is silently cancelled (never pops up). |

## Location

| Key | Type | Default | Meaning |
|---|---|---|---|
| `regionList` | list of regions | — | One [region](Ruleset-Regions.md) is picked at random; its name is substituted into the title/description (`{0}`), and score goes to that region instead of the global research score. |
| `city` | bool | false | Substitute a random **city** name of the picked region instead of the region name. |

Even when `city: false`, a random city of the picked region is still selected internally, to bias
the nationality of any spawned soldiers.

## Payload — score, funds, people & craft

| Key | Type | Default | Meaning |
|---|---|---|---|
| `points` | int | 0 | Score added (negative = subtracted) — to the picked region's X-COM activity if `regionList` is set, otherwise to the global research score. |
| `funds` | int | 0 | Money added (negative = subtracted) on popup. |
| `spawnedPersons` | int | 0 | How many people of `spawnedPersonType` are transferred to HQ (24-hour transfer). |
| `spawnedPersonType` | string | — | `STR_SCIENTIST`, `STR_ENGINEER`, or a [soldier type](Ruleset-Soldiers.md). |
| `spawnedPersonName` | string | — | Language key used as the spawned soldier's name (otherwise a random name is generated). |
| `spawnedSoldier` | map | — | Soldier template (a `soldiers:`-savegame style node) applied to every soldier this event spawns — used to preset stats, rank, nationality, etc. |
| `everyMultiSoldierList` | map soldier→int | — | Soldiers transferred to HQ, all of them, in these quantities (names are always randomly generated). |
| `randomMultiSoldierList` | list of maps | — | One sub-list is picked at random and transferred, in addition to `everyMultiSoldierList`. |
| `spawnedCraftType` | string | — | A [craft](Ruleset-Crafts.md) of this type is given to HQ (immediately, or as a 1-hour transfer, per the `oxceGeoscapeEventsInstantDelivery` option). |

## Payload — items & research

Item lists are **cumulative**: every list is evaluated and the results are summed, then delivered to
the first base (HQ) — instantly if the `oxceGeoscapeEventsInstantDelivery` user option is on, else
as a 1-hour transfer.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `everyMultiItemList` | map item→int | — | These [items](Ruleset-Items.md) in these quantities, all of them. |
| `everyItemList` | list of items | — | One of each listed item. |
| `randomItemList` | list of items | — | Exactly one item from the list, picked uniformly. |
| `randomMultiItemList` | list of maps | — | One item→quantity sub-list is picked uniformly and granted whole. |
| `weightedItemList` | weights (item→int) | — | Exactly one item, picked by weight. |
| `invert` | bool | false | Flip the item payload into a **confiscation**: the same quantities are removed from base stores and from grounded craft instead of being added, and the popup lists what was taken. |
| `researchList` | list of research | — | One still-undiscovered topic from the list is granted (with its `lookup:` article and any `getOneFree` bonus). |

## Follow-up missions

| Key | Type | Default | Meaning |
|---|---|---|---|
| `adhocMissionScriptTags` | list of strings | — | When the event pops up, every [`adhocScripts:`](Ruleset-AdhocScripts.md) command sharing a tag is evaluated and may start an alien mission on the spot. |

## See also

- [`eventScripts:`](Ruleset-EventScripts.md) — what schedules these events
- [`adhocScripts:`](Ruleset-AdhocScripts.md) — the alien missions an event can trigger
- [`research:`](Ruleset-Research.md) · [`items:`](Ruleset-Items.md) · [`crafts:`](Ruleset-Crafts.md) · [`regions:`](Ruleset-Regions.md) · [`cutscenes:`](Ruleset-Cutscenes.md)
