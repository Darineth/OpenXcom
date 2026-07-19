# xcomtd Legacy Mod Reference: Soldier Armor and Soldiers

This document summarizes what the legacy "xcomtd" (X-COM: Terror Defense) master mod added or
changed versus vanilla X-COM: UFO Defense in the domain of soldier armor and soldiers. Sources
(legacy fork at `D:\Code\Projects\OpenXcomDX-Legacy`): `bin/standard/xcomtd/Ruleset/Armors.rul`,
`Soldiers.rul`, the armor item stubs in `Items.rul` (~lines 4480–4497), the custom status-effect
definitions in `Effects.rul`, display strings from `Language/en-US.yml` and the `extraStrings`
block in `Ruleset/Overhaul.rul` (which is where the armor names and Ufopaedia texts actually
live), and inventory layouts in `Inventories.rul`. Engine semantics for legacy-only keys were
confirmed against the legacy fork's source (`src/Mod/Armor.*`, `src/Mod/RuleEffect.h`,
`src/Savegame/BattleUnit.cpp`, `src/Battlescape/TileEngine.cpp`).

## Overview

The mod deletes vanilla's soldier armor line entirely — `delete:` entries remove
`STR_PERSONAL_ARMOR_UC`, `STR_POWER_SUIT_UC`, and `STR_FLYING_SUIT_UC` (plus the HWP armors
`STR_TANK_ARMOR` / `STR_HOVERTANK_ARMOR`, covered by the vehicle domain) — and replaces the
vanilla 3-suit progression (Coverall → Personal Armor → Power Suit / Flying Suit) with **six
researchable, role-specialized suits** plus the unarmored default. Suits differ not just in
armor values but in flat stat grants (`stats:`), percentage stat modifiers (`statModifiers:`,
a legacy-fork key), custom inventory layouts, and always-on status effects granted via the
legacy-fork `equippedEffects:` key (Stealth on the Shadow and Wraith suits).

## Armor table

Damage modifier order: 0=None, 1=AP, 2=Incendiary, 3=HE, 4=Laser, 5=Plasma, 6=Stun, 7=Melee,
8=Acid, 9=Smoke. A modifier of 0 means immunity. All seven suits use `loftempsSet: [3]`
(standard humanoid silhouette).

