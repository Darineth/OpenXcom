# Ruleset chunk: the unit stats block (`UnitStats`)

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`UnitStats`](../src/Mod/Unit.h) · **Loader:**
[`read(ryml::ConstNodeRef, UnitStats*)`](../src/Mod/Unit.cpp)

`UnitStats` is the 12-field bag of soldier/unit attributes. It is not a root of its own: it is a
**map that appears under many field names** — anywhere a rule needs "a value for each stat"
(a soldier's starting range, an armor's bonuses, a transformation's stat change…).

```yaml
armors:
  - type: STR_POWER_SUIT_UC
    stats:                 # flat bonuses granted while worn
      tu: 5
      strength: 10
      firing: -5
    statModifiers:         # [DX] percentage modifiers (0 = unchanged)
      stamina: -10
```

## The 12 keys

Every stat key is optional; omitted keys are simply not touched. Values are integers
(`Sint16`; the engine clamps a unit's resulting max stat to `8000`, and stun to 4× that).

| Key | Meaning |
|---|---|
| `tu` | Time Units — the per-turn action budget. |
| `stamina` | Energy pool for moving/running. |
| `health` | Hit points. |
| `bravery` | Morale resistance (panic threshold). |
| `reactions` | Reaction-fire score (and [DX] overwatch scoring). |
| `firing` | Firing accuracy %. |
| `throwing` | Throwing accuracy %. |
| `strength` | Carry capacity, throw distance, melee/damage bonuses. |
| `psiStrength` | Psi defence base. |
| `psiSkill` | Psi attack skill (0 = cannot use psi). |
| `melee` | Melee accuracy %. |
| `mana` | Mana pool (OXCE mana system). |

## Where the block appears

| Host root | Field(s) | What the numbers mean there |
|---|---|---|
| [`soldiers:`](Ruleset-Soldiers.md) | `minStats`, `maxStats` | Inclusive range the recruit's starting stats are rolled from. |
| [`soldiers:`](Ruleset-Soldiers.md) | `statCaps` | Hard ceiling stats may grow to from combat experience. |
| [`soldiers:`](Ruleset-Soldiers.md) | `trainingStatCaps` | Ceiling for gym/psi-lab training (defaults to `statCaps` when absent). |
| [`soldiers:`](Ruleset-Soldiers.md) | `dogfightExperience` | Stat gain granted for dogfight participation (pilots). |
| [`units:`](Ruleset-Units.md) | `stats` | The unit's absolute stats (aliens, civilians, HWPs). |
| [`armors:`](Ruleset-Armors.md) | `stats` | Flat bonuses added to the wearer while worn. |
| [`armors:`](Ruleset-Armors.md) | `statModifiers` **[DX]** | Percentage modifiers applied to the wearer (`10` = +10%, `0` = unchanged). |
| [`items:`](Ruleset-Items.md) | `stats` **[DX]** | Flat bonuses granted while the item sits in an inventory slot. |
| [`items:`](Ruleset-Items.md) | `statModifiers` **[DX]** | Percentage modifiers granted while the item is equipped. |
| [`soldierBonuses:`](Ruleset-SoldierBonuses.md) | `stats` | Flat bonuses from a commendation/transformation bonus layer. |
| [`soldierTransformation:`](Ruleset-SoldierTransformation.md) | `requiredMinStats`, `requiredMaxStats` | Eligibility window for the project. |
| [`soldierTransformation:`](Ruleset-SoldierTransformation.md) | `flatOverallStatChange`, `percentOverallStatChange`, `percentGainedStatChange` | The stat change the project applies. |
| [`soldierTransformation:`](Ruleset-SoldierTransformation.md) | `rerollStats` | Which stats get re-rolled (non-zero = reroll). |
| [`crafts:`](Ruleset-Crafts.md) | `pilotMinStatsRequired` | Minimum stats a soldier needs to pilot the craft. |

## Semantics & gotchas

- **Merge, not replace.** Most hosts load the block with `merge()`: a key you *omit* keeps the
  inherited/previous value, and a key you set to **`0` is treated as "not set"** and therefore also
  keeps the previous value. To zero out an inherited stat you generally have to go through
  `refNode:`-free redefinition or a negative bonus — plain `firing: 0` is a no-op.
- **Flat vs percent.** `stats` is additive; **[DX]** `statModifiers` is a percentage delta around
  `0` (so `firing: 10` = +10%, `firing: -25` = −25%). DX item modifiers stack additively as
  percentages before being applied.
- **Item stats need a slot.** [DX] item `stats`/`statModifiers` only count while the item occupies a
  real inventory slot on the unit (see [`invs:`](Ruleset-Invs.md) `countStats`), and are recomputed
  whenever the loadout changes.
- Stat-bonus **formulas** (`psiDefence`, `damageBonus`, `recovery:`…) are a different chunk that
  *references* these names as terms — see [Ruleset-StatBonus.md](Ruleset-StatBonus.md).

## See also

- [Stat bonus formulas](Ruleset-StatBonus.md) — formulas whose terms are these stat names
- [`armors:`](Ruleset-Armors.md#stats-recovery--formulas) · [`items:`](Ruleset-Items.md) · [`units:`](Ruleset-Units.md)
- [DX-Features.md](../DX-Features.md) — the *Item Stats & Stat Modifiers* feature
  ([plans/Feature-ItemStatsModifiers.md](../plans/Feature-ItemStatsModifiers.md))
