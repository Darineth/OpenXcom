# Ruleset: `research:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`RuleResearch`](../src/Mod/RuleResearch.h) · **List key:** `name` · **Loader:**
[`RuleResearch::load`](../src/Mod/RuleResearch.cpp)

A research entry is one node of the **tech tree**. It has a cost in scientist-days, a set of
prerequisites, and a set of consequences (topics unlocked, items spawned, cutscenes played). Almost
every other root — [`items:`](Ruleset-Items.md), [`facilities:`](Ruleset-Facilities.md),
[`crafts:`](Ruleset-Crafts.md), [`manufacture:`](Ruleset-Manufacture.md),
[`armors:`](Ruleset-Armors.md) — gates itself behind research via a `requires:` list of these names.

```yaml
research:
  - name: STR_LASER_PISTOL
    cost: 180
    points: 20
    dependencies:
      - STR_LASER_WEAPONS
    unlocks:
      - STR_LASER_RIFLE

  - name: STR_SECTOID_SOLDIER      # an interrogation
    cost: 60
    points: 50
    needItem: true                 # a live Sectoid Soldier must be in the base
    destroyItem: true              # ...and is consumed by the interrogation
    getOneFree:
      - STR_ALIEN_ORIGINS
      - STR_ALIEN_RESEARCH
    lookup: STR_SECTOID            # UFOpaedia article to pop up on completion
```

Entries **merge** across mods/files by `name`, support `refNode:` inheritance and `delete: true`
(see the [index](Ruleset.md#how-rulesets-load-the-60-second-version)).

## Core

| Key | Type | Default | Meaning |
|---|---|---|---|
| `name` | string | — | Unique id; also the display-name string key and (by convention) the matching item/unit name. |
| `refNode` | map | — | Inherit all fields from another (anonymous) node first. |
| `cost` | int | 0 | Scientist-days to complete. **`cost: 0` makes it a "fake"/checkpoint topic** that completes automatically as soon as it becomes available. |
| `points` | int | 0 | Score awarded when the topic is discovered. |
| `listOrder` | int | auto | Sort position in the research list. |
| `requiresBaseFunc` | list of tags | — | [Base-function](Ruleset-Facilities.md#base-functions-providebasefunc--requiresbasefunc--forbiddenbasefunc) tags the base must provide to run this project. |

## Tech-tree semantics

This is the part that trips people up, so here is what the engine
([`SavedGame::getAvailableResearchProjects`](../src/Savegame/SavedGame.cpp) and
[`SavedGame::addFinishedResearch`](../src/Savegame/SavedGame.cpp)) actually does.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `dependencies` | list of research | — | Topics that must **all** be discovered before this one appears in the research list. |
| `unlocks` | list of research | — | Topics put on the "unlocked" list when this one completes — they become available **even if their own `dependencies` are not met**. |
| `requires` | list of research | — | Hard prerequisites that must **all** be discovered; a topic with `requires` is **never shown to the player directly** and can only arrive through someone's `unlocks`. |
| `disables` | list of research | — | Topics permanently disabled (and un-researched) when this one completes. |
| `reenables` | list of research | — | Previously `disables`-d topics reset to "new" when this one completes. |
| `getOneFree` | list of research | — | On completion, **one** undiscovered, non-disabled topic from this list is granted for free. |
| `getOneFreeProtected` | map research → list of research | — | Same, but each list is only eligible if its key topic has already been discovered — lets one interrogation give different freebies depending on what you know. |
| `sequentialGetOneFree` | bool | false | Hand out the `getOneFree`/`getOneFreeProtected` picks in **list order** instead of at random. |
| `unlockFinalMission` | bool | false | Completing this topic unlocks the final mission (only one topic in a mod may do this). |
| `repeatable` | bool | false | The topic is never marked as discovered, so it can be researched over and over. |

Rules the loader enforces or implies:

- **A topic with a non-empty `requires:` must have `cost: 0`** — the mod fails to load otherwise
  (`"has requirements, but the cost is not zero"`). `requires` + `unlocks` is therefore the
  standard idiom for a *protected zero-cost checkpoint*: e.g. `STR_THE_MARTIAN_SOLUTION` requires
  `STR_CYDONIA_OR_BUST` and is only reached when some other topic explicitly `unlocks` it.
- **`dependencies` are an AND.** To express OR, make two zero-cost fake topics that each depend on
  one branch and both `unlocks:` the real topic — the class comment in
  [`RuleResearch.h`](../src/Mod/RuleResearch.h) spells this pattern out.
- A discovered topic **stays in the available list** as long as it still has undiscovered
  `getOneFree` entries or undiscovered "protected unlocks" (i.e. `unlocks:` targets that have
  `requires:`), which is why you can re-interrogate the same alien for more freebies.
- `getOneFree` picks skip topics that are already discovered or permanently disabled.

## Item requirement

| Key | Type | Default | Meaning |
|---|---|---|---|
| `needItem` | bool | false | The base must hold the needed item in stores to start the project (used for interrogations and artifact analysis). |
| `neededItem` | string item | same name as the topic | The [item](Ruleset-Items.md) `needItem` refers to; if omitted the engine looks up an item with the topic's own `name`. |
| `destroyItem` | bool | false | The item is consumed when the research completes. |
| `returnsItem` | bool | false | The item is returned to stores when the research completes. |

When `needItem` is combined with `destroyItem` or `returnsItem` the item is **held** (removed from
stores) for the duration of the project, and given back if the project is cancelled
([`Base::removeResearch`](../src/Savegame/Base.cpp)). With `destroyItem` and the `retainCorpses`
option on, an interrogated live alien leaves its corpse behind instead of vanishing. Defining both
`neededItem` and an item literally named after the topic, with the two disagreeing, is a load error.

## Rewards & side effects on completion

| Key | Type | Default | Meaning |
|---|---|---|---|
| `lookup` | string research | — | The topic whose UFOpaedia article is shown in the "research completed" popup (self-reference is ignored). |
| `cutscene` | string cutscene | — | [Cutscene](Ruleset-Cutscenes.md) played on completion. |
| `spawnedItem` | string item | — | Item added to the base stores on completion. |
| `spawnedItemCount` | int | 1 | How many copies of `spawnedItem` to add. |
| `spawnedItemList` | list of items | — | Additional items (one each) added to the base stores on completion. |
| `spawnedEvent` | string event | — | [Geoscape event](Ruleset-Events.md) fired on completion. |
| `events` | weighted map event → int | — | Weighted pool from which **one** geoscape event is rolled on completion (alternative to `spawnedEvent`). |
| `increaseCounter` | list of strings | — | Named custom counters incremented on completion (readable from Y-Script/mission scripts). |
| `decreaseCounter` | list of strings | — | Named custom counters decremented on completion. |

## Scripting

Research rules expose `tags:` / script values (`ScriptValues<RuleResearch>`) and are visible to
Y-Script as `RuleResearch` (`getCost`, `getPoints`, `getLookup`, `getNeededItem`). See
[Ruleset-Scripting.md](Ruleset-Scripting.md).

## See also

- [`manufacture:`](Ruleset-Manufacture.md) — production projects gated by `requires:` research
- [`facilities:`](Ruleset-Facilities.md) — labs (`labs:` capacity) and the base-function gates
- [`items:`](Ruleset-Items.md) — `requires:` on items, and the `neededItem`/`spawnedItem` links
- [`crafts:`](Ruleset-Crafts.md) / [`craftWeapons:`](Ruleset-CraftWeapons.md) — research-gated hardware
- [`ufopaedia:`](Ruleset-Ufopaedia.md) — the articles `lookup:` opens
