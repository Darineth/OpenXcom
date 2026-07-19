# Legacy xcomtd — Hand-Held Weapons & Ammo

This document summarizes all hand-held soldier weapons and their ammunition added or changed by the legacy **xcomtd** ("X-COM: Terror Defense") master mod versus vanilla *X-COM: UFO Defense*. Sources: `OpenXcomDX-Legacy/bin/standard/xcomtd/Ruleset/Items.rul` (items up to the HWP section), `Ruleset/Vars.rul` (category taxonomy and combat constants), and display names from `Ruleset/Overhaul.rul` (`extraStrings`) plus `Language/en-US.yml`. HWP/vehicle weapons and armor items are covered in separate documents.

## Vanilla items removed

The mod **deletes** these vanilla items outright (top of `Items.rul`):

- `STR_PISTOL` / `STR_PISTOL_CLIP`, `STR_RIFLE` / `STR_RIFLE_CLIP` — replaced by the `STR_BASIC_*` ballistic line.
- `STR_HEAVY_CANNON` + AP/HE/I ammo, `STR_AUTO_CANNON` + AP/HE/I ammo — no direct successors; their niches are filled by the new Machine Gun / Minigun / Heavy Rifle / Grenade Launcher classes.
- `STR_HEAVY_LASER` — replaced by a whole family of new laser weapons (`STR_LASER_*`).
- `STR_HEAVY_PLASMA` + `STR_HEAVY_PLASMA_CLIP`, `STR_PLASMA_RIFLE_CLIP`, `STR_PLASMA_PISTOL_CLIP` — replaced by the expanded plasma family with new `*_AMMO` clip items.
- All vanilla tank weapons (`STR_TANK_CANNON`, etc.) — see the HWP document.

Vanilla keys that are **kept but redefined** with new stats: `STR_LASER_PISTOL`, `STR_LASER_RIFLE`, `STR_PLASMA_PISTOL`, `STR_PLASMA_RIFLE`, `STR_STUN_ROD`, `STR_ROCKET_LAUNCHER` and rockets, `STR_SMALL_LAUNCHER`/`STR_STUN_BOMB`, `STR_BLASTER_LAUNCHER`/`STR_BLASTER_BOMB`, and all grenades.

## Legacy-engine keys (custom to this fork)

These ruleset keys do not exist in modern OXCE and reflect custom systems in the legacy engine (semantics confirmed from the legacy `src/`):

- **`baseAccuracy`** — the weapon's *intrinsic* precision, independent of the soldier. The legacy fork replaced the hit-roll with an aim-cone model: soldier accuracy and weapon `baseAccuracy` each define a Gaussian deviation cone (`Projectile.cpp`), and the UI displays the combined result as an **effective range in tiles** rather than a hit percentage. Higher = tighter cone / longer effective range (Sniper Rifle 120 vs Minigun 6).
- **`accuracyBurst` / `tuBurst` / `burstShots`** — a fourth fire mode, **Burst Shot**, between snap and auto (typically 2–3 shots, better accuracy than auto).
- **`autoDelay`** — delay (animation pacing, ms) between shots of an auto/burst volley.
- **Dual fire** — the engine supports firing both hands at once (`STR_DUAL_FIRE`; accuracy penalty via `getDualFireAccuracy()`); no per-item key, but one-handed weapons (pistols/SMGs) are balanced around it via `oneHandedPenalty: 100`.
- **`reactionsModifier`** — percentage modifier to reaction fire with this weapon (120 = pistols react well; 1 = miniguns/heavies essentially cannot reaction-fire).
- **`overwatchModifier` / `overwatchRange` / `overwatchRadius` / `overwatchShot`** — a custom ordered-overwatch system (soldier watches an area; the weapon defines the accuracy modifier, watch range, watched radius, and which fire mode it uses).
- **`blockBothHands`** — weapon occupies both hands (nothing usable in off hand).
- **`battleClipSize`** — item is stored/bought per round but auto-bundled into magazines of this size in battle (e.g. grenade-launcher rounds come in 6-round drums, Psi Orbs in 12-charge clusters).
- **`blastDropoff`** — power lost per tile of blast radius for explosives (higher = more concentrated at center).
- **`aiRangeClose/Mid/Long/Max` + `aiAttackPriority*`** — per-weapon AI fire-mode preference bands by distance (present on all plasma weapons).
- **`kneelBonus`** — accuracy multiplier when kneeling (also in OXCE, but used aggressively here: snipers 150, machineguns 200).
- Vars.rul constants: `damageRange: 50` (damage rolls **50%–150%** of power instead of vanilla 0–200%), `encumbranceMultipler: 2`, `actionExperience` (per-action XP awards incl. `STR_SNEAK`, `STR_SPRINT`, `STR_DUAL_FIRE`), `damageDropoff` per damage type (AP 0.5, Laser 0.5, Plasma 0.8) — note the `damageDropoff` accessor is **commented out** in the legacy engine (`Mod.cpp`), so this block appears to be dead config.

