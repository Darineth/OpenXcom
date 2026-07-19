# Ruleset: `enviroEffects:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`RuleEnviroEffects`](../src/Mod/RuleEnviroEffects.h) · **List key:** `type` ·
**Loader:** [`RuleEnviroEffects::load`](../src/Mod/RuleEnviroEffects.cpp)

Enviro effects describe **what the battlefield itself does to the units standing on it**: per-faction
damage ticking every turn (heat, cold, radiation, vacuum…), armor swaps forced by the environment,
and the cosmetic re-skinning of the battlescape (palette, map background, shock indicators). A rule
is attached either by the [terrain](Ruleset-Terrains.md) (`enviroEffects:`) or by the
[deployment](Ruleset-AlienDeployments.md) (`enviroEffects:`, which wins if both are set).

```yaml
enviroEffects:
  - type: STR_MARS_ENVIRONMENT
    environmentalConditions:
      STR_FRIENDLY:                 # X-COM units take a hit every turn
        globalChance: 100           # ...in 100% of battles
        chancePerTurn: 20           # ...20% chance per unit per turn
        firstTurn: 2
        weaponOrAmmo: STR_MARS_HEAT_DAMAGE   # an items: entry; its power + damageType are used
        message: STR_MARS_HEAT_WARNING
        color: 29
    armorTransformations:
      STR_PERSONAL_ARMOR_UC: STR_MARS_SUIT_UC   # anything weaker is force-upgraded
    mapBackgroundColor: 2
```

Entries **merge** across mods/files by `type`, support `refNode:` inheritance and `delete: true`
(see the [index](Ruleset.md#how-rulesets-load-the-60-second-version)).

## Fields

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | — | Unique id, referenced from a terrain's or deployment's `enviroEffects:`. |
| `refNode` | map | — | Inherit all fields from another (anonymous) node first. |
| `environmentalConditions` | map `faction: condition` | — | Per-turn damage rules, keyed by `STR_FRIENDLY`, `STR_HOSTILE` or `STR_NEUTRAL`; see [below](#environmentalconditions). |
| `armorTransformations` | map `armor: armor` | — | Force-swap [armors](Ruleset-Armors.md) at battle start (e.g. everything below a space suit becomes a space suit). A swap into a **bigger** armor `size:` is refused. |
| `paletteTransformations` | map `palette: palette` | — | Overwrite one palette's contents with another's for the duration of the battle (recolors the whole battlescape); see [`customPalettes:`](Ruleset-CustomPalettes.md). |
| `mapBackgroundColor` | int | 15 | Palette index the battlescape draws behind/around the map (and behind the turn banner). |
| `ignoreAutoNightVisionUserSetting` | bool | false | Suppress the player's automatic night-vision option on this battlefield. |
| `inventoryShockIndicator` | string | — → `BigShockIndicator` | Sprite shown on the inventory paperdoll for units suffering the environmental condition (empty = the engine's built-in `BigShockIndicator`). |
| `mapShockIndicator` | string | — → `FloorShockIndicator` | Sprite drawn on the map under such units (empty = the engine's built-in `FloorShockIndicator`). |

## `environmentalConditions:`

One **`EnvironmentalCondition`** per faction. At battle start each faction rolls `globalChance` once:
if it fails, that faction is simply immune for the whole battle. Otherwise, at the start of every one
of that faction's turns, each of its units independently rolls `chancePerTurn` and — on success — is
hit by `weaponOrAmmo`'s power and damage type, as if shot
([`NextTurnState::applyEnvironmentalConditionToFaction`](../src/Battlescape/NextTurnState.cpp)).

| Key | Type | Default | Meaning |
|---|---|---|---|
| `globalChance` | int % | 100 | Chance, rolled once per battle, that this condition is active for the faction at all. |
| `chancePerTurn` | int % | 0 | Chance per unit per turn to be hit (0 = the condition does nothing). |
| `firstTurn` | int | 1 | First turn on which the condition can hit. |
| `lastTurn` | int | 1000 | Last turn on which it can hit. |
| `message` | string | — | `STR_*` warning shown on the turn banner when the condition hits. |
| `color` | int | 29 | Palette index of that message's text. |
| `weaponOrAmmo` | string item | — | The [item](Ruleset-Items.md) whose `power` and `damageType` define the hit (nothing is fired — no accuracy, no range falloff, no stat bonus, no area damage, no terrain damage). |
| `side` | int | −1 | Body side that takes the hit (`UnitSide`: 0 front, 1 left, 2 right, 3 rear, 4 under); −1 = random per hit. |
| `bodyPart` | int | −1 | Body part hit (`UnitBodyPart`: 0 head, 1 torso, 2 right arm, 3 left arm, 4 right leg, 5 left leg); −1 = random per hit. |

Note that X-COM units are never hit on turn 1 (killing or panicking units before the battle has
properly started is not supported), and no faction is hit once all enemies are neutralized.

## See also

- [`alienDeployments:`](Ruleset-AlienDeployments.md) / [`terrains:`](Ruleset-Terrains.md) — the two places that attach an enviro effects rule
- [`startingConditions:`](Ruleset-StartingConditions.md) — the *pre-battle* gate (these keys used to live there)
- [`armors:`](Ruleset-Armors.md) — the armors `armorTransformations:` swaps between
- [`items:`](Ruleset-Items.md) — the `weaponOrAmmo` whose power/damage type is applied