| Armor | Front/Side/Rear/Under | Damage modifiers (AP / Inc / HE / Las / Pla / Stun / Melee / Acid / Smoke) | Stat grants (flat) | Stat modifiers (%) | Special |
|---|---|---|---|---|---|
| `STR_NONE_UC` — "NONE" (default, no store item) | 8 / 5 / 3 / 1 | 1 / 1 / 1 / 1 / 1 / 1 / 1.2 / 1.6 / 1 | — | — | Standard inventory. Vulnerable to melee and acid. |
| `STR_SCOUT_ARMOR_UC` — "LOOKOUT SCOUT ARMOR" (item `STR_SCOUT_ARMOR`, "Scout Armor") | 25 / 22 / 20 / 20 | 0.9 / 0.8 / 1 / 1 / 1 / 0.9 / 0.9 / 1.2 / 0.8 | +12 health | — | `STR_SCOUT_INV` layout (adds a scout utility slot). No mobility penalty. |
| `STR_HEAVY_ARMOR_UC` — "GUARDIAN HEAVY ARMOR" (item `STR_HEAVY_ARMOR`, "Heavy Armor") | 40 / 35 / 20 / 20 | 0.7 / 0.5 / 0.6 / 1 / 0.95 / 0.8 / 0.7 / 1 / 0.6 | +30 health | TU −40% | `STR_HEAVY_INV` layout (adds a heavy utility slot). Thick front plate, thin rear. |
| `STR_STEALTH_ARMOR_UC` — "SHADOW STEALTH ARMOR" (item `STR_STEALTH_ARMOR`, "Stealth Armor") | 30 / 25 / 22 / 22 | 0.8 / 0.8 / 0.9 / 1 / 1 / 0.9 / 0.8 / 1.1 / 0.6 | +20 health | TU +10% | `equippedEffects: [STR_STEALTH]` — permanent Stealth status; `STR_SCOUT_INV` layout. |
| `STR_ASSAULT_ARMOR_UC` — "AEGIS ASSAULT ARMOR" (item `STR_ASSAULT_ARMOR`, "Assault Armor") | 50 / 35 / 25 / 25 | 0.5 / 0.2 / 0.5 / 1 / 0.95 / 0.4 / 0.6 / 0.9 / 0 | +50 health, +20 stamina, +15 strength | TU −35% | Smoke-immune; `STR_HEAVY_INV` layout; forced power-suit torso sprite. |
| `STR_FLYING_ARMOR_UC` — "WRAITH FLYING ARMOR" (item `STR_FLYING_ARMOR`, "Flying Armor") | 30 / 35 / 30 / 30 | 0.7 / 0 / 0.8 / 1 / 1 / 0.5 / 0.7 / 1 / 0 | +30 health | TU +20% | **Flight** (`movementType: 1`); `equippedEffects: [STR_STEALTH]`; fire- and smoke-immune; `STR_SCOUT_INV` layout; forced flying-suit torso. |
| `STR_POWER_ARMOR_UC` — "PALADIN POWER ARMOR" (item `STR_POWER_ARMOR`, "Power Armor") | 60 / 45 / 35 / 35 | 0.3 / 0 / 0.4 / 1 / 0.95 / 0 / 0.5 / 0.8 / 0 | +65 health, +40 stamina, +30 strength | TU −35% | Fire-, stun-, and smoke-immune; `STR_HEAVY_INV` layout; forced power-suit torso. No flight. |

Notes on legacy-fork keys (not present in stock OpenXcom/OXCE of that era):

- **`stats:`** on an armor adds flat stat bonuses to the wearer (mostly used here for health,
  effectively a hit-point buffer from the suit).
- **`statModifiers:`** is a percentage modifier applied after flat stats: the engine computes
  `stat * (mod + 100) / 100` (`UnitStats::mod` in `src/Mod/Unit.h`), so `tu: -40` = −40% TU,
  `tu: 20` = +20% TU. Item rules can carry the same keys; per `BattleUnit::calculateStats`,
  armor and carried-item modifiers sum before applying.