Damage types: 1=AP, 2=Incendiary, 3=HE, 4=Laser, 5=Plasma, 6=Stun, 9=Smoke, 11=custom (used only by the Grappling Hook).

## Class taxonomy (from Vars.rul `itemCategories`)

Every tier repeats the same nine classes: **Pistol, SMG, (Assault) Rifle, Shotgun, Marksman Rifle, Sniper Rifle, Machinegun, Minigun, Heavy Rifle** — plus separate Launcher classes (Grenade / Rocket / Small / Arc / Blaster) and Melee/Psi categories.

---

## Ballistic tier (`STR_BASIC_*`) — buyable from the start

All damage type 1 (AP), all clip-fed and purchasable. Prices are buy/sell in $.

| Weapon | Key | Ammo (clip) | Power | Wt | Hands | Snap | Aimed | Burst | Auto | baseAcc | Buy |
|---|---|---|---|---|---|---|---|---|---|---|---|
| Pistol | `STR_BASIC_PISTOL` | Pistol Ammo (8) | 22 | 3 | 1H | 60% / 25TU | 100% / 45TU | 50% / 60TU ×3 | — | 18 | 800 |
| Submachine Gun | `STR_BASIC_SMG` | SMG Ammo (32) | 22 | 6 | 1H | 70% / 30 | 110% / 55 | 60% / 45 ×3 | 40% / 75 ×8 | 26 | 2,200 |
| Assault Rifle | `STR_BASIC_RIFLE` | Assault Rifle Ammo (16) | 35 | 8 | 2H | 80% / 35 | 130% / 65 | 70% / 55 ×3 | 45% / 85 ×8 | 35 | 3,000 |
| Combat Shotgun | `STR_BASIC_SHOTGUN` | Buckshot (6, 22 ×6 pellets) / Slug (6, 50) | 22×6 or 50 | 8 | 2H | 70% / 40 | 110% / 70 | — | — | 26 | 3,600 |
| Marksman Rifle | `STR_BASIC_MARKSMAN` | Marksman Ammo (12) | 40 | 11 | 2H | 65% / 40 | 110% / 70 | 80% / 60 ×2 | 40% / 85 ×6 | 60 | 5,500 |
| Sniper Rifle | `STR_BASIC_SNIPER` | Sniper Ammo (4) | 55 | 14 | 2H | 40% / 45 | 107% / 70 | — | — | 120 | 8,200 |
| Machine Gun | `STR_BASIC_MACHINEGUN` | MG Ammo (80) | 35 | 18 | 2H | 45% / 35 | 70% / 65 | 35% / 55 ×3 | 35% / 80 ×8 | 35 | 7,200 |
| Minigun | `STR_BASIC_MINIGUN` | Minigun Ammo (200) | 22 | 28 | 2H (blocks both) | — | — | 200% / 40 ×5 | 200% / 70 ×20 | 6 | 13,500 |
| Heavy Rifle | `STR_BASIC_HEAVY` | Heavy Ammo (4) | 85 | 30 | 2H (blocks both) | 30% / 55 | 80% / 80 | — | — | 80 | 15,000 |

Notes: the shotgun uses `shotgunSpread: 8` with 6 pellets on buckshot; slugs are single high-power AP rounds ("better armor penetration" per the Ufopaedia text). The minigun has *no* aimed/snap modes at all and a `reactionsModifier` of 1 (cannot meaningfully reaction-fire) but overwatches with auto fire. Overwatch profiles are class-specific: pistols watch a 5-tile range with snap, SMGs/rifles 15–20 with burst, marksman 25 / sniper 30 with burst/aimed, MG 20 with auto (radius 3).

