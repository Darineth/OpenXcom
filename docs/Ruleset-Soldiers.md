# Ruleset: `soldiers:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`RuleSoldier`](../src/Mod/RuleSoldier.h) · **List key:** `type` · **Loader:**
[`RuleSoldier::load`](../src/Mod/RuleSoldier.cpp)

A `soldiers:` entry defines a **recruitable soldier type** — the template the game rolls a new
`Soldier` from when you hire one: its random starting stats and growth caps, default armor, name
pools, rank ladder, salary, and voice/rank sprites. It is the player-side counterpart of
[`units:`](Ruleset-Units.md); the body it wears comes from [`armors:`](Ruleset-Armors.md).

```yaml
soldiers:
  - type: STR_SOLDIER
    minStats:                    # rolled per recruit, uniformly between min and max
      tu: 50
      stamina: 40
      health: 25
      bravery: 10
      reactions: 30
      firing: 40
      throwing: 50
      strength: 20
      psiStrength: 0
      psiSkill: 0
      melee: 20
    maxStats:
      tu: 60
      stamina: 70
      health: 40
      bravery: 60
      reactions: 60
      firing: 70
      throwing: 80
      strength: 40
      psiStrength: 100
      psiSkill: 0
      melee: 40
    statCaps:                    # hard ceiling for in-mission stat growth
      tu: 80
      firing: 120
      # ...
    armor: STR_NONE_UC
    costBuy: 40000
    costSalary: 20000
    standHeight: 22
    kneelHeight: 14
    soldierNames:
      - soldierNames/
```

