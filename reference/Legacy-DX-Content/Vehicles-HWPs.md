# xcomtd Legacy Content: Vehicles / HWPs

This document summarizes the vehicle (HWP) system of the legacy "X-COM: Terror Defense" (xcomtd)
master mod, as defined in `D:\Code\Projects\OpenXcomDX-Legacy\bin\standard\xcomtd\Ruleset\Vehicles.rul`
(vehicle units, armors, engines, addons, armor plates, research, manufacturing),
the HWP item section of `...\Ruleset\Items.rul` (lines ~3322-4479: weapons, ammo, equipment),
the vehicle inventory slot definitions in `...\Ruleset\Inventories.rul`, and display names from
`...\Ruleset\Overhaul.rul` / `...\Language\en-US.yml`. Values below are reported as written in the
rulesets; several keys (`baseAccuracy`, `battleClipSize`, `battleType: 12/13`, `validSlots`,
`equippedEffect`, the `overwatch*`/burst keys) are custom features of the legacy engine fork and are
described by inference. "?" marks values the rulesets leave unclear.

## The system: modular vehicles instead of fixed tank products

Vanilla X-COM sells four monolithic HWP units (Tank/Cannon, Tank/Rocket Launcher, Tank/Laser
Cannon, Hovertank/Plasma + Hovertank/Launcher), each a single indivisible purchase with a fixed
weapon and fixed ammo. xcomtd replaces that entirely with a **chassis + loadout** system:

- **Vehicles are soldier-type units.** Each chassis is defined under the `soldiers:` root with a
  legacy-engine flag `isVehicle: true`. Its `minStats`/`maxStats`/`statCaps` are identical, so
  every unit of a chassis rolls the same fixed stats (no crew, no stat growth; `femaleFrequency: 0`).
  Chassis are bought like soldiers (`costBuy` on the soldier entry) or manufactured, and a global
  `costVehicle: 20000` in `Overhaul.rul` is evidently the monthly upkeep per vehicle (analogous to
  `costSoldier`).
- **Each chassis has its own inventory layout** (`inventoryLayout`, a legacy-engine key pointing at
  an `inventoryLayouts:` entry in `Inventories.rul`). Instead of hands/belt/backpack, a vehicle's
  "inventory" is a set of hardpoints:
  - **`STR_TURRET` ("TURRET 1") / `STR_TURRET_2` ("TURRET 2")** — weapon mounts. Only items with
    `vehicleItem: true` and `validSlots` including the turret slots can be mounted. Moving ammo
    from the ammo rack to a turret costs 0 TU (`costs: STR_AMMO_RACK: 0`), i.e. in-battle reloads
    from the rack are free.
  - **`STR_AMMO_RACK` ("AMMO")** — a 2x3 (6-cell) grid for ammunition (`battleType: 2`).
  - **`STR_ENGINE_LIGHT` / `STR_ENGINE_MEDIUM` / `STR_ENGINE_HEAVY` ("ENGINE")** — the engine bay,
    an irregular grid (13 / 18 / 20 cells respectively) that holds one engine item (2x3, 3x3, or
    4x3 footprint) plus 1x1 addon modules and vehicle equipment (`battleType: 13`, a custom
    "vehicle module" battle type). The bay is `countStats: true` — items here contribute their
    `stats`/`statModifiers` to the unit.
  - **Per-facing armor slots** (`STR_ARMOR_MEDIUM_FRONT/LEFT/RIGHT/REAR/UNDER`, and `_HEAVY_`
    variants) — hold stackable armor plates (`battleType: 12`, custom "armor plate" type, with an
    `armorSide` index 0-4 = front/left/right/rear/under). Medium-class chassis have 2 plate slots
    per facing, heavy chassis 3 per facing; the Scout Car layout has **no** armor slots.
    (A set of `STR_ARMOR_LIGHT_*` 1-slot facings and `STR_VHC_MISC_*` slots is defined in
    `Inventories.rul` but appears in no chassis layout.)
