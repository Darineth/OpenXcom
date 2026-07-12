# Ruleset: `skills:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`RuleSkill`](../src/Mod/RuleSkill.h) · **List key:** `type` · **Loader:**
[`RuleSkill::load`](../src/Mod/RuleSkill.cpp)

A skill is an **active soldier ability** — an extra entry in the battlescape action menu, opened
from the soldier's skill button. A skill is *not* an item: it borrows a compatible item the soldier
already has (by explicit item list and/or by battle type), pays its own TU/energy/… cost, and then
runs whatever the mod's Y-Script decides. Which soldier types get which skills is declared on the
[`soldiers:`](Ruleset-Soldiers.md) entry (`skills:` list).

```yaml
skills:
  - type: STR_SKILL_AIMED_THROW
    targetMode: 6              # BA_THROW — the action the skill performs
    battleType: 4              # BT_GRENADE — any grenade in hand qualifies
    checkHandsOnly: true
    tuUse: 40
    costUse:
      energy: 10
    requiredBonuses:
      - STR_COMMENDATION_BONUS_SHARPSHOOTER
    scripts:
      skillUseUnitScript: |
        ...
```

Entries **merge** across mods/files by `type`, support `refNode:` inheritance and `delete: true`
(see the [index](Ruleset.md#how-rulesets-load-the-60-second-version)).

## Fields

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | — | Unique id; also the display-name string key shown in the skill menu. |
| `refNode` | map | — | Inherit all fields from another (anonymous) node first. |
| `targetMode` | int | 0 (`BA_NONE`) | The `BattleActionType` the skill performs — how it asks for a target (see the table below); values outside 0–16 are clamped to 0. |
| `battleType` | int | 0 (`BT_NONE`) | A `BattleType` an item must have to be usable by this skill (see the table below); values outside 0–11 are clamped to 0. |
| `isPsiRequired` | bool | false | Requires non-zero psi skill (soldiers without it can't use it). |
| `checkHandsOnly` | bool | true | Whether the `compatibleWeapons` search looks only in the hands, or also in the inventory and special-weapon slot. |
| `checkHandsOnly2` | bool | false | Same, but for the `battleType` search. |
| `tuUse` | int | 0 | Time-unit cost of using the skill (shorthand for `costUse: { time: … }`). |
| `costUse` | map | all 0 | Full [use cost](Ruleset-UseCost.md) block: `time`, `energy`, `morale`, `health`, `stun`, `mana`. |
| `flatUse` | bool or map | false | Whether each cost component is a flat value instead of a percentage of the maximum — a single bool sets `time`, or use the same sub-keys as `costUse`. |
| `compatibleWeapons` | list of items | — | Explicit [items](Ruleset-Items.md) that enable this skill when carried. |
| `requiredBonuses` | list of bonus names | — | [`soldierBonuses:`](Ruleset-SoldierBonuses.md) the soldier must already have for the skill to appear. |

An item qualifies for the skill if it is in `compatibleWeapons` **or** matches `battleType`; the two
searches are scoped independently by `checkHandsOnly` and `checkHandsOnly2`.

### `targetMode` values

From [`BattleActionType`](../src/Mod/RuleItem.h). The useful ones:

| Value | Action | Value | Action |
|---|---|---|---|
| 0 | `BA_NONE` (no targeting) | 9 | `BA_AIMEDSHOT` |
| 3 | `BA_KNEEL` | 10 | `BA_HIT` (melee) |
| 4 / 5 | `BA_PRIME` / `BA_UNPRIME` | 11 | `BA_USE` |
| 6 | `BA_THROW` | 12 | `BA_LAUNCH` (waypoints) |
| 7 | `BA_AUTOSHOT` | 13 / 14 | `BA_MINDCONTROL` / `BA_PANIC` |
| 8 | `BA_SNAPSHOT` | 16 | `BA_CQB` |

Values above 16 are rejected (clamped to 0) — DX's added action types (burst, dual-fire, reload,
overwatch) are **not** valid skill target modes.

### `battleType` values

From [`BattleType`](../src/Mod/RuleItem.h): 0 none, 1 firearm, 2 ammo, 3 melee, 4 grenade,
5 proximity grenade, 6 medikit, 7 scanner, 8 mind probe, 9 psi-amp, 10 flare, 11 corpse.

## Scripting

Skills are mostly *made of* script: the `scripts:` sub-node accepts the `SkillScripts` hooks and the
rule carries custom `tags:`. See [Ruleset-Scripting.md](Ruleset-Scripting.md). Without a script, a
skill just performs its `targetMode` action with the found item at the skill's own cost.

## See also

- [`soldiers:`](Ruleset-Soldiers.md) — grants skills to a soldier type (`skills:`, `skillIconSprite`)
- [`items:`](Ruleset-Items.md) — the items a skill borrows (`compatibleWeapons`, `battleType`)
- [`soldierBonuses:`](Ruleset-SoldierBonuses.md) — the `requiredBonuses` gate
- [Use cost/flat blocks](Ruleset-UseCost.md) — the `costUse:` / `flatUse:` syntax
