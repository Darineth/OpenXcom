# Legacy xcomtd — Non-Weapon Equipment

This document summarizes the non-weapon soldier gear defined by the legacy **xcomtd** ("X-COM: Terror Defense") master mod versus vanilla *X-COM: UFO Defense*. Sources: `OpenXcomDX-Legacy/bin/standard/xcomtd/Ruleset/Items.rul` (the `STR_MOTION_SCANNER` … `STR_PSI_ORB` range plus `STR_ELECTRO_FLARE`/`STR_GRAPPLING_HOOK`), `Ruleset/Effects.rul` (the equipped-effect definitions), `Ruleset/Vars.rul` (categories), and display names from `Language/en-US.yml` and `Ruleset/Overhaul.rul` (`extraStrings`). Weapons and HWP items are covered in separate documents.

Legacy-engine features used here (not in modern OXCE): `battleType: 13` (passive **equipped-effect items** worn in armor slots), `equippedEffect` (links an item to an effect in `Effects.rul`), `validSlots` (restricts an item to custom inventory slots, e.g. a head slot or the Scout Armor utility slot), `battleClipSize` (per-round base storage bundled into multi-charge clips in battle), and per-action psi costs in ammo charges (`costPanic`/`costMindControl`/`costClairvoyance`/`costMindBlast` with an `ammo:` component).

## Medical

### Medi-Kit (`STR_MEDI_KIT`)

Manufactured (sell $46,500). Same three-function design as vanilla but with explicit per-use recovery values and a **flat 10 TU** cost (`flatRate: true`).

| Stat | Value |
|---|---|
| Charges | Pain Killer 10, Heal 10, Stimulant 10 |
| Heal (per use) | 1 fatal wound, 3 health |
| Stimulant (per use) | 6 stun recovered, 10 energy |
| TU per use | 10 (flat) |
| Weight / size | 4 / 1×2 |

### Field Surgery Unit (`STR_FIELD_SURGERY_UNIT`)

**New item** — a straight upgrade of the Medi-Kit (sell $127,200): same charge counts (10/10/10) and flat 10 TU use, but roughly double effect per charge.

| Stat | Value |
|---|---|
| Heal (per use) | 2 fatal wounds, 7 health |
| Stimulant (per use) | 13 stun recovered, 22 energy |
| TU per use | 10 (flat) |
| Weight / size | 4 / 1×2 |

## Tools

### Motion Scanner (`STR_MOTION_SCANNER`)

As vanilla in function (`battleType: 7`). Weight 3, 25 TU per use, sell $45,600, manufacture-gated.

### Grappling Hook (`STR_GRAPPLING_HOOK`)

**New item**, tied to the Scout Armor: `requires: STR_SCOUT_ARMOR` and only equippable in the Scout Armor's custom utility slot (`validSlots: STR_SCOUT_UTILITY`). Mechanically implemented as a "firearm" with `power: 0`, custom `damageType: 11`, `arcingShot: true`, `maxRange: 5`, aimed shot only (110% / 50 TU), and an effectively-perfect `baseAccuracy: 132` — i.e. a short-range arcing utility shot (by inference, used for traversal/pulling rather than damage; the damage type 11 is a custom legacy type with no listed weapon using it for harm). Weight 3, not buyable or sellable.

### Electro-flare (`STR_ELECTRO_FLARE`)

As vanilla: throwable light source (`battleType: 10`), light power 15, weight 3, buy $60 / sell $40. Categorized under Grenades/Lights.

## Worn head equipment (equipped-effect items)

Both are `battleType: 13` items restricted to the head slot (`validSlots: STR_HEAD_EQUIP`), 2×1 size, weight 1 — passive effects while worn, a legacy-engine system with no vanilla/OXCE equivalent.

### Headlamp (`STR_HEADLAMP`)

Requires `STR_BATTLE_LIGHTS` ("Combat Lights") research; sell $500. Grants the `STR_HEADLAMP` effect from `Effects.rul`: a **personal light radius of 19** (effect component type 41, magnitude 19) and a component (type 22) that **suppresses/cancels `STR_STEALTH`-class effects** — i.e. wearing a lit headlamp breaks stealth (such as the Stealth Armor's camouflage).

### Night Vision Goggles (`STR_NIGHT_VISION_GOGGLES`)

Requires `STR_NIGHT_VISION` research; sell $75,000. Grants the `STR_NIGHT_VISION` effect (effect component type 42 — night vision, i.e. see in darkness without emitting light, by inference).

## Psionics

### Mind Probe (`STR_MIND_PROBE`)

As vanilla (`battleType: 8`): recovered alien device, displays a target unit's stats. 50 TU per use, weight 5, 2×2, sell $304,000, `recoveryPoints: 1`.

### Psi-Amp (`STR_PSI_AMP`)

Heavily reworked from vanilla. Now **two-handed**, weight 10, 1×3, sell $194,700, and — uniquely — **consumes ammunition**: it loads **Psi Orbs** (`compatibleAmmo: STR_PSI_ORB`). Every psi action costs a **flat 25 TU** (`flatRate: true`) plus a per-action number of orb charges:

| Psi action | Orb charges |
|---|---|
| Panic (`costPanic`) | 1 |
| Mind Control (`costMindControl`) | 4 |
| Clairvoyance (`costClairvoyance`) | 2 |
| Mind Blast (`costMindBlast`) | 0 |

Clairvoyance and Mind Blast are legacy-fork psi actions that do not exist in vanilla (remote viewing and direct psionic damage, by inference from their names; the DX project later re-implemented Mind Blast natively).

### Psi Orb (`STR_PSI_ORB`)

**New item** — the Psi-Amp's power source. Stored/produced per orb (`clipSize: 1`) but bundled into **12-charge clips in battle** (`battleClipSize: 12`). Weight 1, sell $36,700. Requires `STR_PSI_AMP` research like the amp itself.

## Miscellaneous

- **`STR_CORPSE`** — generic corpse item redefined (weight 22, 2×3, `recover: false`); race-specific corpses live with the units/armor rulesets. Nothing notable beyond the non-recovery flag on the generic entry.
- The vanilla **Electro-flare/Medi-Kit/Motion Scanner/Mind Probe** keys are kept; everything else in this document is new content or a rework.