- **Engines drive mobility and carry weight.** Every engine grants `stats: tu: 100` plus a
  `strength` value; the chassis armor then applies a percentage/flat `statModifiers: tu:` bonus
  (Scout Car +20, Hover Tank +30, Medium/Heavy +0). Strength is the vehicle's carrying capacity —
  weapons, plates, and modules all have `weight`, so bigger guns and thicker plate require bigger
  (or supercharged/elerium) engines. Turbocharger addons trade health/reactions/firing for extra
  strength.
- **Weapons come from the item list**, tagged `vehicleItem: true`, with a `turretType` (which
  turret sprite the chassis displays: 0 = cannon, 1 = launcher, 2 = laser, 3 = plasma, 4 = smart
  launcher — matching vanilla turret sprite indices) and `validSlots: [STR_TURRET, STR_TURRET_2]`.
  Medium/Hover/Heavy chassis carry **two** turret weapons at once.
- **Custom item keys** (legacy-engine features): `baseAccuracy` (a weapon-inherent accuracy term,
  presumably combined with the chassis' fixed `firing` stat), `battleClipSize` (single-round
  shells stored/bought as `clipSize: 1` items consolidate into an N-round magazine in battle),
  `accuracyBurst`/`tuBurst`/`burstShots`/`autoDelay` (the mod's burst-fire system),
  `reactionsModifier` and the `overwatchModifier`/`overwatchRange`/`overwatchRadius`/
  `overwatchShot` block (the mod's overwatch fire system), `shotgunPellets`/`shotgunSpread`
  (cluster munitions), `equippedEffect` (grants a named effect from `Effects.rul` while equipped),
  and `stats`/`statModifiers` on non-weapon modules.

## Chassis

All chassis: 2x2 units (`size: 2`), stand/kneel height 16, `TANKS.PCK` sprites, four-part corpse,
damage modifiers 0.35x HE, 0.5x Acid, 0.4x Smoke(?), immune to Incendiary, Stun, and damage type 9.

| Chassis | `STR_` | Acquisition | Health | Reactions | Firing | Armor F/S/R/U | TU mod | Move | Weapon mounts | Engine bay | Armor plate slots | Chassis weight |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Scout Car | `STR_SCOUT_CAR` | Buy $150,000 | 40 | 70 | 40 | 13/9/6/6 | +20 | Ground | 1 turret | Light (13 cells) | none | 65 |
| Medium Tank | `STR_MEDIUM_TANK` | Buy $300,000 | 70 | 40 | 50 | 30/25/18/18 | +0 | Ground | 2 turrets | Medium (18 cells) | 2 per facing | 110 |
| Hover Tank | `STR_HOVER_TANK` | Manufacture $700,000 + 5 Alloys + 20 Elerium (research: Hover Tank ← Advanced Armor) | 70 | 80 | 50 | 30/30/30/30 | +30 | Flying | 2 turrets | Medium (18 cells) | 2 per facing | 80 |
| Heavy Tank | `STR_HEAVY_TANK` | Manufacture $600,000 + 20 Alloys + 10 Elerium (research: Heavy Tank ← Advanced Armor) | 150 | 40 | 55 | 35/27/20/32 | +0 | Ground | 2 turrets | Heavy (20 cells) | 3 per facing | 300 |

All chassis have TU 0, stamina 100, bravery 0(?), psiStrength 100, psiSkill 0 as base soldier
stats — mobility comes entirely from the installed engine (`tu: 100` from any engine, modified by
the chassis TU modifier).

## Engines (`battleType: 13`, engine bay)

All engines grant `tu: 100`; they differ in `strength` (carry capacity), their own weight, and
inventory footprint. S/M/L fit the Light and Medium bays; HX/SHX fit only the Heavy bay.
Progression: base (buyable S/M) → **Supercharged** (research: Supercharged HWP Engines) → **X**
(elerium; research: Elerium HWP Engines) → **Supercharged X**. Heavy engines require the Heavy
Tank research.

| Engine | `STR_` | Strength | Weight | Footprint | Acquisition |
|---|---|---|---|---|---|
| HWP S Engine | `STR_HWP_S_ENGINE` | 75 | 25 | 2x3 | Buy $50,000 |
| HWP M Engine | `STR_HWP_M_ENGINE` | 126 | 85 | 3x3 | Buy $75,000 |
| HWP L Engine | `STR_HWP_L_ENGINE` | 180 | 150 | 4x3 | Mfr $90,000 (Large HWP Engine) |
| HWP Supercharged S Engine | `STR_HWP_SUPERCHARGED_S_ENGINE` | 108 | 90 | 2x3 | Mfr $120,000 |
| HWP Supercharged M Engine | `STR_HWP_SUPERCHARGED_M_ENGINE` | 168 | 185 | 3x3 | Mfr $160,000 |
| HWP Supercharged L Engine | `STR_HWP_SUPERCHARGED_L_ENGINE` | 270 | 450 | 4x3 | Mfr $220,000 |
| HWP SX Engine | `STR_HWP_SX_ENGINE` | 90 | 25 | 2x3 | Mfr $135,000 + 3 Elerium |
| HWP MX Engine | `STR_HWP_MX_ENGINE` | 152 | 85 | 3x3 | Mfr $178,000 + 4 Elerium |
| HWP LX Engine | `STR_HWP_LX_ENGINE` | 216 | 150 | 4x3 | Mfr $245,000 + 5 Elerium |
| HWP Supercharged SX Engine | `STR_HWP_SUPERCHARGED_SX_ENGINE` | 130 | 90 | 2x3 | Mfr $280,000 + 6 Elerium |
| HWP Supercharged MX Engine | `STR_HWP_SUPERCHARGED_MX_ENGINE` | 200 | 185 | 3x3 | Mfr $345,000 + 8 Elerium |
| HWP Supercharged LX Engine | `STR_HWP_SUPERCHARGED_LX_ENGINE` | 324 | 450 | 4x3 | Mfr $480,000 + 10 Elerium |
| HWP HX Engine (heavy bay) | `STR_HWP_HX_ENGINE` | 310 | 200 | 3x3 | Mfr $315,000 + 6 Elerium (Heavy Tank) |
| HWP SHX Engine (heavy bay) | `STR_HWP_SHX_ENGINE` | 420 | 350 | 4x3 | Mfr $410,000 + 7 Elerium (Heavy Tank) |
| HWP Supercharged HX Engine (heavy bay) | `STR_HWP_SUPERCHARGED_HX_ENGINE` | 372 | 375 | 3x3 | Mfr $720,000 + 12 Elerium (Heavy Tank) |
| HWP Supercharged SHX Engine (heavy bay) | `STR_HWP_SUPERCHARGED_SHX_ENGINE` | 540 | 700 | 4x3 | Mfr $910,000 + 14 Elerium (Heavy Tank) |

Note the elerium-supercharged trade-off: X engines match Supercharged strength at a fraction of
the weight (e.g. LX 216 str @ 150 wt vs Supercharged L 270 str @ 450 wt).

## Addon modules (`battleType: 13`, 1x1, engine bay)

Flat `stats` add to the unit; `statModifiers` values also modify stats (evidently a separate,
possibly percentage-based, modifier channel — legacy-engine mechanic).

| Module | `STR_` | Effect | Weight | Research |
|---|---|---|---|---|
| HWP Light Reinforcement | `STR_HWP_LIGHT_REINFORCEMENT` | +10 health | 5 | HWP Frame Reinforcement |
| HWP Heavy Reinforcement | `STR_HWP_HEAVY_REINFORCEMENT` | +20 health | 35 | HWP Frame Reinforcement |
| HWP Light Alloy Reinforcement | `STR_HWP_LIGHT_ALLOY_REINFORCEMENT` | +13 health | 5 | HWP Frame Alloy Reinforcement |
| HWP Heavy Alloy Reinforcement | `STR_HWP_HEAVY_ALLOY_REINFORCEMENT` | +26 health | 35 | HWP Frame Alloy Reinforcement |
| HWP Targeting Module | `STR_HWP_TARGETING_MODULE` | +15 firing (modifier) | 30 | HWP Targeting Improvement |
| HWP Hyperwave Targeting Module | `STR_HWP_HYPERWAVE_TARGETING_MODULE` | +19 firing (modifier) | 30 | Hyperwave HWP Targeting |
| HWP Turbocharger | `STR_HWP_TURBOCHARGER` | +34 strength, -12 health, -8 reactions, -1 firing (modifiers) | 6 | HWP Turbocharging |
| HWP Elerium Turbocharger | `STR_HWP_ELERIUM_TURBOCHARGER` | +40 strength, -12 health, -8 reactions, -1 firing (modifiers) | 6 | HWP Elerium Turbocharging |
| HWP Response Booster | `STR_HWP_RESPONSE_BOOSTER` | +20 reactions, -5 firing (modifiers) | 12 | HWP Response Enhancement |
| HWP Hyperwave Response Booster | `STR_HWP_HYPERWAVE_RESPONSE_BOOSTER` | +25 reactions, -5 firing (modifiers) | 12 | Hyperwave HWP Response Booster |

## Armor plates (`battleType: 12`, per-facing slots)

Each plate adds its armor value to whichever facing slot it occupies (front/left/right/rear/under);
listed `frontArmor`/`sideArmor`/`rearArmor`/`underArmor` are identical per plate. Plates stack
(2 per facing on Medium/Hover, 3 on Heavy; Scout Car cannot mount plates).

| Plate | `STR_` | Armor per plate | Weight | Acquisition |
|---|---|---|---|---|
| HWP Light Armor Plate | `STR_HWP_LIGHT_ARMOR_PLATE` | +8 | 10 | Buy $15,000 |
| HWP Heavy Armor Plate | `STR_HWP_HEAVY_ARMOR_PLATE` | +16 | 30 | Mfr $21,500 (Heavy HWP Armor) |
| HWP Light Alloy Armor Plate | `STR_HWP_LIGHT_ALLOY_ARMOR_PLATE` | +9 | 9 | Mfr $16,000 + 1 Alloy (Alien Alloy HWP Armor) |
| HWP Heavy Alloy Armor Plate | `STR_HWP_HEAVY_ALLOY_ARMOR_PLATE` | +18 | 27 | Mfr $32,000 + 2 Alloys (Alien Alloy HWP Armor) |

## Turret weapons — direct-fire classes (3 tiers x 4 classes)

Four weapon classes, each in Basic (ballistic), Laser, and Plasma tiers — directly mirroring the
mod's infantry weapon classes. All mount in `STR_TURRET`/`STR_TURRET_2`. Damage types:
1 = AP, 2 = Incendiary, 3 = HE, 4 = Laser, 5 = Plasma, 9 = Smoke. Laser-tier weapons need no ammo
(`clipSize: -1`); Basic tier is buyable; Plasma tier is manufactured with alloys + elerium.

### Machine Gun class (snap/aimed + 3-round burst + 8-round auto, overwatch radius 3)

| Weapon | `STR_` | Tier | Power (type) | Clip | Base acc | Snap | Aimed | Burst (3) | Auto (8) | Weight | Turret sprite |
|---|---|---|---|---|---|---|---|---|---|---|---|
| HWP Machine Gun | `STR_HWP_BASIC_MACHINEGUN` | Basic (buy $9,000) | 35 (AP) | 40 | 35 | 80% / 35 TU | 140% / 65 TU | 60% / 55 TU | 60% / 80 TU | 16 (+6 ammo) | 0 (cannon) |
| HWP Pulse Laser | `STR_HWP_LASER_MACHINEGUN` | Laser (mfr $85,000, Heavy Laser Weapons) | 47 (Laser) | ∞ | 39 | 80% / 39 TU | 140% / 72 TU | 60% / 61 TU | 60% / 88 TU | 28 | 2 (laser) |
| HWP Plasma Blaster | `STR_HWP_PLASMA_MACHINEGUN` | Plasma (mfr $175,000 + 8 Alloys + 3 Elerium, Heavy Plasma Weapons) | 58 (Plasma) | 40 | 35 | 72% / 32 TU | 126% / 59 TU | 54% / 50 TU | 54% / 73 TU | 17 (+6 ammo) | 3 (plasma) |

### Minigun class (burst/auto only: 5-round burst, 20-round auto, overwatch radius 2)

| Weapon | `STR_` | Tier | Power (type) | Clip | Base acc | Burst (5) | Auto (20) | Weight | Turret sprite |
|---|---|---|---|---|---|---|---|---|---|
| HWP Minigun | `STR_HWP_BASIC_MINIGUN` | Basic (buy $16,000) | 22 (AP) | 100 | 6 | 200% / 20 TU | 200% / 60 TU | 30 (+7 ammo) | 0 |
| HWP Hyper Pulse Laser | `STR_HWP_LASER_MINIGUN` | Laser (mfr $88,000, Heavy Laser Weapons) | 29 (Laser) | ∞ | 7 | 200% / 22 TU | 200% / 66 TU | 38 | 2 |
| HWP Plasma Pulsar | `STR_HWP_PLASMA_MINIGUN` | Plasma (mfr $189,000 + 9 Alloys + 3 Elerium, Heavy Plasma Weapons) | 36 (Plasma) | 100 | 6 | 200% / 18 TU | 200% / 54 TU | 35 (+7 ammo) | 3 |

(The very low `baseAccuracy` with 200% mode accuracy suggests miniguns are inherently inaccurate
sprayers whose per-shot accuracy comes almost entirely from volume of fire.)

### Light Cannon class (snap/aimed single shots, high base accuracy, overwatch range 25)

| Weapon | `STR_` | Tier | Power (type) | Clip (battle) | Base acc | Snap | Aimed | Weight | Turret sprite |
|---|---|---|---|---|---|---|---|---|---|
| HWP Light Cannon | `STR_HWP_BASIC_LIGHT_CANNON` | Basic (buy $9,000) | 85 (AP) | 1 (6) | 110 | 60% / 55 TU | 130% / 80 TU | 30 (+2/shell) | 0 |
| HWP Light Laser Cannon | `STR_HWP_LASER_LIGHT_CANNON` | Laser (mfr $91,000, Laser Cannons) | 113 (Laser) | ∞ | 121 | 60% / 60 TU | 130% / 85 TU | 46 | 2 |
| HWP Light Plasma Cannon | `STR_HWP_PLASMA_LIGHT_CANNON` | Plasma (mfr $180,000 + 7 Alloys + 3 Elerium, Plasma Cannons; ammo produced 2 per run) | 142 (Plasma) | 1 (6) | 110 | 54% / 52 TU | 117% / 75 TU | 35 (+2/shell) | 3 |

### Heavy Cannon class (explosive shells — all three tiers deal HE with a blast radius)

| Weapon | `STR_` | Tier | Power (type) | Blast | Clip (battle) | Base acc | Snap | Aimed | Weight | Turret sprite |
|---|---|---|---|---|---|---|---|---|---|---|
| HWP Heavy Cannon | `STR_HWP_BASIC_HEAVY_CANNON` | Basic (mfr $15,000, Heavy HWP Cannon research) | 105 (HE) | r1 | 1 (5) | 50 | 50% / 75 TU | 90% / 90 TU | 150 (+5/shell) | 0 |
| HWP Heavy Laser Cannon | `STR_HWP_LASER_HEAVY_CANNON` | Laser (mfr $120,000, Laser Cannons) | 140 (HE) | r1 | ∞ | 55 | 50% / 80 TU | 90% / 93 TU | 230 | 2 |
| HWP Heavy Plasma Cannon | `STR_HWP_PLASMA_HEAVY_CANNON` | Plasma (mfr $370,000 + 20 Alloys + 6 Elerium, Plasma Cannons) | 175 (HE) | r2 | 1 (5) | 50 | 45% / 70 TU | 81% / 85 TU | 170 (+5/shell) | 3 |

### Effective ranges (tiles)

Computed with the legacy engine's aim-cone formula (see the "Effective range" section in
[Weapons.md](Weapons.md): effective range = distance at which hit chance is 50%; `baseAccuracy`
is the hard cap in tiles regardless of firing skill). Columns are at firing stats
**40 / 60 / 80** — chassis stock firing is 40 (Scout Car), 50 (Medium/Hover Tank), 55 (Heavy
Tank), raised further by targeting modules. "—" = mode not available.

| Weapon | baseAcc (cap) | Snap | Aimed | Burst | Auto |
|---|---|---|---|---|---|
| HWP Machine Gun | 35 | 8 / 18 / 27 | 22 / 32 / 35 | 5 / 11 / 18 | 5 / 11 / 18 |
| HWP Pulse Laser | 39 | 9 / 18 / 28 | 23 / 35 / 38 | 5 / 11 / 18 | 5 / 11 / 18 |
| HWP Plasma Blaster | 35 | 7 / 15 / 23 | 19 / 31 / 34 | 4 / 9 / 15 | 4 / 9 / 15 |
| HWP Minigun | 6 | — | — | 6 / 6 / 6 | 6 / 6 / 6 |
| HWP Hyper Pulse Laser | 7 | — | — | 7 / 7 / 7 | 7 / 7 / 7 |
| HWP Plasma Pulsar | 6 | — | — | 6 / 6 / 6 | 6 / 6 / 6 |
| HWP Light Cannon | 110 | 5 / 11 / 19 | 23 / 48 / 75 | — | — |
| HWP Light Laser Cannon | 121 | 5 / 11 / 19 | 23 / 48 / 77 | — | — |
| HWP Light Plasma Cannon | 110 | 4 / 9 / 16 | 18 / 40 / 64 | — | — |
| HWP Heavy Cannon | 50 | 3 / 8 / 13 | 11 / 23 / 35 | — | — |
| HWP Heavy Laser Cannon | 55 | 3 / 8 / 13 | 11 / 23 / 36 | — | — |
| HWP Heavy Plasma Cannon | 50 | 3 / 6 / 11 | 9 / 19 / 30 | — | — |
| HWP Missile Launcher | 40 | — | 23 / 36 / 39 | — | — |
| HWP Artillery Cannon | 100 | — | 3 / 8 / 14 | — | — |
| HWP Smart Launcher | 100 | — | 49 / 84 / 96 | — | — |

Reading: the light cannons are the vehicles' *sniper analogue* (`baseAccuracy` 110–121 — the
longest-ranged direct-fire weapons in the entire mod), heavy cannons sit at rifle-class range
with HE payloads, machine guns are mid-range workhorses with notably better aimed range than
their infantry twins, and miniguns are the same 6–7-tile hard-capped sprayers as on foot. The
Artillery Cannon's tiny 3–14-tile *effective* range against its 40-tile `maxRange` confirms it
is not a direct-fire weapon — it saturates areas with arcing cluster/smoke/HE shells. The Smart
Launcher's waypoint guidance makes it near-guaranteed at any distance.

