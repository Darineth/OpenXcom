# xcomtd (X-COM: Terror Defense) — Craft, Facilities & Starting Base

Reference summary of what the legacy `xcomtd` master mod (built on `xcom1` in the legacy
OpenXcomDX fork) **added or changed versus vanilla X-COM: UFO Defense** in the craft, base
facility, and starting-base domains. Sources: `bin/standard/xcomtd/Ruleset/Crafts.rul`,
`Facilities.rul`, `Overhaul.rul` (`startingBase:` + `ufopaedia:`),
`CraftWeapons.rul_NewAirCombat` (disabled reference data), `Language/en-US.yml`, and the
fork's `AirCombat-Design.md`; vanilla baselines cross-checked against the repo's own
`bin/standard/xcom1` rulesets.

## Craft

Six craft: the five vanilla craft plus one new interceptor, the **Tornado**. The headline
rebalance: the Firestorm becomes an extremely fast pure fighter, while the Lightning and
Avenger are made much *slower* than vanilla and re-focused as transports; all built craft
gain sell values.

| Craft (`STR_`) | Research gate | Speed | Accel | Fuel | Damage cap | Weapons | Soldiers | HWPs | Refuel | Notes |
|---|---|---|---|---|---|---|---|---|---|---|
| Skyranger (`STR_SKYRANGER`) | — | 760 | 2 | 1500 | 150 | 0 | 14 | 3 | rate 50 | Identical to vanilla; adds `autoPatrol: true` |
| Interceptor (`STR_INTERCEPTOR`) | — | 2100 | 3 | 1000 | 100 | 2 | — | — | rate 50 | Identical to vanilla |
| **Tornado** (`STR_TORNADO`) | `STR_IMPROVED_INTERCEPTOR` | 3500 | 6 | 2200 | 160 | 2 | — | — | rate 100, **no Elerium** | **New craft.** Slots between Interceptor and Firestorm; sell $2.54M |
| Firestorm (`STR_FIRESTORM`) | `STR_NEW_FIGHTER_CRAFT` | **11200** (was 4200) | **25** (was 9) | 20 | **800** (was 500) | 2 | — | — | Elerium, rate 20 (was 5) | "Vastly faster" pure fighter; sell $1.12M |
| Lightning (`STR_LIGHTNING`) | `STR_NEW_FIGHTER_TRANSPORTER` | **1400** (was 3100) | **3** (was 9) | **100** (was 30) | 650 (was 800) | 1 | **20** (was 12) | **4** (was 0) | Elerium, rate 20 | Recast as "tier-2 transport half-way between Skyranger and Avenger"; sell $1.36M; custom 30-tile deployment map added |
| Avenger (`STR_AVENGER`) | `STR_ULTIMATE_CRAFT` | **2400** (was 5400) | **5** (was 10) | **200** (was 60) | **2000** (was 1200) | 2 | 26 | 4 | Elerium, rate 10 | Much slower, much tougher; sell $2.14M; `spacecraft: true` (Cydonia-capable, as vanilla) |

Craft manufacturing (from `Manufacture.rul`): Tornado — $800k, 12,000 eng-hrs, 55 Alien
Alloys (no UFO components, no Elerium); Firestorm — $400k, 14,000 hrs, 65 Alloys + 1 Power
Source + 1 Navigation; Lightning — $600k, 18,000 hrs, 85 Alloys + 1 PS + 1 Nav; Avenger —
$900k, 34,000 hrs, 120 Alloys + 2 PS + 1 Nav.

### Custom / legacy-engine craft keys

- `combatSprite` — frame index into the `AirCombatSprites` surface set, used by the fork's
  custom **turn-based air-combat minigame** (see below). Present on Skyranger (1),
  Interceptor (2), Tornado (3).
- `autoPatrol: true` (Skyranger) — OXCE-Plus auto-patrol support.
- `pilots:` entries exist but are **commented out** on every craft (an abandoned pilots
  experiment).
- Everything else (`costSell`, `spacecraft`, `deployment`, `battlescapeTerrainData`) is
  standard OpenXcom/OXCE.

## Craft weapons

The **loaded** rulesets contain no `craftWeapons:` root — in the shipped mod, craft weapons
are vanilla (the Plasma Beam is merely renamed "Plasma Cannon" via strings:
`STR_PLASMA_BEAM: "Plasma Cannon"`). The file `CraftWeapons.rul_NewAirCombat` is
**disabled reference data** (renamed so it does not load) for the fork's turn-based
air-combat minigame documented in `AirCombat-Design.md` — a 1-D range-axis pursuit duel
replacing the real-time dogfight, off by default. Its redefined weapons use minigame-scale
stats and several legacy-engine keys:

