# Legacy xcomtd ("X-COM: Terror Defense") — Content Summary

Reference documentation of the content the legacy **xcomtd** mod added on top of vanilla
*X-COM: UFO Defense*. Per its `metadata.yml`, the mod
is **"X-COM: Terror Defense" v0.1** (authors: Microprose, OpenXCOM, Xusilak, Darineth), a
*master* mod on `xcom1` — it deletes and rebuilds large parts of the vanilla content rather
than layering on top of it. Many of its ruleset keys are custom features of the legacy
OpenXcomDX engine fork (aim-cone accuracy, burst/dual fire, overwatch, status effects,
soldier roles/levels, modular vehicles); those are flagged in each document.

The mod was work-in-progress: version 0.1, many Ufopaedia texts are literal
`-PLACEHOLDER-` strings, a turn-based air-combat ruleset ships disabled, and a pilots
experiment is commented out.

## Documents

| Document | Covers |
|---|---|
| [Weapons.md](Weapons.md) | The hand-held arsenal: the 3-tier × 9-class matrix, launchers, grenades, stun weapons |
| [Equipment.md](Equipment.md) | Non-weapon gear: medical, vision/light, psi (Psi Orb ammo), grappling hook |
| [Armor-Soldiers.md](Armor-Soldiers.md) | The six named armor suits, soldier stat changes, the status-effects system |
| [Vehicles-HWPs.md](Vehicles-HWPs.md) | The modular chassis/engine/turret/armor-plate vehicle system and HWP weapon matrix |
| [Craft-Facilities.md](Craft-Facilities.md) | Craft rebalance + new Tornado, facilities, starting base, disabled air-combat minigame |
| [Research-Manufacture.md](Research-Manufacture.md) | The class-tier tech tree and manufacturing economy |
| [Aliens.md](Aliens.md) | Alien stat/resistance rework, rank-tiered armor, deployment "kits", mission scripting |
| [Systems.md](Systems.md) | Gameplay-system deltas: roles, action XP/levels, combat constants, commendations, inventory, terrain port |
| [Assets.md](Assets.md) | Custom art/sound/map/palette inventory: what was made, borrowed, missing, or unused |

## The big picture

### 1. A weapon *matrix*, not a weapon ladder

The vanilla arsenal (Pistol, Rifle, Heavy Cannon, Auto-Cannon, Heavy Laser, Heavy Plasma)
is deleted and replaced by a strict grid: three tech tiers × nine weapon classes, where
every tier repeats the same class chassis (same weight, hands, and overwatch behavior):

| Class | Role |
|---|---|
| Pistol | Cheap one-handed reaction/backup weapon, dual-fire capable |
| SMG | One-handed auto weapon, reaction-friendly |
| Assault Rifle | The all-rounder — only class with all four fire modes |
| Shotgun | 6-pellet close-quarters spread (ballistic tier can swap to slugs) |
| Marksman Rifle | Mid-long range, signature 2-round burst |
| Sniper Rifle | Extreme intrinsic accuracy, aimed-shot specialist, kneel-dependent |
| Machinegun | Sustained auto fire, kneel-to-hit, area overwatch |
| Minigun | 20-shot suppression hose — no single-shot modes, near-zero accuracy per shot |
| Heavy Rifle | 4-round anti-armor cannon, highest per-shot power, blocks both hands |

Tier scaling: **Ballistic** (`STR_BASIC_*`) is buyable from day one; **Laser** is
manufactured, never reloads (`clipSize: -1`), hits ~1.3× harder but fires ~10% slower;
**Plasma** hits ~1.65× harder and fires ~10% *faster* but is clip-fed and slightly less
accurate. Alien and X-COM plasma weapons are statistically identical twins — the split
exists purely so captured guns are usable early while the manufacturable versions (and the
shared clips!) sit behind a four-stage research spine.

Range identity comes from the legacy aim-cone accuracy model: `baseAccuracy` is each
weapon's hard cap on effective range in tiles (the 50%-hit distance), so miniguns are 6-tile
sprayers for any soldier while sniper rifles (120+) reward skill forever — see the computed
effective-range tables in [Weapons.md](Weapons.md) and [Vehicles-HWPs.md](Vehicles-HWPs.md).

Outside the matrix: a launcher family (new arcing Grenade Launcher, re-roled rockets —
HEAT/Thermobaric/Plasma — Small Launcher that also projects thrown grenades), a
"fission → fusion" elerium-ordnance upgrade ladder across grenades/rockets/demo charges,
and a waypoint mini-family (2-waypoint Arc Rifle / Arc Launcher) generalized from the
Blaster Launcher — which itself was nerfed and pushed to the very end of the tech tree.
Stun options grew from 2 to 4 (Stun Rod, Electrolaser, Shock Grenade, Stun Bomb).

### 2. Armor as a role wardrobe

Vanilla's linear Personal → Power → Flying upgrade is replaced by six named,
role-specialized suits: "Lookout" Scout, "Guardian" Heavy, "Shadow" Stealth (permanent
stealth effect), "Aegis" Assault, "Wraith" Flying (fast stealth skirmisher), "Paladin"
Power (top protection, deliberately *no* flight). Suits trade mobility (+20% to −40% TU)
against protection and stat grants; soldier base health was nerfed so durability comes
from the suit. Stealth/night-vision/headlamp mechanics run on a custom status-effects
engine.

### 3. Modular vehicles instead of fixed tanks

The four vanilla tank products are gone. Vehicles are chassis (Scout Car, Medium Tank,
Hover Tank, Heavy Tank) with hardpoint inventories: turret slots, a zero-TU ammo rack, an
engine bay (16 engine variants supply all TUs/carry capacity, plus addon modules), and
per-facing stackable armor plates. Turret weapons mirror the infantry matrix (Machine
Gun / Minigun / Light Cannon / Heavy Cannon × Basic/Laser/Plasma) plus missile, indirect
artillery, and smart-launcher families. Vehicles cost $20k/month upkeep.

### 4. An economy built on capture, manufacture, and class research

Research is per weapon *class*, not per item ("Laser Sidearms" → "Laser Assault Weapons"
→ …). The laser spine is pure lab work; the plasma spine requires possessing captured
alien weapons at each stage; fission/fusion ordnance requires elerium tech; the HWP tree
is a hub ("Modular HWP Upgrades") fanning into ~8 parallel human-stage → alien-stage
branches. Craft were re-roled (Tornado = new elerium-free interceptor, Firestorm = pure
fighter, Lightning/Avenger = slow tough transports) and base-defense facilities now fall
out of the weapon-tier research instead of standalone topics.

### 5. Aliens rebuilt as an escalating opponent

Alien stats stay near vanilla, but resistances were rewritten per race, leader/commander
ranks got real stat boosts, and race armor is rank-tiered. Deployments hand out the
9-weapon alien plasma family as role "kits" (line/close-quarters/support/precision) with
three item tiers that upgrade far more slowly than vanilla (full tier 2 at month ~19).
Terror ships carry two terrorist ranks, the terror-race schedule is month-scripted, and
Cydonia is a two-stage scripted boss battle.

### 6. A soldier-progression and simulation layer

Custom engine systems the content leans on: 12+ soldier roles with icons and armor
colors, per-action experience feeding a 10-level soldier ladder, the Soldier Diaries
commendations set (~60 medals), paperdoll inventory slots (head/torso/legs/utility),
narrowed damage rolls (50–150%), distance damage dropoff per damage type, doubled
encumbrance, and ordered overwatch. A large terrain library including TFTD-style
underwater terrains (SEABED/CORAL, sunken UFOs), a replaced globe, and custom battlescape
palettes round out the environment content.
