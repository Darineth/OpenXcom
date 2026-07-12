# Ruleset: `regions:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`RuleRegion`](../src/Mod/RuleRegion.h) · **List key:** `type` · **Loader:**
[`RuleRegion::load`](../src/Mod/RuleRegion.cpp)

A region is a **geoscape territory used by the alien-mission machinery**: it defines where alien
missions may spawn (mission zones), which cities exist (point zones), how much a new X-COM base
costs there, and how attractive the region is when the game picks a location for a new mission.
Regions partition the globe alongside [`countries:`](Ruleset-Countries.md) (funding) — every point
on the globe should be in exactly one region.

```yaml
regions:
  - type: STR_NORTH_AMERICA
    cost: 800000                       # base construction cost here
    areas:
      - [ 172.44, 305.15, -71.01, -13.59 ]     # [lonMin, lonMax, latMin, latMax] degrees
    missionZones:
      - # zone 0 (regular UFO waypoints)
        - [ 190.77, 227.11, -66.21, -30.23 ]   # a rectangle area
      - # zone 1 ...
        - [ 251.71, 251.71, -33.94, -33.94, -1, STR_NEW_YORK ]  # a point = a city
    missionWeights:
      STR_ALIEN_RESEARCH: 40
      STR_ALIEN_HARVEST: 40
      STR_ALIEN_TERROR: 20
    regionWeight: 12
```

Entries **merge** across mods/files by `type`, support `refNode:` inheritance and `delete: true`
(see the [index](Ruleset.md#how-rulesets-load-the-60-second-version)).

## Fields

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | — | Unique id; also the region's display-name string key. |
| `refNode` | map | — | Inherit all fields from another (anonymous) node first. |
| `cost` | int $ | 0 | Cost of building a new X-COM base inside this region. |
| `areas` | list of `[lonMin, lonMax, latMin, latMax]` | — | Rectangles (degrees) forming the region's territory; **appended** to any inherited list. A region with no areas is a "technical" region (mission zones only). |
| `deleteOldAreas` | bool | false | Clear the inherited/previous `areas` list before loading this entry's `areas`. |
| `missionZones` | list of zones | — | Each zone is a **list of areas** — `[lonMin, lonMax, latMin, latMax]` plus optional 5th (texture id) and 6th (name) elements. Zones are referenced *by index* from [`ufoTrajectories:`](Ruleset-UfoTrajectories.md) waypoints and [`alienMissions:`](Ruleset-AlienMissions.md) `spawnZone`/`objectiveZone`. |
| `missionWeights` | map name → int | — | Weighted list of [alien mission](Ruleset-AlienMissions.md) types generated in this region (vanilla-style generation; `missionScripts:` can bypass it). |
| `regionWeight` | int | 0 | This region's weight when the game picks a region for a new alien mission (initial value; it shifts during a campaign). |
| `missionRegion` | string region | — | Substitute region: missions targeted at this region actually run in the named one (used e.g. for technical/ocean regions). |
| `provideBaseFunc` | list of tags | — | Base-function tags an X-COM base in this region gains. |
| `forbiddenBaseFunc` | list of tags | — | Base-function tags forbidden for bases in this region. |

## Mission zones, areas and cities

- An area whose `lonMin == lonMax` **and** `latMin == latMax` is a **point**. A point area with a
  **name** (6th element) becomes a **city**: it gets a globe label and is what terror-type
  missions target. The 5th element is the globe **texture id** used to pick the battle terrain
  (see [`globe:`](Ruleset-Globe.md) textures; `-1` is common for cities whose deployment picks its
  own terrain).
- Don't mix point and non-point areas in one zone — the loader logs a warning.
- Areas **crossing the prime meridian** must use the extended syntax: write `[350, 368, 20, 30]`,
  **not** `[350, 8, 20, 30]` — the latter is a load-time error (`lonMin > lonMax` inside a
  mission zone).
- Swapped `latMin`/`latMax` are auto-corrected on load.
- When a mission needs a point in a zone, the engine picks a random area of that zone and a random
  point inside it (`RuleRegion::getRandomPoint`).

## See also

- [`countries:`](Ruleset-Countries.md) — the funding partition of the globe (and `extraGlobeLabels:`)
- [`globe:`](Ruleset-Globe.md) — polygon textures that mission-zone texture ids refer to
- [`ufoTrajectories:`](Ruleset-UfoTrajectories.md) / [`alienMissions:`](Ruleset-AlienMissions.md) — the consumers of mission-zone indices
- [`missionScripts:`](Ruleset-MissionScripts.md) — the modern mission generation that can override `missionWeights`