## Laser tier (`STR_LASER_*`) — manufactured, self-powered

All damage type 4 (Laser), **no ammo** (`clipSize: -1` = built-in power source), not buyable (manufacture only; `costSell` shown). Weight, hands, fire-mode structure and overwatch profiles mirror the ballistic class exactly; TU costs run ~10% higher and power ~30–35% higher than the ballistic counterpart.

| Weapon | Key | Class | Power | Wt | Snap | Aimed | Burst | Auto | baseAcc | Sell |
|---|---|---|---|---|---|---|---|---|---|---|
| Laser Pistol | `STR_LASER_PISTOL` | Pistol | 29 | 3 | 60% / 28 | 100% / 50 | 50% / 66 ×3 | — | 20 | 20,000 |
| Pulse Laser Pistol | `STR_LASER_SMG` | SMG | 29 | 6 | 70% / 33 | 110% / 61 | 60% / 50 ×3 | 40% / 83 ×8 | 29 | 28,000 |
| Laser Rifle | `STR_LASER_RIFLE` | Assault Rifle | 47 | 8 | 80% / 39 | 130% / 72 | 70% / 61 ×3 | 45% / 94 ×8 | 39 | 36,900 |
| Prismatic Laser | `STR_LASER_SHOTGUN` | Shotgun | 29 ×6 pellets | 8 | 70% / 44 | 110% / 77 | — | — | 29 | 36,100 |
| Strike Beam Rifle | `STR_LASER_MARKSMAN` | Marksman | 53 | 10 | 65% / 44 | 110% / 77 | 80% / 66 ×2 | 40% / 94 ×6 | 66 | 38,000 |
| Precision Beam Rifle | `STR_LASER_SNIPER` | Sniper | 73 | 14 | 40% / 50 | 107% / 77 | — | — | 132 | 51,100 |
| Pulse Laser | `STR_LASER_MACHINEGUN` | Machinegun | 47 | 18 | 45% / 39 | 70% / 72 | 35% / 61 ×3 | 35% / 88 ×8 | 39 | 53,000 |
| Hyper Pulse Laser | `STR_LASER_MINIGUN` | Minigun | 29 | 28 | — | — | 200% / 44 ×5 | 200% / 77 ×20 | 7 | 65,400 |
| Heavy Laser Rifle | `STR_LASER_HEAVY` | Heavy | 113 | 30 | 30% / 61 | 80% / 88 | — | — | 88 | 61,000 |

The shotgun-class laser gets its pellets from the weapon itself (`shotgunPellets: 6`, `shotgunSpread: 10`) since there is no ammo item.

## Plasma tier — X-COM (`STR_XCOM_PLASMA_*`) vs alien (`STR_PLASMA_*`)

All damage type 5 (Plasma), **clip-fed** — both variants share the *same* `STR_PLASMA_*_AMMO` clip items. Power runs ~60–70% above ballistic; TU costs are ~10% *lower* than ballistic (plasma is the fast tier), but per-mode accuracy percentages are ~10% lower.

**The X-COM vs alien split:** each class exists twice with **identical combat stats, weight, and sell price**. The differences are:

- **Research gating** — the alien version `requires: STR_ALIEN_PLASMA` (understanding captured weapons); the X-COM version requires the corresponding X-COM plasma tech tier: `STR_PLASMA_SIDEARMS` (pistol/SMG), `STR_PLASMA_ASSAULT_WEAPONS` (rifle/shotgun), `STR_PRECISION_PLASMA_WEAPONS` (marksman/sniper), `STR_HEAVY_PLASMA_WEAPONS` (machinegun/minigun/heavy).
- **The ammo requires the X-COM tech**, not `STR_ALIEN_PLASMA` — so captured alien plasma guns only become sustainable once X-COM can manufacture the matching clips.
- The alien versions carry the extra category `STR_CAT_ALIEN_EQUIPMENT` and their own sprites; X-COM versions are the manufacturable equivalents (i.e. captured loot vs produced gear — not built-in vs clip as in some mods; **both are clip-based here**, a change from vanilla where only alien plasma used clips).
- Only the plasma tier carries `aiRange*`/`aiAttackPriority*` AI fire-mode bands and `recoveryPoints` (3–5 per weapon, 1 per clip).

