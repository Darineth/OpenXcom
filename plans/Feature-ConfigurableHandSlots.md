# Feature: Configurable Hand Slots

**Status:** Implemented (Jun 2026). Builds clean (Release/Win32). `hand:` field on `RuleInventory`
with legacy id fallback; **each `RuleInventoryLayout` resolves and validates its own hands**
(`resolveHands`), so handedness is unique *per layout*, not globally — the same `hand: left` section
can appear in many layouts. Auto-equip (`BattleUnit::addItem`, `BattlescapeGame`) resolves hands from
the unit's own layout; `Mod::getInventoryRightHand/LeftHand` return the default layout's hands as a
global convenience. `_activeHand` / `_preferredHandForReactions` are a handedness enum with save
migration. All group-(B) two-hand logic untouched. The inventory-screen hand shortcuts (reload
off-hand, ctrl-click-to-hand) re-cache the selected unit's hands from its layout on every unit/armor
switch (`Inventory::refreshHandSlots`), with null guards for layouts that omit a hand.

## Resolved decisions (Jun 2026)

- **Q1 — `hand: right|left` enum** (values `right`/`left`/`none`, default `none`).
- **Q2 — Store handedness, not the id.** `_activeHand` / `_preferredHandForReactions` become a
  handedness enum; saves store `right`/`left` and a load-time migration maps the old
  `STR_RIGHT_HAND`/`STR_LEFT_HAND` strings.
- **Q3 — Two-hand model locked, validated per layout.** Each inventory layout may have at most one
  `right` and one `left` hand (error on extras *within a layout*); the same hand section can be reused
  across layouts. True N-hands (more than two in one layout) deferred.
- **Q4 — Auto-equip strategy unchanged** (firearm → right first, melee/psi → left), just resolved via
  the property.

## Summary

Decouple a slot's **handedness** (is this the right hand? the left hand?) from the hard-coded section
ids `STR_RIGHT_HAND` / `STR_LEFT_HAND`. Today the engine recognises hands *only* by those two literal
ids; a custom or layout-specific hand slot named anything else is not a "real" hand, so it can't
receive auto-equipped weapons, fire, react, render a held item, or be toggled as the active hand.

The proposed change makes handedness a **slot property** (`hand: right` / `hand: left`) that any
`invs:` section can declare, and routes every "the right/left hand" lookup through that property
instead of the magic ids. A legacy fallback keeps `STR_RIGHT_HAND`/`STR_LEFT_HAND` working with no
ruleset changes, so the base game and all existing mods are unaffected.

**Scope is hand *identity*, not hand *count*.** We keep the two-hand model (exactly one right, one
left). Generalising to N hands is a much larger, separate effort (see "Out of scope").

## Motivation

From the running scratch notes: *"Hand-handling slots for weapon shenanigans. Don't like the fixed
left/right hand slots at all."* Concretely, this blocked the per-armor inventory-layout work: an
attempt to give the `ONEHAND` test layout an *inline/renamed* right hand failed because auto-equip
(`BattleUnit::addItem` → `getInventoryRightHand()`), the cached `Inventory::_inventorySlotRightHand`,
and hand-weapon resolution (`getRightHandWeapon` → `slot->isRightHand()`) all resolve the right hand
as the global section with id `STR_RIGHT_HAND`. A layout could only ever reuse that exact id; it could
not define a differently-named or specialised hand.

This feature is the unlock for differently-named hand slots, per-armor hand layouts (e.g. a one-armed
unit, a creature whose "hand" is a `STR_MAW`), and is a prerequisite for any later N-hand work.

## OXCE / OXCE-Plus audit (Jun 2026)

Upstream keeps the two hard-coded hands; there is **no** configurable-hand-slot mechanism to reuse.
What OXCE *does* provide is all **item-side** two-hand logic, which is orthogonal and stays as-is:

- `oneHandedPenalty` / `oneHandedPenaltyGlobal` (accuracy when a two-handed weapon is used with the
  other hand occupied), `blockBothHands` (item forbids dual-wield), `isExplodingInHands`,
  `isTwoHanded` (`Extended.txt`). These are `RuleItem` flags about *weapons*, not about *slots*.
- The hand *slots* themselves are still recognised only as `STR_RIGHT_HAND` / `STR_LEFT_HAND`.

Conclusion: slot-side configurable handedness is a genuine DX delta.

## What "hard-coded hands" is today

Hands are identified by **two mechanisms**, both keyed to the literal ids. Everything else is a
*consumer* of these:

