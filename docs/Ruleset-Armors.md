# Ruleset: `armors:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`Armor`](../src/Mod/Armor.h) · **List key:** `type` · **Loader:**
[`Armor::load`](../src/Mod/Armor.cpp)

An armor entry is more than a wearable suit: it is the **physical definition of a battle unit's
body**. Soldiers get theirs from the equipped armor item; aliens, civilians and HWPs reference an
armor from their [`units:`](Ruleset-Units.md) entry. Protection, movement, vision, sprites, sounds
and hit-box geometry all live here.

```yaml
armors:
  - type: STR_STEALTH_SUIT_UC        # unique id; also the UFOpaedia/UI string key
    spriteSheet: XCOM_1.PCK          # battlescape sprite sheet
    spriteInv: MAN_1                 # inventory paperdoll base name
    storeItem: STR_STEALTH_SUIT      # the base-stores item that grants this armor
    corpseBattle: [ STR_CORPSE_ARMOR ]
    frontArmor: 50
    sideArmor: 40
    rearArmor: 30
    underArmor: 30
    camouflageAtDay: 6               # see "Vision & stealth" below
    camouflageAtDark: 3
    cloak: { dynamic: true, breaksOn: [attack] }   # [DX]
```

Entries **merge** across mods/files by `type`, support `refNode:` inheritance and `delete: true`
(see the [index](Ruleset.md#how-rulesets-load-the-60-second-version)).

## Identity & linked items

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | — | Unique id. Also used as the display-name string key (`STR_..._UC`). |
| `ufopediaType` | string | `type` | UFOpaedia article to open for this armor. |
| `refNode` | map | — | Inherit all fields from another (anonymous) node first. |
| `storeItem` | string | — | Item consumed/tracked in stores to equip this armor. `STR_NONE` = free (no item). |
| `corpseBattle` | list of items | — | Corpse item(s) left on the battlefield, one per tile of the unit (`size*size` entries for 2×2). **Required** — every armor must end up with battle and geo corpses or loading fails. |
| `corpseGeo` | string item | first `corpseBattle` | Corpse item recovered at debriefing. |
| `corpseItem` | string item | — | Legacy single-corpse shorthand: sets both `corpseBattle` (as a 1-entry list) and `corpseGeo`. Mutually exclusive with `corpseBattle`. |
| `selfDestructItem` | string item | — | Explosive detonated on death (e.g. Cyberdisc). |
| `builtInWeapons` | list of items | — | Items force-added to the unit's inventory at spawn (fixed weapons). Plain item-name list — [`weaponSets:`](Ruleset-WeaponSets.md) only apply to [`units:`](Ruleset-Units.md), not armors. |
| `specialWeapon` | string item | — | Innate special weapon (e.g. Zombie claws, psi organ). |
| `requires` | list of research | — | Research needed before soldiers may wear it ([research](Ruleset-Research.md)). |
| `requiresAward` / `requiresBonus` | lists | — | Commendation / soldier-bonus gates on wearing it. |
| `units` | list of soldier types | all | Restricts which [`soldiers:`](Ruleset-Soldiers.md) types may wear it. |
| `ranks` | list | all | Restricts wearing by soldier rank. |
| `isPilotArmor` | bool | false | Marks armor as pilot-only gear. |
| `group` | int | 0 | Armor group id (used by starting-condition filters). |
| `listOrder` | int | auto | Sort position in lists. |

## Protection & damage model

| Key | Type | Default | Meaning |
|---|---|---|---|
| `frontArmor` | int | 0 | Front side armor value. |
| `sideArmor` | int | 0 | Left **and** right side armor. |
| `leftArmorDiff` | int | 0 | Delta applied to the left side (asymmetric armor: left = `sideArmor + leftArmorDiff`). |
| `rearArmor` | int | 0 | Rear armor. |
| `underArmor` | int | 0 | Under armor (explosions beneath, falls). |
| `damageModifier` | list of up to 20 floats | all 1.0 | Damage multiplier per damage type, in [ResistType order](Ruleset-DamageTypes.md) (0 = none, 1 = AP, 2 = IN, 3 = HE, 4 = laser, 5 = plasma, 6 = stun, 7 = melee, 8 = acid, 9 = smoke, 10–19 = the OXCE custom types `DT_10`…`DT_19`). `0.0` = immune. |
| `overKill` | float | 0.5 | How much damage past death (× max HP) turns the corpse to goo (corpse destruction threshold). |
| `fearImmune` | bool | false¹ | Immune to morale-loss panic sources. |
| `bleedImmune` | bool | false¹ | Cannot receive fatal wounds. |
| `painImmune` | bool | false¹ | Wounds don't reduce accuracy/TU (no pain penalties). |
| `zombiImmune` | bool | false¹ | Cannot be zombified (2×2 units always immune). |
| `ignoresMeleeThreat` / `createsMeleeThreat` | bool | false / true¹ | CQB threat participation. |
| `canBleedOut` **[DX]** | bool | auto | Tri-state bleedout eligibility: absent = the mod-wide/legacy rule decides; `true`/`false` force it. See [DX-Features](../DX-Features.md). |
| `evasion` **[DX]** | int % | 100 | Defensive reaction-fire evasion scale — `> 100` = harder to react-fire against. Offense (own reactions) untouched. |
| `evasionSprint` / `evasionSneak` **[DX]** | map | from `evasionDefaults:` | Per-armor override of how sprint/sneak reshape evasion: `{ statPercent, tuPenaltyPercent }`, or the momentum model `{ evasionPercentPerTile, maxMomentumTiles }`. See [`evasionDefaults:`](Ruleset-DX-Globals.md#evasiondefaults). |

¹ Setting `size: 2` flips these to the big-unit defaults (fear/bleed/pain/zombi immune, no melee
threat) before the explicit keys apply.

## Movement

| Key | Type | Default | Meaning |
|---|---|---|---|
| `movementType` | int | 0 walk | 0 = walk, 1 = fly, 2 = slide, 3 = float, 4 = sink (`MT_*`; float/sink are the TFTD underwater modes). |
| `moveCost` | map of pairs | see below | Move-cost multipliers, each a `[time%, energy%]` pair. Sub-keys: `basePercent`, `baseFlyPercent`, `baseClimbPercent`, `baseNormalPercent`, `walkPercent`, `runPercent`, `strafePercent`, `sneakPercent` **[DX]**, `flyWalkPercent`, `flyRunPercent`, `flyStrafePercent`, `flyUpPercent`, `flyDownPercent`, `climbUpPercent`, `climbDownPercent`, `gravLiftPercent`. |
| `turnCost` | int | 1 | TU per 45° turn. |
| `turnBeforeFirstStep` | bool | false | Unit must fully face its path before stepping. |
| `allowsRunning` | bool | auto | May sprint. Default: engine rule (soldiers yes). |
| `allowsStrafing` | bool | auto | May strafe. |
| `allowsSneaking` **[DX]** | bool | auto | May sneak (Alt-move). DX shows "Cannot sneak in this armor!" when false. |
| `allowsKneeling` | bool | auto | May kneel. |
| `allowsMoving` | bool | true | May move at all (turrets: false). |
| `standHeight` / `kneelHeight` / `floatHeight` | int | from unit | Hit-box/LOS heights in voxels (−1 = inherit from the unit/soldier definition). |
| `size` | int | 1 | Footprint side length: 1 (1×1) or 2 (2×2). |
| `loftempsSet` (or single `loftemps`) | list of ints | — | LOFT template ids stacked bottom-to-top — the unit's 3-D collision silhouette. Effectively required: the count must equal `size`² or loading reports an error. |
| `meleeOriginVoxelVerticalOffset` | int | 0 | Tweak to where melee attacks originate vertically. |

Armors that omit `moveCost` keys fall back to the **[DX]** mod-wide
`moveCostDefaults:` node (see [DX globals](Ruleset-DX-Globals.md)), which itself defaults to the
stock values.

## Vision & stealth

This cluster is the most misunderstood part of the armor rules, so here are the real semantics,
straight from [`BattleUnit::getMaxViewDistance`](../src/Savegame/BattleUnit.cpp) and
[`TileEngine::visible`](../src/Battlescape/TileEngine.cpp).

| Key | Type | Default | Meaning |
|---|---|---|---|
| `visibilityAtDay` | int tiles | 0 → engine 20 | The **wearer's own** max view distance in daylight (0 = engine default: `maxViewDistance`, 20). |
| `visibilityAtDark` | int tiles | 0 → engine 9/20 | The wearer's max view distance in darkness (0 = default: 9 for X-COM, mod max — 20 — for aliens). |
| `camouflageAtDay` | int | 0 | How hard **this wearer** is to see in daylight. **Positive = an absolute cap, in tiles, on how far away anyone can spot the wearer. Negative = subtracted from the observer's view range.** 0 = no camouflage. |
| `camouflageAtDark` | int | 0 | Same, when the wearer's tile is dark. |
| `antiCamouflageAtDay` / `antiCamouflageAtDark` | int | 0 | Counter-camo on the **observer**: added back to the camo-reduced range, but never above the observer's own base range. |
| `cloak` **[DX]** | map | off | Makes the camouflage **dynamic** — see below. |
| `heatVision` | int 0–100 | 0 | % of smoke the wearer sees through (100 = smoke is invisible to it). (Stored internally as `visibilityThroughSmoke` — the YAML key is only `heatVision`.) |
| `visibilityThroughFire` | int | 100 | Same idea for fire density on the line of sight. |
| `psiVision` | int tiles | 0 | Sense **all** live units within N tiles, through walls, ignoring light/smoke. |
| `psiCamouflage` | int | 0 | Defense against psi-vision: positive = cap on the distance psi-vision works against this wearer, negative = subtracted from it. |
| `alwaysVisible` | bool | false | The unit never hides (drawn regardless of spotting). |
| `personalLight` | int | 15 | Light radius the wearer emits (friendly units). |
| `personalLightHostile` / `personalLightNeutral` | int | 0 | Per-faction personal light for hostiles/civilians. |

### How camouflage actually resolves

For each observer→target pair the engine computes the observer's effective max view distance
(day/dark chosen by the **target tile's** lighting):

```
range = observer.visibilityAt{Day|Dark}                # observer's own base range
if (target.camouflage > 0)  range = target.camouflage  # positive: absolute cap in tiles
else                        range += target.camouflage # negative: relative reduction
if (range < 1) range = 1                               # floor: adjacent is ALWAYS visible
range += observer.antiCamouflage                       # observer claws range back...
range = min(range, observer base range)                # ...but never beyond its own base
```

Consequences worth knowing:

- **Camouflage never affects line of sight or line of fire** — only the spotting distance.
- **`camouflageAtDay: 1` is the strongest legal stealth**: the wearer can only be spotted from an
  adjacent tile. True invisibility is impossible by design (the 1-tile floor).
- A **burning** target loses its camouflage entirely and is treated as lit.
- Because the check lives in the one `TileEngine::visible()` funnel, the **AI, reaction fire,
  overwatch, FOV and the visible-unit buttons** all honor camouflage with no extra config.
- `psiVision` bypasses the whole pipeline (its own `psiCamouflage` contest applies instead).

### [DX] Dynamic cloak

```yaml
    camouflageAtDay: 6      # the values that apply WHILE THE CLOAK IS UP
    camouflageAtDark: 3
    cloak:
      dynamic: true                            # opt in; omit the node for stock always-on camo
      breaksOn: [walk, run, attack, useItem]   # default set; also accepts: sneak, turn
```

With `dynamic: true` the camouflage above becomes a **state**: up at the start of the wearer's
turn, dropped until its next turn the moment it performs an action listed in `breaksOn`
(as reported through the unified [unit-action hook](../DX-Features.md#unit-action-reports-battlescapegameunitacted)).
Sneaking and turning are outside the default set — creep while cloaked, expose yourself when you
act. A unit with active camouflage renders as a checkerboard **ghost** (body and held items). See
[DX-Features.md](../DX-Features.md#stealth--cloaking-armor-dynamic-cloak--ghost-render) and the
design doc [plans/Feature-StealthArmor.md](../plans/Feature-StealthArmor.md).

## Stats, recovery & formulas

| Key | Type | Default | Meaning |
|---|---|---|---|
| `stats` | [UnitStats](Ruleset-UnitStats.md) map | all 0 | Flat stat bonuses granted while worn (`tu: 10`, `firing: -5`, …). |
| `statModifiers` **[DX]** | UnitStats map | all 0 | **Percentage** stat modifiers while worn (`firing: 10` = +10%). |
| `psiDefence` | [stat bonus](Ruleset-StatBonus.md) | psiStrength ×1 + psiSkill ×0.2 | Formula for resisting psi attacks. |
| `meleeDodge` | [stat bonus](Ruleset-StatBonus.md) | 0 | Formula for the chance to dodge melee. |
| `meleeDodgeBackPenalty` | float | 0 | Fraction of the dodge lost when attacked from behind. |
| `recovery` | map of stat bonuses | engine defaults | Per-turn regeneration formulas, sub-keys `time`, `energy`, `morale`, `health`, `stun`, `mana` — each a [stat bonus](Ruleset-StatBonus.md). |
| `weight` | int | 0 | Carried-weight penalty of the armor itself. |
| `instantWoundRecovery` | bool | false | Post-mission wound recovery is instant. |

## Appearance & audio

| Key | Type | Default | Meaning |
|---|---|---|---|
| `spriteSheet` | string | — | Battlescape unit sprite sheet (PCK). |
| `spriteInv` | string | — | Inventory paperdoll base name (per-look suffixes appended). |
| `allowInv` | bool | true | Whether the inventory screen is usable (false for e.g. dogs/tanks). |
| `inventoryLayout` **[DX]** | string | standard | The [inventory layout](Ruleset-InventoryLayouts.md) (slot set) this armor gives its wearer. |
| `drawingRoutine` | int | 0 | Which hardcoded body-part draw routine to use (soldier, cyberdisc, dog…). |
| `drawBubbles` | bool | false | Draw the breathing-bubbles animation (underwater). |
| `forcedTorso` | int | 0 | 0 gender-based, 1 always male torso, 2 always female torso. |
| `deathFrames` | int | 3 | Frames in the collapse animation. |
| `constantAnimation` | bool | false | Animate even when idle (e.g. Cyberdisc). |
| `customArmorPreviewIndex` | int/list | — | CustomArmorPreviews sprite(s) shown in UFOpaedia/inventory. |
| `layersDefaultPrefix` / `layersSpecificPrefix` / `layersDefinition` | strings/maps | — | Layered paperdoll system (composited paperdolls instead of one image). |
| `spriteFaceGroup` / `spriteHairGroup` / `spriteRankGroup` / `spriteUtileGroup` | int color group | 0 | Palette blocks (index/16) that the recolor pipeline may replace: face, hair, rank accent, and the "utile" accent block — **[DX] per-role armor colors recolor the utile group** (see [roles](Ruleset-Roles.md)). |
| `spriteFaceColor` / `spriteHairColor` / `spriteRankColor` / `spriteUtileColor` | list of ints | — | Replacement color per look (M0 F0 M1 F1 M2 F2 M3 F3). |
| `moveSound` | sound id | −1 | Movement sound override. |
| `deathMale` / `deathFemale` | sound ids | — | Death screams (per gender). |
| `selectUnitMale/Female`, `startMovingMale/Female`, `selectWeaponMale/Female`, `annoyedMale/Female` | sound ids | — | Voice response banks (see also [`unitResponseSounds:`](Ruleset-UnitResponseSounds.md)). |

## AI & targeting

| Key | Type | Default | Meaning |
|---|---|---|---|
| `specab` | int | 0 | Special ability: 0 none, 1 explode on death, 2 burn floor, 3 both. |
| `ai:` → `targetWeightAsHostile` etc. | ints | engine | How attractive this unit is as an AI target in each faction relation (`targetWeightAsHostile`, `targetWeightAsHostileCivilians`, `targetWeightAsFriendly`, `targetWeightAsNeutral`). |
| `allowTwoMainWeapons` | bool | false | AI may carry two main weapons. |

## Scripting

Armors expose Y-Script hooks (`scripts:` sub-node — e.g. `recolorUnitSprite`, `selectUnitSprite`,
`visibilityUnit`, `newTurnUnit`…) and custom `tags:`. See [Ruleset-Scripting.md](Ruleset-Scripting.md).

## See also

- [`units:`](Ruleset-Units.md) — the stat/AI half of non-soldier units that references an armor
- [`items:`](Ruleset-Items.md) — the store item (`storeItem`) and [DX] directional armor on items
- [Stat bonus formulas](Ruleset-StatBonus.md) — the `psiDefence`/`meleeDodge`/`recovery` syntax
- [DX-Features.md](../DX-Features.md) — feature-level docs for every **[DX]** field above