| Weapon (X-COM / Alien name) | Keys | Ammo (clip) | Power | Wt | Snap | Aimed | Burst | Auto | baseAcc | Sell |
|---|---|---|---|---|---|---|---|---|---|---|
| Plasma Pistol / Alien Plasma Pistol | `STR_XCOM_PLASMA_PISTOL` / `STR_PLASMA_PISTOL` | `STR_PLASMA_PISTOL_AMMO` (8) | 36 | 3 | 54% / 23 | 90% / 41 | 45% / 55 ×3 | — | 18 | 84,000 |
| Plasma Blaster Pistol / Alien … | `STR_XCOM_PLASMA_SMG` / `STR_PLASMA_SMG` | `STR_PLASMA_SMG_AMMO` (32) | 36 | 6 | 63% / 27 | 99% / 50 | 54% / 41 ×3 | 36% / 68 ×8 | 26 | 102,000 |
| Plasma Rifle / Alien Plasma Rifle | `STR_XCOM_PLASMA_RIFLE` / `STR_PLASMA_RIFLE` | `STR_PLASMA_RIFLE_AMMO` (16) | 58 | 8 | 72% / 32 | 117% / 59 | 63% / 50 ×3 | 41% / 77 ×8 | 35 | 126,500 |
| Plasma Burst Rifle / Alien … | `STR_XCOM_PLASMA_SHOTGUN` / `STR_PLASMA_SHOTGUN` | `STR_PLASMA_SHOTGUN_AMMO` (6, ×6 pellets) | 36 ×6 | 8 | 63% / 36 | 99% / 64 | — | — | 26 | 93,500 |
| Plasma Shock Rifle / Alien … | `STR_XCOM_PLASMA_MARKSMAN` / `STR_PLASMA_MARKSMAN` | `STR_PLASMA_MARKSMAN_AMMO` (12) | 67 | 10 | 59% / 36 | 99% / 64 | 72% / 55 ×2 | 36% / 77 ×6 | 60 | 141,000 |
| Plasma Beam Rifle / Alien … | `STR_XCOM_PLASMA_SNIPER` / `STR_PLASMA_SNIPER` | `STR_PLASMA_SNIPER_AMMO` (4) | 92 | 14 | 36% / 41 | 96% / 64 | — | — | 120 | 157,300 |
| Plasma Blaster / Alien Plasma Blaster | `STR_XCOM_PLASMA_MACHINEGUN` / `STR_PLASMA_MACHINEGUN` | `STR_PLASMA_MACHINEGUN_AMMO` (80) | 58 | 18 | 41% / 32 | 63% / 59 | 32% / 50 ×3 | 32% / 73 ×8 | 35 | 203,000 |
| Plasma Pulsar / Alien Plasma Pulsar | `STR_XCOM_PLASMA_MINIGUN` / `STR_PLASMA_MINIGUN` | `STR_PLASMA_MINIGUN_AMMO` (200) | 36 | 28 | — | — | 200% / 36 ×5 | 200% / 64 ×20 | 6 | 231,000 |
| Heavy Plasma Rifle / Alien … | `STR_XCOM_PLASMA_HEAVY` / `STR_PLASMA_HEAVY` | `STR_PLASMA_HEAVY_AMMO` (4) | 142 | 30 | 27% / 50 | 72% / 73 | — | — | 80 | 221,000 |

(The Heavy Plasma pair is mis-categorized as `STR_RIFLE` instead of `STR_CAT_RIFLE` in the ruleset — an apparent typo.)

## Arc weapons, stun weapons

| Weapon | Key | Ammo (clip) | Power / type | Wt | Fire modes | Notes |
|---|---|---|---|---|---|---|
| Arc Rifle | `STR_ARC_RIFLE` | Arc Rifle Ammo (4) | 92 Plasma | 14 | Aimed only: 200% / 65TU | `waypoints: 2` — a 2-waypoint guided shot (fire around corners); requires `STR_ARC_WEAPONS`; baseAcc 100; overwatch range 35. Reuses the Plasma Beam Rifle sprites. |
| Electrolaser | `STR_ELECTROLASER` | Electrolaser Cell (6) | 40 Stun | 4 | Snap 80% / 30, Aimed 120% / 50 | Ranged stun pistol (1-handed); cell has `blastRadius: 0` (single-target). Not buyable (`costBuy: 0`). |
| Stun Rod | `STR_STUN_ROD` | — (built-in) | 65 Stun (melee) | 4 | Melee 100% / 30TU | Buyable ($1,260); `skillApplied: false` — flat 100% hit. Power reduced from vanilla's 90. |

