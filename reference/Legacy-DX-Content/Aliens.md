# Legacy xcomtd — Aliens

Reference summary of what the legacy **xcomtd** ("X-COM: Terror Defense") master mod added or
changed versus vanilla X-COM: UFO Defense for the alien side. Sources:
`OpenXcomDX-Legacy/bin/standard/xcomtd/Ruleset/Aliens.rul` (deployments, missions,
`alienItemLevels`, unit stats), `Ruleset/AlienArmor.rul` (alien inventory paperdolls),
`Ruleset/Armors.rul` (alien armor values, lines 304+), and `Language/en-US.yml` /
`Overhaul.rul` extraStrings (display names). The mod does **not** redefine `alienRaces:`
membership lists — race compositions (which terror units accompany which race) are inherited
from the fork's base xcom1 rules, i.e. vanilla.

## Overview of changes

- **Loadouts completely rebuilt** around the mod's expanded 9-weapon alien plasma family
  (Pistol, Blaster Pistol/SMG, Rifle, Burst Rifle/shotgun, Shock Rifle/marksman, Beam
  Rifle/sniper, Blaster/machine gun, Pulsar/minigun, Heavy Plasma Rifle) instead of vanilla's
  three plasma weapons. Each deployment slot has three item-set tiers that upgrade over the
  campaign.
- **Unit stats are mostly vanilla-like**; the visible edits are small (e.g. Sectoid strength
  40, bravery 85) plus stat-graded Floater/Snakeman leaders and commanders.
- **Armor values redefined** for every alien in `Armors.rul`, with rank-tiered armor
  (`*_ARMOR0/1/2`) and a reworked resistance table per race. The Sectopod's armor is cut
  drastically (90/80/80/60 vs vanilla ~145/130/130/90).
- **Deployments add terrorist ranks (6/7) to Terror Ships and Battleships**, blaster
  launchers only on high ranks at the top item tier, and new deployment types
  (`STR_PORT_ATTACK`) plus a scripted two-stage Cydonia with a `BOSSBATTLE` map script and a
  `STR_MIXED` ("Mixed") race final stage.
