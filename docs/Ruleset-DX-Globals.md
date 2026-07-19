# Ruleset: DX global singletons **[DX]**

Back to the [ruleset index](Ruleset.md).

**Loaders:** [`Mod::loadFile`](../src/Mod/Mod.cpp) (the `// DX:` block near the end) ·
[`ArmorMoveCostDefaults` / `ArmorEvasionDefaults` / `ArmorSneakDefaults` / `ArmorBleedoutDefaults`](../src/Mod/Armor.h) ·
[`OverwatchDefaults`](../src/Mod/RuleItem.h) ([`::load`](../src/Mod/RuleItem.cpp))

Everything on this page is **[DX]** — added by OpenXcom DX, absent in stock OXCE/OXCE-Plus. These
are all **singletons**: one map each, merged field-by-field across mods, reset to the built-in
defaults on every mod reload.

Four of the six are *defaults* nodes: they set the mod-wide fallback that a per-armor / per-item
field overrides. Tuning them lets a mod reshape a mechanic game-wide without touching every rule.

```yaml
moveCostDefaults:
  sneakPercent: [200, 50]      # sneaking: double the TU per tile, walking's energy
  runPercent:   [50, 100]      # sprinting: half the TU, double walking's energy

evasionDefaults:
  sprint: { statPercent: 60,  tuPenaltyPercent: 50 }
  sneak:  { statPercent: 90,  tuPenaltyPercent: 0 }

sneakDefaults:
  maxLight: 5                  # no creeping while glowing

overwatchDefaults:
  overwatchRange:     20
  overwatchMinRange:  5
  overwatchConeAngle: 30
  overwatchShot:      aimed
  overwatchModifier:  80

bleedoutDefaults:
  deathHealthPercent: 50
  bufferWounds:       5

health:
  proportionalRecovery: true
  recoveryDaysMin:      20
  recoveryDaysMax:      30
  fieldSurgeryResearch: STR_FIELD_SURGERY_UNIT
  fieldSurgeryDaysMin:  15
  fieldSurgeryDaysMax:  25
```

Each node above is documented with its own example inline; see the linked per-root reference pages
for the full field tables.

---

## `health:`

