# xcomtd (X-COM: Terror Defense) — Research Tree & Manufacture

Reference summary of the legacy `xcomtd` master mod's tech tree and manufacturing versus
vanilla X-COM: UFO Defense. Sources: `bin/standard/xcomtd/Ruleset/Research.rul` (weapon/
armor/craft/lore tree), `Manufacture.rul`, `Vehicles.rul` (the HWP research + manufacture
appendix at its end), `Overhaul.rul` (`extraStrings` display names), and
`Language/en-US.yml`. Costs below are scientist-days (`cost:`) unless stated. Display
names come from the mod's `en-US` strings.

## Big picture

Vanilla's flat "Laser Pistol → Laser Rifle → Heavy Laser" item-by-item research is replaced
by **weapon-class tier research**: you research a *class* (e.g. "Laser Assault Weapons")
which then unlocks *manufacturing* of the weapons in that class. Deleted vanilla research:
`STR_LASER_PISTOL/RIFLE`, `STR_HEAVY_LASER`, `STR_LASER_DEFENSE`, all individual plasma
weapon+clip topics, `STR_PLASMA_CANNON`, `STR_PLASMA_DEFENSE`, `STR_FUSION_MISSILE`,
`STR_FUSION_DEFENSE`, `STR_BLASTER_BOMB`, `STR_PERSONAL_ARMOR`. New branches: fission/
fusion ordnance, arc weapons, electrolasers, a 6-suit armor line, night-ops equipment,
field surgery, and a large modular-HWP upgrade tree.

Legacy-engine research keys (by inference; not standard OXCE): **`needsAnyItems`** — the
project needs at least one of the listed items in stores (capture-gated plasma tiers);
**`needsAllItems`** — needs all listed items. Standard keys used heavily: `needItem`
(needs its own artifact), `dependencies` (all required), `unlocks` (completing the
unlocker makes the target available), `getOneFree`, `lookup`, `requires` (hard gate).

## Laser branch (`STR_LASER_WEAPONS` → cannons)

Linear spine with two side-branches; no capture requirements, pure lab work:

| Tier | Research (`STR_`) | Cost | Depends on |
|---|---|---|---|
| 0 | Laser Weapons (`STR_LASER_WEAPONS`) | 200 | — |
| 1 | Laser Sidearms (`STR_LASER_SIDEARMS`) | 250 | Laser Weapons |
| 2 | Laser Assault Weapons (`STR_LASER_ASSAULT_WEAPONS`) | 600 | Laser Sidearms |
| 3a | Electrolasers (`STR_ELECTROLASERS`) | 1000 | Laser Assault Weapons |
| 3b | Precision Laser Weapons (`STR_PRECISION_LASER_WEAPONS`) | 800 | Laser Assault Weapons |
| 4 | Heavy Laser Weapons (`STR_HEAVY_LASER_WEAPONS`) | 1500 | Precision Laser Weapons |
| 5 | Laser Cannons (`STR_LASER_CANNONS`) | 1500 | Heavy Laser Weapons |

Unlocked manufacture per tier: Sidearms → Laser Pistol, Pulse Laser Pistol
(`STR_LASER_SMG`); Assault → Laser Rifle, Prismatic Laser (`STR_LASER_SHOTGUN`);
Electrolasers → Electrolaser + Cell (stun weapon line); Precision → Strike Beam Rifle
(`STR_LASER_MARKSMAN`), Precision Beam Rifle (`STR_LASER_SNIPER`); Heavy → Pulse Laser
(`STR_LASER_MACHINEGUN`), Hyper Pulse Laser (`STR_LASER_MINIGUN`), Heavy Laser Rifle
(`STR_LASER_HEAVY`); Laser Cannons → craft Laser Cannon, Laser Defenses facility, HWP
laser cannons, and is a prerequisite for Plasma Weapons, Fission Weapons, and Combat
Lights.

## Plasma branch (capture-gated)

Mirrors the laser spine but each tier is gated on **possessing captured alien weapons**
(`needsAnyItems`) — the alien originals are usable-after-research and X-COM copies
(`STR_XCOM_PLASMA_*`) are manufactured:

