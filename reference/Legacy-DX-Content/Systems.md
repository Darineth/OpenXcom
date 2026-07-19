# Legacy xcomtd — Gameplay Systems & Rules Changes

Reference summary of the gameplay-system deltas the legacy **xcomtd** master mod (plus its
legacy-engine fork features) introduced versus vanilla X-COM: UFO Defense. Sources:
`OpenXcomDX-Legacy/bin/standard/xcomtd/Ruleset/Overhaul.rul` (ufopaedia, roles, costVehicle,
extra assets), `Ruleset/Vars.rul` (experience/damage constants), `Ruleset/Soldiers.rul`
(levels), `Ruleset/Commendations.rul`, `Ruleset/Inventories.rul`, plus a short note on the
`Terrain.rul`/`Globe.rul`/`MapScripts.rul`/`Palettes.rul` terrain port and
`Language/en-US.yml` for display names. Several root keys used here (`roles:`,
`actionExperience:`, `constants:`, `invs:`/`inventoryLayouts:`, `customPalettes:`) are
**legacy-fork engine features**, not standard OXC/OXCE rulesets of that era; descriptions of
those are partly inferred from the ruleset content.

## Soldier roles system (`roles:` — legacy-engine feature)

Soldiers can be assigned a *role* with an icon and a default armor color. The fork's base
xcom1 rules define 12 roles; xcomtd's `Overhaul.rul` adds 3 more:

- Base roles (`xcom1/roles.rul`): `STR_ROLE_NONE` "No Role" (blank), "Infantry" (INF, blue),
  "Sniper" (SNP, black), "Scout" (SCT, yellow), "Rocketeer" (RCK, red), "Assault" (ASL,
  light purple), "Heavy Weapons" (HVY, brown), "Grenadier" (GRN, orange), "Medic" (MDC,
  white), "Psion" (PSI, purple), "Demolitions" (DMO), "Specialist" (SPC).
- xcomtd additions: `STR_ROLE_MACHINEGUNNER` "Machine Gunner" (MGN, dark green),
  `STR_ROLE_ANTIARMOR` "Anti-armor" (AAR), `STR_ROLE_MARKSMAN` "Designated Marksman" (MRK,
  gray).

Each role has `iconSprite` + `smallIconSprite` (23×23 and 16×16 PNGs under
`Resources/Roles/`) and an optional `defaultArmorColor` (a `STR_COLOR_*` key), implying the
engine auto-colored a soldier's armor by role and showed role icons in lists. Each role also
has a `_SHORT` string (3-letter tag) for compact UI.

## Action-based experience & soldier levels (`actionExperience:`, `levelExperience:`)

A custom per-action XP system (legacy-engine feature) replaces/extends vanilla's
stat-improvement rolls:

| Action | XP | Action | XP |
|---|---|---|---|
| `STR_SNAP_SHOT` | 30 | `STR_REACTION_FIRE` | 30 |
| `STR_AUTO_SHOT` / `STR_BURST_SHOT` | 50 | `STR_HEAL` | 30 |
| `STR_AIMED_SHOT` | 50 | `STR_STUN` | 100 |
| `STR_DUAL_FIRE` | 50 | `STR_KILL` | 75 |
| `STR_THROW` / `STR_MELEE` | 30 | `STR_PSIONICS` | 30 |
| `STR_WALK` | 1 | `STR_SNEAK` / `STR_SPRINT` | 2 |

Plus `baseMissionExperience: 50` per mission. Note the action list itself reveals other
legacy-engine combat features: **burst shot**, **dual fire** (two-weapon firing), **sneak**
and **sprint** movement modes.

`Soldiers.rul` gives soldiers 10 experience levels via
`levelExperience: [500, 1000, 2000, 3000, 4000, 5000, 7500, 10000, 12500, 15000]`.
(Soldier stat ranges are also tweaked — e.g. starting health 20–30, health cap 45 — see the
Armor/Soldiers reference doc.)

## Damage variance, dropoff and encumbrance (`Vars.rul`)

- `constants: damageRange: 50` — weapon damage rolls **50%–150%** of listed power instead of
  vanilla's 0–200%. A major consistency change: shots neither whiff for 0 nor double.
