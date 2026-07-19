# Ruleset: `ai:` (global AI tuning)

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`Mod`](../src/Mod/Mod.h) · **Singleton** (one map, not a keyed list) ·
**Loader:** [`Mod::loadFile`](../src/Mod/Mod.cpp) · **Consumer:**
[`AIModule`](../src/Battlescape/AIModule.cpp)

The `ai:` node is a top-level singleton that tunes the tactical AI mod-wide: when it starts using
each weapon class, how it scores fire modes, and how attractive each kind of target is. Like every
singleton it **merges field-by-field** across mods.

```yaml
ai:
  useDelayGrenade: 2
  useDelayBlaster: 5
  extendedFireModeChoice: true
  targetWeightAsHostileCivilians: 25
```

## Use delays

The first battle **turn** the AI is allowed to use each weapon class (0 = from turn 1; a high value
like 999 effectively disables the class).

| Key | Type | Default | Meaning |
|---|---|---|---|
| `useDelayFirearm` | int turn | 0 | First turn the AI may fire firearms. |
| `useDelayGrenade` | int turn | 3 | First turn the AI may throw grenades. |
| `aiUseDelayProxy` | int turn | 999 | First turn the AI may plant proximity grenades. **Note the inconsistent key name** — it really is `aiUseDelayProxy` inside the `ai:` node, not `useDelayProxy`. |
| `useDelayBlaster` | int turn | 3 | First turn the AI may use blaster (waypoint) launchers. |
| `useDelayMelee` | int turn | 0 | First turn the AI may make melee attacks. |
| `useDelayPsionic` | int turn | 0 | First turn the AI may use psionic attacks. |
| `useDelayMedikit` | int turn | 999 | First turn the AI may use self-target medikits. |

Two **legacy top-level aliases** (outside the `ai:` node) are still read first and then overridden
by the `ai:` values if both are present: `turnAIUseGrenade` (→ `useDelayGrenade`) and
`turnAIUseBlaster` (→ `useDelayBlaster`).

## Fire-mode choice

| Key | Type | Default | Meaning |
|---|---|---|---|
| `extendedFireModeChoice` | bool | false | Use the extended scoring algorithm to pick which attack (aimed/snap/auto…) the AI uses, instead of the vanilla heuristic. |
| `fireChoiceIntelCoeff` | int | 5 | How much the unit's **intelligence** stat weighs into the extended fire-mode score (sniping preference). |
| `fireChoiceAggroCoeff` | int | 5 | How much the unit's **aggression** stat weighs into the extended fire-mode score. |
| `respectMaxRange` | bool | false | If true, the AI will not attempt shots beyond the weapon's `maxRange`. |

## Behavior toggles

| Key | Type | Default | Meaning |
|---|---|---|---|
| `destroyBaseFacilities` | bool | false | During base defense, aliens may keep destroying base facilities even after first contact with X-COM. |
| `pickUpWeaponsMoreActively` | bool | false | Alien AI picks up weapons from the ground more actively. |
| `pickUpWeaponsMoreActivelyCiv` | bool | false | Same, for the civilian AI. |
| `reactionFireThreshold` | int % | 0 | Aliens only take reaction shots with at least this hit chance (0 = vanilla: any chance). |
| `reactionFireThresholdCiv` | int % | 0 | Same, for civilians. |

## Target weights

How attractive a potential target is when the AI scores whom to attack
(`AIAttackWeight`; higher = more attractive, negative = actively avoided). Armors can override
these per-unit via their own `ai:` sub-node (see [armors](Ruleset-Armors.md#ai--targeting)).

| Key | Type | Default | Meaning |
|---|---|---|---|
| `targetWeightThreatThreshold` | int | 50 | Weight above which a unit is considered a real threat. |
| `targetWeightAsHostile` | int | 100 | Base weight of a hostile unit. |
| `targetWeightAsHostileCivilians` | int | 50 | Base weight of a civilian, as seen by aliens. |
| `targetWeightAsFriendly` | int | −200 | Base weight of a same-faction unit (friendly fire avoidance). |
| `targetWeightAsNeutral` | int | −100 | Base weight of a neutral relation (X-COM ↔ civilians). |

## See also

- [Ruleset-Globals.md](Ruleset-Globals.md) — the other global tuning singletons
- [`units:`](Ruleset-Units.md) — per-unit AI stats (aggression, intelligence, spotter/sniper)
- [`items:`](Ruleset-Items.md#ai--targeting) — per-item AI hints (`attraction`, `ai: meleeHitCount`)
- [`armors:`](Ruleset-Armors.md#ai--targeting) — per-armor target-weight overrides
