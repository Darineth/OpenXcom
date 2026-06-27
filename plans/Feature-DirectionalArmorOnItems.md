# Feature: Directional Armor on Items

**Status:** Implemented (Phase 4).

## Motivation

The damage model already resolves hits per-side (front/left/right/rear/under) and the wearer's
armor comes entirely from the `Armor` ruleset plus soldier bonuses. DX wants equipped *items* to
contribute directional armor as well, so that gear — plates, shields, vehicle modules, sided
attachments — can add protection to specific facings without baking every combination into a
distinct `Armor` definition.

This is the item-side analogue of the already-shipped **item stats & stat modifiers** feature
(see [Feature-ItemStatsModifiers.md](Feature-ItemStatsModifiers.md)): there we let equipped items
add to a unit's `UnitStats`; here we let equipped items add to a unit's per-side max armor. It is
also a building block for the later **sided slots / modular-vehicle** work, where an inventory
slot maps to a body facing and the item dropped into it armors that facing.

## OXCE / OXCE-Plus Audit

### Current upstream behavior in this DX tree

1. **Per-side armor on units/armor already exists and is fully wired.**
   - `Armor` exposes `getFrontArmor()`, `getLeftSideArmor()`, `getRightSideArmor()`,
     `getRearArmor()`, `getUnderArmor()` ([Armor.h](../src/Mod/Armor.h#L224-L232)).
   - `UnitSide` is `{ SIDE_FRONT, SIDE_LEFT, SIDE_RIGHT, SIDE_REAR, SIDE_UNDER, SIDE_MAX }`
     ([Unit.h](../src/Mod/Unit.h#L43)).
   - `BattleUnit` stores `_maxArmor[SIDE_*]` / `_currentArmor[SIDE_*]`, computed in
     `updateArmorFromSoldier` / `updateArmorFromNonSoldier` from the `Armor` values plus a loop
     over the soldier's bonus rules (`RuleSoldierBonus::getFrontArmor()` etc.), then clamped to
     `>= 0` ([BattleUnit.cpp](../src/Savegame/BattleUnit.cpp#L188-L213)).
   - The damage code resolves the impact side and subtracts that side's armor
     (`BattleUnit::getArmor(UnitSide)` / `damage()` at
     [BattleUnit.cpp](../src/Savegame/BattleUnit.cpp#L1732)).
   - `RuleSoldierBonus` already carries directional armor, so the *bonus* path is the existing
     precedent for "something other than the base armor adds per-side armor."

2. **`RuleItem` has no directional armor.**
   - The only armor-like field on an item is the scalar `getArmor()` — the item's own structural
     HP / explosive resistance ([RuleItem.h](../src/Mod/RuleItem.h#L913)). There is no per-side
     payload and nothing that feeds the wearer's `_maxArmor`.

3. **Item-stats precedent for equipped-item contributions already exists.**
   - `RuleItem` has `stats` / `statModifiers` with fast `_hasStats` / `_hasStatModifiers` no-op
     flags ([RuleItem.h](../src/Mod/RuleItem.h#L457-L706)).
   - `BattleUnit::computeEffectiveBaseStats` walks `_inventory`, skips items with no slot, and
     folds equipped-item `stats`/`statModifiers` into the effective `UnitStats`;
     `refreshBaseStats()` recaches it and is re-run from `BattleItem` on every move
     ([BattleUnit.cpp](../src/Savegame/BattleUnit.cpp#L4392-L4451),
     [BattleItem.cpp](../src/Savegame/BattleItem.cpp#L569-L620)).

### Delta to implement

- Add directional armor fields to `RuleItem` (`frontArmor`, `sideArmor`, `rearArmor`,
  `underArmor`), with a no-op flag.
- Fold equipped-item directional armor into the wearer's `_maxArmor[SIDE_*]`, mirroring the
  existing soldier-bonus loop, and keep `_currentArmor` consistent.
- Surface the new fields in Stats-for-Nerds (the `Armor` per-side block is already shown there).

Everything else (the per-side damage application, the `UnitSide` enum, current-vs-max bookkeeping)
is reused unchanged.

## Proposed Ruleset Surface

### RuleItem

```yaml
items:
  - type: STR_BALLISTIC_PLATE
    # existing scalar field (item's own HP vs explosions) — unchanged:
    armor: 30
    # NEW: per-side armor granted to the wearer while equipped:
    frontArmor: 20
    sideArmor: 8      # applied to BOTH left and right, matching Armor's single sideArmor
    rearArmor: 4
    underArmor: 0
```

### Interpretation

- `frontArmor` / `sideArmor` / `rearArmor` / `underArmor`: flat additive bonuses to the wearer's
  corresponding max armor side(s). `sideArmor` adds to both `SIDE_LEFT` and `SIDE_RIGHT`, exactly
  like `Armor`'s single `sideArmor` value expands to both sides.
- All fields default to `0`, so an item that sets none of them is a no-op.
- The existing scalar `armor:` key is **untouched** — it remains the item's structural HP and is
  unrelated to this feature.

## Runtime Stacking Model

Mirror the existing soldier-bonus armor loop and the item-stats fold:

1. Start `_maxArmor[side]` from the base `Armor` value (current behavior).
2. Add all `RuleSoldierBonus` directional armor (current behavior).
3. **New:** add directional armor from every equipped inventory item that has a slot, summing per
   side (`sideArmor` adding to both left and right).
4. Clamp each side to `>= 0` (current behavior).
5. Set `_currentArmor[side] = _maxArmor[side]` at unit init (current behavior).

Stacking is additive across items, consistent with how multiple soldier bonuses stack and how
item `stats` stack. No multiplicative interaction.

### Refresh policy (current vs. max armor)

Unlike `UnitStats` (cheap to recompute, no "current" pool), armor has a `current`/`max` split, so
the timing matters:

- **Computed at unit construction / armor refresh**, alongside the soldier-bonus loop in
  `updateArmorFromSoldier` / `updateArmorFromNonSoldier`. This is the natural, already-existing
  point where `_maxArmor` and `_currentArmor` are established for the mission.
- Directional-armor items are expected to live in **loadout-only / armor-type slots** that are set
  up before the mission and locked during it (the `allowCombatSwap: false` / `battleType: BT_ARMOR`
  slot rules from the inventory-typing work). Because the loadout is fixed at battle start, the
  init-time computation is sufficient and avoids the thorny question of how to grow/shrink a
  *current* armor pool mid-battle.
- **Out of scope for this feature:** live re-application of directional armor when an item is
  moved mid-combat. If a later feature allows hot-swapping armor pieces in combat, it will define
  how `_currentArmor` reconciles with a changed `_maxArmor` (e.g. clamp current to new max, or add
  the delta). For now the model is "max armor is fixed by the starting loadout," which matches the
  legacy-DX intent and the sided-slot gating.

## Proposed DX Design

### 1) Data model and YAML loading

- `src/Mod/RuleItem.h` / `src/Mod/RuleItem.cpp`
  - Added `int _frontArmorBonus`, `_sideArmorBonus`, `_rearArmorBonus`, `_underArmorBonus`
    (names chosen to avoid colliding with the existing scalar `_armor` / `getArmor()`).
  - Added a fast `bool _hasDirectionalArmor` no-op flag, set during load.
  - Parsed `frontArmor` / `sideArmor` / `rearArmor` / `underArmor` in `RuleItem::load`.
  - Added getters (`getFrontArmorBonus()`, `getSideArmorBonus()`, `getRearArmorBonus()`,
    `getUnderArmorBonus()`, `hasDirectionalArmor()`).

### 2) Apply equipped-item directional armor to the wearer

- `src/Savegame/BattleUnit.cpp` / `.h`
  - Added a cached `int _maxArmorBase[SIDE_MAX]` holding the per-side armor from the armor rule
    plus soldier bonuses (no items), set in both `updateArmorFromSoldier` and
    `updateArmorFromNonSoldier` right after the existing bonus loop.
  - Added a tracked `int _armorDamage[SIDE_MAX]` (max - current), kept in sync at every armor
    write (combat damage, `setArmor`, `setMaxArmor`) and zeroed when armor is (re)initialised.
    It is **not** persisted; the save still stores only the absolute current armor.
  - Added `recalculateMaxArmor(bool reloadingFromSave)`: sums the directional armor of every
    equipped inventory item that occupies a slot (`sideArmor` adding to both `SIDE_LEFT` and
    `SIDE_RIGHT`), sets `_maxArmor[side] = max(0, _maxArmorBase[side] + itemBonus[side])`, then
    reconciles current armor:
    - normal path: `_currentArmor[side] = clamp(newMax - _armorDamage[side], 0, newMax)` — the
      tracked damage is preserved across the max change, so removing then re-equipping a plate
      never refunds armor (the original `== oldMax` heuristic did, which was the reported bug).
    - `reloadingFromSave`: the just-loaded `_currentArmor` is authoritative, so the damage is
      derived from it (`_armorDamage[side] = newMax - current`).
  - `refreshBaseStats(bool reloadingFromSave = false)` forwards the flag to
    `recalculateMaxArmor`, so every existing refresh point keeps per-side max armor in sync:
    item moves via `BattleItem` use the normal path, and the save-reload rebuild loop in
    `SavedBattleGame::load` passes `true`. The armor arrays are zeroed at the top of the two
    `updateArmorFrom*` paths so the early `refreshBaseStats()` call reads defined values.
  - Known limitation (accepted): because damage is tracked per side and not per item, swapping a
    *different* plate of equal armor onto an already-damaged side is not distinguished from
    re-equipping the same one. Per-item armor-damage tracking was deemed overkill, especially as
    removing armor plates mid-battle is rarely allowed.

### 3) Tooling / visibility

- `src/Ufopaedia/StatsForNerdsState.cpp`
  - The item article now lists `frontArmor` / `sideArmor` / `rearArmor` / `underArmor`
    (reusing the existing translation keys already used by the `Armor` article).

### 4) Localization

- No new strings required: the per-side property names reuse the keys already defined for the
  `Armor` Stats-for-Nerds block.

## Compatibility

- **Backward compatibility:** full. All new keys default to `0` / unset, and `_hasDirectionalArmor`
  short-circuits the new loop for every existing item.
- **Save compatibility:** none of the new data is persisted on the savegame — it is derived from
  rules + the equipped loadout and recomputed at unit init, exactly like the existing soldier-bonus
  armor. `_currentArmor` is already serialized and unaffected in layout.
- **Ruleset compatibility:** existing content is unchanged unless it opts into the new keys. The
  scalar `armor:` field keeps its current meaning.

## Risks and Mitigations

1. **Risk:** confusion between the item's scalar `armor` (its own HP) and the new directional
   bonuses (the wearer's armor).
   - *Mitigation:* keep distinct field/getter names (`getArmor()` vs `getFrontArmorBonus()` …),
     and document the distinction in `DX-Features.md` and Stats-for-Nerds labels.

2. **Risk:** mid-battle armor swaps producing inconsistent `current` vs `max` armor.
   - *Mitigation:* scope this feature to init-time computation and rely on loadout-only / armor
     slot gating; explicitly defer live re-application (see Refresh policy).

3. **Risk:** double counting if a future sided-slot feature also adds armor.
   - *Mitigation:* funnel all item directional armor through the single `RuleItem` helper and the
     one inventory loop, so there is exactly one application point to coordinate with later work.

## Verification

1. Win32 Release build passes after implementation. ✅
2. An item with `frontArmor`/`sideArmor`/`rearArmor`/`underArmor` raises the matching
   `_maxArmor` sides at battle start (verify via Stats-for-Nerds and by taking directional hits).
3. Stats-for-Nerds shows the item's directional armor fields. ✅
4. Removing/omitting the keys leaves armor totals identical to current behavior (no-op default).

## Out of Scope (This Feature)

- Sided inventory slots and `BT_ARMOR` slot typing (separate Phase 4 inventory item).
- Live mid-combat re-application of directional armor on item move.
- Modular-vehicle armor assembly built on top of sided slots.