- `constants: encumbranceMultipler: 2` — an encumbrance system multiplier (legacy-engine
  feature; exact formula in engine code — presumably scales the TU/energy penalty for
  carrying more than strength allows).
- `damageDropoff:` — per-damage-type dropoff rates: AP (type 1) `0.5`, Laser (4) `0.5`,
  Plasma (5) `0.8`. A legacy-engine feature: damage attenuates with distance at a per-type
  rate (plasma keeps power at range better than AP/laser ?).
- `useCustomCategories: true` + a large `itemCategories:` taxonomy (damage types, weapon
  classes, individual weapon families, tools, vehicle parts) used to organize
  equip/purchase lists — the item taxonomy itself is covered in the Weapons reference doc.

## Commendations (`Commendations.rul`)

A full soldier medals/decorations system (the community "Soldier Diaries / Commendations"
ruleset bundled and tweaked — this later became OXCE's soldier-diary commendations). Medals
have up to 10 award levels (noted decoration levels), sprites, and localized names in en-US,
en-GB, fr, ru, it. Criteria groups:

| Group | Medals (display name) | Trigger criterion |
|---|---|---|
| Mission counts | Military Cross (missions), Defender Medal (base defenses), Terran Cross (alien base assaults), Longest Night Ribbon (night terror missions), Night Stalker (night missions), {region} Campaign Ribbon (missions in one region), Order of Earth (important missions), To Hell and Back Medal (all mission types), TECHINT Badge (all UFO types), Globetrotter (all regions) |
| Service time | XCOM Service Medal (1 month), Longevity Medal (3–30 months) |
| Kill totals | Merit Star (career kills 10–100), {race} Xenocide Medal (kills by race), {weapon} Proficiency Medal (kills with one weapon), Nike Cross (killed every alien type), Taking Names (kill of every rank), Armis Potens Medal (kills with every battle type) |
| Special kills (killCriteria filters) | Order of the Hammer (a soldier of each of the 5 races), Bolt's Cross (live Commander/Leader capture), Inferno Star (fire kills), Sapper Medal / Grenadier's Medal (explosive/grenade kills), Metal Menace Citation (Sectopods), Hercules Medal (terror units), Order of David (headshots?/sniper criteria), Enfilade Citation (flank kills), rear-shot medal, Prowler Cross (proximity-trap kills), Imperio (mind-control kills), Crucio (psi-panic kills), Master of Puppets (kills via MC'd aliens), Order of the Reaper (post-mortem kills), Medal of Sacrifice (martyr kills) |
| Captures & restraint | Order of Restraint (stun captures), Mercy Cross (stunned every alien type) |
| Medical / support | Star of Asclepius (shot at 10+ times unharmed), Man of Steel (hit 5+ times survived), Hippocratic Star (wounds healed), Field Surgeon (whole medikit used), Angel's Cross (revives) |
| Wounds & fate | Purple Heart (wounded), Crimson Heart (days wounded), Medal of Heroism (fell unconscious), Order of the Fallen (KIA), Order of the Forgotten (MIA), Black Cross (friendly fire!), Star of Valor (lone survivor), Iron Man (solo mission) |
| Skill feats | Marksman Citation (long-distance hits), Good Luck Citation (low-accuracy hits), Performance Citation (multi-kills per turn), Swiftness Citation (reaction-fire kills), Athena Citation (stat gain), Legion of Valor (bravery gain) |
| Career standing | Fallen Star (best of rank), Heroic Order (best soldier), First to Serve (original 8 — criterion effectively disabled at 999) |

## Inventory layout changes (`Inventories.rul` — legacy-engine feature)

The `invs:` root redefines battlescape inventory slots and adds a per-armor
`inventoryLayouts:` mechanism (vanilla has one fixed layout):

- **Soldier paperdoll additions**: HEAD / TORSO / LEGS equipment slots (1×2, marked
  `allowGenericItems: false`, `allowCombatSwap: false` — dedicated equipment such as armor
  plates/headlamps/goggles that can't be swapped mid-combat), and a UTILITY slot variant for
  Scout and Heavy armor. Slots gained legacy keys `countStats`, `textAlign`,
  `allowGenericItems`, `allowCombatSwap`.
- **Three personal layouts**: `STR_STANDARD_INV`, `STR_SCOUT_INV`, `STR_HEAVY_INV`
  (standard slots + head/torso/legs, scout/heavy add their UTILITY slot).
- **Vehicle inventories** (for the mod's modular HWP system): layouts `STR_SCOUT_CAR_INV`,
  `STR_HOVER_TANK_INV`, `STR_MEDIUM_TANK_INV`, `STR_HEAVY_TANK_INV` composed of TURRET 1/2
  weapon slots, a 6-slot AMMO rack (item moves between rack and turrets cost 0 TU via
  `costs:`), light/medium/heavy ENGINE grids (13–20 slots), EQUIPMENT slots (2/4/6), and
  per-facing ARMOR plate slots (front/left/right/rear/under with 1–3 plate capacity by
  weight class, each tagged with `armorSide`).

## Ufopaedia restructuring (`Overhaul.rul`)

- **Deletes** vanilla articles for replaced content: Pistol, Rifle, Heavy Cannon +
  AP/HE/I ammo, Auto-Cannon + ammo, Heavy Laser, Heavy Plasma, and all four vanilla
  HWPs/ammo.
- **Adds** articles for the mod's weapon families (ballistic Pistol/SMG/Assault
  Rifle/Shotgun/Marksman/Sniper/Machine Gun/Minigun/Heavy Rifle/Grenade Launcher; laser and
  plasma equivalents; Arc Rifle/Launcher; Electrolaser; rockets incl. Fission/Fusion/Plasma;
  Fission Explosive; Shock Grenade; Incendiary Grenade; Field Surgery Unit), each gated by
  the mod's new research topics (e.g. "Laser Sidearms", "Precision Plasma Weapons", "Heavy
  Plasma Weapons").
- Ammo articles are mostly hidden (`section: STR_NOT_AVAILABLE`).
- Adds armor articles for the six named suits: "Lookout" Scout, "Guardian" Heavy, "Shadow"
  Stealth, "Aegis" Assault, "Wraith" Flying, "Paladin" Power Armor, and the Tornado
  interceptor (requires "Improved Interceptor").
- Alien plasma weapons are displayed as "Alien Plasma X" with separate `STR_XCOM_PLASMA_*`
  names for X-COM manufactured copies.

## Other

- `costVehicle: 20000` — a global monthly cost (salary-like) for vehicles/HWPs
  (legacy-engine key, analogous to soldier salary; inferred).
- `extraSprites` / `extraSounds` / `extraStrings` in `Overhaul.rul` exist to register the
  large body of new art: BIGOBS/FLOOROB/HANDOB sprites for every new weapon, HWP weapon and
  addon items, armor-plate items, Stealth Armor sprite sheets and inventory paperdolls, role
  icons, Tornado craft art, a sniper-rifle fire sound, and English names/descriptions for
  everything (several still "-PLACEHOLDER-").

## TFTD-style terrain port (Terrain/Globe/MapScripts/Palettes)

`Terrain.rul` (~8,300 lines) imports a massive terrain library in the style of Hobbes'
Terrain Pack — airfields, Area 51 complexes, many urban variants (dawn, downtown, industrial,
island, native, port, railyard, slum...), farmland/cult sites, biome blends
(grassland/savanna/steppe/taiga/tundra × desert/forest/polar/mountain), ships (cargo ship,
liner), plus **TFTD underwater terrains**: `SEABED` and `SEACORAL` with sunken UFO / USO /
galleon / cargo-ship variants (the CORAL/SEABED map files ship in the mod's MAPS folder).
`MapScripts.rul` adds the matching map scripts, `Globe.rul` replaces the globe polygon data,
and `Palettes.rul` defines four `customPalettes` (`PAL_BATTLESCAPE_0`–`3`, a legacy-engine
feature) supplying alternate battlescape palettes for the ported environments. Terrain
content is otherwise out of scope here — this is only a pointer.