## HWP Missile Launcher and missile family

**HWP Missile Launcher** (`STR_HWP_MISSILE_LAUNCHER`) — buy $7,300, weight 10, turret sprite 1
(launcher), aimed-only: 140% accuracy / 80 TU (`baseAccuracy` 40), no reaction fire
(`reactionsModifier: 0`), overwatch capable. Battle magazine of 4 missiles (`battleClipSize: 4`);
missiles bought/built as single rounds. Six warheads:

| Missile | `STR_` | Power (type) | Blast | Acquisition |
|---|---|---|---|---|
| HWP HEAT Missile | `STR_HWP_HEAT_MISSILE` | 100 (AP) | — | Buy $1,500 |
| HWP Thermobaric Missile | `STR_HWP_THERMOBARIC_MISSILE` | 60 (HE) | r6, dropoff 4 | Buy $1,700 |
| HWP Incendiary Missile | `STR_HWP_INCENDIARY_MISSILE` | 90 (Incendiary) | — | Buy $2,100 |
| HWP Plasma Missile | `STR_HWP_PLASMA_MISSILE` | 180 (Plasma) | — | Mfr $12,500 + 2 Alloys + 1 Elerium (Heavy Plasma Weapons) |
| HWP Fission Missile | `STR_HWP_FISSION_MISSILE` | 90 (HE) | r7, dropoff 9 | Mfr $14,000 + 2 Alloys + 1 Elerium (Fission Warheads) |
| HWP Fusion Missile | `STR_HWP_FUSION_MISSILE` | 115 (HE) | r8, dropoff 9 | Mfr $27,000 + 2 Alloys + 3 Elerium (Fusion Warheads) |