Entries **merge** across mods/files by `type`, support `refNode:` inheritance and `delete: true`
(see the [index](Ruleset.md#how-rulesets-load-the-60-second-version)).

## Identity & availability

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | — | Unique id; also the display-name string key. |
| `refNode` | map | — | Inherit all fields from another (anonymous) node first. |
| `requires` | list of research | — | [Research](Ruleset-Research.md) needed before this soldier type can be hired. |
| `requiresBuyBaseFunc` | list of base functions | — | Base functions (provided by [facilities](Ruleset-Facilities.md)) the base must have to hire here. |
| `requiresBuyCountry` | string country | — | Only hirable while this [country](Ruleset-Countries.md) still funds X-COM. |
| `monthlyBuyLimit` | int | 0 | Maximum hires per month (0 = unlimited). |
| `monthlyBuyLimitMessage` | string key | — | Message shown when the monthly hire limit is hit. |
| `costBuy` | int | 0 | One-off hiring cost. |
| `costSalary` | int | 0 | Base monthly salary. |
| `costSalarySquaddie` / `costSalarySergeant` / `costSalaryCaptain` / `costSalaryColonel` / `costSalaryCommander` | int | 0 | Added to `costSalary` at that rank; defining any of them makes salary rank-dependent. |
| `transferTime` | int hours | 0 | Delivery time after hiring. |
| `value` | int | 20 | Score value (used in the "soldier lost" score penalty). |
| `group` | int | 0 | Soldier-type group id (used by UI grouping/filters). |
| `listOrder` | int | auto | Sort position in the hire/list screens. |

## Stats & growth

| Key | Type | Default | Meaning |
|---|---|---|---|
| `minStats` | [UnitStats](Ruleset-UnitStats.md) | all 0 | Lower bound of the random roll for a new recruit's stats. |
| `maxStats` | [UnitStats](Ruleset-UnitStats.md) | all 0 | Upper bound of that roll. |
| `statCaps` | [UnitStats](Ruleset-UnitStats.md) | all 0 | Ceiling that battle experience may raise a stat to. |
| `trainingStatCaps` | [UnitStats](Ruleset-UnitStats.md) | = `statCaps` | Separate (usually lower) ceiling for gym/psi-lab training. |
| `dogfightExperience` | [UnitStats](Ruleset-UnitStats.md) | all 0 | Per-stat improvement chances awarded to pilots after a dogfight. |

Stat sub-keys **merge**: a partial `maxStats:` only overrides the keys it names.

## Body, armor & equipment

| Key | Type | Default | Meaning |
|---|---|---|---|
| `armor` | string armor | — | **Required.** Default [armor](Ruleset-Armors.md) a fresh recruit wears (load error if missing). |
| `specialWeapon` | string item | — | Innate special weapon [item](Ruleset-Items.md) the soldier always carries (firearms/melee used this way must define their own `clipSize`, or load throws). |
| `standHeight` | int voxels | 0 | Height when standing. |
| `kneelHeight` | int voxels | 0 | Height when kneeling. |
| `floatHeight` | int voxels | 0 | Elevation above the floor when flying. |
| `moraleLossWhenKilled` | int % | 100 | Percentage modifier on the morale hit this soldier's death inflicts on the squad. |
| `allowPromotion` | bool | true | Whether the soldier participates in the promotion ladder. |
| `allowPiloting` | bool | true | Whether the soldier may be assigned as a craft pilot. |
| `skills` | list of skill names | — | Active [skills](Ruleset-Skills.md) this soldier type may use in battle. |
| `spawnedSoldier` | map | — | YAML template merged into a soldier of this type when one is *spawned* (e.g. recovered civilian, transformation output) rather than hired. |

## Names, looks & ranks

| Key | Type | Default | Meaning |
|---|---|---|---|
| `soldierNames` | list of paths | — | `.nam` name-pool files; a trailing `/` loads every `.nam` in that folder, and the literal entry `delete` clears inherited pools. |
| `femaleFrequency` | int % | 50 | Chance a new recruit is female. |
| `statStrings` | list | — | [StatString](Ruleset-StatStrings.md) definitions appended to this type's soldier names. |
| `rankStrings` | list of string keys | — | Custom names for the soldier's rank ladder (overrides the default Rookie…Commander strings). |
| `rankSprite` | sprite offset | 42 | Rank icon frame in `BASEBITS.PCK` (soldier lists). |
| `rankBattleSprite` | sprite offset | 20 | Rank icon frame in `SMOKE.PCK` (battlescape). |
| `rankTinySprite` | sprite offset | 0 | Rank icon frame in the `TinyRanks` set. |
| `skillIconSprite` | sprite offset | 1 | Skill-menu icon frame in `SPICONS.DAT`. |
| `showTypeInInventory` | bool | false | Show the soldier *type* next to the name on the inventory screen. |
| `armorForAvatar` | string armor | — | Armor whose paperdoll is used for the soldier's avatar portrait. |
| `avatarOffsetX` | int | 67 | X offset of the avatar image. |
| `avatarOffsetY` | int | 48 | Y offset of the avatar image. |
| `flagOffset` | int | 0 | Offset into the nationality-flag sprites. |

## Sounds

Each key takes a sound id or a list of ids (one is picked at random), resolved against `BATTLE.CAT`
/ [`extraSounds:`](Ruleset-ExtraSounds.md). All default to empty (the armor's / engine's sound is
used instead).

| Key | Type | Default | Meaning |
|---|---|---|---|
| `deathMale` / `deathFemale` | sound id(s) | — | Death scream. |
| `panicMale` / `panicFemale` | sound id(s) | — | Played when panicking. |
| `berserkMale` / `berserkFemale` | sound id(s) | — | Played when going berserk. |
| `selectUnitMale` / `selectUnitFemale` | sound id(s) | — | Voice on selecting the unit. |
| `startMovingMale` / `startMovingFemale` | sound id(s) | — | Voice on ordering a move. |
| `selectWeaponMale` / `selectWeaponFemale` | sound id(s) | — | Voice on picking a firing mode. |
| `annoyedMale` / `annoyedFemale` | sound id(s) | — | Voice when clicked repeatedly. |

The equivalent keys on [`armors:`](Ruleset-Armors.md#appearance--audio) override these when the
soldier is wearing that armor; see also [`unitResponseSounds:`](Ruleset-UnitResponseSounds.md).

## Scripting

Soldier types carry custom `tags:` (script values) — see
[Ruleset-Scripting.md](Ruleset-Scripting.md). The `RuleSoldier` script object exposes `getType`
plus the `StatsMin.` / `StatsMax.` / `StatsCap.` stat accessors.

## Notes

- The mana / health "missing → counts as wounded" thresholds are **not** per-soldier keys: they come
  from the mod-wide `manaWoundThreshold` / `healthWoundThreshold` constants.
- A soldier type with no usable `soldierNames:` pool logs a load error (total pool weight < 1).

## See also

- [UnitStats block](Ruleset-UnitStats.md) — the stat names used by `minStats`/`maxStats`/`statCaps`
- [`armors:`](Ruleset-Armors.md) — the body the soldier wears (`armor:`, `units:` gate)
- [`skills:`](Ruleset-Skills.md) — the active abilities listed in `skills:`
- [`soldierTransformation:`](Ruleset-SoldierTransformation.md) — converting one soldier type into another
- [`soldierBonuses:`](Ruleset-SoldierBonuses.md) / [`commendations:`](Ruleset-Commendations.md) — post-hire stat layers
- [`roles:`](Ruleset-Roles.md) **[DX]** — the player-authored role a soldier can be assigned