## Launchers

| Weapon | Key | Compatible ammo | Fire modes | Wt | Notes |
|---|---|---|---|---|---|
| Grenade Launcher | `STR_GRENADE_LAUNCHER` | GL-HE / GL-HE-X1 Fission / GL-HE-X2 Fusion / GL-I | Snap 60%/40, Aimed 100%/70, Burst 50%/70 ×3, Auto 30%/95 ×6 | 24 | **New weapon.** Arcing shot, `maxRange: 25`, buyable ($12,500), baseAcc 15. Rounds are bought singly but load as 6-round drums (`battleClipSize: 6`). |
| Rocket Launcher | `STR_ROCKET_LAUNCHER` | HEAT / Thermobaric / Incendiary / Plasma / Fission / Fusion rockets | Snap 70%/50, Aimed 140%/80 | 6 | Buyable ($4,000), baseAcc 40. |
| Small Launcher | `STR_SMALL_LAUNCHER` | Stun Bomb, **plus thrown grenades**: Grenade, Smoke, Incendiary, Alien Grenade, Shock Grenade | Snap 60%/40, Aimed 100%/75 | 6 | Alien tech (`requires: STR_SMALL_LAUNCHER`). Now doubles as a general grenade projector. |
| Arc Launcher | `STR_ARC_LAUNCHER` | Same six rockets as the Rocket Launcher | Aimed only: 200% / 90TU | 20 | **New.** A `waypoints: 2` guided rocket launcher (requires `STR_ARC_WEAPONS`); baseAcc 100. |
| Blaster Launcher | `STR_BLASTER_LAUNCHER` | Blaster Bomb | Aimed only: 200% / 80TU | 16 | `waypoints: 10` as vanilla; baseAcc 100. |

### Launcher ammunition

| Ammo | Key | Power | Type | Blast r. | blastDropoff | Wt | Buy / Sell |
|---|---|---|---|---|---|---|---|
| GL-HE Explosive Grenades | `STR_GL_HE_AMMO` | 35 | 3 HE | 3 | 8 | 6 | 200 / 150 |
| GL-HE-X1 Fission Grenades | `STR_GL_HE_FISSION_AMMO` | 47 | 3 HE | 4 | 8 | 6 | — / 6,000 |
| GL-HE-X2 Fusion Grenades | `STR_GL_HE_FUSION_AMMO` | 58 | 3 HE | 5 | 8 | 6 | — / 11,000 |
| GL-I Incendiary Grenades | `STR_GL_I_AMMO` | 30 | 2 IN | 3 | 8 | 6 | 450 / 310 |
| HEAT Rocket | `STR_SMALL_ROCKET` | 100 | 1 **AP** | — | — | 4 | 600 / 480 |
| Thermobaric Rocket | `STR_LARGE_ROCKET` | 65 | 3 HE | 6 | 4 | 6 | 900 / 720 |
| Incendiary Rocket | `STR_INCENDIARY_ROCKET` | 90 | 2 IN | — | — | 6 | 1,200 / 960 |
| Plasma Rocket | `STR_PLASMA_ROCKET` | 180 | 5 **Plasma** | — | — | 4 | — / 18,700 |
| Fission Rocket | `STR_FISSION_ROCKET` | 90 | 3 HE | 7 | 9 | 6 | — / 32,300 |
| Fusion Rocket | `STR_FUSION_ROCKET` | 115 | 3 HE | 8 | 9 | 6 | — / 51,000 |
| Stun Bomb | `STR_STUN_BOMB` | 70 | 6 Stun | 5 | — | 3 | — / 15,200 |
| Blaster Bomb | `STR_BLASTER_BOMB` | 140 | 3 HE | 4 | 30 | 3 | — / 17,028 |

The vanilla Small/Large Rockets were redesigned: the "HEAT Rocket" is a single-target AP anti-armor round (no blast radius), the "Thermobaric Rocket" trades power for a wide, flat blast (dropoff 4), and the Plasma Rocket is a point-damage 180-power plasma warhead. The Blaster Bomb was cut from vanilla's 200 power to 140 with a tight radius-4/dropoff-30 blast. (A `STR_FUSION_CLUSTER_ROCKET` name exists in the strings but no such hand-held item is defined in this range.)

