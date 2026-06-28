# Feature: Configurable Inventory Layouts

**Status:** Implemented (Phase 4). Parent feature for
[Configurable Weapon Slots / Unload](Feature-ConfigurableWeaponSlotsUnload.md).

**What shipped:** `RuleInventoryLayout` (`inventoryLayouts:` node, `ref:`/inline sections, `refNode`
reuse, guaranteed ground section) + `Armor.inventoryLayout`; a synthesized default layout preserving
vanilla behavior in historical order; `BattleUnit` resolves/caches its layout from its armor. The
inventory UI (`Inventory`/`AlienInventory` grid, labels, hit-testing), item placement, quick-move
(ctrl+click), and start-of-mission auto-equip all iterate the unit's layout; placement primitives and
the template/equipment-layout apply paths reject non-layout slots (items stay on the ground, with a
warning for templates). `RuleInventory::getCost` was hardened against undefined section-pair costs.
See `DX-Features.md` for the shipped behavior. Remaining edge: battlescape-save items whose slot became
invalid due to a layout/armor definition change after the save are not yet relocated on load.

## Summary

Let a unit's available inventory **sections (slots)** vary by what it is and what it wears, instead
of every unit in the game sharing one hard-coded global grid. A modder defines named layouts
(`RuleInventoryLayout`) — each a complete set of inventory sections — and assigns them per armor.
The engine resolves the active layout per unit and drives the inventory screen, item placement, and
equipment templates from it.

Defaults reproduce today's behavior exactly: with no layouts defined, every unit uses an implicit
layout synthesized from the existing global `invs` set, so unmodified mods are unaffected.

## Motivation

The inventory grid is currently global and identical for every unit:

- `Inventory::drawGrid` iterates `mod->getInventories()` with no unit/armor check
  ([Inventory.cpp:199](../src/Battlescape/Inventory.cpp#L199)).
- `Inventory::getSlotInPosition` does the same
  ([Inventory.cpp:561](../src/Battlescape/Inventory.cpp#L561)).
- Item placement, stacking, and ground arrange all read the same global map
  ([Inventory.cpp:860](../src/Battlescape/Inventory.cpp#L860)).

This blocks several DX goals that depend on a unit's slot set being a property of the unit:

- Outfits/armors that genuinely grant **more or fewer** carrying slots (not just block existing
  cells), or relocate them.
- Non-humanoid layouts (vehicles, drones, beasts) without two hands, or with bespoke mounts.
- Typed/utility slots and role loadouts (Phase 4 follow-ups) that need the slot set to be
  data-defined and per-unit.

## OXCE / OXCE-Plus Audit

Audit result (Jun 2026): **no per-armor or per-unit inventory layout exists upstream.** Verified in
this engine's code (current base: "Extended 8.6.1"):

- **Slots are a single global set.** Loaded once into `Mod::_invs` from the `invs` ruleset
  ([Mod.cpp:2938](../src/Mod/Mod.cpp#L2938)) and exposed via `Mod::getInventories()`. The inventory
  UI iterates that one map unconditionally for both drawing and hit-testing
  ([Inventory.cpp:199](../src/Battlescape/Inventory.cpp#L199),
  [Inventory.cpp:561](../src/Battlescape/Inventory.cpp#L561)). No code path varies the grid by unit.
- **Armor's only inventory control is a boolean.** `allowInv` → `Armor::hasInventory()` toggles the
  inventory screen on/off wholesale ([Armor.cpp:88](../src/Mod/Armor.cpp#L88),
  [Armor.cpp:1064](../src/Mod/Armor.cpp#L1064)). Armor has no slot/section fields.
- **Slot restriction is item-keyed, not unit-keyed.** `RuleItem.supportedInventorySections`
  ([RuleItem.cpp:578](../src/Mod/RuleItem.cpp#L578)) limits which sections a given *item* may occupy;
  `canBePlacedIntoInventorySection(const RuleInventory*)` takes only the section, with no unit/armor
  argument ([RuleItem.cpp:1132](../src/Mod/RuleItem.cpp#L1132)).
- **Soldier has no layout field** either — only `showTypeInInventory`
  ([RuleSoldier.h:94](../src/Mod/RuleSoldier.h#L94)).

Mods that appear to give different armors different slots do so by parking immovable
(`fixedWeapon: true`) zero-weight filler items into existing cells via `Armor.builtInWeapons`
([SavedBattleGame.cpp:1948](../src/Savegame/SavedBattleGame.cpp#L1948)) — a *subtract-only* hack that
can block cells but never add or relocate a slot. This feature is the data-clean superset.

Conclusion: genuine DX delta. No part of it is provided by OXCE/OXCE-Plus.

## Goals

- Define named inventory layouts in the ruleset, each a complete set of sections.
- Resolve an active layout per unit, keyed by armor (the field every unit always has).
- Drive the inventory screen, item placement/arrange, and equipment templates from the resolved
  layout instead of the global map.
- Let a layout add, omit, or relocate sections relative to the vanilla grid (superset of the
  cell-blocking hack), with reuse between layouts via the standard `refNode` mechanic.
- Preserve current behavior exactly when no layouts are defined.

## Non-goals

- No firing/accuracy/damage changes; this is inventory structure only.
- No typed-slot semantics, combat-swap filtering, or utility-slot behavior — those are the
  follow-on Phase 4 items and build *on top* of this.
- No automatic role/loadout assignment.
- No unload destination policy — that lives in the
  [companion doc](Feature-ConfigurableWeaponSlotsUnload.md).

## Proposed Ruleset Design

### New node: `inventoryLayouts`

A layout is an ordered list of sections. Each section reuses the existing `invs` shape (so the
section parser can be shared), and can either define slots inline or reference an existing global
`invs` section by id for reuse.

```yaml
inventoryLayouts:
  - type: STR_LAYOUT_DEFAULT     # required unique id
    sections:
      - ref: STR_RIGHT_HAND      # reuse a globally-defined invs section verbatim
      - ref: STR_LEFT_HAND
      - ref: STR_BELT
      - id: STR_BACK_PACK        # ...or define inline (same fields as an `invs` entry)
        x: 192
        y: 40
        type: 0                  # 0=slot, 1=hand, 2=ground
        slots: [ {x:0,y:0}, {x:1,y:0}, ... ]
        costs: { STR_RIGHT_HAND: 8, ... }

  - type: STR_LAYOUT_BEAST       # e.g. a unit with no hands and a single mount
    sections:
      - ref: STR_LEFT_HAND       # used as the single "mount"
      - ref: STR_BELT
```

### Reuse between layouts (`refNode`)

Layouts use the engine's standard `refNode` parent mechanic (same as `RuleInventory` and most other
rule types): the parent node is loaded first, then this node's values override it. A child that omits
`sections:` inherits the parent's sections; a child that provides `sections:` replaces them wholesale.
Combined with YAML anchors, this covers "a family of similar layouts" without copy-paste:

```yaml
inventoryLayouts:
  - type: STR_LAYOUT_BASE
    sections: &humanoidSections
      - ref: STR_RIGHT_HAND
      - ref: STR_LEFT_HAND
      - ref: STR_BELT
      - ref: STR_BACK_PACK
  - type: STR_LAYOUT_VARIANT
    refNode: { sections: *humanoidSections }   # inherit, then override other fields as needed
```

There is intentionally **no** `removeSections:`/subtract operator and no implicit "include all global
sections" — a layout is the sections it lists (or inherits via `refNode`). Curated, explicit lists
avoid the screen-position overlap that an implicit "everything + my custom slot" would invite.

### Assignment

Layouts are keyed off **armor** only. Every `BattleUnit` always has an armor (soldiers, aliens,
HWPs, beasts alike), so armor-keying can express every layout without a second key. There is
deliberately no `RuleSoldier.inventoryLayout` / `Unit.inventoryLayout`; a creature's slots live on
its outfit/body armor.

```yaml
armors:
  - type: STR_HEAVY_SUIT
    inventoryLayout: STR_LAYOUT_HEAVY

  - type: STR_BEAST_HIDE
    inventoryLayout: STR_LAYOUT_BEAST
```

### Resolution order (per unit)

1. `Armor.inventoryLayout`, if set.
2. else the **implicit default layout** synthesized from the global `invs` set (current behavior).

The `INV_GROUND` section is always available regardless of layout (you can always drop to the
ground); layouts that omit a ground section get the global one appended implicitly.

## Runtime Behavior

- Add `RuleInventoryLayout` (`src/Mod/`) holding an ordered `std::vector<const RuleInventory*>` of
  resolved sections (inline-defined sections are owned by the layout; `ref:` entries point at
  `Mod::_invs`). Register the loader in `Mod.cpp` alongside `invs`, ordered *after* `invs` so refs
  resolve.
- Synthesize `STR_INVENTORY_LAYOUT_DEFAULT` from `_invs` at mod-load finalize time so a layout always
  exists. Sections are collected in `_invs` map (id) iteration order to match how the engine
  historically walked the global inventory map, keeping auto-placement order byte-for-byte unchanged.
- Add `Armor::getInventoryLayout()` (resolved pointer), and a `BattleUnit::getInventoryLayout()` that
  applies the resolution order (armor → default) and caches the result.
- Refactor inventory consumers to iterate the **unit's** layout sections instead of
  `mod->getInventories()`:
  - `Inventory::drawGrid`, `getSlotInPosition`, item draw/stack/arrange
    ([Inventory.cpp:199](../src/Battlescape/Inventory.cpp#L199),
    [561](../src/Battlescape/Inventory.cpp#L561),
    [860](../src/Battlescape/Inventory.cpp#L860)).
  - `AlienInventory` ([AlienInventory.cpp:121](../src/Battlescape/AlienInventory.cpp#L121)).
  - Auto-placement for fixed/built-in items and pre-battle equipment templates so items land in
    sections the unit's layout actually has.
- Keep `RuleItem.supportedInventorySections` working unchanged — it filters *which* of the unit's
  available sections an item may enter.

## Compatibility

- **No layouts defined** → implicit default == current global grid. Zero behavior change; this is
  the regression-safety contract.
- **Save compatibility:** items persist their slot by `RuleInventory` id (string), so existing saves
  load unchanged under the default layout. A unit whose resolved layout *changes* (e.g. armor swap,
  or a mod update that removes a section) may hold items in a now-absent section — on load/equip,
  reassign such items via best-fit into the new layout, else drop to ground, and log it. No silent
  loss.
- **`allowInv: false`** still short-circuits before layout resolution (no inventory at all).

## Implementation Plan

### Milestone 1: Rule classes + loader
- Add `RuleInventoryLayout` and the `inventoryLayouts` loader; share the section parser with `invs`.
- Add an `inventoryLayout` field + resolved getter on `Armor`.
- Synthesize the implicit default layout from `_invs`; validate refs, unique section ids per layout,
  and at most one ground section.
- Register new source files in `src/CMakeLists.txt` and `OpenXcom.2010.vcxproj`.

### Milestone 2: Per-unit resolution
- `BattleUnit::getInventoryLayout()` with the resolution order + cache; recompute on armor change.

### Milestone 3: Engine consumers
- Route all `getInventories()` inventory-UI/placement reads through the unit's layout.
- Update auto-placement (built-in/fixed items, equipment templates) to target layout sections.

### Milestone 4: Edge cases + polish
- Layout-change item reassignment/fallback on load and on armor swap, with a clear log.
- Confirm `supportedInventorySections` interaction.

### Milestone 5: Documentation
- Update `DX-Features.md` (field names, defaults, resolution order) and tick
  `DX-Implementation-Checklist.md`. DX-only strings (any new warnings) go in `Language/DX/`.

## Risks and Mitigations

- **Regressing the vanilla grid.** Mitigation: implicit default synthesized from `_invs`; manual
  regression on a stock humanoid before/after.
- **Items stranded in vanished sections after a layout change.** Mitigation: best-fit reassignment →
  ground fallback → logged; never silently dropped.
- **Scattered `getInventories()` call sites.** Mitigation: funnel through one
  `BattleUnit::getInventoryLayout()` accessor; audit every iteration of the global map.
- **Ground section assumptions.** Mitigation: always guarantee an `INV_GROUND` section.

## Verification Checklist

- Mod with no `inventoryLayouts` behaves identically to today (grid, placement, templates).
- An armor with a custom layout shows exactly its sections; an armor without one falls back to the
  implicit default; aliens/HWPs resolve via their armor like everything else.
- A layout can add a section the global grid lacks, and relocate/remove sections.
- A unit with no hand slots opens inventory without null-slot crashes (coordinates with the unload
  feature).
- Saved game with a mid-campaign layout change reassigns or grounds stranded items, with a log line.
- `allowInv: false` still disables inventory entirely.

## Resolved Decisions

Settled during design review (Jun 2026):

- **Keying: armor only.** Layouts attach to `Armor.inventoryLayout`; no `RuleSoldier`/`Unit` field.
  Every unit always has an armor, so a single key expresses everything; a creature's slots live on
  its body/outfit armor.
- **No tag/type section references (yet).** Sections are targeted by explicit id only. Tag/type
  indirection is deferred to the typed-slots / `INV_UTILITY` follow-up, when there's an actual
  consumer for it.
- **Aliens / HWPs use armor too.** They resolve via their armor like everything else; no separate
  `units:`-level layout field.
- **No per-layout `costs` override.** A `ref:` reuses a section verbatim (including its costs); to
  change costs, define that section inline in the layout. No merge/override layer.
- **Reuse via `refNode`, not a bespoke operator.** Layouts use the engine's standard `refNode` parent
  mechanic for inheritance (parent loaded first, child overrides; child `sections:` replaces the
  inherited list wholesale). No `extends:`/`removeSections:` and no implicit "include all global
  sections" — a layout is exactly the sections it lists or inherits. Curated lists sidestep the
  screen-position overlap that an implicit include-all would invite.
- **No overlap validation yet.** The loader does not check for sections whose screen cells overlap;
  modders are responsible for positioning. A validation pass can be added later if it proves needed.
- **Default layout preserves legacy order.** The synthesized `STR_INVENTORY_LAYOUT_DEFAULT` lists the
  global sections in `_invs` map (id) order, matching historical iteration so auto-placement is
  unchanged.
