# Ruleset: `alienDeployments:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`AlienDeployment`](../src/Mod/AlienDeployment.h) · **List key:** `type` ·
**Loader:** [`AlienDeployment::load`](../src/Mod/AlienDeployment.cpp)

An alien deployment is the **complete setup of one tactical mission**: who spawns and with what
(`data:`), on which [terrain](Ruleset-Terrains.md) and with which
[map script](Ruleset-MapScripts.md), what the briefing says, what counts as winning or losing, and —
for mission sites and alien bases — how the target behaves on the Geoscape. Its `type` is either a
[UFO](Ruleset-Ufos.md) type (used when that UFO is assaulted), a craft type (base defense), or a
free-standing mission id referenced by [`alienMissions:`](Ruleset-AlienMissions.md).

```yaml
alienDeployments:
  - type: STR_TERROR_MISSION
    data:
      - alienRank: 1                 # index into the alienRaces: members list
        lowQty: 4                    # count on Beginner
        highQty: 8                   # count on Superhuman (mid difficulties interpolate)
        dQty: 2                      # + random 0..2
        itemSets:                    # one set per alien item level (0/1/2)
          - [ STR_PLASMA_PISTOL, STR_PLASMA_PISTOL_CLIP ]
          - [ STR_PLASMA_RIFLE,  STR_PLASMA_RIFLE_CLIP ]
          - [ STR_HEAVY_PLASMA,  STR_HEAVY_PLASMA_CLIP ]
    civilians: 8
    terrains: [ URBAN ]
    shade: 5
    script: URBAN_SCRIPT
    briefing:
      title: STR_TERROR_MISSION
      desc: STR_TERROR_MISSION_BRIEFING
      background: BACK03.SCR
    markerName: STR_TERROR_SITE
    points: 5
    despawnPenalty: 1000
```