- **Alien inventory paperdolls**: `AlienArmor.rul` assigns a `spriteInv` full-screen image to
  every alien/civilian armor (Resources/AlienInventory/*.png) so aliens can be viewed in the
  inventory screen — a legacy-engine (early OXCE-era) feature.

## Unit stats by race

Stats are identical across ranks of a race unless noted. `value` (score) varies by rank
(soldier lowest, commander highest) — not tabulated here.

| Race | TU | Sta | HP | Bra | Rea | Fir | Thr | Str | PsiStr | PsiSkill | Melee | Notes |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Sectoid (all 6 ranks) | 54 | 90 | 30 | 85 | 63 | 52 | 58 | 40 | 40 | 0 (Leader 50, Cmdr 60) | 76 | Strength 40 (vanilla 30); psi attackers are Leader/Commander as in vanilla |
| Cyberdisc | 62 | 90 | 120 | 110 | 64 | 60 | — | 90 | 100 | 0 | 0 | `specab: 1` (explodes on death), livingWeapon |
| Floater Sol/Eng/Nav/Med | 50 | 90 | 35 | 85 | 50 | 50 | 58 | 40 | 35 | 0 | 70 | |
| Floater Leader | 55 | 95 | 40 | 85 | 60 | 60 | 58 | 47 | 40 | 0 | 70 | Stat-boosted + `FLOATER_ARMOR1` |
| Floater Commander | 60 | 100 | 45 | 85 | 66 | 63 | 65 | 48 | 45 | 0 | 76 | `FLOATER_ARMOR2` |
| Reaper | 62 | 90 | 148 | 90 | 64 | 0 | — | 90 | 35 | 0 | 80 | livingWeapon |
| Snakeman Sol/Nav/Eng | 40 | 80 | 45 | 85 | 45 | 58 | 65 | 47 | 40 | 0 | 76 | |
| Snakeman Leader | 40 | 80 | 45 | 85 | 55 | 65 | 65 | 47 | 45 | 0 | 76 | `SNAKEMAN_ARMOR1` |
| Snakeman Commander | 45 | 84 | 55 | 85 | 65 | 58 | 65 | 47 | 50 | 0 | 76 | `SNAKEMAN_ARMOR2` |
| Chryssalid | 110 | 140 | 96 | 100 | 70 | 0 | — | 110 | 50 | 0 | 80 | livingWeapon |
| Muton Sol/Nav/Eng | 56 | 90 | 125 | 85 | 60 | 54 | 62 | 70 | 25 | 0 | 76 | Armor tier rises by rank (ARMOR0/1/2) |
| Silacoid | 40 | 80 | 114 | 100 | 40 | 0 | — | 70 | 80 | 0 | 80 | `specab: 2` (burns floor), livingWeapon |
| Celatid | 70 | 90 | 68 | 90 | 40 | 100 | — | 70 | 80 | 0 | 80 | livingWeapon |
| Ethereal Soldier | 68 | 96 | 55 | 85 | 75 | 74 | 80 | 48 | 50 | 40 | 85 | |
| Ethereal Leader | 68 | 96 | 55 | 85 | 75 | 74 | 80 | 48 | 60 | 45 | 85 | `ETHEREAL_ARMOR1` |
| Ethereal Commander | 68 | 96 | 55 | 85 | 75 | 74 | 80 | 48 | 65 | 50 | 85 | `ETHEREAL_ARMOR2` |
| Sectopod | 62 | 90 | 96 | 110 | 64 | 60 | — | 90 | 100 | 0 | 80 | livingWeapon |
| Zombie | 40 | 110 | 84 | 110 | 40 | 0 | — | 84 | 80 | 0 | 80 | `spawnUnit: STR_CHRYSSALID_TERRORIST` |
| Civilians (M/F) | 35 | 65 | 30 | 85 | 30 | 30 | 50 | 20 | 5 | 0 | 50 | |

X-COM HWP crew units (tanks/hovertanks) are also declared in `units:` here (TU 70/100, HP 90,
Fir 60, Str 60, `TANK_ARMOR`/`HOVERTANK_ARMOR`) — see the Vehicles/HWPs reference doc.

## Armor & resistances (`Armors.rul`)

Damage-modifier index order: AP, IN(fire), HE, Laser, Plasma, Stun, Melee, Acid, Smoke
(unlisted = ×1.0). Most aliens are smoke-immune (×0); Sectoids, Celatids and civilians are
not.

| Armor | Front/Side/Rear/Under | Resist (< 1.0) | Vulnerable (> 1.0) |
|---|---|---|---|
| `SECTOID_ARMOR0` (all ranks) | 4 / 3 / 2 / 2 | — | Stun 1.2, Melee 1.2, Acid 1.6 |
| `FLOATER_ARMOR0` | 8 / 6 / 4 / 12 | — | Stun 1.2, Melee 1.2, Acid 1.6 |
| `FLOATER_ARMOR1` (Leader) | 16 / 12 / 8 / 12 | — | same |
| `FLOATER_ARMOR2` (Cmdr) | 24 / 18 / 12 / 16 | — | same |
| `CYBERDISC_ARMOR` | 40 all sides | **HE 0.6**, Smoke 0 | — |
| `REAPER_ARMOR` | 35 / 35 / 35 / 10 | HE 0.8 | **Fire 1.7** |
| `SNAKEMAN_ARMOR0` | 20 / 18 / 16 / 12 | Fire 0.7 | — |
| `SNAKEMAN_ARMOR1` | 20 / 24 / 22 / 20 | Fire 0.7 | — |
| `SNAKEMAN_ARMOR2` | 26 / 26 / 22 / 20 | Fire 0.7 | — |
| `CHRYSSALID_ARMOR` | 34 all sides | Fire 0.8, Stun 0.9 | — |
| `MUTON_ARMOR0/1/2` | 20/20/20/10 → 24/24/24/15 → 28/28/28/20 | **AP 0.6** | — |
| `SILACOID_ARMOR` | 50 / 50 / 50 / 10 | Fire immune (0) | HE 1.3 |
| `CELATID_ARMOR` | 20 all sides | — | Stun 1.2, Melee 1.2, Acid 1.6 |
| `ETHEREAL_ARMOR0/1/2` | 35 → 40 → 45 all sides | Fire 0.7, Stun 0.8 | — |
| `SECTOPOD_ARMOR` | 90 / 80 / 80 / 60 | HE 0.5, Plasma 0.8, Melee 0.5 | Laser 1.2 |
| `ZOMBIE_ARMOR` | 4 all sides | AP 0.6, HE 0.8, Laser 0.7, Plasma 0.7, Stun immune (0) | — |
| `CIVM/CIVF_ARMOR` | 0 | — | Melee 1.2, Acid 1.6 |

Notable vs vanilla: the Sectopod's armor is roughly **cut in half** (vanilla ~145/130/130/90)
and its laser weakness is softened (1.2 vs vanilla 1.5); the Cyberdisc is *HE-resistant*
here; rank-tiered `*_ARMOR0/1/2` values are rebalanced across the board.

## Weapon loadout patterns (deployments)

`alienRank` in deployment data maps to: 0 = Commander, 1 = Leader, 2 = Engineer, 3 = Medic,
4 = Navigator, 5 = Soldier, 6/7 = Terrorist slots. Every slot's `itemSets` has **three
tiers** (chosen per-month via `alienItemLevels`); the pattern is consistent across all
deployments:

- **Soldiers (rank 5)** are split into several parallel squad entries, each a weapon-family
  "kit" that upgrades by tier:
  - *Line kit*: Plasma Pistol → Plasma Rifle → Plasma Rifle + spare ammo + 2 Alien Grenades.
  - *Close-quarters kit*: Plasma SMG → Plasma Burst Rifle (shotgun) → shotgun + grenades.
  - *Support kit*: Plasma Rifle → Plasma Blaster (MG) → Blaster + extra ammo.
  - *Precision kit*: Plasma Shock Rifle (marksman) → Plasma Beam Rifle (sniper) → Heavy
    Plasma Rifle (larger UFOs only).
  - *Inside-UFO guard squads* (`percentageOutsideUfo: 0`): shotgun → Plasma Pulsar (minigun)
    → Pulsar / Heavy Plasma.
- **Navigators (4)**: light sidearm kits (Pistol/SMG/shotgun/Rifle).
- **Medics (3)**: Small Launcher + 2–4 Stun Bombs + Mind Probe, on every UFO that carries
  medics.
- **Engineers (2)**: Pistol/shotgun/Rifle; on Battleships and base missions their tier-2 set
  becomes a **Blaster Launcher** + 4 Blaster Bombs.
- **Leaders (1)**: shotgun/minigun/heavy kits; Blaster Launcher at tier 2 on Battleships,
  base assault/defense and Cydonia.
- **Commanders (0)**: Plasma MG at tier 0, **Blaster Launcher at tiers 1–2** (Battleship,
  alien base, base defense, Cydonia only).
- **Terrorists (6/7)**: empty item sets (living weapons). Newly present on **Terror Ships
  and Battleships** (2–3 + 1–2, 50% outside), base defense, and Cydonia — not just terror
  sites.
- Mind Probes: carried by the Small Scout pilot and by rank-2 crew on Large Scouts, in
  addition to medics.
- The Abductor carries a dedicated **abduction team**: 4–6 rank-5 soldiers armed with Small
  Launchers and Stun Bombs (80% outside).

### Deployment roster changes (crew shape)

| Deployment | Crew pattern (base counts; `dQty` adds up to that many more on higher difficulty) |
|---|---|
| Small Scout | 1 soldier (pistol/SMG/rifle + Mind Probe), 80% outside |
| Medium Scout | 2 soldier kits (1–2 each) + 1 navigator |
| Large Scout | 2 outdoor soldier kits (2–3 each) + indoor minigun squad + navigator + engineer (w/ Mind Probe) |
| Harvester | 6 soldier kits (incl. MG + precision + indoor guards) + navigator + medic + engineer + leader |
| Abductor | 2 soldier kits + stun-launcher team (4–6) + indoor guards + navigator + medic + engineer + leader |
| Terror Ship | 5 outdoor + 2 indoor soldier kits + navigator + medic + engineer + leader + terrorists (ranks 6: 2–3, 7: 1–2) |
| Supply Ship | 5 soldier kits + indoor guards + navigator + medic + engineer + leader (no commander) |
| Battleship | Large multi-kit crew, all ranks 0–7; blasters on engineer/leader/commander tier 2; terrorists 0–1 each |
| Terror site / Port Attack | Terror-Ship-like ground force + both terrorist ranks; Port Attack is a Terrain-Pack-compatibility clone of the terror deployment |
| Alien Base assault | Big garrison (rank-5 entry of 5–7 indoor troops) + all ranks incl. commander w/ blasters; `UBASE` terrain, `ALIENBASE` script, shade 15 |
| Base Defense | 60×60 `XBASE`, large crew incl. both terrorist ranks and blaster-armed ranks 0–2 |
| Cydonia landing | Forced `race: STR_SECTOID`, mostly 100%-outside grenade-heavy kits, terrorists, `MARS` terrain, `noRetreat`, leads to final stage |
| Cydonia interior (`STR_MARS_THE_FINAL_ASSAULT`) | `race: STR_MIXED` ("Mixed"), `script: BOSSBATTLE`, `finalMission`; rank-0 squads (2–3 each) with rifle/heavy/sniper/MG kits + grenades, leaders/engineers with Blaster Launchers, plus 3–5 each of ranks 3–6 with empty sets |

Terror sites keep 16 civilians and deliberately set no terrain (comment: compatibility with
terrain mods); duration 4–10.

## Item tier progression (`alienItemLevels`)

20 monthly rows of 10 weighted rolls over tiers 0/1/2. Progression is **much slower than
vanilla** (which saturates at all-tier-2 around month 8): here tier 1 first appears in month
1, tier 2 becomes majority around months 7–9, and the table only reaches all-tier-2 at row
19 (month 19+, then repeats).

## Mission race weights (`alienMissions`)

Race weight tables are keyed by month; even months carry the real weights for most missions
(odd-month entries are defined but all-zero — apparently an authoring artifact of the legacy
format ?). Patterns:

- `STR_ALIEN_RESEARCH` — Sectoid-heavy early (70/20/10 S/F/Sn), Mutons join from month 4
  (~30%), never Ethereal.
- `STR_ALIEN_HARVEST` — Sectoid/Floater, Snakeman only month 0, Muton from month 4; never
  Ethereal. Waves end with a Battleship escort.
- `STR_ALIEN_ABDUCTION` — Sectoid/Floater only, all game.
- `STR_ALIEN_INFILTRATION` — Sectoid/Floater/Snakeman early; Muton spike (50%) at month 7;
  Ethereal 40% from month 10. Final waves: Terror Ship + Supply Ship + 2 Battleships.
- `STR_ALIEN_BASE` — mixed; Snakeman/Muton mid-game; Ethereal 50% by month 10; month 11+
  fallback row gives every race a share.
- `STR_ALIEN_TERROR` — fully month-by-month: m0 Floater 100%, m1–2 Sectoid-led, m3–4
  Snakeman rise (80% at m4), m5–7 Snakeman/Muton, Ethereals appear m8 and peak 50% at m10.
- `STR_ALIEN_RETALIATION` — race from the trigger UFO; escalating scout waves then 2
  Battleships (`spawnUfo: STR_BATTLESHIP` for the final run).
- `STR_ALIEN_SUPPLY` — race from the base; single Supply Ship wave.

## Misc

- `AlienArmor.rul` adds `spriteInv` inventory-screen images for every alien/civilian armor
  (a legacy-engine alien-inventory feature; images under `Resources/AlienInventory/`).
- Deployment `briefing:` blocks (palette, music, background, `showTarget`/`showCraft`,
  `textOffset`) are a legacy-engine extension used on terror, base, and Cydonia missions.
- Alien ranks used for research/interrogation strings (`rank: STR_LIVE_*`) match vanilla;
  Mutons still only have Soldier/Navigator/Engineer, Ethereals Soldier/Leader/Commander.
