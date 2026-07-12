# Ruleset chunk: stat bonus formulas (`RuleStatBonus`)

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`RuleStatBonus`](../src/Mod/RuleStatBonus.h) · **Loader:**
[`RuleStatBonus::load`](../src/Mod/RuleStatBonus.cpp)

A *stat bonus* is OXCE's reusable "formula from the unit's stats" block. The same syntax appears
under many field names across the rules — wherever a number should scale with who is using the
thing rather than being a constant.

## Where it appears

| Host | Field(s) |
|---|---|
| [`items:`](Ruleset-Items.md) | `damageBonus`, `meleeBonus`, `accuracyMultiplier`, `meleeMultiplier`, `throwMultiplier`, `closeQuartersMultiplier`, `powerRangeReduction`… |
| [`armors:`](Ruleset-Armors.md) | `psiDefence`, `meleeDodge`, and the `recovery:` sub-keys (`time`, `energy`, `morale`, `health`, `stun`, `mana`) |
| [`soldiers:`](Ruleset-Soldiers.md) / [`soldierBonuses:`](Ruleset-SoldierBonuses.md) | soldier-level recovery variants |

The **field name is the node name** — e.g. an armor's psi defence formula is the map under
`psiDefence:`, an item's damage formula is the map under `damageBonus:`.

## Syntax

A stat bonus is a map of **term → coefficient(s)**. Each term contributes
`coefficient × value`; the total is the sum of all terms.

```yaml
    damageBonus:            # damage += 0.4*strength  (the classic "strengthApplied")
      strength: 0.4

    psiDefence:             # psiStrength + psiSkill/5
      psiStrength: 1.0
      psiSkill: 0.2

    accuracyMultiplier:     # a POLYNOMIAL: 0.5*firing + 0.01*firing^2
      firing: [0.5, 0.01]
```

Giving a term a **list instead of a scalar** supplies coefficients for successive **powers** of the
value, up to 4: `stat: [a, b, c, d]` means `a·v + b·v² + c·v³ + d·v⁴`. A scalar is shorthand for
the first coefficient.

Defining the node **replaces** the engine default for that formula entirely (the defaults are
listed per host field in its own doc — e.g. `psiDefence` defaults to `psiStrength: 1.0`).

### Advanced: script form

Instead of a map, the value may be a **string**: the name of a Y-Script
(`bonusStatsScripts`) that computes the bonus. See [Ruleset-Scripting.md](Ruleset-Scripting.md).

## Available terms

From [`statDataMap`](../src/Mod/RuleStatBonus.cpp). `v` is the term's value for the acting unit.

### Constants

| Term | Value |
|---|---|
| `flatOne` | 1 (a plain constant — `flatOne: 50` adds a flat 50) |
| `flatHundred` | 100 |

### Base stats ([UnitStats](Ruleset-UnitStats.md))

`tu`, `stamina`, `health`, `bravery`, `reactions`, `firing`, `throwing`, `strength`,
`psiStrength`, `psiSkill`, `melee`, `mana` — the unit's **max** (base+bonus) stat.

Combined products: `psi` (= psiSkill × psiStrength), `strengthMelee`, `strengthThrowing`,
`firingReactions`.

### Scaled variants

Every base/combined term also has a `…Scaled` form divided by 100 (products by 10000) —
`firingScaled` is `firing / 100`, handy for multiplier-style formulas: `firingScaled: 1.0` means
"× firing%".

### Current-state terms

| Term | Value |
|---|---|
| `healthCurrent`, `manaCurrent`, `tuCurrent`, `energyCurrent`, `moraleCurrent`, `stunCurrent` | The unit's **current** (not max) value |
| `healthNormalized`, `manaNormalized`, `tuNormalized`, `energyNormalized`, `moraleNormalized`, `stunNormalized` | Current ÷ max (0.0–1.0) |
| `fatalWounds` | Total fatal wounds |
| `rank`, `rankUnified` | Soldier rank index (unified spans soldier types) |
| `energyRegen` | The unit's basic per-turn energy regeneration |

### Example: a wound-sensitive dodge

```yaml
armors:
  - type: STR_EXAMPLE_ARMOR
    meleeDodge:
      reactions: 0.6
      fatalWounds: -5      # each wound costs 5 dodge
    recovery:
      energy:
        energyRegen: 1.0
        healthNormalized: [0, 20]   # up to +20 energy/turn at full health (quadratic)
```

## Gotchas

- Terms use the unit's **stats with bonuses applied** (armor `stats:`, soldier bonuses, [DX] item
  stat modifiers), not the bare soldier record.
- Unknown term names are a soft **load error** (logged, term ignored) — check the log if a formula
  seems inert.
- Coefficients are floats; results are computed at ×1000 fixed-point precision internally, so
  small fractions are fine.
- "Stats for Nerds" displays these formulas symbolically — useful to verify what actually loaded.

## See also

- [UnitStats block](Ruleset-UnitStats.md) — the stat names themselves
- [Ruleset-Scripting.md](Ruleset-Scripting.md) — replacing a formula with a script
- Host docs: [armors](Ruleset-Armors.md#stats-recovery--formulas) · [items](Ruleset-Items.md)