## HWP Artillery Cannon and shell family

**HWP Artillery Cannon** (`STR_HWP_ARTILLERY_CANNON`) — mfr $52,000 (HWP Artillery research),
weight 750 (a heavy-engine proposition), turret sprite 1, **arcing shot** (`arcingShot: true`,
indirect fire), aimed-only 50% / 96 TU, `shotgunSpread: 65` (used by cluster shells), no reaction
or overwatch fire. Battle magazine of 5 shells. Eight shell types (cluster shells split into 6
sub-munitions via `shotgunPellets: 6`):

| Shell | `STR_` | Power (type) | Blast | Notes | Acquisition |
|---|---|---|---|---|---|
| HWP Artillery Shell | `STR_HWP_ARTILLERY_CANNON_HE` | 75 (HE) | r6, dropoff 10 | | Mfr $2,500 |
| HWP Artillery Cluster Shell | `STR_HWP_ARTILLERY_CANNON_CLUSTER` | 30 (HE) x6 | r5, dropoff 0 | 6 pellets | Mfr $3,500 |
| HWP Artillery Incendiary Shell | `STR_HWP_ARTILLERY_CANNON_I` | 50 (Incendiary) | r5, dropoff 10 | | Mfr $2,400 |
| HWP Artillery Smoke Shell | `STR_HWP_ARTILLERY_CANNON_SMOKE` | 80 (Smoke) | r8 | | Mfr $1,000 |
| HWP Fission Shell | `STR_HWP_FISSION_EXPLOSIVE_SHELL` | 90 (HE) | r7, dropoff 9 | | Mfr $22,000 + 1 Alloy + 2 Elerium (Fission Explosives) |
| HWP Fission Cluster Shell | `STR_HWP_FISSION_CLUSTER_SHELL` | 40 (HE) x6 | r6, dropoff 0 | 6 pellets | Mfr $25,000 + 1 Alloy + 2 Elerium (Fission Explosives) |
| HWP Fusion Shell | `STR_HWP_FUSION_EXPLOSIVE_SHELL` | 115 (HE) | r8, dropoff 9 | | Mfr $40,000 + 2 Alloys + 4 Elerium (Fusion Explosives) |
| HWP Fusion Cluster Shell | `STR_HWP_FUSION_CLUSTER_SHELL` | 50 (HE) x6 | r7, dropoff 0 | 6 pellets | Mfr $45,000 + 2 Alloys + 4 Elerium (Fusion Explosives) |