## Grenades & explosives

| Item | Key | Power | Type | Blast r. | Wt | Buy / Sell | Notes |
|---|---|---|---|---|---|---|---|
| Grenade | `STR_GRENADE` | 40 | 3 HE | 5 | 2 | 300 / 240 | Vanilla 50 → 40 |
| Alien Grenade | `STR_ALIEN_GRENADE` | 70 | 3 HE | 5 | 2 | — / 14,850 | Vanilla 90 → 70; requires research |
| High Explosive | `STR_HIGH_EXPLOSIVE` | 120 | 3 HE | 9 (dropoff 6) | 20 | 1,500 / 1,200 | Demolition charge |
| Fission Explosive | `STR_FISSION_EXPLOSIVE` | 145 | 3 HE | 10 (dropoff 8) | 20 | — / 17,000 | **New** — elerium implosion demolition charge |
| Fusion Explosive | `STR_FUSION_EXPLOSIVE` | 170 | 3 HE | 11 (dropoff 10) | 20 | — / 53,000 | **New** — top-tier demolition charge |
| Incendiary Grenade | `STR_INCENDIARY_GRENADE` | 50 | 2 IN | 5 | 2 | 260 / 180 | **New** (no vanilla equivalent) |
| Shock Grenade | `STR_SHOCK_GRENADE` | 50 | 6 Stun | 3 | 2 | — / 18,500 | **New** — throwable mini Stun Bomb |
| Smoke Grenade | `STR_SMOKE_GRENADE` | 60 | 9 Smoke | 5 | 2 | 150 / 120 | ~vanilla |
| Proximity Grenade | `STR_PROXIMITY_GRENADE` | 40 | 3 HE | 5 | 2 | 500 / 400 | Vanilla 70 → 40 |

---

## Effective range (the aim-cone accuracy model)

The legacy engine replaces the vanilla percent-to-hit roll entirely (`BattleUnit::calculateEffectiveRange`, `calculateChanceToHit` in the legacy `BattleUnit.cpp`). Soldier accuracy and weapon `baseAccuracy` each define a Gaussian angular deviation; combined they yield an **effective range in tiles — the distance at which a shot has a 50% chance to hit**. Hit chance at any distance is:

```
chanceToHit = effectiveRange / (distance + effectiveRange)
```

so a target at 2× effective range is hit ~33% of the time, at 3× ~25%, and pointblank approaches 100%. The soldier-side input is `firing stat × fire-mode accuracy% / 100`, further multiplied by kneeling (`kneelBonus`/100, default ×1.15), health/fatal-wound modifiers, an energy penalty below 50% stamina, `oneHandedPenalty` (two-handed weapon with full off hand), and dual-fire. Two useful closed-form limits fall out of the engine's formula:

- **Weapon-limited cap:** as soldier skill grows, effective range asymptotes to exactly **`baseAccuracy` tiles**. That is what `baseAccuracy` *is* — the weapon's maximum effective range. A Minigun (`baseAccuracy: 6`) is a 6-tile weapon for every soldier who will ever hold it; a sniper rifle (120) is effectively uncapped.
- **Soldier-limited floor:** with a perfect weapon, effective range ≈ `(firing × mode%)² / 116` — quadratic in soldier skill, so skill gains compound (60→80 firing nearly doubles effective range).

### Computed effective ranges (tiles, standing, healthy)

