# Ruleset: `ufos:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`RuleUfo`](../src/Mod/RuleUfo.h) · **List key:** `type` · **Loader:**
[`RuleUfo::load`](../src/Mod/RuleUfo.cpp)

A UFO type defines an **alien craft on the geoscape**: its flight/combat stats, dogfight
presentation, scoring, hunter-killer behavior, and the battlescape map used when it lands or
crashes. UFO types are referenced from [`alienMissions:`](Ruleset-AlienMissions.md) waves and fly
along [`ufoTrajectories:`](Ruleset-UfoTrajectories.md).

```yaml
ufos:
  - type: STR_SMALL_SCOUT
    size: STR_VERY_SMALL
    sprite: 0                     # dogfight window image
    marker: 2                     # globe marker while flying
    damageMax: 50                 # craft-stats block, inline at top level
    speedMax: 2200
    accel: 12
    power: 0
    range: 0
    score: 50
    reload: 56
    breakOffTime: 200
    battlescapeTerrainData:
      name: UFO1A
      mapDataSets: [ BLANKS, UFO1 ]
      mapBlocks:
        - name: UFO1A
          width: 10
          length: 10
```

Entries **merge** across mods/files by `type`, support `refNode:` inheritance and `delete: true`
(see the [index](Ruleset.md#how-rulesets-load-the-60-second-version)).

## Identity & presentation

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | — | Unique id; also the UFO's display-name string key. |
| `refNode` | map | — | Inherit all fields from another (anonymous) node first. |
| `size` | string | `STR_VERY_SMALL` | Size class: `STR_VERY_SMALL`, `STR_SMALL`, `STR_MEDIUM_UC`, `STR_LARGE`, `STR_VERY_LARGE` (a legacy `STR_MEDIUM` is auto-corrected). Drives the radius/visibility/blob defaults below. |
| `radius` | int px | −1 | Dogfight-window circle radius; −1 = derived from `size` (2/3/4/5/6). |
| `visibility` | int | 0 | Detection modifier added to radar checks; 0 = derived from `size` (−30/−15/0/+15/+30). |
| `blobSize` | int 0–7 | −1 | Radar-blob size in the dogfight window; −1 = derived from `size` (0–4); values above 7 clamp to 7. |
| `sprite` | int | −1 | `INTERWIN.DAT` image index shown in the dogfight window. |
| `marker` / `markerLand` / `markerCrash` | int | −1 | Globe marker sprite while flying / landed / crashed (−1 = engine defaults; mod sprite offset applied above 8). |
| `modSprite` | string surface | — | Named surface to use as the dogfight "preview" image instead of `sprite`. |
| `hitImage` | string surface | — | Surface shown in the base-destroyed screen when this UFO (missile) hits a base. |
| `fireSound` | sound id (GEO.CAT) | −1 | Sound when the UFO fires in a dogfight. |
| `alertSound` | sound id (GEO.CAT) | −1 | Sound for the "UFO detected" alert. |
| `huntAlertSound` | sound id (GEO.CAT) | −1 | Sound for the "UFO on intercept course" (hunter-killer) alert. |
| `hitSound` | sound id (GEO.CAT) | −1 | Sound played on the base-destroyed screen (missile strike). |

## Combat & scoring

| Key | Type | Default | Meaning |
|---|---|---|---|
| `power` | int | 0 | Maximum damage per shot of the UFO's dogfight weapon. |
| `range` | int km | 0 | The UFO weapon's firing range. |
| `reload` | int game-sec | 0 | The UFO weapon's reload time (aggressive interceptors shave 10s off). |
| `breakOffTime` | int game-sec | 0 | How long the UFO fights before breaking off the dogfight. |
| `score` | int | 0 | Points awarded for shooting this UFO down. |
| `missionScore` | int | 1 | Points awarded (to the aliens) every 30 minutes while flying a mission (doubled when landed). |

## Craft-stats block

The [craft-stats block](Ruleset-Crafts.md#the-craft-stats-block) (`RuleCraftStats`) is loaded
**inline at the entry's top level** — `damageMax`, `speedMax`, `accel`, `fuelMax`, `armor`
(flat damage reduction per hit), `avoidBonus`, `hitBonus`, `powerBonus`, `shieldCapacity`,
`shieldRecharge`, `shieldRechargeInGeoscape`, `shieldBleedThrough`, `radarRange`, `radarChance`,
`sightRange`, etc. See [Ruleset-Crafts.md](Ruleset-Crafts.md) for the full key list and meanings;
notable UFO defaults from the constructor: `sightRange: 268`, `radarRange: 672` (used by
hunter-killers to find prey).

UFOs extend the block (`RuleUfoStats`) with two extra keys, also valid inside `raceBonus`:

| Key | Type | Default | Meaning |
|---|---|---|---|
| `craftCustomDeploy` | string deployment | — | [alienDeployment](Ruleset-AlienDeployments.md) override used when assaulting this UFO (crash/landing site). |
| `missionCustomDeploy` | string deployment | — | alienDeployment override for the mission-site this UFO spawns. |
| `raceBonus` | map race → stats | — | Per-[alien-race](Ruleset-AlienRaces.md) additive stat bonuses: each value is another craft-stats(+deploy) block added on top when the UFO is crewed by that race. |

## Hunter-killers & missiles

| Key | Type | Default | Meaning |
|---|---|---|---|
| `hunterKillerPercentage` | int % | 0 | Chance the spawned UFO is a hunter-killer that actively attacks X-COM craft (mission waves can override). |
| `huntMode` | int | 0 | Preferred prey: 0 = interceptors, 1 = transports, 2 = random per UFO. |
| `huntSpeed` | int % | 100 | Cruising speed while hunting, as a percentage of `speedMax`. |
| `huntBehavior` | int | 2 | In-dogfight behavior: 0 = flees when damaged, 1 = kamikaze (never flees), 2 = random pick at spawn. |
| `softlockThreshold` | int | 100 | Shots a hunter-killer fires without result before it gives up and disengages (anti-softlock). |
| `missilePower` | int | 0 | Marks the UFO as an alien missile: base facilities destroyed on a successful retaliation strike (−1 = destroy whole base). |
| `missileStopChance` | int % | 0 | Chance a successful missile strike ends the retaliation mission. |
| `unmanned` | bool | false | Unmanned craft (drone/missile): no crew is generated, shooting it down yields no battle. |
| `instaHyper` | bool | false | Show hyperwave-decoder info during base defense even if the UFO was never hyper-detected. |
| `noAlert` | bool | false | Suppress the UFO-detected popup/alert for this type. |
| `splashdownSurvivalChance` | int % | 100 | Chance the UFO survives (as a crash site) when shot down over fake water. |
| `fakeWaterLandingChance` | int % | 0 | Chance the UFO decides to land on a fake-water texture (hybrid mods). |

## Battlescape

| Key | Type | Default | Meaning |
|---|---|---|---|
| `battlescapeTerrainData` | map | — | An inline [terrain](Ruleset-Terrains.md) definition (`name`, `mapDataSets`, `mapBlocks`) — the UFO's own map, placed into the mission terrain. |

## Scripting

UFOs expose Y-Script hooks (`scripts:` sub-node, `ufoScripts` — e.g. dogfight hooks) and custom
`tags:`. See [Ruleset-Scripting.md](Ruleset-Scripting.md).

## See also

- [`crafts:`](Ruleset-Crafts.md) — the player-side counterpart and the shared craft-stats block
- [`craftWeapons:`](Ruleset-CraftWeapons.md) — what the player shoots back with
- [`alienMissions:`](Ruleset-AlienMissions.md) / [`ufoTrajectories:`](Ruleset-UfoTrajectories.md) — when and where UFOs spawn and fly
- [`alienDeployments:`](Ruleset-AlienDeployments.md) — crew rosters for crash/landing assaults
