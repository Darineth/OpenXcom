# Ruleset: `ufoTrajectories:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`UfoTrajectory`](../src/Mod/UfoTrajectory.h) · **List key:** `id` · **Loader:**
[`UfoTrajectory::load`](../src/Mod/UfoTrajectory.cpp)

A trajectory is a **flight plan**: an ordered list of waypoints, each saying *which mission zone of
the region to fly to*, *at what altitude*, and *at what fraction of the UFO's top speed*. Each wave
of an [`alienMissions:`](Ruleset-AlienMissions.md) entry names one trajectory, and every
[UFO](Ruleset-Ufos.md) the wave spawns follows it from waypoint 0 onward; when the last waypoint is
reached the UFO despawns (and the mission scores its `points`)
([`AlienMission.cpp`](../src/Savegame/AlienMission.cpp)).

```yaml
ufoTrajectories:
  - id: P4                 # scout: sweep the region, land once, leave
    groundTimer: 3000      # x5 = 15000 seconds on the ground
    waypoints:
      - [0, 3, 50]         # zone 0, STR_HIGH_UC, 50% speed
      - [3, 2, 50]         # zone 3, STR_LOW_UC
      - [3, 0, 30]         # zone 3, STR_GROUND -> lands here
      - [0, 3, 50]
      - [0, 4, 100]        # leaves the map (last waypoint)
```

Entries **merge** across mods/files by `id`, support `refNode:` inheritance and `delete: true`
(see the [index](Ruleset.md#how-rulesets-load-the-60-second-version)).

## Fields

| Key | Type | Default | Meaning |
|---|---|---|---|
| `id` | string | — | Unique id, referenced from an [alien mission](Ruleset-AlienMissions.md) wave's `trajectory:`. |
| `groundTimer` | int | 5 | How long a landed UFO stays on the ground, in **5-second ticks** (the engine sets `secondsRemaining = groundTimer × 5`). |
| `waypoints` | list of `[zone, altitude, speed]` | — | The flight path, in order; each entry is a 3-element sequence (see below). |

### Waypoint triple

| Slot | Type | Meaning |
|---|---|---|
| 0 — `zone` | int | Index into the target [region's](Ruleset-Regions.md) `missionZones:`; a random point of that zone is chosen (a random *landing* point if the altitude is `STR_GROUND`). |
| 1 — `altitude` | int 0–4 | 0 = `STR_GROUND` (the UFO lands), 1 = `STR_VERY_LOW`, 2 = `STR_LOW_UC`, 3 = `STR_HIGH_UC`, 4 = `STR_VERY_HIGH`. |
| 2 — `speed` | int % | Percentage of the UFO's maximum speed to use on the leg *starting* at this waypoint. |

## Behavior notes

- A trajectory must have **at least two waypoints** — the engine throws
  ("Missing second waypoint!") when a UFO cannot be given a destination.
- Waypoint 0 is the spawn point. For missions with an Earth-based `operationType` the UFO instead
  starts at its alien base, but altitudes/speeds still come from the trajectory.
- Landing behavior is overridden in special cases: an objective wave of a supply mission always
  lands on the alien base regardless of the waypoint's zone, and a mission-site objective wave may
  land on the `spawnZone` area (or on an X-COM base) instead.
- Reaching the final waypoint ends that UFO's run: it despawns and the mission's `points` are
  awarded to the region/country.

## The reserved trajectory `__RETALIATION_ASSAULT_RUN`

The engine looks this id up **by name** (`UfoTrajectory::RETALIATION_ASSAULT_RUN`) for the final leg
of a retaliation mission — the battleship's run at the discovered X-COM base. It must exist in the
mod (the lookup is fatal if missing), and its waypoints are deliberately ignored on arrival: when a
UFO on this trajectory reaches its destination, the engine converts it straight into a base defense
instead of continuing the path.

## See also

- [`alienMissions:`](Ruleset-AlienMissions.md) — waves reference trajectories by id
- [`ufos:`](Ruleset-Ufos.md) — the craft that fly them (their `speedMax` is what `speed` scales)
- [`regions:`](Ruleset-Regions.md) — the `missionZones:` a waypoint's `zone` indexes into
- [`missionScripts:`](Ruleset-MissionScripts.md) — what starts the missions in the first place
