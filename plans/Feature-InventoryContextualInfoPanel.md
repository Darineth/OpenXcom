# Feature: Contextual Inventory Info Panel

**Status:** Implemented (Jun 2026). (Supersedes the scratch TODO "Replace the stats display with
weapon shot accuracy and medikit quantities when hovered.")

## Outcome (as shipped)

Hovering an item replaces the soldier stat panel (below the weight line) with information about that
item, built by `InventoryState::showItemStats` and wired through `invMouseOver`/`invMouseOut`
(`updateStats` remains the no-hover path; a `_statPanelShowsItem` flag drives the revert):

- **Granted unit stats** — one `STAT>VAL` line per stat the item modifies, flat and/or percentage
  (e.g. `FA>+5`, `RE>+10%`, `Str>+3 +5%`), using the **same per-stat bar colors** as the soldier
  panel (`barTUs`/`barReactions`/`barFiring`/… from the `stats` interface).
- **Directional armor bonuses** (DX item-armor feature) — `F>/L>/R>/B>/U>` lines in the matching
  armor bar colors (side covers L+R).
- **Item-specific info below the stats** — weapon shot modes with base accuracy
  (`Snap>`/`Aimed>`/`Auto>`/`Burst>` in the firing color, `Melee>` in the melee color) and medikit
  charges (`Heal>` = health color, `Stim>` = energy color, `Pain>` = morale color).
- **Weight line** shows the hovered item's own weight (`Weight>N`) in the normal weight color.
- Items with none of the above fall back to the unit stats; hover-out restores the unit panel.
- Reuses the existing stat-line/armor-row widgets, so it is gated by `showMoreStatsInInventoryView`
  (those rows are hidden when the option is off).
- New `STR_DX_INV_*` strings in `Language/DX/`.

## Summary

Make the inventory's right-side stat panel **context-sensitive**: when the player hovers an item,
the panel shows information relevant to that item's type instead of (or in addition to) the wearer's
unit stats. When nothing is hovered, it shows the unit stats as today.

Per-type content (initial scope):

- **Weapons** — the available shot modes (snap / aimed / auto / burst / melee) with each mode's
  accuracy and TU cost.
- **Medikits** — the available actions (heal / stimulant / pain killer) with remaining charges and
  per-use recovery amounts.
- **Stat-granting items** — any unit-stat bonuses the item provides (`RuleItem.stats` /
  `RuleItem.statModifiers`, the DX item-stats feature).
- **Anything else / nothing hovered** — the unit stat block (current behavior).

This also restores the medikit quantities readout that was dropped when the redundant `_txtAmmo`
text was removed in the [ammo-count feature](Feature-InventoryAmmoCount.md).

## OXCE / OXCE-Plus audit (Jun 2026)

What already exists upstream / in DX, confirmed in code:

- **Unit stat block** in the inventory is a **DX** addition ("Inventory stat display revamp"):
  `InventoryState::updateStats` fills `_txtStatLine1..7` + armor rows at x245 with the wearer's
  HP/TU/RE/FA/TA/PSk/PSt/armor ([InventoryState.cpp:698](../src/Battlescape/InventoryState.cpp#L698)).
  It is **always** the unit's stats — it does not change on hover. Gated by
  `Options::showMoreStatsInInventoryView`.
- **Hover already shows some item info**: `invMouseOver` sets the item name (+weight) in `_txtItem`
  at the bottom, and the right-side ammo preview (`_selAmmo`). An **Alt-hover damage tooltip**
  (`calculateCurrentDamageTooltip`, OXCE-Plus) temporarily replaces the item name with damage info.
  There is **no** shot-mode/medikit/stat panel on hover.
- **The data is readily available** on `RuleItem`:
  - Shot modes: `getConfigSnap/Aimed/Auto/Melee/Burst()` → `RuleItemAction { accuracy, range, shots,
    cost, name }`; plus `getAccuracy*()` / `getCost*()` and `getAccuracyMultiplier()`.
  - Medikit: `getHeal/PainKiller/StimulantQuantity()`, `getWound/Health/Energy/Stun/MoraleRecovery()`,
    `getPainKillerRecovery()`, action names, use costs.
  - Item stats: `getStats()` and `getStatModifiers()` (DX `UnitStats`).
- **`StatsForNerds`** (Ufopaedia) already presents exhaustive item stats, but as a separate full
  screen, not an at-a-glance inventory panel.

Conclusion: the underlying data exists; the contextual *panel* (switching the inventory stat lines
by hovered-item type) is a genuine DX delta. No upstream feature does this.

## Proposed design

### Behavior

- The existing `_txtStatLine1..7` + armor rows become a **generic N-line panel**. A new builder
  decides what to put in them based on hover state:
  - `_mouseHoverItem == nullptr` (or a plain item) → unit stats (today's `updateStats` output).
  - hovered **weapon** → one line per available shot mode: `<Mode>: <acc>%  <tu>TU` (and shots for
    burst/auto). Mode names from the `RuleItemAction.name`/short name or localized `STR_*`.
  - hovered **medikit** → one line per enabled action with charges + recovery (e.g.
    `Heal x3  (HP +N, wound -1)`), respecting `getMedikitType`/action availability.
  - hovered **stat-granting item** → one line per non-zero stat in `getStats()` /
    `getStatModifiers()` (flat `+N` and/or `±N%`).
- Driven from `invMouseOver` / `invMouseOut` / `think` (already the hover hooks) by calling the
  builder; `updateStats` stays the "no hover" path.

### Rendering

- Reuse the 7 stat lines + 5 armor rows (≈12 lines, y24–128 at x245) as the panel area. Lines beyond
  a type's content are blanked. No new widgets needed for a first cut; if more rows are required,
  add lines.
- Localized strings in `Language/DX/` (`STR_DX_INV_*`) for mode/medikit/stat labels; no hard-coded
  text. Numbers via `.arg()`.

### Accuracy shown for weapons

Open question (below): show the **weapon's base accuracy %** per mode, or the **effective** chance
(`weapon% × unit firing accuracy`, the same product the battlescape uses). Effective is more useful
but depends on the wearer; base is simpler and wearer-independent.

## Resolved decisions (as implemented)

1. **Replace with the item's own contributions.** While hovering, the panel (below weight) shows the
   item's stat/armor bonuses + item-specific info in the same `STAT>VAL` style, restored on hover-out.
2. **Weapon accuracy: base % only**, one line per shot mode; melee included.
3. **Medikit: charges only**, per supported action.
4. **Stat items: both flat and %**, non-zero stats only, plus directional armor bonuses.
5. **Trigger: plain hover.** Alt-hover damage tooltip unchanged and independent.
6. **Colors match the soldier panel.** Each value uses its corresponding `stats`-interface bar color
   (final choice — *not* a green/red buff/debuff scheme): shot accuracies → firing color; medikit
   Heal/Stim/Pain → health/energy/morale colors.
7. **Weight shows the hovered item's own weight** (in the normal weight color), reverting on hover-out.
8. **Gating: reuses the stat-line rows**, so it follows `showMoreStatsInInventoryView` (panel hidden
   when that option is off). No research gating.

## Out of scope (for now)

- No new full screen; this stays within the inventory panel.
- No changes to the battlescape action menu or `StatsForNerds`.