## HWP Smart Launcher

**HWP Smart Launcher** (`STR_HWP_SMART_LAUNCHER`) — the vehicle-mounted Blaster Launcher
equivalent, and in this mod the *prerequisite* for the infantry Blaster Launcher (the research
chain is Fusion Weapons + Arc Weapons + Heavy Tank → HWP Smart Launcher → Blaster Launcher).
Mfr $615,000 + 10 Alloys + 10 Elerium; weight 850; turret sprite 4; **10 waypoints**
(`waypoints: 10`), aimed 200% / 96 TU, no reaction/overwatch fire.

| Ammo | `STR_` | Power (type) | Blast | Acquisition |
|---|---|---|---|---|
| HWP Smart Fusion Bomb | `STR_HWP_SMART_FUSION_BOMB` | 175 (HE) | r9, dropoff 17 | Mfr $220,000 + 2 Alloys + 4 Elerium + **2 Blaster Bombs** |

## Vehicle equipment (`battleType: 13`, engine bay)

| Item | `STR_` | Effect | Research gate | Notes |
|---|---|---|---|---|
| HWP Headlights | `STR_HWP_HEADLIGHTS` | `equippedEffect: STR_HEADLAMP` (battlefield light source) | Combat Lights (`STR_BATTLE_LIGHTS`) | 1x1, weight 1 |
| HWP Night Vision Module | `STR_HWP_NIGHT_VISION_MODULE` | `equippedEffect: STR_NIGHT_VISION` | Night Vision (`STR_NIGHT_VISION`) | 1x1, weight 1 |