**Feature:** [Proportional Wound Recovery + Field Surgery](../DX-Features.md#proportional-wound-recovery--field-surgery) ·
**Design doc:** [plans/Feature-ProportionalWoundRecovery.md](../plans/Feature-ProportionalWoundRecovery.md) ·
**Consumer:** [`BattleUnit::postMissionProcedures`](../src/Savegame/BattleUnit.cpp)

`health:` is an OXCE node (`woundThreshold`, `replenishAfterMission` — the mana node's twin) that DX
**extends** with a different wound-recovery model.

Stock OXCE recovery days are a roll on the **absolute** health lost
(`RNG(0.5 × loss, 1.5 × loss)`), which punishes tough soldiers: a 60 HP hit costs the same
convalescence whether the soldier has 40 or 120 max HP. With `proportionalRecovery: true`, recovery
scales with the **fraction** of max health lost:

```
days = healthLost × RNG(recoveryDaysMin, recoveryDaysMax) / maxHealth
```

so a full-HP wipeout is always ~`recoveryDays` days regardless of the soldier, and a tough soldier
convalesces faster from the same absolute damage. Once `fieldSurgeryResearch` is researched, the
whole roster switches to the shorter `fieldSurgeryDays*` band.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `woundThreshold` | int | 100 | *(OXCE)* Missing-health threshold above which a soldier counts as "wounded" in the UI/craft-eligibility checks. Per-soldier-type override: `RuleSoldier`'s own `woundThreshold`. |
| `replenishAfterMission` | bool | true | *(OXCE)* Whether health is refilled to max after a mission (recovery days are still computed from the loss). |
| `proportionalRecovery` **[DX]** | bool | false | Enable the fraction-of-max-health recovery model above (false = stock OXCE absolute-loss roll). |
| `recoveryDaysMin` **[DX]** | int days | 20 | Lower bound of the recovery band at 100% health loss. |
| `recoveryDaysMax` **[DX]** | int days | 30 | Upper bound of the recovery band at 100% health loss. |
| `fieldSurgeryResearch` **[DX]** | research name | `STR_FIELD_SURGERY_UNIT` | [Research topic](Ruleset-Research.md) that unlocks the shortened band. Empty string = never. If the topic isn't defined by any ruleset the gate simply stays dormant. |
| `fieldSurgeryDaysMin` **[DX]** | int days | 15 | Lower bound once Field Surgery is researched. |
| `fieldSurgeryDaysMax` **[DX]** | int days | 25 | Upper bound once Field Surgery is researched. |

Note the DX keys only take effect when `proportionalRecovery: true` — the Field Surgery gate rides on
the proportional model, not on the stock formula.

---

## `moveCostDefaults:`

**Feature:** [Mod-configurable Armor Move-Cost Defaults](../DX-Features.md#mod-configurable-armor-move-cost-defaults-movecostdefaults) ·
**Design doc:** [plans/Feature-MoveCostDefaults.md](../plans/Feature-MoveCostDefaults.md) ·
**Consumer:** [`Armor`](../src/Mod/Armor.h) getters, [`Pathfinding`](../src/Battlescape/Pathfinding.cpp)

The mod-wide fallback for an [armor's `moveCost:` block](Ruleset-Armors.md#movement). An armor that
omits a given key inherits it from here; this node itself defaults to the stock OXCE values. Tuning
it once retunes movement for every armor in the mod.

**Every value is a `[timePercent, energyPercent]` pair** — a percentage multiplier applied to the
tile's base TU / energy cost.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `basePercent` | `[time%, energy%]` | `[100, 100]` | Base scaling applied to every ground move. |
| `baseFlyPercent` | pair | `[100, 100]` | Base scaling for flying moves. |
| `baseClimbPercent` | pair | `[100, 100]` | Base scaling for climbing moves. |
| `baseNormalPercent` | pair | `[100, 100]` | Base scaling for plain (non-fly, non-climb) moves. |
| `walkPercent` | pair | `[100, 50]` | Walking. |
| `runPercent` | pair | `[75, 75]` | Sprinting ([DX sprint mode](../DX-Features.md#sprint--sneak-movement-modes)). |
| `strafePercent` | pair | `[100, 50]` | Strafing. |
| `sneakPercent` **[DX]** | pair | `[100, 50]` | Sneaking ([DX sneak mode](../DX-Features.md#sprint--sneak-movement-modes)). |
| `flyWalkPercent` | pair | `[100, 50]` | Flying at walking pace. |
| `flyRunPercent` | pair | `[75, 75]` | Flying at sprint pace. |
| `flyStrafePercent` | pair | `[100, 50]` | Flying while strafing. |
| `flyUpPercent` | pair | `[100, 0]` | Ascending under own power. |
| `flyDownPercent` | pair | `[100, 0]` | Descending under own power. |
| `climbUpPercent` | pair | `[100, 50]` | Climbing up (stairs/ramps). |
| `climbDownPercent` | pair | `[100, 50]` | Climbing down. |
| `gravLiftPercent` | pair | `[100, 0]` | Grav-lift travel. |

---

## `evasionDefaults:`

**Feature:** [Movement-mode Evasion](../DX-Features.md#movement-mode-evasion-sprintsneak-alter-reaction-fire-evasion) ·
**Design docs:** [plans/Feature-MovementModeEvasion.md](../plans/Feature-MovementModeEvasion.md),
[plans/Feature-ReactionScoringSplit.md](../plans/Feature-ReactionScoringSplit.md) ·
**Consumer:** [`Armor`](../src/Mod/Armor.h), [`TileEngine`](../src/Battlescape/TileEngine.cpp)

DX splits the reaction score into an **offensive** term (your chance to react-fire) and a
**defensive evasion** term (how hard you are to react-fire against). A mover's evasion is
`reactions × (currentTU / maxTU)` — this node reshapes that formula per movement mode:

```
evasion = reactions × statPercent/100
        × [ 1 − (1 − currentTU/maxTU) × tuPenaltyPercent/100 ]
```

`tuPenaltyPercent: 100` is the vanilla TU term; `0` removes the low-TU penalty entirely (full
evasion no matter how spent the unit is).

Two sub-maps, `sprint:` and `sneak:`, each with the same two keys. Armors override per-unit via
`evasionSprint:` / `evasionSneak:` (and scale the whole thing with
[`evasion:`](Ruleset-Armors.md#protection--damage-model)).

| Key | Type | Default | Meaning |
|---|---|---|---|
| `sprint: statPercent` | int % | 100 | Percent of the reactions stat a sprinting unit keeps for evasion. |
| `sprint: tuPenaltyPercent` | int % | 100 | How strongly low TU cuts a sprinting unit's evasion (100 = vanilla, 0 = none). |
| `sneak: statPercent` | int % | 100 | Percent of the reactions stat a sneaking unit keeps for evasion. |
| `sneak: tuPenaltyPercent` | int % | 100 | How strongly low TU cuts a sneaking unit's evasion. |

Defaults `{100, 100}` for both ⇒ evasion is exactly the plain reaction score, i.e. the feature is
inert until a mod configures it.

---

## `sneakDefaults:`

**Features:** [Sprint & Sneak Movement Modes](../DX-Features.md#sprint--sneak-movement-modes),
[Light Equipment](../DX-Features.md#light-equipment-directional-cone-light--sneak-light-gate) ·
**Design docs:** [plans/Feature-SprintSneakModes.md](../plans/Feature-SprintSneakModes.md),
[plans/Feature-LightEquipment.md](../plans/Feature-LightEquipment.md)

Mod-wide sneak-mode parameters. Currently one key: the **"no creeping while glowing" gate**.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `maxLight` | int | 5 | A unit emitting more light than this cannot enter sneak mode. |

The light counted is the unit's total emission: its armor's `personalLight` (when the per-unit
personal-light toggle is on), any carried lit light source (flare, torch, cone light), and being on
fire. With the default 5, the stock `personalLight: 15` **blocks sneaking** — toggle it off to creep,
or drop the lit item. Ordering a sneak move while glowing shows *"Emitting too much light to
sneak!"* and walks normally instead.

Whether an armor may sneak **at all** is the separate per-armor
[`allowsSneaking:`](Ruleset-Armors.md#movement) flag.

---

## `overwatchDefaults:`

**Feature:** [Overwatch](../DX-Features.md#overwatch-set-and-hold-reaction-fire-cone-based) ·
**Design doc:** [plans/Feature-Overwatch.md](../plans/Feature-Overwatch.md) ·
**Consumer:** [`RuleItem`](../src/Mod/RuleItem.h) getters

Overwatch is set-and-hold reaction fire over a directional cone. The per-weapon fields
(`overwatchRange:` etc. on an [item](Ruleset-Items.md)) are the real config; this node sets the
**mod-wide fallback for weapons that don't declare their own**.

The key names here are **identical to the item-level ones** (so the node reads like a weapon's
overwatch block).

| Key | Type | Default | Meaning |
|---|---|---|---|
| `overwatchRange` | int tiles | 0 | Maximum watched distance. **0 = overwatch unavailable** — this is why overwatch is opt-in per weapon by default; raising it here enables overwatch mod-wide. |
| `overwatchMinRange` | int tiles | 0 | Near dead zone: targets closer than this are not engaged. |
| `overwatchConeAngle` | int degrees | 40 | **Full width** of the watched cone (not the half-angle). |
| `overwatchModifier` | int % | 100 | Scales the offensive reaction score used for the overwatch trigger roll. |
| `overwatchShot` | enum | `snap` | Which shot the weapon reserves: `snap`, `burst`, `auto`, or `aimed`. Any other value falls back to `snap`. |

---

## `bleedoutDefaults:`

**Features:** [Bleedout & Indicators](../DX-Features.md#bleedout--indicators-negative-health-dying-state),
[Medikit / Stabilization Rework](../DX-Features.md#medikit--stabilization-rework-out-for-the-mission--target-readout) ·
**Design doc:** [plans/Feature-Bleedout.md](../plans/Feature-Bleedout.md)

With bleedout, a unit crossing 0 HP does not die: it enters a **negative-health dying state** and
only actually dies once its health reaches `−deathHealthPercent%` of its maximum. Entering bleedout
adds `bufferWounds` fatal torso wounds — the drain that keeps it sinking toward that threshold until
someone stabilizes it.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `deathHealthPercent` | int % | 50 | Death threshold as a percent of max health **below zero** (50 ⇒ dies at −maxHealth/2). |
| `bufferWounds` | int | 5 | Fatal torso wounds added when a unit first enters bleedout. |
| `lockoutForMission` | bool | true | A unit that ever bled out is **out for the rest of the mission**: it can be healed to survive (and is recovered at debriefing) but never revives to fight. `false` = it heals and rejoins normally. |

Whether a given unit can bleed out at all is the per-armor tri-state
[`canBleedOut:`](Ruleset-Armors.md#protection--damage-model) (absent = the engine's default rule
decides, i.e. soldiers yes / most aliens no).

## See also

- [Ruleset-Armors.md](Ruleset-Armors.md) — the per-armor overrides for move cost, evasion, cloak, bleedout
- [Ruleset-Items.md](Ruleset-Items.md) — the per-weapon `overwatch*` fields
- [DX-Features.md](../DX-Features.md) — feature-level descriptions of everything above
