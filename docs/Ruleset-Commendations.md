# Ruleset: `commendations:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`RuleCommendations`](../src/Mod/RuleCommendations.h) · **List key:** `type` ·
**Loader:** [`RuleCommendations::load`](../src/Mod/RuleCommendations.cpp)

A commendation is a **medal**: an award the engine hands to a soldier automatically at debriefing
when its criteria are met, based on the soldier's diary (`SoldierDiary`). Each medal can be awarded
repeatedly — each award raises its **decoration level**, which can also grant an increasingly strong
[soldier bonus](Ruleset-SoldierBonuses.md). The award check lives in
[`SoldierDiary::manageCommendations`](../src/Savegame/SoldierDiary.cpp).

```yaml
commendations:
  - type: STR_MEDAL_KILLS
    description: STR_MEDAL_KILLS_DESCRIPTION
    sprite: 3
    criteria:
      totalKills: [10, 25, 50, 100, 200]   # one threshold per decoration level
    soldierBonusTypes:
      - STR_BONUS_KILLS_1
      - STR_BONUS_KILLS_2

  - type: STR_MEDAL_SNIPER
    sprite: 7
    criteria:
      killsWithCriteriaCareer: [5, 15, 30]
    killCriteria:
      - - [1, ["BT_FIREARM", "STATUS_DEAD", "SIDE_REAR"]]   # each kill: firearm, killed, hit from behind
```

Entries **merge** across mods/files by `type`, support `refNode:` inheritance and `delete: true`
(see the [index](Ruleset.md#how-rulesets-load-the-60-second-version)).

## Fields

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | — | Unique id; also the medal's display-name string key. |
| `refNode` | map | — | Inherit all fields from another (anonymous) node first. |
| `description` | string key | `""` | String key for the medal's description text in the soldier's diary. |
| `sprite` | int | 0 | Frame index of the medal's image in the commendation sprite set. |
| `criteria` | map name → list of ints | — | The award conditions: one **threshold per decoration level** for each named criterion (see below). All listed criteria must be met; once the soldier is past the last level, the medal stops being awarded. |
| `killCriteria` | nested list | — | Extra per-kill conditions used by the `killsWithCriteria*` criteria (see below). |
| `soldierBonusTypes` | list of bonus names | — | [Soldier bonus](Ruleset-SoldierBonuses.md) per decoration level (index-clamped: levels beyond the list keep the last entry). |
| `missionMarkerFilter` | list of strings | — | Restrict `totalMissions` counting to missions with these UFO/site marker types. |
| `missionTypeFilter` | list of strings | — | Restrict `totalMissions` counting to these mission types. |
| `requires` | list of research | — | [Research](Ruleset-Research.md) that must be done before the medal can be awarded at all. |
| `units` | list of soldier types | all | Restricts the medal to these [soldier types](Ruleset-Soldiers.md). |

## `criteria` — the award conditions

Each entry is `criterionName: [level0, level1, …]`: the value the soldier's diary stat must reach
for the *next* decoration level. Names come straight from
[`SoldierDiary::manageCommendations`](../src/Savegame/SoldierDiary.cpp).

### Career/mission tallies

`totalKills`, `totalStuns`, `totalMissions`, `totalWins`, `totalScore`, `totalMonthlyService`,
`totalImportantMissions`, `totalBaseDefenseMissions`, `totalTerrorMissions`, `totalNightMissions`,
`totalNightTerrorMissions`, `totalAlienBaseAssaults`, `totalUfosShotDown`, `totalUfosDamage`,
`totalAllUFOs` (killed at least one of every UFO type), `totalAllMissionTypes`,
`totalValientCrux`, `globeTrotter`.

### Feats & mishaps

`totalDaysWounded`, `totalTimesWounded`, `totalFellUnconcious`, `totalShotAt10Times`,
`totalHit5Times`, `totalFriendlyFired` (not awarded if the soldier is KIA/MIA), `total_lone_survivor`,
`totalIronMan`, `totalLongDistanceHits`, `totalLowAccuracyHits`, `totalReactionFire`,
`totalTrapKills`, `totalAllAliensKilled`, `totalAllAliensStunned`, `totalMartyrKills`,
`totalPostMortemKills`, `totalSlaveKills`, `totalWoundsHealed`, `totalWholeMedikit`, `totalRevives`,
`totalSoldierRevives`, `totalHostileRevives`, `totalNeutralRevives`, `totalStatGain`,
`totalBraveryGain`, `bestOfRank`, `bestSoldier`, `isDead`, `isMIA`.

### Modular ("noun") criteria

`totalKillsWithAWeapon`, `totalKillsByRace`, `totalKillsByRank`, `totalMissionsInARegion` — these are
tracked **per noun** (per weapon, race, rank, region), so the same medal is awarded separately for
each, with its own decoration level, and the noun is shown as part of the medal name.

### Kill-pattern criteria

`killsWithCriteriaCareer`, `killsWithCriteriaMission`, `killsWithCriteriaTurn` — count how many
*groups* of kills matching `killCriteria` the soldier scored over a career / within a single mission
/ within a single turn. The mission and turn variants are "peak achievements": counted once per
mission/turn in which the pattern is fulfilled.

## `killCriteria` — matching individual kills

Three levels of nesting:

```yaml
    killCriteria:
      -                                        # OR block 1 (any block satisfies)
        - [2, ["STR_LIVE_SOLDIER", "DT_AP"]]   # AND: 2 kills, each a soldier-rank alien killed by AP
        - [1, ["STR_LIVE_COMMANDER"]]          # AND: plus 1 commander kill
      -                                        # OR block 2
        - [5, ["BT_MELEE"]]
```

Every `[count, [details…]]` pair inside one OR block must be fulfilled. A kill matches a `details`
list only if it matches **every** string in it; each string is tested against, in order:

- the victim's **rank** (`STR_LIVE_SOLDIER`, `STR_LIVE_COMMANDER`, …) or **race** (`STR_SECTOID`, …);
- the **weapon** or **weapon ammo** item id (`__GUNBUTT` = the gun's melee attack);
- the kill's **status** (`STATUS_DEAD`, `STATUS_UNCONSCIOUS`, `STATUS_PANICKING`, …), the victim's
  **faction** (`FACTION_HOSTILE`, `FACTION_NEUTRAL`, `FACTION_PLAYER`), the **side** hit
  (`SIDE_FRONT`, `SIDE_LEFT`, `SIDE_RIGHT`, `SIDE_REAR`, `SIDE_UNDER`) and the **body part**
  (`BODYPART_HEAD`, `BODYPART_TORSO`, `BODYPART_LEFTARM`, …);
- the weapon's **battle type** (`BT_FIREARM`, `BT_MELEE`, `BT_GRENADE`, …);
- the ammo's **damage type** (`DT_AP`, `DT_IN`, `DT_HE`, `DT_LASER`, `DT_PLASMA`, `DT_STUN`,
  `DT_MELEE`, `DT_ACID`, `DT_SMOKE`, `DT_10` … `DT_19`).

## See also

- [`soldierBonuses:`](Ruleset-SoldierBonuses.md) — what `soldierBonusTypes` awards per decoration level
- [`soldiers:`](Ruleset-Soldiers.md) — the `units:` gate; medals show in the soldier's diary
- [`soldierTransformation:`](Ruleset-SoldierTransformation.md) — projects can require commendations
- [`armors:`](Ruleset-Armors.md) — can require a commendation to be worn (`requiresAward`)
- [`alienRaces:`](Ruleset-AlienRaces.md) / [`units:`](Ruleset-Units.md) — the race/rank strings the kill criteria match
