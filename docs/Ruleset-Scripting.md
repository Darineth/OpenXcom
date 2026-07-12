# Ruleset: scripting (`extended:`, `scripts:`, `tags:`)

Back to the [ruleset index](Ruleset.md).

**Engine classes:** [`ScriptGlobal` / `ScriptValues`](../src/Engine/Script.h) ·
[`ModScript`](../src/Mod/ModScript.h) (the hook catalogue) · `ModScriptGlobal` (in
[`Mod.cpp`](../src/Mod/Mod.cpp)) · **Loaders:**
[`ScriptGlobal::load`](../src/Engine/Script.cpp) (the `extended:` singleton),
`ScriptGroup::load` (each rule's `scripts:` sub-node),
`ScriptValues::load` (each rule's `tags:` sub-node)

OXCE ships a small **bytecode VM** ("Y-Script") that mods can hook into. This page documents the
**ruleset surface**: where scripts and tags attach, and what the hooks are called. It does **not**
teach the Y-Script language — for syntax, opcodes, and the per-hook argument lists, see
[Extended.txt](../Extended.txt) (and the in-game *"dump script reference"* debug output, which is
generated from the same tables).

```yaml
extended:
  tags:
    RuleArmor:
      ARMOR_IS_SHINY: int      # declare a custom field on every armors: entry
    BattleUnit:
      UNIT_CHARGE: int         # declare a per-unit runtime variable (saved with the game)

armors:
  - type: STR_SHINY_ARMOR
    tags:
      ARMOR_IS_SHINY: 1        # set the rule-level tag on this armor
    scripts:
      recolorUnitSprite:
        - offset: 1
          code: |
            var int shiny;
            armor.getTag shiny Tag.ARMOR_IS_SHINY;
            if eq shiny 1;
              add_shade new_pixel -2;
            end;
            return new_pixel;
```

---

## The `extended:` singleton

One map, merged across mods. It is where **tags are declared** and where **global (all-object)
scripts** are registered.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `tags` | map: script type → (tag name → value type) | — | Declares custom fields/variables. The outer key is a **script type name** (see the table below); the inner map is `TAG_NAME: type`. The only value types are `int` and `RuleList` (a mod id). |
| `tagsFile` | filename | — | Pull the `extended: tags:` block from another file in the same mod, so several rulesets can share one tag declaration. Only the `tags` sub-node is imported. |
| `globals` | map: tag name → value | — | Sets values of `RuleMod`-typed tags — mod-wide script variables readable from any hook. |
| `scripts` | map: hook name → list of script entries | — | **Global scripts**: script bodies attached to a hook for *every* object, not just one rule. See below. |

### Declaring tags

A tag is a named `int` slot bolted onto an engine type. Declare it once under `extended: tags:`, then
set it on individual rules (or read/write it from scripts at runtime). The outer key is the type's
**script name**:

| Script type | Where its `tags:` are set | Nature |
|---|---|---|
| `RuleArmor` | [`armors:`](Ruleset-Armors.md) entries | Rule (immutable) |
| `RuleItem` | [`items:`](Ruleset-Items.md) entries | Rule |
| `RuleSoldier` | [`soldiers:`](Ruleset-Soldiers.md) entries | Rule |
| `RuleSkill` | [`skills:`](Ruleset-Skills.md) entries | Rule |
| `RuleCraft` | [`crafts:`](Ruleset-Crafts.md) entries | Rule |
| `RuleUfo` | [`ufos:`](Ruleset-Ufos.md) entries | Rule |
| `RuleCountry` | [`countries:`](Ruleset-Countries.md) entries | Rule |
| `RuleResearch` | [`research:`](Ruleset-Research.md) entries | Rule |
| `RuleSoldierBonus` | [`soldierBonuses:`](Ruleset-SoldierBonuses.md) entries | Rule |
| `RuleMod` | `extended: globals:` | Rule (mod-wide) |
| `BattleUnit` | runtime only | Savegame (persisted per unit) |
| `BattleUnitVisibility` | runtime only | Scratch, per visibility check |
| `BattleItem` | runtime only | Savegame (per item) |
| `GeoscapeSoldier` | runtime only | Savegame (per soldier) |
| `Craft`, `Ufo`, `Country` | runtime only | Savegame |
| `BattleGame`, `GeoscapeGame` | runtime only | Savegame (per battle / per campaign) |

Rule-level tags are set with a `tags:` sub-node **inside the rule entry**; runtime tags start at 0 and
are read/written from scripts (`obj.getTag` / `obj.setTag`) and serialized with the save.

Tag names are global across types: reusing one name for two different script types is a load error.

### Global scripts (`extended: scripts:`)

The same hook names as the per-rule `scripts:` node, but registered once and run for **every**
object that reaches the hook. Each entry is:

| Key | Type | Meaning |
|---|---|---|
| `new` | string | Register a new script under this name (error if the name exists). |
| `override` | string | Replace an existing global script of this name. |
| `update` | string | Modify an existing one. |
| `ignore` | string | Keep a placeholder/no-op under this name. |
| `delete` | string | Remove a previously registered global script. |
| `offset` | number (non-zero) | **Required.** Sort order — global scripts run in ascending offset; negative offsets run before the object's own scripts. `0` is rejected. |
| `code` | string (block) | The Y-Script body. |

```yaml
extended:
  scripts:
    newTurnUnit:
      - new: RegenerateShields
        offset: 1
        code: |
          # ...
```

---

## Where `scripts:` and `tags:` attach

| Root | `scripts:` (hook group) | `tags:` |
|---|---|---|
| [`armors:`](Ruleset-Armors.md) | ✔ battle-unit hooks | ✔ `RuleArmor` |
| [`items:`](Ruleset-Items.md) | ✔ battle-item hooks | ✔ `RuleItem` |
| [`skills:`](Ruleset-Skills.md) | ✔ skill hooks | ✔ `RuleSkill` |
| [`countries:`](Ruleset-Countries.md) | ✔ country hooks | ✔ `RuleCountry` |
| [`ufos:`](Ruleset-Ufos.md) | ✔ UFO hooks | ✔ `RuleUfo` |
| [`crafts:`](Ruleset-Crafts.md) | ✔ craft hooks | ✔ `RuleCraft` |
| [`soldierBonuses:`](Ruleset-SoldierBonuses.md) | ✔ soldier-bonus hooks | ✔ `RuleSoldierBonus` |
| [`soldiers:`](Ruleset-Soldiers.md) | — | ✔ `RuleSoldier` |
| [`research:`](Ruleset-Research.md) | — | ✔ `RuleResearch` |

Note the asymmetry: **battle-unit hooks live on the armor, not on `units:`/`soldiers:`** — an armor
*is* the battle-unit body, so that is where its behaviour scripts hang
(see [Ruleset-Armors.md](Ruleset-Armors.md)).

The **bonus-stats** hooks are different again: they are not listed under `scripts:` but replace a
[stat bonus formula](Ruleset-StatBonus.md) in place — see the last section.

---

## Hook catalogue

From [`src/Mod/ModScript.h`](../src/Mod/ModScript.h). Names are exactly the YAML keys under a
`scripts:` node.

### Battle-unit hooks — attach to `armors:`

| Hook | Fires when |
|---|---|
| `recolorUnitSprite` | Each unit sprite pixel is drawn — remap its color. |
| `selectUnitSprite` | Choosing which sprite frame to draw for the unit. |
| `selectMoveSoundUnit` | Choosing the footstep/move sound. |
| `reactionUnitAction` | A unit performs an action that others might react to (attacker side). |
| `reactionUnitReaction` | A unit is deciding whether to react-fire (reactor side). |
| `tryPsiAttackUnit` / `tryMeleeAttackUnit` | Resolving a psi / melee attack's hit roll. |
| `hitUnit` | A projectile/melee hit lands on the unit, before damage. |
| `damageUnit` | Damage is applied (health/stun/wounds/armor all writable). |
| `damageSpecialUnit` | The "special" damage pass (secondary effects). |
| `healUnit` | The unit is healed (medikit, regeneration). |
| `createUnit` | The battle unit is created/spawned. |
| `newTurnUnit` | Start of each turn, per unit. |
| `returnFromMissionUnit` | Post-mission: stat gains, recovery, transformation into the geoscape soldier. |
| `awardExperience` | Experience is being awarded for a hit. |
| `visibilityUnit` | Every observer→target visibility check (the camouflage/spotting funnel). |
| `aiCalculateTargetWeight` | The AI scores this unit as a target. |
| `statsForNerdsArmor` | The "Stats for Nerds" pedia page for this armor is rendered. |

### Battle-item hooks — attach to `items:`

| Hook | Fires when |
|---|---|
| `recolorItemSprite` | Each item sprite pixel is drawn. |
| `selectItemSprite` | Choosing which item sprite frame to draw. |
| `vaporParticleAmmo` / `vaporParticleWeapon` | Spawning vapor/trail particles for a shot (ammo's and weapon's script). |
| `reactionWeaponAction` | Reaction-fire scoring, from the weapon's side. |
| `tryPsiAttackItem` / `tryMeleeAttackItem` | The item's own psi / melee hit roll. |
| `hitUnitAmmo` | The **ammo's** version of `hitUnit`. |
| `damageUnitAmmo` | The ammo's version of `damageUnit`. |
| `damageSpecialUnitAmmo` | The ammo's version of `damageSpecialUnit`. |
| `createItem` | The battle item is created. |
| `newTurnItem` | Start of each turn, per item (grenade timers, etc.). |
| `sellCostItem` / `buyCostItem` | Computing the item's sell / buy price in the base. |
| `statsForNerdsItem` | The item's "Stats for Nerds" page is rendered. |

### Other hooks

| Hook | Attaches to | Fires when |
|---|---|---|
| `skillUseUnit` | `skills:` | A soldier skill is used. |
| `newMonthCountry` | `countries:` | Monthly funding/pact evaluation for a country. |
| `detectUfoFromBase` / `detectUfoFromCraft` | `ufos:` | Radar detection rolls. |
| `statsForNerdsUfo` | `ufos:` | The UFO's "Stats for Nerds" page. |
| `statsForNerdsCraft` | `crafts:` | The craft's "Stats for Nerds" page. |
| `applySoldierBonuses` | `soldierBonuses:` | A soldier bonus layer is applied to a battle unit. |

---

## Bonus-stats hooks — the script form of a stat bonus

These hooks do **not** live under a `scripts:` node. Each one *is* a
[stat bonus formula](Ruleset-StatBonus.md) field: wherever a stat-bonus map is accepted, you may
instead give a **string** naming a script, and the script computes the value.

| Field (hook) | Host |
|---|---|
| `psiDefence`, `meleeDodge` | [`armors:`](Ruleset-Armors.md) |
| `recovery:` → `time`, `energy`, `morale`, `health`, `mana`, `stun` | [`armors:`](Ruleset-Armors.md) (the `…RecoveryBonusStats` parsers) |
| `recovery:` → `time`, `energy`, `morale`, `health`, `mana`, `stun` | [`soldierBonuses:`](Ruleset-SoldierBonuses.md) (the separate `…SoldierRecoveryBonusStats` parsers) |
| `damageBonus`, `meleeBonus`, `accuracyMultiplier`, `meleeMultiplier`, `throwMultiplier`, `closeQuartersMultiplier` | [`items:`](Ruleset-Items.md) |

Internally each registers a parser named after the field (`psiDefenceBonusStats`,
`healthRecoveryBonusStats`, …), all sharing the same argument set: the acting unit, the current
value, the weapon and ammo, and the skill in use. See
[Ruleset-StatBonus.md](Ruleset-StatBonus.md#advanced-script-form).

---

## Gotchas

- An **unknown hook name** under a `scripts:` node is a hard load **exception**, not a warning — a
  typo will refuse to start the game (which is the point: it can't be silently ignored).
- A key ending in `#` under `extended: scripts:` is skipped — the convention for commenting out a
  block without deleting it.
- `offset: 0` on a global script is rejected; use any non-zero value.
- Runtime tags (`BattleUnit`, `GeoscapeSoldier`, …) are **saved**, so removing a tag declaration
  from a mod after a campaign has started drops those values on load.
- Scripts are performance-sensitive: `recolorUnitSprite` and `visibilityUnit` in particular run
  extremely often.

## See also

- [Extended.txt](../Extended.txt) — the Y-Script language, opcodes, and per-hook argument lists
- [Ruleset-StatBonus.md](Ruleset-StatBonus.md) — the formula blocks these hooks can replace
- [Ruleset-Armors.md](Ruleset-Armors.md) · [Ruleset-Items.md](Ruleset-Items.md) — the two biggest
  script hosts
