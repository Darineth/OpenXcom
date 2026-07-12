# Ruleset: `soldierBonuses:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`RuleSoldierBonus`](../src/Mod/RuleSoldierBonus.h) · **List key:** `name` ·
**Loader:** [`RuleSoldierBonus::load`](../src/Mod/RuleSoldierBonus.cpp)

A soldier bonus is a **named stat layer** that can be attached to a soldier: extra stats, extra
armor, extra vision, better regeneration. Bonuses are never bought or equipped directly — they are
granted by a [`commendations:`](Ruleset-Commendations.md) award level or by a
[`soldierTransformation:`](Ruleset-SoldierTransformation.md) project, and a soldier can hold several
at once (their effects add up, on top of the [armor](Ruleset-Armors.md)'s own bonuses).

```yaml
soldierBonuses:
  - name: STR_BONUS_VETERAN
    stats:
      firing: 5
      reactions: 5
      bravery: 10
    frontArmor: 2
    visibilityAtDark: 3
    recovery:
      energy:
        flatOne: 2        # +2 energy regen per turn
```

Entries **merge** across mods/files by `name`, support `refNode:` inheritance and `delete: true`
(see the [index](Ruleset.md#how-rulesets-load-the-60-second-version)).

## Fields

| Key | Type | Default | Meaning |
|---|---|---|---|
| `name` | string | — | Unique id; also the display-name string key in the soldier's bonus list. |
| `refNode` | map | — | Inherit all fields from another (anonymous) node first. |
| `stats` | [UnitStats](Ruleset-UnitStats.md) map | all 0 | Flat stat bonus added while the soldier holds this bonus (sub-keys merge). |
| `frontArmor` | int | 0 | Added to the wearer's front armor value. |
| `sideArmor` | int | 0 | Added to **both** side armor values. |
| `leftArmorDiff` | int | 0 | Extra delta applied to the left side only (left bonus = `sideArmor + leftArmorDiff`). |
| `rearArmor` | int | 0 | Added to rear armor. |
| `underArmor` | int | 0 | Added to under armor. |
| `visibilityAtDay` | int tiles | 0 | Added to the soldier's daylight view distance. |
| `visibilityAtDark` | int tiles | 0 | Added to the soldier's night view distance. |
| `psiVision` | int tiles | 0 | Added to the soldier's psi-vision radius (sensing units through walls). |
| `heatVision` | int 0–100 | 0 | Added to the percentage of smoke the soldier sees through. |
| `visibilityThroughFire` | int | 0 | Added to the fire density the soldier can see through. |
| `recovery` | map of stat bonuses | none | Per-turn regeneration added by this bonus; sub-keys `time`, `energy`, `morale`, `health`, `stun`, `mana`, each a [stat bonus formula](Ruleset-StatBonus.md). |
| `listOrder` | int | auto | Sort position in the soldier's bonus list. |

All the numeric fields are **additive** on top of the armor's equivalents (see
[armors](Ruleset-Armors.md#protection--damage-model) and
[vision & stealth](Ruleset-Armors.md#vision--stealth) for what each one means); a soldier's total is
the sum over every bonus it holds.

## Scripting

Soldier bonuses accept the `SoldierBonusScripts` hooks under `scripts:` and carry custom `tags:` —
see [Ruleset-Scripting.md](Ruleset-Scripting.md). The `RuleSoldierBonus` script object exposes the
`Stats.` accessors.

## See also

- [`commendations:`](Ruleset-Commendations.md) — awards a bonus per decoration level (`soldierBonusTypes`)
- [`soldierTransformation:`](Ruleset-SoldierTransformation.md) — grants a bonus on completion (`soldierBonusType`)
- [`skills:`](Ruleset-Skills.md) — can require a bonus (`requiredBonuses`)
- [`armors:`](Ruleset-Armors.md) — can require a bonus to be worn (`requiresBonus`)
- [Stat bonus formulas](Ruleset-StatBonus.md) — the `recovery:` syntax · [UnitStats](Ruleset-UnitStats.md) — the `stats:` keys
