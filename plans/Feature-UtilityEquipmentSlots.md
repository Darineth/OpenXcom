# Feature: Utility Equipment Slots (`INV_UTILITY` / `INV_EQUIP`)

**Status:** Implemented (Jun 2026). Builds clean (Release/Win32). Final Phase 4 item. Depends on
the already-shipped *Typed Inventory Slots* and *Configurable Inventory Layouts* features.
Shipped as the full legacy re-port (`INV_UTILITY` + reserved `INV_EQUIP`, `isSingleItem()` family,
per-unit `getUtilitySlot()`/`getUtilityItem()`), plumbing-only — no gameplay consumer yet.
`AlienInventory` stays a hands-only viewer (utility items don't show there, by design).

Checklist item: *Phase 4 — Utility equipment slots — `INV_UTILITY`. (needs typed slots).*

## Decisions (confirmed with user, Jun 2026)

- **Approach = full legacy re-port.** Add `INV_UTILITY` (and its sibling `INV_EQUIP`) as new
  `InventoryType` enum values that behave like a **hand** for fit/cost/geometry/ownership
  (single occupant, "always fits"), plus a **per-unit cached slot** with
  `getUtilitySlot()` / `getUtilityItem()` accessors — mirroring the existing hand resolution on
  `RuleInventoryLayout`. This is faithful to legacy DX §23 and gives downstream systems a clean
  semantic handle.
- **Plumbing only.** No new gameplay mechanic consumes `getUtilityItem()` yet. This lays the
  data/inventory plumbing so layouts can define utility slots and items occupy them; later
  features (Phase 8 effects / light equipment, Phase 7 roles, Phase 10 modular vehicles) wire in
  as consumers. Because a utility slot is "just another section" in a layout, the typed-slot
  rules (`battleType`, `allowCombatSwap`, `countStats`, `costs`) compose with it for free.

## Why a distinct enum type (not just a typed `INV_SLOT`)

DX already lets a modder build a dedicated quick-access pocket as a typed `INV_SLOT` section
(`battleType` filter + a bespoke slot grid in a layout). A distinct `INV_UTILITY` type adds two
things that the slot machinery can't express on its own:

1. **Hand-like single-occupant behavior** — one item, "always fits," a single bounding box
   rather than an N×M slot grid (no fiddly per-cell `slots:` list to author).
2. **A per-unit semantic handle** — `BattleUnit::getUtilityItem()` so any future system can ask
   "what's in this unit's utility slot" without knowing the modder's section id. This is the
   piece the *Configurable Inventory Layouts* doc explicitly deferred "until there's an actual
   consumer"; the re-port builds the handle now so the consumer side is trivial later.

## OXCE / OXCE-Plus + DX audit (Jun 2026)

- **Enum today:** `enum InventoryType { INV_SLOT, INV_HAND, INV_GROUND }`
  ([RuleInventory.h:33](../src/Mod/RuleInventory.h#L33)). No utility/equip type. No upstream
  OXCE equivalent — this is a genuine DX delta.
- **Handedness is already decoupled from type.** `_hand` (`isRightHand()`/`isLeftHand()`) is a
  separate slot property from `_type` ([RuleInventory.cpp:63-84](../src/Mod/RuleInventory.cpp#L63)).
  Utility slots are **not** hands, so they are never wielded, reaction-fired from, or treated as
  the active hand — that logic keys off `isRightHand()/isLeftHand()`, which utility slots never
  set. Good: the wielding path needs no changes.
- **Layout already caches its hands.** `RuleInventoryLayout::resolveHands()`
  ([RuleInventoryLayout.cpp:128](../src/Mod/RuleInventoryLayout.cpp#L128)) scans the resolved
  section list for the right/left hand and caches pointers; `BattleUnit::addItem` etc. read
  `layout->getRightHand()`. The utility slot resolution mirrors this exactly.
- **Typed-slot rules already apply per-section regardless of type.** `battleType`,
  `allowCombatSwap`, `countStats`, and `costs` are enforced on the destination section in
  `Inventory`/`BattleUnit` independent of `_type`, so they work on a utility section with no
  extra wiring.
- **`type:` parses as an int** via `reader.tryRead("type", _type)`. Appending
  `INV_UTILITY = 3`, `INV_EQUIP = 4` to the enum makes `type: 3` / `type: 4` load with no parser
  change. **Append only** (do not renumber `INV_SLOT/HAND/GROUND` = 0/1/2) — saved games and
  existing rulesets depend on those ordinals.
- **Legacy §23** ([Legacy-DX-Features.md:1131](../Legacy-DX-Features.md#L1131)) confirms the
  shape: `UTILITY_W = 2 × UTILITY_H = 2`, hand-family fit/cost, armor-defined per-unit
  `_utilitySlot` / `getUtilityItem()`.

## Proposed design

### Enum + `RuleInventory` (the type)

```cpp
enum InventoryType { INV_SLOT, INV_HAND, INV_GROUND, INV_UTILITY, INV_EQUIP }; // append only
```

- **Box dimensions.** `UTILITY_W/H` and `EQUIP_W/H` constants (legacy `2×2`) are the **defaults**;
  a section may override its box via **rule-defined `width:`/`height:`** (slot cells), so an equip
  slot for larger gear is a ruleset value, not a compile-time constant. Two small getters return
  the bounding box for *any* single-occupant type so the geometry code doesn't branch per-type:
  ```cpp
  int getBoxWidth()  const;  // HAND_W fixed; utility/equip: rule `width`  or the type default
  int getBoxHeight() const;  // HAND_H fixed; utility/equip: rule `height` or the type default
  ```
  Hands stay fixed (`2×3`, always-fit). `getBoxWidth/Height()` feed grid draw, `checkSlotInPosition`,
  and the utility/equip size-check, so a rule-set size flows everywhere automatically.
- **Single-item predicate.** Add `bool isSingleItem() const { return _type == INV_HAND ||
  _type == INV_UTILITY || _type == INV_EQUIP; }` — "single occupant, always fits, single bounding
  box." This is the join point for the re-port: geometry/fit/ownership call sites switch from
  `getType() == INV_HAND` to `isSingleItem()`; **wielding/reload** call sites stay strict
  `INV_HAND`.

### Geometry / fit / draw — convert to `isSingleItem()`

| Call site | Current | Change |
|---|---|---|
| [RuleInventory::checkSlotInPosition](../src/Mod/RuleInventory.cpp#L160) | `if (_type == INV_HAND)` uses `HAND_W/H` | `if (isSingleItem())` using `getBoxWidth/Height()` |
| [RuleInventory::fitItemInSlot](../src/Mod/RuleInventory.cpp#L212) | `if (_type == INV_HAND) return true;` | hand → unconditional `true`; **utility/equip → size-check** the item against `getBoxWidth/Height()` (unlike a hand, an oversized item is rejected) |
| [Inventory::drawGrid](../src/Battlescape/Inventory.cpp#L277) | `else if (getType() == INV_HAND)` draws box | `else if (isSingleItem())` with `getBoxWidth/Height()` |
| [Inventory::drawItems](../src/Battlescape/Inventory.cpp#L390) | `else if (getType() == INV_HAND)` positions sprite; else `continue` (item not drawn!) | `else if (isSingleItem())` so **utility items render** |
| [BattleUnit::fitItemToInventory](../src/Savegame/BattleUnit.cpp#L3076) | `if (slot->getType() == INV_HAND)` single-occupant place (no fit check) | `if (slot->isSingleItem())` **and** `slot->fitItemInSlot(rule, 0, 0)` so auto-equip honors the utility/equip size-check too |
| [BattleItem::occupiesSlot](../src/Savegame/BattleItem.cpp#L671) | `if (getType() == INV_HAND) return true;` | `if (isSingleItem()) return true;` |
| [BattleItem::getMoveToCost](../src/Savegame/BattleItem.cpp#L598) | `INV_HAND && dest INV_GROUND` "easy to drop" | source `isSingleItem()` |

> **`drawItems` is the load-bearing one:** today anything that isn't `INV_SLOT`/`INV_HAND` hits
> `else { continue; }` and is never drawn. Without this conversion, a utility item would be
> invisible on the paperdoll.

### Wielding / reload — stays strict `INV_HAND` (no change)

These are about an item being *held in a hand* (wielded/reloaded), not about slot geometry, and
must **not** include utility/equip:

- Two-handed indicator ([Inventory.cpp:405](../src/Battlescape/Inventory.cpp#L405)) — a wielding
  visual, stays hand-only. *(The item-sprite position and ammo-count badge, by contrast, ARE
  generalized to utility/equip: sprites draw at the box cell-origin like a grid slot rather than
  with the hand sprite offset, and the ammo badge sits at the box top-right corner via
  `getBoxWidth()`. Only hands use the 2×3 hand sprite offset.)*
- Battle shift-unload "only hand weapons" ([Inventory.cpp:903](../src/Battlescape/Inventory.cpp#L903)).
- Reload move-cost / free-hand search
  ([Inventory.cpp:1165](../src/Battlescape/Inventory.cpp#L1165), [:1200](../src/Battlescape/Inventory.cpp#L1200),
  [:1569](../src/Battlescape/Inventory.cpp#L1569), [:1626](../src/Battlescape/Inventory.cpp#L1626),
  [BattleUnit.cpp:3852](../src/Savegame/BattleUnit.cpp#L3852)).

### Per-unit utility slot (the handle)

- **`RuleInventoryLayout`**: extend `resolveHands()` (or a parallel `resolveUtility()`) to cache
  the **first** `INV_UTILITY` section into `_utilitySlot`, with `getUtilitySlot()`. (One utility
  slot per layout for now; multiple is a future extension.) Two `INV_UTILITY` sections in one
  layout is **not** a uniqueness error the way two same-side hands are (each still works as a
  normal single-item slot; only the per-unit handle picks one), so we take the first and emit a
  `LOG_WARNING` rather than throwing — the extras keep functioning as slots.
- **`BattleUnit`**: `const RuleInventory* getUtilitySlot() const` → delegates to
  `getInventoryLayout()->getUtilitySlot()` (null-safe; mod-default fallback if no layout).
  `BattleItem* getUtilityItem() const` → scans `_inventory` for the item whose
  `getSlot() == getUtilitySlot()` (mirrors `getRightHandWeapon()`).
- **`INV_EQUIP`**: recognized as a hand-family type (enum + geometry + fit), but its per-unit
  accessor is **deferred** — legacy never documented a distinct `INV_EQUIP` consumer, so adding
  `getEquipItem()` now would be guessing semantics (the same mistake we avoided with
  `allowGenericItems`). It ships as a usable single-occupant slot type for forward-compat; the
  dedicated handle lands when a real consumer defines what "equip" means.

### Script bindings

Add `INV_UTILITY` / `INV_EQUIP` custom consts in `RuleInventory::ScriptRegister`
([RuleInventory.cpp:395](../src/Mod/RuleInventory.cpp#L395)) alongside the existing
`INV_GROUND/SLOT/HAND`.

### Save compatibility

**None required.** Items serialize their slot by id, so an item in a utility section round-trips
with the existing format. The slot type and per-unit handle are derived from rules at load, not
persisted. No `SavedGame`/`SavedBattleGame` schema change.

## Files touched

- [src/Mod/RuleInventory.h](../src/Mod/RuleInventory.h) / [.cpp](../src/Mod/RuleInventory.cpp) —
  enum values, box-dim constants + getters, `isSingleItem()`, geometry conversions, script consts.
- [src/Mod/RuleInventoryLayout.h](../src/Mod/RuleInventoryLayout.h) / [.cpp](../src/Mod/RuleInventoryLayout.cpp) —
  `_utilitySlot` cache + `getUtilitySlot()`.
- [src/Savegame/BattleUnit.h](../src/Savegame/BattleUnit.h) / [.cpp](../src/Savegame/BattleUnit.cpp) —
  `getUtilitySlot()` / `getUtilityItem()`, `fitItemToInventory` hand-like.
- [src/Battlescape/BattleItem.cpp](../src/Savegame/BattleItem.cpp) — `occupiesSlot`,
  `getMoveToCost` hand-like.
- [src/Battlescape/Inventory.cpp](../src/Battlescape/Inventory.cpp) — `drawGrid`, `drawItems`
  hand-like.
- `bin/standard/dx-test/dx-test.rul` — a `STR_UTILITY` section (`type: 3`) and a test layout
  that includes it, for manual verification.
- `DX-Features.md`, `DX-Roadmap.md` — docs.

No new C++ player-facing string is strictly required (a utility section's `id` is its label, like
any section); a default `STR_UTILITY` goes in the test ruleset, not engine code.

## Verification (manual, in-game)

1. Define a layout with a `STR_UTILITY` (`type: 3`) section; assign it to a test armor.
2. In the inventory screen: the utility box draws (grid), an item drops into it and **renders**,
   the move-cost is charged, and `occupiesSlot` blocks a second item.
3. With `allowCombatSwap: false` on the utility section, the item is locked in/out during battle
   (typed-slot rule composes).
4. With `battleType: BT_GRENADE` (etc.), only matching items fit (typed-slot rule composes).
5. Build clean (Release/Win32) with `-Wall -Wextra`.

## Out of scope

- A gameplay consumer of `getUtilityItem()` (quick-use action, effects/light equipment) — later
  phases.
- `INV_EQUIP` per-unit handle (`getEquipItem()`) — deferred until a consumer defines it.
- Multiple utility slots per unit (first-wins for now).
- Utility-slot ammo-count badge (wielding-only polish for now).
