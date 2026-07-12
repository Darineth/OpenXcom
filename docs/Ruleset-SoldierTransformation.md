# Ruleset: `soldierTransformation:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`RuleSoldierTransformation`](../src/Mod/RuleSoldierTransformation.h) ·
**List key:** `name` · **Loader:**
[`RuleSoldierTransformation::load`](../src/Mod/RuleSoldierTransformation.cpp)

A soldier transformation is a **base project performed on one soldier**: training, cybernetic
augmentation, cloning, resurrection, turning a corpse into an item — anything that consumes a
soldier (dead, wounded or fit) plus money/items/time and produces a changed soldier. It is the
mechanism behind every "convert soldier A into soldier B" feature. Available projects appear on the
soldier's transformation list once their `requires` research is done.

```yaml
soldierTransformation:
  - name: STR_CYBERNETIC_AUGMENTATION
    requires:
      - STR_CYBERNETICS
    requiresBaseFunc: [ PSILAB ]
    producedSoldierType: STR_CYBORG
    allowsLiveSoldiers: true
    allowsWoundedSoldiers: true
    minRank: 1
    cost: 250000
    recoveryTime: 20
    requiredItems:
      STR_ALIEN_ALLOYS: 10
    flatOverallStatChange:
      health: 10
      strength: 15
    percentGainedStatChange:
      firing: -50          # lose half of the firing skill earned in the field
    soldierBonusType: STR_BONUS_CYBORG
```