| Weapon | Damage | Range (positions) | Acc | `tuAimed` | `shotsAimed` | Ammo |
|---|---|---|---|---|---|---|
| `STR_STINGRAY` | 70 | 4 | 70 | 30 | 1 | 6 |
| `STR_AVALANCHE` | 100 | 6 | 80 | 50 | 1 | 3 |
| `STR_CANNON_UC` | 10 | 2 | 50 | 10 | 5 | 200 |
| `STR_FUSION_BALL_UC` | 230 | 6 | 100 | 50 | 1 | 2 |
| `STR_LASER_CANNON_UC` | 70 | 2 | 35 | 10 | 3 | 100 |
| `STR_PLASMA_BEAM_UC` | 140 | 4 | 50 | 25 | 1 | 100 |
| `STR_UFO_SMALL_SCOUT_PLASMA` (new, UFO-mounted) | 10 | 2 | 60 | 20 | 5 | 1000 |
| `STR_UFO_MEDIUM_SCOUT_PLASMA` (new, UFO-mounted) | 20 | 3 | 60 | 25 | 2 | 1000 |

Legacy-engine keys (by inference from `AirCombat-Design.md`): `tuAimed` = Time cost to fire
in the initiative queue; `shotsAimed` = projectiles per burst; `range` = positions on the
range axis (not km); `projectileType`/`projectileSpeed` = minigame projectile visuals. The
two `STR_UFO_*_PLASMA` entries arm UFOs with real craft-weapon definitions (a fork feature —
`weapons:` list on `RuleUfo`).

## Base facilities

`Facilities.rul` is essentially a copy of the vanilla xcom1 file — **all build costs, build
times, sizes, capacities, radar ranges and defense values are unchanged** (Small Radar
1695/10%, Large Radar 2577/20%, Hyper-wave Decoder 2759/100%, Missile Def 500/50%, Laser Def
600/60%, Plasma Def 900/70%, Fusion Def 1200/80%, etc.). The only substantive changes are
the **research gates on the defense facilities**, rewired to the mod's renamed weapon
chains:

| Facility | Vanilla gate | xcomtd gate |
|---|---|---|
| Laser Defenses (`STR_LASER_DEFENSES`) | `STR_LASER_DEFENSE` | `STR_LASER_CANNONS` ("Laser Cannons") |
| Plasma Defenses (`STR_PLASMA_DEFENSES`) | `STR_PLASMA_DEFENSE` | `STR_PLASMA_CANNONS` ("Plasma Cannons") |
| Fusion Ball Defenses (`STR_FUSION_BALL_DEFENSES`) | `STR_FUSION_DEFENSE` | `STR_FUSION_WARHEADS` ("Fusion Warheads") |

(The standalone Laser/Plasma/Fusion Defense research topics were deleted — the base-defense
guns now fall out of the corresponding weapon-tier research automatically.) Grav Shield,
Mind Shield, Psionic Laboratory, Hyper-wave Decoder gates are unchanged. No new facilities
were added.

## Starting base

`Overhaul.rul` `startingBase:` keeps the **vanilla layout exactly** (Access Lift at 2,2;
Hangars at 2,0 / 0,4 / 4,4; Living Quarters 3,2; General Stores 2,3; Laboratory 3,3;
Workshop 4,3; Small Radar 1,3) and vanilla staffing (8 soldiers via `randomSoldiers`,
10 scientists, 10 engineers). The differences are in the equipment, swapped to the mod's
new ballistic weapon lineup:

- **Skyranger-1** starts loaded with 8 Grenades, 3 Pistols (`STR_BASIC_PISTOL`) + 5 clips,
  6 Assault Rifles (`STR_BASIC_RIFLE`) + 12 clips, and 1 Machine Gun
  (`STR_BASIC_MACHINEGUN`) + 2 clips — replacing vanilla's Pistols/Rifles/Heavy Cannon.
- **Interceptor-1 / -2**: Stingray + Cannon each (as vanilla).
- **Base stores**: Avalanche launcher + 10 missiles, 2 Cannons + rounds, Stingray launcher +
  25 missiles, 5 Grenades, 5 Smoke Grenades, 2 Pistols + 8 clips, 2 Assault Rifles + 8
  clips, 1 Machine Gun + 6 clips, 1 Rocket Launcher + 4 Small ("HEAT") Rockets — the
  vanilla Auto-Cannon/Heavy Cannon stock is gone.

## UFOpaedia (cross-check)

The `ufopaedia:` sections (Overhaul.rul + UFOPaedia.rul) delete the articles for everything
the mod removed (vanilla Pistol/Rifle/Heavy Cannon/Auto-Cannon + ammo, Heavy Laser, Heavy
Plasma, all four vanilla tank variants + HWP ammo) and add articles for the new content:
the full ballistic line (`STR_BASIC_*` incl. SMG, Shotgun, Marksman, Sniper, Minigun, Heavy
Rifle, Grenade Launcher), the 9-weapon laser and plasma lines, Arc Rifle/Launcher,
Electrolaser, fission/fusion/plasma rockets and explosives, Shock Grenade, Field Surgery
Unit, Psi Orb, the six named armor suits, and the Tornado craft. Many article texts are
literal `-PLACEHOLDER-` strings — the mod was work-in-progress.