These plug into the mod's equipped-effects system (`Effects.rul`), mirroring the infantry
Headlamp / Night Vision Goggles items.

## Research tree (Vehicles.rul + Research.rul)

The hub is **Modular HWP Upgrades** (`STR_MODULAR_HWP_UPGRADES`, cost 50), which fans out into
parallel upgrade branches, each usually with an alien-tech second stage:

- Heavy HWP Armor (200) → Alien Alloy HWP Armor (700, +UFO Construction) — armor plates
- HWP Frame Reinforcement (220) → HWP Frame Alloy Reinforcement (500, +UFO Construction)
- HWP Targeting Improvement (250) → Hyperwave HWP Targeting (800, +Hyper-wave Transmissions)
- HWP Turbocharging (350) → HWP Elerium Turbocharging (900, +UFO Power Source)
- HWP Response Enhancement (250) → Hyperwave HWP Response Booster (800, +Hyper-wave Transmissions)
- Heavy HWP Cannon (250); HWP Artillery (400)
- Large HWP Engine (500) → Supercharged HWP Engines (700) → Elerium HWP Engines (900, +UFO Power Source)
- Heavy Tank (2000) and Hover Tank (2000), both gated on Advanced Armor
- HWP Smart Launcher (1000; Fusion Weapons + Arc Weapons + Heavy Tank), which unlocks the infantry Blaster Launcher