| Tier | Research | Cost | Depends on | Needs any of (captured) |
|---|---|---|---|---|
| 0 | Plasma Weapons (`STR_PLASMA_WEAPONS`) | 600 | **Laser Cannons** | — |
| 1 | Plasma Sidearms (`STR_PLASMA_SIDEARMS`) | 1300 | Plasma Weapons | Alien Plasma Pistol, Alien Plasma Blaster Pistol (`STR_PLASMA_SMG`) |
| 2 | Plasma Assault Weapons (`STR_PLASMA_ASSAULT_WEAPONS`) | 1600 | Plasma Sidearms | Alien Plasma Rifle, Alien Plasma Burst Rifle (`STR_PLASMA_SHOTGUN`) |
| 3 | Precision Plasma Weapons (`STR_PRECISION_PLASMA_WEAPONS`) | 1800 | Plasma Assault | Alien Plasma Shock Rifle (`STR_PLASMA_MARKSMAN`), Alien Plasma Beam Rifle (`STR_PLASMA_SNIPER`) |
| 4 | Heavy Plasma Weapons (`STR_HEAVY_PLASMA_WEAPONS`) | 2500 | Precision Plasma | Alien Plasma Blaster (`STR_PLASMA_MACHINEGUN`), Alien Plasma Pulsar (`STR_PLASMA_MINIGUN`), Alien Heavy Plasma Rifle (`STR_PLASMA_HEAVY`) |
| 5 | Plasma Cannons (`STR_PLASMA_CANNONS`) | 3000 | Heavy Plasma Weapons | — |

Plasma Cannons unlocks the craft Plasma Cannon (renamed Plasma Beam), Plasma Defenses,
HWP plasma cannons, and feeds Fusion Power and Night Vision. Plasma Sidearms also gates
Small Launcher (550, needs item) and Stun Bomb (180, needs item) research.

## Fission / Fusion / Arc ordnance (new branches)

- **Fission Weapons** (`STR_FISSION_WEAPONS`, 530) — deps Alien Alloys + Elerium-115 +
  Laser Cannons. Children: **Fission Explosives** (600 → Fission Explosive demo charge,
  GL-HE-X1 Fission Grenades, HWP fission shells), **Fission Warheads** (800 → Fission
  Rocket, HWP Fission Missile). Also gates Alien Grenade research (200, needs item) →
  **Shock Grenade** (800, deps Alien Grenade + Stun Bomb; throwable stun bomb). UFO Power
  Source research now *depends on Fission Weapons*.
- **Fusion Power** (`STR_FUSION_POWER`, 920) — deps UFO Power Source + Plasma Cannons.
  Gates **Fusion Weapons** (1250, `needsAllItems` Blaster Launcher + Blaster Bomb) →
  **Fusion Warheads** (1000 → craft Fusion Ball Launcher/Ball, Fusion Ball Defenses,
  Fusion Rocket, HWP Fusion Missile) and **Fusion Explosives** (1000 → Fusion Explosive,
  GL-HE-X2 Fusion Grenades, HWP fusion shells).
- **Arc Weapons** (`STR_ARC_WEAPONS`, 1000) — deps Hyper-wave Transmissions + Grav Shield,
  `needsAllItems` Blaster Launcher + Bomb → Arc Rifle + ammo, Arc Launcher.
- **Blaster line (heavily delayed vs vanilla):** HWP Smart Launcher (`STR_HWP_SMART_LAUNCHER`,
  1000; deps Fusion Weapons + Arc Weapons + **Heavy Tank**) → **Blaster Launcher** (900;
  deps HWP Smart Launcher + **Ethereal Commander** interrogation, `needsAllItems` launcher
  + bomb). Blaster Launcher/Bomb manufacture exists but sits at the very end of the tree.

## Armor branch (six named suits, replaces the vanilla three)

`STR_PERSONAL_ARMOR` research deleted. All suits are manufactured (Alloys + Elerium):

| Suit (display name) | Research | Cost | Depends on |
|---|---|---|---|
| "Lookout" Scout Armor (`STR_SCOUT_ARMOR`) | 600 | Alien Alloys | also unlocks Grappling Hook manufacture |
| "Guardian" Heavy Armor (`STR_HEAVY_ARMOR`) | 650 | Alien Alloys | |
| "Shadow" Stealth Armor (`STR_STEALTH_ARMOR`) | 900 | UFO Power Source + **Chryssalid Corpse** | |
| "Aegis" Assault Armor (`STR_ASSAULT_ARMOR`) | 800 | UFO Power Source + **Muton Corpse** | |
| "Wraith" Flying Armor (`STR_FLYING_ARMOR`) | 1200 | **Hover Tank** + Stealth Armor | |
| "Paladin" Power Armor (`STR_POWER_ARMOR`) | 1100 | **Heavy Tank** + Assault Armor | |