1. **`RuleInventory::_hand`** (0 = not a hand, 1 = left, 2 = right) is set in
   [`RuleInventory::load`](../src/Mod/RuleInventory.cpp#L62) purely from
   `_id == "STR_RIGHT_HAND"` / `"STR_LEFT_HAND"`, exposed as `isRightHand()` / `isLeftHand()`.
2. **`Mod::getInventoryRightHand()` / `getInventoryLeftHand()`**
   ([Mod.h:773](../src/Mod/Mod.h#L773)) are literally `getInventory("STR_RIGHT_HAND")` /
   `getInventory("STR_LEFT_HAND")`.

Plus two `BattleUnit` **string state members** that store a hand id directly:
`_activeHand` and `_preferredHandForReactions` (both default/compared against the literal ids;
[BattleUnit.cpp:109](../src/Savegame/BattleUnit.cpp#L109),
[:3789](../src/Savegame/BattleUnit.cpp#L3789),
[:3916](../src/Savegame/BattleUnit.cpp#L3916)).

### The consumers split into two groups

**(A) Identity lookups — the only thing this feature changes.** These just need to find "the
right/left hand slot/weapon"; they don't care *why* a slot is the right hand:

| Area | Sites |
|---|---|
| Mod hand-slot accessors | `getInventoryRightHand/LeftHand` ([Mod.h:773-775](../src/Mod/Mod.h#L773)) |
| `_hand` classification | [RuleInventory.cpp:62-72](../src/Mod/RuleInventory.cpp#L62), `isRightHand/isLeftHand` |
| Hand-weapon getters | `BattleUnit::getRightHandWeapon/getLeftHandWeapon` ([:3738](../src/Savegame/BattleUnit.cpp#L3738)) — iterate inventory by `isRightHand/isLeftHand` |
| Inventory UI cache | `_inventorySlotRightHand/LeftHand` ([Inventory.cpp:159](../src/Battlescape/Inventory.cpp#L159)) and its auto-place uses (962, 1156-1169, 1199, 1551) |
| Auto-equip placement | `BattleUnit::addItem` ([:3091](../src/Savegame/BattleUnit.cpp#L3091), 3194/3199/3248), `BattlescapeGame.cpp:2971/2981` |
| Active-hand + reaction-hand **storage** | `_activeHand`, `_preferredHandForReactions`, `getActiveHand`, `isRightHandPreferredForReactions`, the toggles ([:3771-3925](../src/Savegame/BattleUnit.cpp#L3771)) |
| Script bindings | `getRuleInventoryRightHand/LeftHand` ([Mod.cpp:6771](../src/Mod/Mod.cpp#L6771)), `getRightHandWeapon/getLeftHandWeapon`, `isRightHand/isLeftHand` |

**(B) Real two-hand game/render logic — stays two-handed, unchanged in behavior.** These genuinely
depend on left-vs-right and must keep working; they keep asking "is this the right hand?" via the
property, so once (A) is generalised they need *no logic change*:

- **Dual-wield accuracy penalty** — applied when both hands hold weapons
  ([BattleUnit.cpp:2575](../src/Savegame/BattleUnit.cpp#L2575)).
- **Fatal-wound → arm accuracy** — right hand uses `BODYPART_RIGHTARM` wounds, left uses
  `BODYPART_LEFTARM` ([:2614](../src/Savegame/BattleUnit.cpp#L2614)).
- **`blockBothHands`** enforcement ([SavedBattleGame.cpp:1231](../src/Savegame/SavedBattleGame.cpp#L1231)).
- **Unit sprite rendering** — held-item L/R offsets, blit order per facing, two-handed sprite
  selection ([UnitSprite.cpp:199](../src/Battlescape/UnitSprite.cpp#L199),
  [:960](../src/Battlescape/UnitSprite.cpp#L960), [:1009](../src/Battlescape/UnitSprite.cpp#L1009),
  [:1468](../src/Battlescape/UnitSprite.cpp#L1468)); alien-inventory mirror offsets
  ([AlienInventory.cpp:133](../src/Battlescape/AlienInventory.cpp#L133)).
- **Death arm animation** ([BattleUnit.cpp:3743](../src/Savegame/BattleUnit.cpp#L3743)).
- **Reaction-fire weapon selection & per-hand disable** — `getWeaponForReactions`,
  `_reactionsDisabledForLeft/RightHand`, `getBestWeapon` active-hand tiebreaker, TileEngine reaction
  filtering ([BattleUnit.cpp:3621](../src/Savegame/BattleUnit.cpp#L3621),
  [:3930](../src/Savegame/BattleUnit.cpp#L3930),
  [TileEngine.cpp:2727](../src/Battlescape/TileEngine.cpp#L2727)).
- **UI hand buttons / tooltips / skill weapon lookup / surrender-drop / debrief unload** — all just
  call the getters in (A).

The whole point of the design is that group (B) is large but **untouched**: by making (A) resolve
handedness from a property, the two-hand logic keeps working on whatever slots are flagged.

## Proposed design

### 1. `hand:` field on `RuleInventory`

```yaml
invs:
  - id: STR_RIGHT_HAND
    hand: right        # right | left | none (default none)
    type: 1
    # ...
```

- Parse into the existing `_hand` int (`right` → 2, `left` → 1, `none`/absent → 0). `isRightHand()` /
  `isLeftHand()` are unchanged.
- **Legacy fallback:** if `hand:` is absent, fall back to the current id check
  (`_id == "STR_RIGHT_HAND"` → right, `"STR_LEFT_HAND"` → left). So existing rulesets and the base
  `STR_RIGHT_HAND`/`STR_LEFT_HAND` sections keep their handedness with zero changes.
- **Validation is per layout, not global.** `RuleInventoryLayout::resolveHands()` (run in `afterLoad`
  and for the synthesized default layout) scans the layout's resolved sections and caches its single
  right/left hand, erroring if one layout has two of a side. The *same* `hand: left` section may
  appear in many layouts; only two left hands *in one layout* is an error.

### 2. Resolve hands per layout / per unit

- `RuleInventoryLayout::getRightHand()/getLeftHand()` expose the layout's hand sections.
- Per-unit code (auto-equip in `BattleUnit::addItem` and `BattlescapeGame`) resolves hands from the
  unit's own layout (`getInventoryLayout()->getRightHand()`), so a renamed hand works.
- `Mod::getInventoryRightHand()/LeftHand()` keep working as a global convenience, now returning the
  **default layout's** hands (vanilla `STR_RIGHT_HAND`/`STR_LEFT_HAND` unmodded) — used only as a
  fallback when no unit/layout is in context.
- The inventory screen (`Inventory`) caches the *selected unit's* hand sections and refreshes them in
  `setSelectedUnit` (and the ctor), so its hand shortcuts follow the displayed unit; null when the
  layout omits a hand, guarded at every use.

### 3. Generalise the two string state members

`_activeHand` and `_preferredHandForReactions` currently store the literal hand id. Decouple them
from the id — **store handedness, not the slot name.** (See Q2 for the chosen representation.) The
toggles/getters/comparisons (`setActiveRightHand`, `getActiveHand`, `isRightHandPreferredForReactions`,
…) operate on handedness, and the save format migrates old `"STR_RIGHT_HAND"`/`"STR_LEFT_HAND"`
strings on load.

### 4. Everything in group (B) is unchanged

They already ask via `isRightHand()` / the getters; once (1)-(3) land they transparently work on the
flagged slots.

### Files touched (anticipated)

- `src/Mod/RuleInventory.{h,cpp}` — `hand:` parse + legacy fallback (centralised handedness).
- `src/Mod/Mod.{h,cpp}` — property-based `getInventoryRightHand/LeftHand`, caching, validation.
- `src/Savegame/BattleUnit.{h,cpp}` — `_activeHand` / `_preferredHandForReactions` representation +
  save migration; the toggles/getters.
- No change expected to UnitSprite / TileEngine / AlienInventory / firing logic (group B).

## Backward compatibility

- No ruleset uses `hand:` today → the legacy id fallback makes every existing mod and the base game
  behave identically.
- Save migration maps old `activeHand: STR_RIGHT_HAND` (and the reaction-preference string) to the
  new representation on load; new saves write the new form.
- Script API (`isRightHand`, `getRightHandWeapon`, `getRuleInventoryRightHand`, …) keeps the same
  names and semantics — just backed by the property.

## Open questions (need answers before coding)

- **Q1 — Field shape.** `hand: right|left` enum (proposed), or a pair of bools
  `rightHand: true` / `leftHand: true`, or reuse a numeric `hand: 1|2`? The enum reads best; confirm.
- **Q2 — `_activeHand` / `_preferredHandForReactions` representation.** Store handedness as an enum
  (`right`/`left`, cleanest, needs save-format migration) — or keep storing a slot **id string** but
  resolve the right/left ids dynamically (no save change, but re-introduces id coupling)? Recommend
  the enum + migration.
- **Q3 — Two-hand invariant.** Confirm we *require* at most one right + one left this release (error
  otherwise), with true N-hands explicitly deferred. If a layout omits a hand entirely, that already
  works (the unload feature handles missing hands).
- **Q4 — Auto-equip strategy stays literal.** `addItem` puts firearms in the right hand first, melee
  /psi in the left. Keep that exact strategy (just resolved via the property), or make it
  configurable later? Recommend: keep as-is now.

## Out of scope

- **N generic hands** (more than two, or unordered "mounts"). The audit shows dual-wield accuracy,
  sprite L/R offsets + blit order, arm-wound mapping, and the reaction active/preferred/disabled
  per-hand UI are all built on exactly-two-hands with left/right meaning. Generalising those is a
  separate, much larger feature; this doc deliberately keeps the two-hand model and only frees the
  *identity* from the magic ids.
- Item-side two-hand rules (`isTwoHanded`, `oneHandedPenalty`, `blockBothHands`) — already exist
  upstream, unchanged.
- Per-armor/per-unit *override* of which slot is the hand beyond what inventory layouts already give
  (a layout simply lists the flagged hand section it wants).