Laser/Plasma turret weapons ride the infantry weapon research lines instead (Heavy Laser
Weapons, Laser Cannons, Heavy Plasma Weapons, Plasma Cannons), and the exotic munitions ride
Fission/Fusion Warheads and Fission/Fusion Explosives.

## Analysis

- **From 4 SKUs to a build system.** Vanilla's four fixed tank products become 4 chassis x 16
  engines x ~15 turret weapons x 10 addon modules x 4 plate types. The Scout Car is a cheap, fast,
  fragile single-turret spotter with no plate slots; the Medium Tank is the workhorse; the Hover
  Tank trades plate depth for flight, all-around 30 armor, and the best reactions (80); the Heavy
  Tank doubles hull HP, takes triple plate stacks and the exclusive heavy engines, and gates the
  endgame Smart Launcher.
- **Tier scaling mirrors infantry.** Each direct-fire class comes in Basic → Laser → Plasma with
  the same fire modes; laser trades top-end damage for infinite ammo, plasma pays alloys/elerium
  for ~1.6x basic damage. Damage per class scales roughly: MG 35/47/58, minigun 22/29/36, light
  cannon 85/113/142, heavy cannon 105/140/175 — with heavy cannons always HE (area) regardless of
  tier, so "laser heavy cannon" is an HE gun, not a laser-damage gun.
- **Weight is the real constraint.** Engine strength (75-540) versus weapon/plate/module weight
  creates a genuine loadout economy: an Artillery Cannon (750) or Smart Launcher (850) demands the
  Heavy Tank's SHX-class engines, while turbochargers buy strength at the price of durability and
  accuracy.
- **The support layer is entirely new**: free in-battle reloading from an ammo rack, dual turrets,
  burst/auto fire modes, overwatch parameters per weapon, arcing cluster artillery with smoke
  rounds, waypoint-guided fusion bombs, and light-source/night-vision vehicle equipment — none of
  which exists for vanilla HWPs.