Entries **merge** across mods/files by `name`, support `refNode:` inheritance and `delete: true`
(see the [index](Ruleset.md#how-rulesets-load-the-60-second-version)).

## Availability & cost

| Key | Type | Default | Meaning |
|---|---|---|---|
| `name` | string | — | Unique id; also the display-name string key of the project. |
| `refNode` | map | — | Inherit all fields from another (anonymous) node first. |
| `requires` | list of research | — | [Research](Ruleset-Research.md) needed before the project appears. |
| `requiresBaseFunc` | list of base functions | — | Base functions ([facilities](Ruleset-Facilities.md)) the base must provide. |
| `cost` | int | 0 | Cash cost per transformation. |
| `requiredItems` | map item → int | — | [Items](Ruleset-Items.md) consumed from base stores. |
| `requiredCommendations` | map commendation → int | — | [Commendations](Ruleset-Commendations.md) (and their decoration level) the soldier must hold. |
| `transferTime` | int hours | 0 | Time the finished soldier spends in transit back to the base. |
| `recoveryTime` | int days | 0 | Wound-recovery days the soldier gets on completion. |
| `listOrder` | int | auto | Sort position in the project list. |

## Who is eligible

| Key | Type | Default | Meaning |
|---|---|---|---|
| `allowedSoldierTypes` | list of soldier types | all | Restricts input to these [soldier types](Ruleset-Soldiers.md). |
| `allowsLiveSoldiers` | bool | false | Living, unwounded soldiers may be used. |
| `allowsWoundedSoldiers` | bool | false | Soldiers in wound recovery may be used. |
| `allowsDeadSoldiers` | bool | false | Dead soldiers (memorial entries) may be used. |
| `needsCorpseRecovered` | bool | true | With `allowsDeadSoldiers`, the body must actually have been recovered. |
| `minRank` | int | 0 | Minimum soldier rank (0 = rookie). |
| `requiredMinStats` | [UnitStats](Ruleset-UnitStats.md) | all 0 | Minimum stats the soldier must have. |
| `requiredMaxStats` | [UnitStats](Ruleset-UnitStats.md) | all 9999 | Maximum stats the soldier may have (above this it's ineligible). |
| `includeBonusesForMinStats` | bool | false | Count [soldier bonuses](Ruleset-SoldierBonuses.md) when checking `requiredMinStats`. |
| `includeBonusesForMaxStats` | bool | false | Count soldier bonuses when checking `requiredMaxStats`. |
| `requiredPreviousTransformations` | list of names | — | Other transformations the soldier must already have undergone. |
| `forbiddenPreviousTransformations` | list of names | — | Transformations that disqualify the soldier. |

## What comes out

| Key | Type | Default | Meaning |
|---|---|---|---|
| `producedSoldierType` | string soldier type | same type | The [soldier type](Ruleset-Soldiers.md) the soldier becomes. |
| `producedSoldierArmor` | string armor | — | [Armor](Ruleset-Armors.md) the produced soldier ends up wearing. |
| `keepSoldierArmor` | bool | false | Keep the soldier's current armor instead of resetting/replacing it. |
| `producedItem` | string item | — | The soldier ceases to exist entirely and is replaced by this [item](Ruleset-Items.md) in stores. |
| `createsClone` | bool | false | Produce a *new* soldier (new id) and leave the source soldier alone. |
| `soldierBonusType` | string bonus | — | [Soldier bonus](Ruleset-SoldierBonuses.md) granted on completion. |
| `removeTransformations` | list of names | — | Previous transformations (and their bonuses) erased by this one. |
| `reset` | bool | false | Wipe *all* previous transformation records and assigned soldier bonuses. |
| `resetRank` | bool | false | Reset the produced soldier's rank to rookie. |
| `events` | map event → weight | — | Weighted pick of a geoscape [event](Ruleset-Events.md) to fire after the transformation completes. |

## Stat changes

The change applied to the soldier's current stats is the **sum of six terms** (see
[`Soldier::calculateStatChanges`](../src/Savegame/Soldier.cpp)): a fixed flat part, a random flat
part, and the same pairing again as a percentage of the soldier's **current** stats and of its
**gained** stats (current − starting, i.e. what it earned in the field).

| Key | Type | Default | Meaning |
|---|---|---|---|
| `flatOverallStatChange` | [UnitStats](Ruleset-UnitStats.md) | all 0 | Fixed flat change added to every stat listed. |
| `flatMin` / `flatMax` | UnitStats | all 0 | Additional flat change, rolled uniformly per stat between min and max. |
| `percentOverallStatChange` | UnitStats (%) | all 0 | Fixed change as a percentage of the soldier's **current** stats. |
| `percentMin` / `percentMax` | UnitStats (%) | all 0 | Additional percentage of current stats, rolled between min and max. |
| `percentGainedStatChange` | UnitStats (%) | all 0 | Fixed change as a percentage of the soldier's **gained** stats (current − initial). |
| `percentGainedMin` / `percentGainedMax` | UnitStats (%) | all 0 | Additional percentage of gained stats, rolled between min and max. |
| `rerollStats` | UnitStats (flags) | all 0 | For clone projects: which stats are re-rolled fresh from the produced type's `minStats`/`maxStats` instead of carried over. |
| `showMinMax` | bool | false | Show the min/max range in the UI on two lines instead of a single line with `?` for randomized stats. |

Bravery is rounded to whole tens after summing.

### Bounds

| Key | Type | Default | Meaning |
|---|---|---|---|
| `lowerBoundAtMinStats` | bool | true | Penalties may not push a stat below the produced soldier type's `minStats`. |
| `upperBoundAtMaxStats` | bool | false | Cap the result at the produced type's `maxStats`. |
| `upperBoundAtStatCaps` | bool | false | Cap the result at the produced type's `statCaps` (used when `upperBoundAtMaxStats` is off). |
| `upperBoundType` | int | 0 | How the cap bites: 0 = dynamic (soft when the soldier type doesn't change, hard when it does), 1 = always soft (over-cap gains are compressed, not truncated), 2+ = always hard. |

## See also

- [`soldiers:`](Ruleset-Soldiers.md) — the input/output types, and the `minStats`/`maxStats`/`statCaps` the bounds refer to
- [`soldierBonuses:`](Ruleset-SoldierBonuses.md) — what `soldierBonusType` grants
- [`commendations:`](Ruleset-Commendations.md) — the `requiredCommendations` gate
- [UnitStats block](Ruleset-UnitStats.md) — the stat keys used throughout