- **`equippedEffects: [ ... ]`** grants the listed status effects (see Effects section) for as
  long as the armor is worn. Items have a matching singular `equippedEffect:` key (used by the
  mod's Headlamp and Night Vision Goggles items, and by vehicle engine parts).
- **`inventoryLayout:`** selects a per-armor inventory screen layout (`inventoryLayouts:` in
  `Inventories.rul`): `STR_SCOUT_INV` = standard slots + head/torso/leg equip slots + a
  1-slot "scout utility" slot; `STR_HEAVY_INV` = same but with a "heavy utility" slot. The
  unarmored default uses `STR_STANDARD_INV`. (The equip/utility slots have `countStats: true`,
  i.e. items in them contribute their `stats`/`statModifiers` to the wearer.)
- Weight: no `weight:` key appears on any armor; encumbrance is expressed through the TU
  percentage modifiers instead.

Armor store items (`Items.rul`): all six suits are storage-only stubs — `size` 0.8–1.5,
`costSell` from $54k (Scout) / $65k (Heavy) through $195k (Assault) / $210k (Stealth) up to
$454k (Power) / $513k (Flying). None are buyable; all are manufactured after research
(research/manufacture chains are another agent's domain, but `Research.rul` shows the
ordering: Scout, Heavy, and Stealth are independent entries; Flying requires Stealth; Power
requires Assault).

Alien/civilian armors in the same file (SECTOID_ARMOR0, FLOATER_ARMOR0–2, SNAKEMAN_ARMOR0–2,
MUTON_ARMOR0–2, ETHEREAL_ARMOR0–2, CYBERDISC, REAPER, SECTOPOD, CHRYSSALID, ZOMBIE, SILACOID,
CELATID, CIVM/CIVF) are out of scope here; the notable pattern is that most alien races get
**three tiers** of armor (rank/difficulty scaling) and nearly all aliens are smoke-immune
(smoke modifier 0) while X-COM's unarmored soldiers are not.

## Progression and design roles

Vanilla offers a single linear ladder: nothing → Personal Armor → Power Suit → Flying Suit,
purely trading money for protection. xcomtd instead builds a **branching, role-based
wardrobe**, with each suit's Ufopaedia text (in `Overhaul.rul` `extraStrings`) spelling out its
niche:

- **"Lookout" Scout Armor** — the entry suit: light alien-alloy protection "without
  encumbering the wearer". Modest armor, +12 HP, zero mobility cost. Roughly the Personal
  Armor slot, but weaker (25 front vs vanilla Personal's 50) and cheap.
- **"Guardian" Heavy Armor** — the breaching/anchor suit "for soldiers pushing into fortified
  territory": strong front plate (40) that "will deflect light plasma weapons", weak rear (20),
  and a crushing −40% TU. High protection at a real mobility price — a tradeoff vanilla never
  asked players to make.
- **"Shadow" Stealth Armor** — from Chryssalid active-camouflage research: mid armor, +10% TU
  from its elerium core, and a permanent Stealth effect that halves enemy sight range against
  the wearer as long as they avoid loud/fast actions (see below). The infiltrator/flanker suit.
- **"Aegis" Assault Armor** — evolution of Heavy using Muton-skin-inspired alloy layering plus
  servo assists: 50 front armor, big flat grants (+50 HP, +20 stamina, +15 strength), strong
  resistances (0.2 incendiary, 0.4 stun, smoke-immune), still −35% TU. The frontline juggernaut
  short of true power armor.
- **"Wraith" Flying Armor** — the hybrid mobility suit: flight, +20% TU, fire/smoke immunity,
  and it *retains the Shadow's active camouflage* (also carries `STR_STEALTH`). Trades raw
  plate (30 front) for being fast, airborne, and hard to see — a very different beast from
  vanilla's Flying Suit, which was simply "Power Suit plus flight".
- **"Paladin" Power Armor** — the pinnacle: 60 front armor "rivalling the protection rating of
  small tanks", immune to fire, stun, and smoke, +65 HP/+40 stamina/+30 strength ("allowing the
  soldier to carry much more"), −35% TU. Notably it does **not** fly — flight and top
  protection are split between two endgame suits instead of stacking as in vanilla.

Net effect: instead of vanilla's strictly-better upgrades, the player fields a mixed squad —
fast scouts, stealthy flankers, slow tanks, and a flying skirmisher — and the mobility
modifiers (+20% to −40% TU) make suit choice a genuine tactical decision.

## Soldiers (`Soldiers.rul`)

`STR_SOLDIER` is a near-copy of the base xcom1 definition (same costs $40k buy / $20k salary,
same TU/stamina/bravery/reactions/firing/throwing/strength/psi/melee ranges and caps, same
heights and 25% female frequency) with these deltas:

| Field | Vanilla xcom1 | xcomtd |
|---|---|---|
| `minStats.health` | 25 | **20** |
| `maxStats.health` | 40 | **30** |
| `statCaps.health` | 60 | **45** |
| `soldierNames` | `SoldierName/` | `delete`, then `CelebrateDiversity/` (replacement name pool) |
| `levelExperience` | (absent) | **[500, 1000, 2000, 3000, 4000, 5000, 7500, 10000, 12500, 15000]** |

The health nerf is deliberate: soldiers' innate HP is low (cap 45 vs vanilla 60), and durability
is instead supposed to come from armor `stats.health` grants (+12 up to +65) — armor, not meat,
keeps soldiers alive.

`levelExperience` is a **legacy-fork soldier leveling system** (`RuleSoldier::_levelExperience`
in the legacy engine; UI strings `STR_LEVEL`/`STR_LEVEL_SHORT` in `en-US.yml`): the array gives
cumulative experience thresholds for 10 soldier levels. (The mechanics of what levels grant are
engine-side and out of scope for this ruleset summary.)

## The Effects system (`Effects.rul`) as it relates to armor

The legacy fork adds an `effects:` ruleset root (`RuleEffect` / `BattleEffect` in the engine) —
a general status-effect system with initial/ongoing/final components, durations, stacking, and
cancel triggers. Component `type` numbers map to the engine enum `EffectComponentType`
(1=stat modifier, 2/3/4=max health/energy/TU, 11/12/13=health/energy/TU damage-or-restore,
21=remove effect, 22=prevent effect, 30=stealth, 40=circular light, 41=directional light,
42=night vision). `cancelTriggers` numbers map to `EffectTrigger` (1=move, 2=walk, 3=sneak,
4=sprint, 5=turn, 10=activate, 11=attack, 12=melee, 13=shoot, 14=throw). `duration: -1` means
indefinite (for equipped effects, as long as the granting armor/item is worn).

Effects defined by the mod:

- **`STR_STEALTH` — "Stealth"** (granted by Stealth and Flying armor via `equippedEffects`):
  duration −1, `effectClass: STR_STEALTH`, ongoing component type 30 (stealth) magnitude **50**,
  cancel triggers [2, 4, 10, 11] = walking, sprinting, activating, attacking. Engine mechanics
  (`TileEngine`): a stealth magnitude of N reduces enemies' maximum view distance against the
  unit by N% (50 ⇒ enemies see the wearer at half range); magnitude ≥100 = invisible. Moving in
  *sneak* mode (a legacy-fork movement mode alongside run/strafe) does **not** break stealth;
  normal walking does. Stealthed soldiers render with a special translucent shader in the
  inventory screen.
- **`STR_INVISIBILITY` — "Invisibility"**: same triggers/duration but magnitude **100** (view
  distance 0 — cannot be seen until cancelled). Not referenced by any armor; presumably for
  items/testing.
- **`STR_NIGHT_VISION` — "Night Vision"**: duration −1, ongoing component type 42. Referenced
  not by armors but by items via `equippedEffect:` — the mod's `STR_NIGHT_VISION_GOGGLES` item
  and vehicle light-engine parts. (Engine support was partial: `TileEngine.cpp` computes the
  flag but carries a TODO about applying it to dark view distance.)
- **`STR_HEADLAMP` — "Headlamp"** (item `STR_HEADLAMP` via `equippedEffect:`): duration −1;
  ongoing components: type 41 (directional light) magnitude **19** (personal directional light
  radius/power 19) plus type 22 (prevent effect) with `affectsEffectClass: STR_STEALTH` — i.e.
  wearing a lit headlamp **suppresses any Stealth-class effect**, so you cannot combine the
  headlamp with Stealth/Flying armor camouflage. Units with an active light source are also
  barred from sneak movement by the pathfinder.
- **`STR_INCENDIARY_BURN` — "Burning"**: duration 2, `isNegative: true`, 5 incendiary damage
  (type 11) on application and per ongoing tick — the fork's reimplementation of catching fire
  as a status effect. **`STR_DAMAGE_TEST`** is a leftover 5-turn test effect (5 initial /
  10 ongoing incendiary, armor-ignoring).

So for armor purposes: `equippedEffects` + the effect system is how xcomtd implements active
camouflage on the Shadow/Wraith suits, and the same machinery powers the headlamp/night-vision
accessory items that occupy the new head-equip inventory slots. All of this is legacy-engine
functionality (no equivalent existed in stock OXCE of that era; modern OXCE has different
camouflage/vision keys).
