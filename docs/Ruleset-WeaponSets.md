# Ruleset: `weaponSets:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`RuleWeaponSet`](../src/Mod/RuleWeaponSet.h) · **List key:** `type` · **Loader:**
[`RuleWeaponSet::load`](../src/Mod/RuleWeaponSet.cpp)

A weapon set is a **named, reusable list of items**. Its one consumer is the
`weightedBuiltInWeaponSets:` field of [`units:`](Ruleset-Units.md): when a non-soldier unit spawns,
the engine picks one weapon set by weighted random roll and adds **all** of that set's items to the
unit as fixed (built-in) equipment. This lets a unit type carry randomized loadout packages without
repeating the item lists inline.

```yaml
weaponSets:
  - type: SECTOID_KIT_PLASMA
    weapons: [ STR_PLASMA_PISTOL, STR_PLASMA_PISTOL_CLIP ]
  - type: SECTOID_KIT_GRENADIER
    weapons: [ STR_PLASMA_PISTOL, STR_PLASMA_PISTOL_CLIP, STR_ALIEN_GRENADE ]

units:
  - type: STR_SECTOID_SOLDIER
    weightedBuiltInWeaponSets:      # one map per alien item level; weights pick ONE set
      - SECTOID_KIT_PLASMA: 70
        SECTOID_KIT_GRENADIER: 30
```

Entries **merge** across mods/files by `type`, support `refNode:` inheritance and `delete: true`
(see the [index](Ruleset.md#how-rulesets-load-the-60-second-version)).

## Fields

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | — | Unique id, referenced from `weightedBuiltInWeaponSets:` in [`units:`](Ruleset-Units.md). |
| `refNode` | map | — | Inherit all fields from another (anonymous) node first. |
| `weapons` | list of item types | — | The [`items:`](Ruleset-Items.md) in the set; every one is added when the set is chosen. |

## Semantics worth knowing

- Item names are resolved to real rules **after** all mods load
  (`RuleWeaponSet::afterLoad` → `Mod::linkRule`), so a set may reference items defined later or by
  another mod; an unknown item name is a load error.
- The selection happens in
  [`SavedBattleGame::initUnit`](../src/Savegame/SavedBattleGame.cpp): the unit's
  `weightedBuiltInWeaponSets:` list is indexed by the mission's **alien item level** (clamped to
  the last entry), the weighted options choose a set name, and the set's items are added the same
  way as plain `builtInWeapons:` fixed items.
- A plain, non-random built-in loadout doesn't need a weapon set: units have
  `builtInWeapons:`/`builtInWeaponSets:` (inline item lists per item level), and
  [`armors:`](Ruleset-Armors.md) have their own `builtInWeapons:` item list.

## See also

- [`units:`](Ruleset-Units.md) — `weightedBuiltInWeaponSets:`, `builtInWeapons:`, alien item levels
- [`items:`](Ruleset-Items.md) — the items referenced here (fixed-weapon flags live on the item)
- [`armors:`](Ruleset-Armors.md) — armor-granted built-in weapons