Values are Snap / Aimed / Burst / Auto at reference firing stats **40 / 60 / 80**; "—" = mode not available. Computed with the engine's exact formula from each weapon's `baseAccuracy` and mode accuracy percentages. Kneeling multiplies the soldier term by `kneelBonus` (weapons' values noted below) — e.g. a firing-60 sniper kneeling (kneelBonus 150) jumps from 34 to ~69 tiles aimed.

**Ballistic tier:**

| Weapon | baseAcc (cap) | Snap | Aimed | Burst | Auto |
|---|---|---|---|---|---|
| Pistol | 18 | 5 / 10 / 14 | 11 / 17 / 18 | 3 / 7 / 11 | — |
| Submachine Gun | 26 | 6 / 13 / 20 | 14 / 23 / 25 | 5 / 10 / 16 | 3 / 5 / 8 |
| Assault Rifle | 35 | 8 / 18 / 27 | 20 / 31 / 34 | 7 / 14 / 22 | 3 / 6 / 11 |
| Combat Shotgun | 26 | 6 / 13 / 20 | 14 / 23 / 25 | — | — |
| Marksman Rifle | 60 | 6 / 13 / 22 | 16 / 33 / 48 | 9 / 19 / 31 | 3 / 5 / 9 |
| Sniper Rifle | 120 | 3 / 5 / 9 | 15 / 34 / 57 | — | — |
| Machine Gun | 35 | 3 / 6 / 11 | 7 / 14 / 22 | 3 / 4 / 7 | 3 / 4 / 7 |
| Minigun | 6 | — | — | 6 / 6 / 6 | 6 / 6 / 6 |
| Heavy Rifle | 80 | 3 / 3 / 5 | 9 / 19 / 32 | — | — |

**Laser tier** (mode accuracy equals ballistic, `baseAccuracy` ~10% higher — marginally longer-ranged):

| Weapon | baseAcc (cap) | Snap | Aimed | Burst | Auto |
|---|---|---|---|---|---|
| Laser Pistol | 20 | 5 / 10 / 15 | 12 / 18 / 20 | 3 / 7 / 12 | — |
| Pulse Laser Pistol | 29 | 7 / 14 / 21 | 15 / 25 / 28 | 5 / 10 / 17 | 3 / 5 / 8 |
| Laser Rifle | 39 | 9 / 18 / 28 | 20 / 34 / 38 | 7 / 14 / 23 | 3 / 6 / 11 |
| Prismatic Laser | 29 | 7 / 14 / 21 | 15 / 25 / 28 | — | — |
| Strike Beam Rifle | 66 | 6 / 13 / 22 | 16 / 33 / 50 | 9 / 19 / 32 | 3 / 5 / 9 |
| Precision Beam Rifle | 132 | 3 / 5 / 9 | 16 / 34 / 57 | — | — |
| Pulse Laser | 39 | 3 / 6 / 11 | 7 / 14 / 23 | 3 / 4 / 7 | 3 / 4 / 7 |
| Hyper Pulse Laser | 7 | — | — | 7 / 7 / 7 | 7 / 7 / 7 |
| Heavy Laser Rifle | 88 | 3 / 3 / 5 | 9 / 19 / 33 | — | — |

**Plasma tier** (X-COM and alien versions identical; `baseAccuracy` equals ballistic but mode accuracy ~10% lower — the shortest-ranged tier, compensating its top damage and speed):

| Weapon | baseAcc (cap) | Snap | Aimed | Burst | Auto |
|---|---|---|---|---|---|
| Plasma Pistol | 18 | 4 / 8 / 13 | 10 / 16 / 18 | 3 / 6 / 10 | — |
| Plasma Blaster Pistol | 26 | 5 / 11 / 18 | 12 / 21 / 25 | 4 / 9 / 14 | 3 / 4 / 7 |
| Plasma Rifle | 35 | 7 / 15 / 23 | 17 / 29 / 34 | 5 / 12 / 19 | 3 / 5 / 9 |
| Plasma Burst Rifle | 26 | 5 / 11 / 18 | 12 / 21 / 25 | — | — |
| Plasma Shock Rifle | 60 | 5 / 11 / 18 | 13 / 27 / 43 | 7 / 15 / 26 | 3 / 4 / 7 |
| Plasma Beam Rifle | 120 | 3 / 4 / 7 | 13 / 28 / 47 | — | — |
| Plasma Blaster | 35 | 3 / 5 / 9 | 5 / 12 / 19 | 3 / 3 / 6 | 3 / 3 / 6 |
| Plasma Pulsar | 6 | — | — | 6 / 6 / 6 | 6 / 6 / 6 |
| Heavy Plasma Rifle | 80 | 3 / 3 / 4 | 7 / 16 / 27 | — | — |

**Specials and launchers:**

| Weapon | baseAcc (cap) | Snap | Aimed | Burst | Auto |
|---|---|---|---|---|---|
| Arc Rifle / Arc Launcher / Blaster Launcher | 100 | — | 49 / 84 / 96 | — | — |
| Electrolaser | 26 | 8 / 16 / 23 | 16 / 24 / 26 | — | — |
| Grenade Launcher | 15 | 5 / 9 / 13 | 11 / 14 / 15 | 3 / 7 / 11 | 3 / 3 / 5 |
| Small Launcher | 15 | 5 / 9 / 13 | 11 / 14 / 15 | — | — |
| Rocket Launcher | 40 | 7 / 14 / 23 | 23 / 36 / 39 | — | — |
| Grappling Hook | 132 | — | 16 / 36 / 60 | — | — |

(Alien built-in weapons for reference: Cyberdisc `baseAccuracy` 30, Sectopod 50.)

### Design reading

`baseAccuracy` does the class differentiation that TU/accuracy percentages alone can't: the Minigun and Hyper Pulse Laser are *hard-capped* at 6–7 tiles no matter who fires them, launchers are capped at 15 (lob it close), pistols at ~18–20, rifles at ~35–39, while marksman (60–66), heavy (80–88), and sniper (120–132) weapons keep rewarding soldier skill essentially forever. Fire modes then trade inside that envelope: a rifle's aimed shot at firing 60 is a 31-tile weapon, its auto mode a 6-tile weapon. The waypoint weapons' 200% aimed accuracy makes them near-guaranteed hits at any practical distance — balanced by TU cost, ammo economy, and (for the Blaster Launcher) its position at the end of the tech tree.

## Analysis

**Tier scaling.** The three tiers are strict re-skins of one nine-class chassis. Within a class, weight, hand requirements, inventory size and overwatch behavior are held constant across tiers; what changes is:

- **Power:** laser ≈ ballistic ×1.3 (e.g. rifle 35 → 47 → 58; heavy 85 → 113 → 142; pistol 22 → 29 → 36). Plasma ≈ ballistic ×1.65.
- **Speed vs accuracy:** lasers pay ~10% *more* TU per shot than ballistics at equal accuracy percentages; plasma pays ~10% *less* TU but with ~10% lower accuracy percentages — ballistic = baseline, laser = slower and harder-hitting with no ammo logistics, plasma = fastest and hardest-hitting but clip-dependent and least accurate per mode.
- **Logistics:** ballistic = buy everything; laser = manufacture once, never reload (`clipSize: -1`); plasma = manufacture weapon *and* clips (or capture alien weapons, which still need manufactured clips).
- **`baseAccuracy`** (intrinsic precision / effective range) rises slightly for lasers (rifle 35→39, sniper 120→132) and stays at ballistic values for plasma.

**Class roles.** Pistol/SMG: cheap one-handed reaction weapons (reactionsModifier 115–120, dual-fire capable). Assault rifle: all-rounder with all four fire modes. Shotgun: 6-pellet spread (`shotgunPellets`), snap/aimed only, short overwatch range (5) — close-quarters burst damage; the ballistic one uniquely swaps to slug ammo for single high-power shots. Marksman: mid-long range with a signature 2-round burst; kneelBonus 133. Sniper: huge `baseAccuracy` (120+), aimed-mode specialist, kneelBonus 150, poor reactions. Machinegun: sustained auto fire, kneelBonus 200 (must kneel), auto-mode overwatch with radius 3. Minigun: 20-shot auto hose with `baseAccuracy` 6 and *no* single-shot modes — pure suppression, worthless against armor (lowest per-shot power in tier). Heavy Rifle: 4-round anti-armor cannon (highest per-shot power), blocks both hands, reactionsModifier 1.

**X-COM vs alien plasma:** statistically identical twins; the split exists purely for the research/economy loop (captured "Alien …" versions usable after one research, X-COM manufacturable versions and — crucially — their shared clips gated behind a four-branch plasma tech tree). Both are clip-based, unlike vanilla.

**Other patterns.** Elerium tech ("fission" → "fusion") forms an upgrade ladder across explosives, GL rounds, and rockets (+~20–25% power and +1 blast radius per step). The waypoint mechanic was generalized from the Blaster Launcher into a mini-family: Arc Rifle (2-waypoint plasma shot) and Arc Launcher (2-waypoint rocket). Stun options expanded from 2 to 4 (Stun Rod, Electrolaser, Shock Grenade, Stun Bomb). Vanilla one-shot grenade power levels were generally *nerfed* (Grenade 50→40, Alien Grenade 90→70, Proximity 70→40, Blaster Bomb 200→140) while big deliberate demolition charges got stronger and a damage roll of 50–150% (instead of 0–200%) makes all damage far more predictable.