**Advanced Armor** (`STR_ADVANCED_ARMOR`, 1000; deps Fusion Power + UFO Construction) is a
separate keystone that gates the late craft (Firestorm/Avenger/Grav Shield) and both tanks
— despite the name it is a craft/vehicle tech, not a suit.

## Equipment & support tech

- Motion Scanner (180) and Medi-Kit (210) — researchable from start (as vanilla).
- **Combat Lights** (`STR_BATTLE_LIGHTS`, 210; dep Laser Cannons) → Headlamp, HWP
  Headlights. **Night Vision** (500; deps UFO Power Source + UFO Construction + Plasma
  Cannons) → Night Vision Goggles, HWP Night Vision Module. (New night-ops line.)
- **Alien Medicine** (free; from Floater Medic or Sectoid Medic interrogation) + **Alien
  Surgery** (150, needs item) → **Field Surgery Unit** (550) → manufacturable battlefield
  surgery item.
- Mind Probe (600, needs item) — now manufacturable after research.

## Psionics branch

- **Psionics** (`STR_PSIONICS`, 420) — unlocked by interrogating a Sectoid Leader/Commander
  or any Ethereal (Soldier/Leader/Commander).
- **Psi-Lab** (`STR_PSI_LAB`, 420) — deps **Medi-Kit + Hyper-Wave Decoder + Psionics**
  (harder than vanilla's straight Psi-Lab-from-Sectoid-Leader) → Psionic Laboratory
  facility.
- **Psi-Amp** (500; deps Psi Lab + Mind Probe) → Psi-Amp and **Psi Orb** (`STR_PSI_ORB`,
  new psionic item) manufacture. **Mind Shield** (360; deps Psi Lab + Mind Probe).

## Craft & detection branch

- **Improved Interceptor** (`STR_IMPROVED_INTERCEPTOR`, 500; dep Alien Alloys) → **Tornado**
  craft; also a *prerequisite for UFO Navigation* research.
- UFO Power Source (450, needs item; dep Fission Weapons) + UFO Navigation (450, needs
  item; dep Improved Interceptor) → **UFO Construction** (450).
- **New Fighter-Transporter** (`STR_NEW_FIGHTER_TRANSPORTER`, 700; dep UFO Construction) →
  **Lightning**. **New Fighter Craft** (750; deps NFT + Advanced Armor) → **Firestorm**.
  **Ultimate Craft** (900; deps NFT + Advanced Armor) → **Avenger**. **Grav Shield** (930;
  deps NFT + Advanced Armor). Note vanilla's order is inverted: the transporter now comes
  *before* the fighter.
- **Hyper-wave Transmissions** (`STR_HYPER_WAVE_TRANSMISSIONS`, 350; new intermediate,
  unlocked by interrogating any Navigator) → **Hyper-Wave Decoder** (400; deps Motion
  Scanner + HW Transmissions).

## Interrogation & lore chain

Structure follows vanilla (interrogations unlock `STR_ALIEN_ORIGINS` → Leader Plus →
`STR_THE_MARTIAN_SOLUTION` → Commander Plus → `STR_CYDONIA_OR_BUST`) with generous
`getOneFree` loot added:

- **Navigators** (any race) — unlock Hyper-wave Transmissions; free alien-mission intel
  article. **Engineers** — free UFO-type article. **Medics** — unlock Alien Medicine; free
  live-alien or autopsy topic. **Leaders/Commanders** — unlock Leader Plus/Commander Plus;
  Sectoid Leader/Commander and all Ethereals also unlock Psionics.
- All corpse autopsies are researchable (180 each, 50 pts); terrorist captures (170) unlock
  Alien Origins like crew do.
- Ethereal Commander interrogation is additionally required for the Blaster Launcher (see
  above).

## HWP branch (from `Vehicles.rul`, all-new)

A modular vehicle system replacing the four vanilla tanks (tank research/manufacture and
Ufopaedia entries deleted). Root: **Modular HWP Upgrades** (`STR_MODULAR_HWP_UPGRADES`,
50) opening parallel mini-chains, each ending in manufacturable modules:

- **Armor:** Heavy HWP Armor (200) → Alien Alloy HWP Armor (700, +UFO Construction) —
  armor plates (light/heavy, alloy variants).
- **Frame:** HWP Frame Reinforcement (220) → Frame Alloy Reinforcement (500, +UFO
  Construction) — reinforcement add-ons.
- **Targeting:** HWP Targeting Improvement (250) → Hyperwave HWP Targeting (800, +HW
  Transmissions) — targeting modules.
- **Response:** HWP Response Enhancement (250) → Hyperwave HWP Response Booster (800, +HW
  Transmissions).
- **Engines:** Large HWP Engine (500) → Supercharged HWP Engines (700) → Elerium HWP
  Engines (900, +UFO Power Source) — a matrix of S/M/L, supercharged, and X (Elerium)
  engines.
- **Guns:** Heavy HWP Cannon (250), HWP Artillery (400) — heavy cannon and 4-shell
  artillery system (HE/cluster/incendiary/smoke), plus fission/fusion shells from the
  explosives branch and laser/plasma HWP guns from the weapon branches.
- **Chassis:** **Heavy Tank** (`STR_HEAVY_TANK`, 2000) and **Hover Tank**
  (`STR_HOVER_TANK`, 2000), both dep **Advanced Armor**; manufactured ($600k/40 space and
  $700k/30 space resp., Alloys + Elerium). They gate Power/Flying armor and the Smart
  Launcher. (A Scout Car and Medium Tank exist as starting vehicles in `Vehicles.rul`.)

## Manufacture summary

Deleted vanilla projects: laser/plasma hovertanks and Tank/Laser Cannon, HWP Fusion Bomb,
Heavy Laser, Heavy Plasma, Blaster Launcher/Bomb (re-added later in the tree with new
gates). What is buildable, grouped (workshop `space` / eng-hours / $ cost; materials in
Alloys **A** / Elerium **E**):

- **Laser weapons** (no special materials): Laser Pistol 300h/$8k; Pulse Laser Pistol
  400h/$15k; Laser Rifle 400h/$20k; Prismatic Laser 300h/$18.6k; Strike Beam Rifle
  450h/$27.1k; Precision Beam Rifle 500h/$28.6k; Pulse Laser 550h/$29.5k; Hyper Pulse
  Laser 650h/$37.4k; Heavy Laser Rifle 700h/$32k; Electrolaser 450h/$25k + Cells.
- **X-COM plasma weapons** (1–2 A each; ammo 1–5 E per clip): Plasma Pistol $56k → Heavy
  Plasma Rifle $160k, Plasma Pulsar $172k topping the line; every weapon has a separate
  Elerium-fueled ammo project (a change from vanilla's clip-scavenging).
- **Arc weapons:** Arc Rifle 1100h/$178k (5E 2A), Arc Launcher 2000h/$251k (12E 5A).
- **Ordnance:** Fission/Fusion Rockets, Plasma Rocket, Fission/Fusion Explosives, Alien
  Grenade, Shock Grenade, GL fission/fusion grenade batches (`producedItems: 6` per run —
  batch production), Stun Bomb, Small Launcher, Blaster Launcher/Bomb.
- **Equipment:** Motion Scanner, Medi-Kit, Field Surgery Unit, Grappling Hook, Headlamp,
  Night Vision Goggles, HWP Headlights/NV Module, Mind Probe, Psi-Amp, Psi Orb.
- **Armor:** the six suits, 5–40 A + 6–20 E, up to 4500h (Wraith) / $280k.
- **Craft & craft weapons:** Tornado/Lightning/Firestorm/Avenger (see Craft doc); Laser
  Cannon $182k, Plasma Cannon $226k (15E), Fusion Ball Launcher $242k + Fusion Balls (4E).
- **UFO components:** Alien Alloys, UFO Power Source, UFO Navigation (as OXCE allows).
- **HWP:** both tank chassis, 16 engine variants, all HWP weapons/ammo/armor/add-on
  modules listed above.

Data quirk: two manufacture entries write their double requirement as a single
comma-joined string (`requires: [STR_SMALL_LAUNCHER,STR_STUN_BOMB]` and
`[STR_BLASTER_LAUNCHER,STR_BLASTER_BOMB]`) — presumably parsed by the legacy engine as two
topics, but as written it is one malformed id (?).
