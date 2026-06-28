# Feature: Configurable Weapon Slots and Unload Behavior

**Status:** Implemented (Phase 4) — lean approach (no new ruleset config).

**What shipped vs. this plan:** rather than the configurable `weaponHandlingSlots` / `unloadRules`
policy surface proposed below, DX shipped an automatic, layout-derived version that needs no ruleset
config. `Inventory::unload` now restricts its candidate hands to the hand sections in the unit's
inventory layout, and when no off-hand is free the ejected ammo is best-fit into another inventory
slot (ctrl+click style, ordered by `EXTENDED_INVENTORY_SLOT_SORTING`) before falling back to the
ground; TU cost is computed from the actual destination. Default two-hand layouts are unchanged when
the off-hand is free. The configurable policy surface below is deferred unless a concrete need arises.

## Summary

Promote the current Phase 4 unload-slot bugfix into a full data-driven feature:

1. Configure which inventory slots count as "weapon handling" slots per layout.
2. Configure unload destination rules (weapon destination, ammo destination, fallback policy).
3. Remove hard assumptions that right/left hands always exist.

This feature targets battlescape inventory interaction only. It does not change damage, firing, or
ruleset item balance.

## Motivation

Current unload behavior is hand-centric and assumes the presence of two hand slots:

- `Inventory::unload` initializes free slots from `_inventorySlotRightHand` and `_inventorySlotLeftHand`.
- Shift-unload during battle is restricted to items already in hand slots.
- If no free hand exists, unload fails with `STR_ONE_HAND_MUST_BE_EMPTY`.

That is valid for vanilla-style humanoid layouts, but becomes incorrect for custom inventory layouts
that omit hand slots or use alternate weapon slots.

DX Phase 4 already includes inventory layouts and typed slots. Unload behavior should be part of the
same data backbone rather than a one-off fallback patch.

## OXCE-Plus Audit

Audit result (Jun 2026): upstream behavior in this area is fixed-logic, not configurable.

- No per-layout or per-unit unload destination policy exists.
- The unload path still depends on right/left hand slot pointers.
- Existing options around unload focus on TU costs and quick-unload semantics, not destination
  policy by layout.

Conclusion: this is a genuine DX delta and belongs in Phase 4.

## Goals

- Make unload logic layout-aware and safe on non-hand layouts.
- Allow modders to declare preferred handling slots and destination priority.
- Keep vanilla behavior as default without requiring mod changes.
- Keep TU accounting deterministic and explainable.

## Non-goals

- No UI redesign in this feature (existing inventory UI remains).
- No new item-type balancing rules.
- No automatic role/loadout assignment logic.

## Proposed Ruleset Design

Add optional layout-level configuration (final host class depends on final layout implementation,
currently planned under `RuleInventoryLayout`).

```yaml
inventoryLayouts:
  - type: STR_LAYOUT_SOLDIER

    # Ordered list of slots considered valid weapon-manipulation destinations.
    # Defaults to [STR_RIGHT_HAND, STR_LEFT_HAND] when omitted.
    weaponHandlingSlots:
      - STR_RIGHT_HAND
      - STR_LEFT_HAND

    unloadRules:
      # Where to move the weapon before unloading when needed for TU/action semantics.
      # Values: keepCurrent, firstHandlingSlot, bestFitInventory, ground
      weaponMovePolicy: firstHandlingSlot

      # Where removed ammo/rounds go.
      # Values: secondHandlingSlotOrGround, bestFitInventoryOrGround, groundOnly
      ammoDestinationPolicy: secondHandlingSlotOrGround

      # Whether shift-unload in battle requires the source weapon to already be in a handling slot.
      # Preserves current behavior when true.
      requireHandlingSlotForBattleShiftUnload: true

      # Whether unload fails when weapon cannot be moved to a handling slot.
      # When false, system may continue using best-fit or ground fallback per policy.
      requireFreeHandlingSlot: true
```

Notes:

- Defaults reproduce current behavior.
- `bestFitInventory` and `bestFitInventoryOrGround` use standard fit checks and slot cost rules.
- Ground fallback uses existing arrange/stack behavior.

## Runtime Behavior

For unload action resolution:

1. Resolve the active layout and read its `weaponHandlingSlots` and `unloadRules`.
2. Determine candidate destination slots in policy order.
3. Compute TU costs from actual move path(s) used.
4. Apply move/unload atomically if TU check passes; otherwise show existing TU/space warning.
5. If no destination succeeds:
   - follow configured hard-fail rules; or
   - fall back to ground when policy allows.

## Compatibility

Compatibility defaults preserve current behavior for mods that do not define the new fields.

- Missing `weaponHandlingSlots` -> implied right/left hand set.
- Missing `unloadRules` -> current unload semantics.

This allows incremental adoption by mods with custom layouts.

## Implementation Plan

### Milestone 1: Safe layout-aware core

- Refactor `Inventory::unload` to operate on a resolved list of handling slots, not fixed hand pointers.
- Add helper functions for destination search and policy application.
- Preserve existing behavior through default policy values.

### Milestone 2: Ruleset loading

- Add new layout fields to rule classes/loaders.
- Validate unknown slot IDs and malformed policy strings with clear ruleset errors.

### Milestone 3: TU and warnings polish

- Ensure TU cost calculation matches actual chosen move path.
- Keep warning strings stable where possible; add new localized warnings only if needed.

### Milestone 4: Documentation

- Update `Extended.txt` with new YAML fields and examples.
- Update `DX-Features.md` with shipped behavior and defaults.

## Risks and Mitigations

- Risk: policy complexity introduces hidden edge cases.
  - Mitigation: strict default behavior and clear fallback ordering.
- Risk: TU calculation diverges from actual move outcome.
  - Mitigation: compute cost from concrete selected path, not hypothetical hand assumptions.
- Risk: regressions for vanilla human inventory.
  - Mitigation: explicit compatibility defaults and manual regression checks on vanilla layouts.

## Verification Checklist

- Vanilla soldier unload/unprime behavior unchanged.
- Shift-unload outside battle still works as current quick-unload.
- Layout without hand slots can unload without null-slot assumptions.
- Ammo destination respects configured policy in both TU and non-TU modes.
- Warnings are coherent when no destination is available.

## Open Questions

- Should `weaponHandlingSlots` accept slot tags/types in addition to explicit slot IDs?
- Should per-armor/per-unit overrides be supported now, or deferred until role/loadout work?
- Should AI inventory logic consume the same policy surface later, or remain separate?
