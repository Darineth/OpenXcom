# Ruleset: `alienRaces:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`AlienRace`](../src/Mod/AlienRace.h) · **List key:** `id` · **Loader:**
[`AlienRace::load`](../src/Mod/AlienRace.cpp)

An alien race is a **rank → unit-type lookup table**. Missions and UFOs pick a race; the
[`alienDeployments:`](Ruleset-AlienDeployments.md) entry then asks for "one alienRank 5" and the race
answers with the concrete [`units:`](Ruleset-Units.md) type to spawn. The race also carries its own
retaliation behavior and the deployment used when the player raids its base.

```yaml
alienRaces:
  - id: STR_SECTOID
    members:
      - STR_SECTOID_COMMANDER     # 0 = commander
      - STR_SECTOID_LEADER        # 1 = leader
      - STR_SECTOID_ENGINEER      # 2 = engineer
      - STR_SECTOID_MEDIC         # 3 = medic
      - STR_SECTOID_NAVIGATOR     # 4 = navigator
      - STR_SECTOID_SOLDIER       # 5 = soldier
      - STR_CYBERDISC_TERRORIST   # 6 = terror unit
    retaliationAggression: 0
```

Entries **merge** across mods/files by `id`, support `refNode:` inheritance and `delete: true`
(see the [index](Ruleset.md#how-rulesets-load-the-60-second-version)).

## Fields

| Key | Type | Default | Meaning |
|---|---|---|---|
| `id` | string | — | Unique id; also the race's display-name string key. |
| `refNode` | map | — | Inherit all fields from another (anonymous) node first. |
| `members` | list of unit types | — | The race roster **indexed by alien rank** — position *is* the rank (see below). |
| `membersRandom` | list of lists of unit types | — | Same, but each rank is a *list*: the engine picks one uniformly at random per spawn. **Overrides `members` entirely** when present. |
| `baseCustomDeploy` | string deployment | — | [Deployment](Ruleset-AlienDeployments.md) supplying the weapon/item loadout for this race's units inside an alien base. |
| `baseCustomMission` | string deployment | — | Deployment used for the *battle layout* when the player assaults this race's alien base. |
| `retaliationAggression` | int | 0 | How eager the race is to launch a retaliation mission after the player shoots down one of its UFOs. |
| `retaliationMissionWeights` | map month → weighted options | — | Which retaliation [alien missions](Ruleset-AlienMissions.md) this race sends, as a weight table per mission type, keyed by the game month it takes effect from (the highest key ≤ the current month wins). |
| `listOrder` | int | auto | Sort position in race lists (UFOpaedia, debug). |

## Rank order

The index into `members` / `membersRandom` is the `AlienRank` enum
([AlienRace.h](../src/Mod/AlienRace.h)) — the same number an
[`alienDeployments:`](Ruleset-AlienDeployments.md) entry uses in its `alienRank:` field:

| Index | Rank |
|---|---|
| 0 | Commander |
| 1 | Leader |
| 2 | Engineer |
| 3 | Medic |
| 4 | Navigator |
| 5 | Soldier |
| 6 | Terrorist (terror unit) |
| 7 | Terrorist 2 (second terror unit) |

Asking for a rank the race does not define throws a load/runtime error ("does not have a member at
position/rank N"), so a race must list every rank its deployments can request. Ranks a mod does not
need can be filled with a placeholder unit type.

## See also

- [`units:`](Ruleset-Units.md) — the unit types listed as members (their `race:`/`rank:` fields mirror this table)
- [`alienDeployments:`](Ruleset-AlienDeployments.md) — asks for units by `alienRank`, and the `baseCustom*` targets
- [`alienMissions:`](Ruleset-AlienMissions.md) — chooses the race for a mission; the retaliation weights point back here
- [`commendations:`](Ruleset-Commendations.md) — kill criteria can match a race id
