# Feature: Item Stats & Stat Modifiers

**Status:** Implemented (Phase 4).

## Motivation

Phase 4 needs a generic way for equipped gear to change unit stats without hardcoding each effect.

The goal is to let rulesets express two kinds of stat changes:

- flat bonuses with `stats` (for items and armor);
- percentage adjustments with `statModifiers` (for items and armor).

This is a core dependency for later inventory-layout and modular-vehicle work.

## OXCE / OXCE-Plus Audit

### Current upstream behavior in this DX tree

1. `Armor` already supports flat stat bonuses via `stats`.
- Load path: `Armor::load` merges `reader["stats"]` into `_stats`.
- Runtime usage: `Soldier::prepareStatsWithBonuses` and `BattleUnit::updateArmorFrom*` add armor stats.

2. `RuleItem` currently has no generic `stats` block.
- It has specialized stat formulas (`damageBonus`, `accuracyMulti`, etc.), but no all-stat `UnitStats` payload.

3. Neither `Armor` nor `RuleItem` currently has `statModifiers`.

4. Inventory slot-level `countStats` filtering is not in this tree yet (scheduled under later Phase 4 inventory typing work).

### Delta to implement

- Add `stats` support to items.
- Add `statModifiers` support to both items and armor.
- Apply these effects to effective unit stats in battlescape and soldier prep flows.

## Proposed Ruleset Surface

### RuleItem

```yaml
items:
  - type: STR_EXAMPLE_ITEM
    stats:
      firing: 5
      reactions: 3
    statModifiers:
      strength: 10
      stamina: -10
```

### Armor

```yaml
armors:
  - type: STR_EXAMPLE_ARMOR
    stats:
      tu: 8
    statModifiers:
      health: 20
      reactions: -5
```

### Interpretation

- `stats`: flat additive values.
- `statModifiers`: percentage deltas where `0` means unchanged.
- Missing fields behave as no-op.

## Runtime Stacking Model

To keep results deterministic and easy to tune, the plan is:

1. Start from base stats (unit stats + existing soldier bonus stack).
2. Add all flat `stats` contributions.
3. Apply combined percentage effect from `statModifiers`.
4. Clamp with existing `UnitStats::obeyFixedMinimum` safeguards.

### Percentage combination rule

Use additive deltas around 0:

- each source contributes `(modifier)` per stat;
- total delta per stat is summed across sources;
- final contribution is `baseStat * totalDelta / 100` (integer rounded like current stat math).

This avoids multiplicative explosion from many stacked items and matches legacy-DX balancing intent.

## Implemented DX Design

### 1) Data model and YAML loading

- `src/Mod/RuleItem.h` / `src/Mod/RuleItem.cpp`
  - Added `UnitStats _stats` and `UnitStats _statModifiers`.
  - Added fast no-op flags: `_hasStats`, `_hasStatModifiers`.
  - Added YAML support for `stats` and `statModifiers`.
  - Added getters for both payloads and flags.

- `src/Mod/Armor.h` / `src/Mod/Armor.cpp`
  - Added `UnitStats _statModifiers` and `_hasStatModifiers`.
  - Added YAML support for `statModifiers`.
  - Added getter and no-op check.

### 2) Effective stat recomputation paths

- `src/Savegame/Soldier.cpp`
  - Extended `prepareStatsWithBonuses` to apply armor `statModifiers` after flat bonuses.

- `src/Savegame/BattleUnit.cpp`
  - Applied armor `statModifiers` in non-soldier armor refresh (`updateArmorFromNonSoldier`).
  - Extended `getBaseStats()` to recompute effective stats including equipped-item `stats` and
    `statModifiers` (for slotted inventory items).

- Preserved current armor flat stats and soldier-bonus ordering.

### 3) Equipped-item scan policy (Phase 4 sequencing)

Because `countStats` slot filtering is a later checklist item, this feature currently:

- applies item effects from equipped inventory items with a valid inventory slot;
- does not introduce slot-specific filtering yet.

When typed-slot work lands, `countStats` can narrow which slots contribute without changing this feature's core data model.

### 4) Script and tooling surface

- Expose new fields through existing script registration patterns where useful.
- Add Stats-for-Nerds/Ufopaedia visibility only if needed for debugging; otherwise keep UI unchanged for this phase.

## Compatibility

- Backward compatibility: full, because all new fields default to no-op.
- Save compatibility: no new save fields required (computed from rules + equipped state).
- Ruleset compatibility: existing content unchanged unless new keys are used.

## Risks and Mitigations

1. Risk: hidden stat jumps from broad inventory scanning.
- Mitigation: document initial policy clearly; add follow-up `countStats` integration in typed-slot phase.

2. Risk: order-of-operations mismatch with legacy behavior.
- Mitigation: keep one centralized helper for stat recomputation and unit tests-by-build plus manual checks.

3. Risk: repeated recomputation cost.
- Mitigation: keep no-op flags and compute only on known refresh points.

## Verification

1. Win32 Release build passes after implementation.
2. The change is backward compatible by default: missing new keys remain no-op.

## Out of Scope (This Feature)

- Slot-level `countStats` filtering.
- Directional armor item fields (`frontArmor`/`sideArmor`/`rearArmor`/`underArmor`, `armorSide`).
- Typed-slot and utility-slot mechanics.

Those remain in later Phase 4 checklist items.