Entries **merge** across mods/files by `type`, support `refNode:` inheritance and `delete: true`
(see the [index](Ruleset.md#how-rulesets-load-the-60-second-version)).

## The roster (`data:`)

`data:` is a list of **`DeploymentData`** entries — one per alien rank you want on the map. The
quantity actually spawned is interpolated from `lowQty`/`medQty`/`highQty` by difficulty
(Beginner → `lowQty`, Superhuman → `highQty`; `medQty` anchors the middle if given), then `dQty` and
`extraQty` add a random `0..N` on top (on `demigod` mods, the maximum instead of a roll).

| Key | Type | Default | Meaning |
|---|---|---|---|
| `alienRank` | int | 0 | Rank index looked up in the mission's [`alienRaces:`](Ruleset-AlienRaces.md) member list, and matched against node ranks when placing the unit. |
| `customUnitType` | string | — | Spawn this exact [`units:`](Ruleset-Units.md) type instead of resolving rank → race member. |
| `lowQty` | int | 0 | Count on Beginner. |
| `medQty` | int | 0 | Count on Veteran (0 = interpolate between low and high). |
| `highQty` | int | 0 | Count on Superhuman. |
| `dQty` | int | 0 | Extra units, rolled `0..dQty`. |
| `extraQty` | int | 0 | Second extra-unit roll, `0..extraQty` (kept separate so `refNode` families can add on top of `dQty`). |
| `percentageOutsideUfo` | int % | 0 | Chance each unit of this rank spawns outside the UFO rather than inside (only applies to UFO missions unless `forcePercentageOutsideUfo`). |
| `itemSets` | list of item lists | — | Equipment by alien item level (0, 1, 2 — index chosen from `alienItemLevels:`; a too-high level clamps to the last set). **Required** unless the unit is a living weapon. |
| `extraRandomItems` | list of item lists | — | For each list, **one** item is picked at random and added to the unit. |

## Units, civilians & map

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | — | Unique id (a UFO type, a craft type, or a mission id). |
| `refNode` | map | — | Inherit all fields from another (anonymous) node first. |
| `data` | list | — | The alien roster; see [above](#the-roster-data). |
| `reinforcements` | list | — | Mid-battle spawn waves; see [below](#reinforcements-reinforcements). |
| `civilians` | int | 0 | Number of civilians spawned, drawn from the terrain's `civilianTypes`. |
| `civiliansByType` | map `unit: count` | — | Civilians of specific [`units:`](Ruleset-Units.md) types (added to `civilians`). |
| `civilianSpawnNodeRank` | int | 0 | Node rank civilians spawn at. |
| `ignoreLivingCivilians` | bool | false | Surviving civilians are ignored for scoring/commendations. |
| `markCiviliansAsVIP` | bool | false | Civilians count as VIPs (see `vipSurvivalPercentage`). |
| `terrains` | list of strings | — | [Terrain](Ruleset-Terrains.md) pool for the battle (one is picked); empty = derive from the globe texture / UFO. |
| `script` | string | terrain's script | The [`mapScripts:`](Ruleset-MapScripts.md) entry that assembles the map. |
| `mapScripts` | list of strings | — | Pool of map scripts; one is rolled per battle and takes priority over `script`. |
| `width` / `length` / `height` | int | 0 | Map dimensions in tiles/levels (0 = let the map script's `resize` decide). |
| `shade` | int | −1 | Fixed global light shade (0 = day … 15 = pitch black); −1 = derive from the Geoscape clock. |
| `minShade` / `maxShade` | int | −1 | Clamp the derived shade into this range. |
| `depth` | `[min, max]` | `[0, 0]` | Battle depth range (TFTD); overrides the terrain's `depth`. |
| `music` | list of strings | — | Battlescape music pool (overrides the terrain's). |
| `enviroEffects` | string | — | [`enviroEffects:`](Ruleset-EnviroEffects.md) applied (wins over the terrain's). |
| `startingCondition` | string | — | [`startingConditions:`](Ruleset-StartingConditions.md) applied before the battle. |
| `customUfo` | string | — | UFO type whose map a blank `addUFO` map-script command should use when there is no real UFO. |
| `nextStage` | string | — | Deployment of the **next stage** of a multi-stage mission (empty = last stage). |
| `race` | string | — | Alien race forced for this deployment (used mainly by `nextStage` and New Battle). |
| `randomRace` | list of strings | — | Race pool rolled per battle (overrides `race`). |
| `noWeaponPile` | bool | false | Hide the recovered-items ("weapon pile") tile contents from the player. |
| `bughuntMinTurn` | int | 0 | Turn from which bug-hunt mode may trigger (0 = never). |
| `cheatTurn` | int | 20 | Turn from which the AI is allowed to "see" the player's units. |
| `forcePercentageOutsideUfo` | bool | false | Apply `percentageOutsideUfo` on non-UFO missions too. |

## Objectives, win & lose

| Key | Type | Default | Meaning |
|---|---|---|---|
| `objectiveType` | int | −1 | The special tile type that must be destroyed/reached (`SpecialTileType` in [MapData.h](../src/Mod/MapData.h): e.g. 2 UFO power source, 14 MUST_DESTROY); −1 = no tile objective. |
| `objectivesRequired` | int | 0 | How many such tiles must be destroyed (0 = all of them). |
| `objectivePopup` | string | — | `STR_*` message shown when the objective is met. |
| `objectiveComplete` | `[STR_KEY, score]` | — | Debriefing line + score awarded when the objective was completed. |
| `objectiveFailed` | `[STR_KEY, score]` | — | Debriefing line + score (usually negative) when it was not. |
| `missionCompleteText` / `missionFailedText` | string | — | Custom debriefing headline replacing the default win/lose text. |
| `turnLimit` | int | 0 | Battle turn limit (0 = none). |
| `chronoTrigger` | int | 0 | What the turn limit does: 0 = force lose, 1 = force abort, 2 = force win, 3 = force win with surrender. |
| `escapeType` | int | 0 | Which tiles let units escape when aborting: 0 = none, 1 = exit area, 2 = entry area, 3 = either. |
| `keepCraftAfterFailedMission` | bool | false | The craft is not lost when the mission is failed/aborted. |
| `allowObjectiveRecovery` | bool | false | The objective item/tile can still be recovered on a failed mission. |
| `vipSurvivalPercentage` | int % | 0 | Percentage of VIP units that must survive for the mission to count as a success. |
| `finalDestination` | bool | false | Winning this mission wins the campaign. |
| `winCutscene` / `loseCutscene` / `abortCutscene` | string | — | [`cutscenes:`](Ruleset-Cutscenes.md) played on that outcome. |
| `missionBountyItem` | string item | — | Item granted (to the base stores) on success. |
| `missionBountyItemCount` | int | 1 | How many. |
| `unlockedResearch` | string | — | [Research](Ruleset-Research.md) unlocked on success. |
| `unlockedResearchOnFailure` / `unlockedResearchOnDespawn` | string | — | Research unlocked on failure / on despawn of the site. |
| `counterSuccess` / `counterFailure` / `counterDespawn` / `counterAll` | string | — | Name of a custom savegame counter to **increase** on that outcome (`counterAll` = any outcome). |
| `decreaseCounterSuccess` / `…Failure` / `…Despawn` / `…All` | string | — | Same, but **decrease** the counter. |
| `successEvents` / `failureEvents` / `despawnEvents` | weighted map | — | Weighted pool of [`events:`](Ruleset-Events.md) to spawn on that outcome. |
| `points` | int | 0 | Score the aliens gain while this site/base exists (half-hourly for sites, daily for bases). |
| `despawnPenalty` | int | 0 | Score X-COM loses if the site despawns un-attacked. |
| `abortPenalty` | int | 0 | Score X-COM loses on aborting the mission. |

## Briefing & Geoscape presentation

`briefing:` is a **`BriefingData`** map, also reusable per reinforcement wave:

| Key | Type | Default | Meaning |
|---|---|---|---|
| `title` | string | — | `STR_*` headline of the briefing screen. |
| `desc` | string | — | `STR_*` briefing body text. |
| `palette` | int | 0 | Which briefing palette variant to use. |
| `textOffset` | int | 0 | Vertical offset (px) of the briefing text block. |
| `music` | string | `GMDEFEND` | Music track for the briefing screen. |
| `background` | string | `BACK16.SCR` | Background image. |
| `cutscene` | string | — | [Cutscene](Ruleset-Cutscenes.md) played before the briefing. |
| `showCraft` | bool | true | Show the "craft: …" line. |
| `showTarget` | bool | true | Show the "target: …" line. |

Geoscape-facing fields:

| Key | Type | Default | Meaning |
|---|---|---|---|
| `alert` | string | `STR_ALIENS_TERRORISE` | `STR_*` message of the "mission detected" popup. |
| `alertBackground` | string | `BACK03.SCR` | Background of that popup. |
| `alertDescription` | string | — | Text shown by the target-info [Info] button. |
| `alertSound` | sound id | −1 | Sound played with the alert (offset into `GEO.CAT`). |
| `markerName` | string | `STR_TERROR_SITE` | `STR_*` label of the globe marker. |
| `markerIcon` | int | −1 | Globe marker sprite index (mod offsets apply). |
| `duration` | `[min, max]` | `[0, 0]` | How long the site stays on the globe, in hours. |
| `alienBaseDiscoveredMessage` | string | — | `STR_*` message shown when a base of this type is discovered (empty = engine default). |
| `isHidden` | bool | false | Hide this deployment from the New Battle screen. |
| `alienBase` | bool | false | Treat as an alien base (New Battle screen). |
| `fakeUnderwaterSpawnChance` | int % | 0 | Chance an alien base of this type spawns on a fake-underwater globe texture. |

## Alien base behavior

Only meaningful when the deployment is used for an alien base.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `genMission` | weighted map | — | Weighted pool of [`alienMissions:`](Ruleset-AlienMissions.md) this base generates. |
| `genMissionFreq` | int % | 0 | Monthly chance to generate one of them. |
| `genMissionLimit` | int | 1000 | Maximum number of such missions alive at once. |
| `genMissionRaceFromAlienBase` | bool | true | Generated missions inherit the base's race instead of rolling their own. |
| `huntMissionWeights` | map `month: weighted map` | — | Hunt-mission pools, keyed by the number of months passed (the highest key ≤ current month wins). |
| `huntMissionMaxFrequency` | int | 60 | Minimum spacing between hunt missions, in minutes. |
| `huntMissionRaceFromAlienBase` | bool | true | Hunt missions inherit the base's race. |
| `baseDetectionRange` | int | 0 | Range (in globe units) at which the base detects X-COM craft. |
| `baseDetectionChance` | int % | 100 | Chance to actually detect a craft in range (rolled every 10 minutes). |
| `baseSelfDestructCode` | string | — | [Research](Ruleset-Research.md) topic that lets X-COM self-destruct this base. |
| `alienBaseUpgrades` | map `month: weighted map` | — | Weighted pool of deployments this base may upgrade into, keyed by the base's age in months. |
| `resetAlienBaseAgeAfterUpgrade` | bool | false | Reset the base's age when upgrading **from** this deployment. |
| `resetAlienBaseAge` | bool | false | Reset the base's age when upgrading **into** this deployment. |
| `upgradeRace` | string | — | New race the base takes when upgrading into this deployment. |
| `alienRaceEvolution` | list of `[months, fromRace, toRace]` | — | Race evolution rules for the base; the first entry whose `months` ≤ the current month and whose `fromRace` matches applies (entries are sorted by months, descending). |

## Reinforcements (`reinforcements:`)

A list of **`ReinforcementsData`** waves, checked at the start of every turn
([`NextTurnState`](../src/Battlescape/NextTurnState.cpp)). Each wave has its own `data:` roster
(same `DeploymentData` structure as [above](#the-roster-data)) and its own `briefing:`.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | — | Wave id (used to count runs against `maxRuns`). |
| `data` | list | — | The units to spawn; same fields as the deployment `data:`. |
| `briefing` | BriefingData | — | Optional pop-up briefing when the wave lands (only shown if `title` is set). |
| `minDifficulty` / `maxDifficulty` | int | 0 / 4 | Difficulty range in which this wave can trigger. |
| `objectiveDestroyed` | bool | false | Trigger when all objectives have been destroyed (instead of on a turn schedule). |
| `turns` | list of ints | — | Exact turns on which the wave triggers (takes priority over `minTurn`/`maxTurn`). |
| `minTurn` / `maxTurn` | int | 0 / −1 | Turn window in which the wave may trigger (−1 = no upper bound). |
| `executionOdds` | int % | 100 | Chance the wave actually runs when its schedule matches. |
| `maxRuns` | int | −1 | How many times this wave may ever run (−1 = unlimited). |
| `useSpawnNodes` | bool | true | Spawn on map route nodes (false = spawn on plain tiles of compliant blocks). |
| `mapBlockFilterType` | int | 3 | Which cells count as spawn cells: 0 = anywhere, 1 = only cells marked by the [map script](Ruleset-MapScripts.md) (`markAsReinforcementsBlock`), 2 = only cells matched by this wave's filters, 3 = union of both, 4 = intersection of both. |
| `spawnBlocks` | list of strings | — | Compass keywords selecting map-edge cells: `NORTH`, `SOUTH`, `EAST`, `WEST`, `NW`, `NE`, `SW`, `SE`, `EDGES` (all four sides). Empty = all cells. |
| `spawnBlockGroups` | list of ints | — | Further restrict spawn cells to blocks in these [block groups](Ruleset-Terrains.md#block-groups). |
| `spawnNodeRanks` | list of ints | — | Only spawn on route nodes of these ranks. |
| `spawnZLevels` | list of ints | — | Only spawn on these Z levels. |
| `randomizeZLevels` | bool | true | Shuffle the Z levels instead of preferring the lowest. |
| `minDistanceFromXcomUnits` | int tiles | 0 | Reject spawn points within this distance of any live X-COM unit (values ≤ 1 disable the check). |
| `maxDistanceFromBorders` | int tiles | 0 | Only spawn within this distance of the map border (0, or ≥ 10, disables the check). |
| `forceSpawnNearFriend` | bool | true | If no compliant spawn point is found, spawn next to a friendly unit instead of failing. |

## See also

- [`alienMissions:`](Ruleset-AlienMissions.md) — the geoscape missions that reference a deployment
- [`alienRaces:`](Ruleset-AlienRaces.md) — the rank → unit lists `alienRank` indexes into
- [`terrains:`](Ruleset-Terrains.md) / [`mapScripts:`](Ruleset-MapScripts.md) — the map the deployment plays out on
- [`startingConditions:`](Ruleset-StartingConditions.md) / [`enviroEffects:`](Ruleset-EnviroEffects.md) — the pre-battle and in-battle modifiers a deployment can attach
- [`items:`](Ruleset-Items.md) — the `itemSets:` equipment and the `missionBountyItem`
