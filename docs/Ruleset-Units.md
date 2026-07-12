# Ruleset: `units:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`Unit`](../src/Mod/Unit.h) · **List key:** `type` · **Loader:**
[`Unit::load`](../src/Mod/Unit.cpp)

A `units:` entry defines a **non-soldier battle unit** — aliens, civilians and HWP/tank units.
It is the *stat and behavior* half of the unit: its physical body (sprites, protection,
movement, hit box) comes from the [`armors:`](Ruleset-Armors.md) entry it references via
`armor:`. Player soldiers are defined by [`soldiers:`](Ruleset-Soldiers.md) instead.

```yaml
units:
  - type: STR_SECTOID_SOLDIER
    race: STR_SECTOID
    rank: STR_LIVE_SOLDIER
    stats:
      tu: 54
      stamina: 90
      health: 30
      bravery: 80
      reactions: 63
      firing: 52
      throwing: 58
      strength: 30
      psiStrength: 40
      psiSkill: 0
      melee: 76
    armor: SECTOID_ARMOR0
    standHeight: 16
    kneelHeight: 12
    value: 10
    deathSound: 10
    intelligence: 3
    aggression: 5
```

Entries **merge** across mods/files by `type`, support `refNode:` inheritance and `delete: true`
(see the [index](Ruleset.md#how-rulesets-load-the-60-second-version)).

## Identity & recovery

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | — | Unique id; also the display-name string key. |
| `refNode` | map | — | Inherit all fields from another (anonymous) node first. |
| `race` | string | — | Race id: links the unit into [`alienRaces:`](Ruleset-AlienRaces.md) member lists, names its living weapon (see `livingWeapon`), and feeds kill statistics. |
| `rank` | string | — | Rank string (e.g. `STR_LIVE_SOLDIER`) used for display, AI deployment roles and [commendation](Ruleset-Commendations.md) kill criteria. |
| `liveAlien` | string item | auto | Item recovered when the unit is captured alive. Default: the [item](Ruleset-Items.md) with the same name as `type`, if one exists; an explicit `""` disables recovery. |
| `capturable` | bool | true | Whether stunning and winning yields a live capture; false = only the corpse is recovered. |
| `civilianRecoveryType` | string | — | What saving this civilian recovers: `STR_ENGINEER`/`STR_SCIENTIST` (base personnel), a [soldier type](Ruleset-Soldiers.md), or an item type. |
| `spawnedPersonName` | string | — | Custom display-name string for the person recovered via `civilianRecoveryType`. |
| `spawnedSoldier` | map | — | YAML template merged into the soldier created when `civilianRecoveryType` is a soldier type. |
| `value` | int | 0 | Score awarded for killing/capturing it (and lost if it kills or escapes, per debriefing rules). |
| `moraleLossWhenKilled` | int % | 100 | Percentage modifier on the morale loss its death inflicts on its side. |
| `showFullNameInAlienInventory` | int | −1 | Alien-inventory title: 1 = full name ("Sectoid Leader"), 0 = race only; −1 = use the mod-wide default. |

## Stats & body

| Key | Type | Default | Meaning |
|---|---|---|---|
| `stats` | [UnitStats](Ruleset-UnitStats.md) map | all 0 | The unit's stats; non-zero sub-keys override inherited values (merge). |
| `armor` | string | — | **Required.** The [`armors:`](Ruleset-Armors.md) entry defining the unit's body (load error if missing). |
| `standHeight` | int voxels | 0 | Height when standing (hit box / LOS origin). |
| `kneelHeight` | int voxels | 0 | Height when kneeling. |
| `floatHeight` | int voxels | 0 | Elevation above the floor when flying. `standHeight + floatHeight` must not exceed 25 (load error). |
| `energyRecovery` | int | 30 | Energy regained per turn. |
| `specab` | int | 0 | Special ability: 0 none, 1 explode on death, 2 burn floor while moving, 3 both. |
| `spawnUnit` | string unit | — | Unit this one converts into on stun/kill/capture (e.g. Zombie → Chryssalid). |
| `canPanic` | bool | true | Whether the unit can panic at all. |
| `canBeMindControlled` | bool | true | Whether psi mind control can affect it. |
| `berserkChance` | int % | −1 | Chance that a panic event becomes berserk instead; −1 = vanilla 1-in-3. |

## AI behavior

| Key | Type | Default | Meaning |
|---|---|---|---|
| `intelligence` | int | 0 | Number of turns the AI remembers your troops' positions. |
| `aggression` | int | 0 | Chance of seeking revenge / not taking cover. |
| `spotter` | int | 0 | Turns that sniper-AI units may keep using this unit's spotting info; −1 = same as `intelligence`. |
| `sniper` | int % | 0 | Chance the unit acts on information gained by spotter units (fire from outside its own LOS). |
| `isLeeroyJenkins` | bool | false | AI charges the nearest enemy blindly. |
| `waitIfOutsideWeaponRange` | bool | false | Unit gets "stuck" trying to fire from outside weapon range (vanilla bug preserved as an option). |
| `pickUpWeaponsMoreActively` | int | −1 | 1/0 forces eager weapon pickup on/off; −1 = use the mod-wide AI default (separate defaults for hostiles and civilians). |
| `avoidsFire` | bool | auto | Penalize pathfinding through fire; default true unless `specab` ≥ 2 (floor burners don't care). |
| `vip` | bool | false | Marks a VIP unit (special debriefing handling). |
| `cosmetic` | bool | false | Purely decorative unit. |
| `ignoredByAI` | bool | false | The AI never targets this unit. |
| `canSurrender` | bool | false | Unit may surrender when conditions allow. |
| `autoSurrender` | bool | false | Unit surrenders automatically once every other enemy has surrendered too (implies `canSurrender`). |

## Built-in weapons & loadout

| Key | Type | Default | Meaning |
|---|---|---|---|
| `livingWeapon` | bool | false | Terrorist-style unit: skips the deployment item set and instead gets the fixed weapon item named `<RACE minus STR_>_WEAPON` (e.g. `SECTOID_WEAPON`). |
| `meleeWeapon` | string item | — | Built-in melee weapon (e.g. `STR_FIST`), always available in addition to any loadout. |
| `psiWeapon` | string item | `ALIEN_PSI_WEAPON` | Built-in psi weapon item used if the unit has psi skill. |
| `builtInWeaponSets` | list of item lists | — | Fixed items added at spawn; the set is picked by the mission's alien item level (index clamped to the last set). |
| `builtInWeapons` | list of items | — | Shorthand: appended to `builtInWeaponSets` as one more set. |
| `weightedBuiltInWeaponSets` | list of weighted maps | — | Per item level, a weighted random pick of a [`weaponSets:`](Ruleset-WeaponSets.md) name whose items are added at spawn. |

Built-in items here are *in addition to* the armor's own `builtInWeapons` (see
[armors](Ruleset-Armors.md#identity--linked-items)).

## Sounds

All sound fields accept a single id or a list (one is chosen at random), resolved against
`BATTLE.CAT` / [`extraSounds:`](Ruleset-ExtraSounds.md).

| Key | Type | Default | Meaning |
|---|---|---|---|
| `deathSound` | sound id(s) | — | Played on death. |
| `panicSound` / `berserkSound` | sound id(s) | — | Played when panicking / going berserk. |
| `aggroSound` | sound id(s) | — | Warcry when the AI spots/charges. |
| `selectUnitSound` / `startMovingSound` / `selectWeaponSound` / `annoyedSound` | sound id(s) | — | Voice response bank (see also [`unitResponseSounds:`](Ruleset-UnitResponseSounds.md)). |
| `moveSound` | sound id | −1 | Movement sound; overrides the armor's. |

## See also

- [`armors:`](Ruleset-Armors.md) — the body half: protection, movement, vision, sprites
- [`alienRaces:`](Ruleset-AlienRaces.md) — maps deployment ranks to unit types
- [`alienDeployments:`](Ruleset-AlienDeployments.md) — where and with what loadout units spawn
- [`soldiers:`](Ruleset-Soldiers.md) — the player-soldier equivalent of this rule
