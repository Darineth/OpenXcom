# Feature: Item User Restrictions (vehicle vs. soldier gear)

**Status:** Implemented (Jul 2026). Builds clean (Release/Win32); the demonstrator mod loads with
zero ruleset errors. Shipped exactly as decided below.

Spun out of [Modular Vehicles](Feature-ModularVehicles.md): now that a chassis and a soldier are both
`soldiers:` entries sharing one item pool, the equipment lists mix engines and armor plates in with
rifles and medikits. Neither side can use the other's gear, but nothing says so.

## Motivation

A chassis' hardpoints are typed, so a tank engine *physically* can't be dropped into a rifleman's
hand. But the two units still **see** each other's gear:

- Open a chassis' inventory and the ground tile lists every rifle, grenade, medikit and clip in the
  craft — none of which it can use.
- Open a rifleman's inventory and the ground lists HWP engines, targeting modules and armor plates —
  which he *can* stuff into his backpack, because a generic `INV_SLOT` accepts any battle type.
- The craft equipment screen offers both sets to everyone.

The ask: items should declare who they're for, and the equipping UI should filter on it.

## Audit (Jul 2026)

### What already exists

| Capability | Provided by | Gap |
|---|---|---|
| Restrict an item to named inventory sections | **`supportedInventorySections:`** on `items:` ([RuleItem.cpp:1246](../src/Mod/RuleItem.cpp#L1246)) | Restricts *placement*, not *display*. Ground is always exempt by design. Requires enumerating sections per item. |
| Restrict a section to one item class | **`battleType:`** on `invs:` (DX typed slots) | Slot-side only; a generic belt/backpack has no filter, so it accepts anything. |
| Decide which sections a unit even has | **`inventoryLayout`** on `armors:` (DX) | Already does the heavy lifting for chassis — a tank has no belt or backpack. |
| **Restrict an *armor* to soldier types** | **`units:`** on `armors:` — `Armor::getCanBeUsedBy()` ([Armor.cpp:1138](../src/Mod/Armor.cpp#L1138)), documented at [Ruleset-Armors.md](../docs/Ruleset-Armors.md) | **This is the precedent.** The same vocabulary does not exist on `items:`. |
| Per-unit filtering of the inventory ground tile | — | **Does not exist.** `Inventory::arrangeGround` ([Inventory.cpp:1849](../src/Battlescape/Inventory.cpp#L1849)) lays out every item on the tile unconditionally. |

So: `RuleItem` has **no** unit-gating field of any kind, and no equipping UI filters by unit.

### What legacy DX did

The legacy mod tagged HWP gear with **`vehicleItem: true`** plus a `validSlots:` list
([Vehicles-HWPs.md](../reference/Legacy-DX-Content/Vehicles-HWPs.md)) — i.e. a **binary** user flag
alongside a per-item slot whitelist. DX already shipped the `validSlots` half as
`supportedInventorySections`; the binary user flag is the piece never ported.

### The useful asymmetry

The two directions are **not** equally hard, and that shapes the design:

- **Soldier gear on a vehicle** is nearly free to filter *without any new ruleset key*. A chassis'
  layout contains only typed hardpoints (turret = firearm, rack = ammo, bay = equipment, plates =
  armor plate). So "does this unit's layout have any non-ground section that would accept this item?"
  already excludes medikits, scanners and grenades from a tank, derived purely from data that exists.
- **Vehicle gear on a soldier** is *not* derivable, because a standard belt/backpack is an untyped
  `INV_SLOT` that accepts anything. Keeping engines and plates out of a rifleman's list needs an
  explicit statement on the item.

Fortunately the explicit half is the small half: a mod has a handful of vehicle parts and hundreds of
ordinary items, so tagging the parts is cheap while tagging every rifle would not be.

## Options

**A. `units:` on `items:` — mirror `Armor.units:`** *(recommended)*
A list of `soldiers:` types allowed to equip the item; empty = anyone. Same key name, same semantics
and same `Collections::sortVectorHave` implementation as the armor version modders already know.
Maximally general — it expresses "only these two chassis types can mount this turret" or "only
hybrids can use this psi-amp", not just a vehicle/soldier split. Combined with the derived filter
above, a mod only tags its vehicle parts.

**B. Binary `vehicleItem: true`** — the literal legacy port. Simplest to author and matches the
phrasing of the request, but hardcodes a two-way split the engine has otherwise avoided, and can't
express per-chassis-type restrictions. Would sit awkwardly next to `Armor.units:`.

**C. Derive everything from layout compatibility, no new key** — zero ruleset burden, and genuinely
sufficient for the vehicle side. But it cannot keep vehicle parts out of a soldier's backpack (see
asymmetry above), so it solves only half the problem.

## Decisions (confirmed with user, Jul 2026)

**Both flags ship**, doing different jobs — the coarse one carries the 99% case for free, the
general one exists for finer restrictions later.

| Key | Default | Job |
|---|---|---|
| `vehicleItem` | `false` | May a **vehicle chassis** equip this at all? Default-deny is the whole point: every existing item in every existing mod is automatically excluded from chassis with **zero tagging**. |
| `units` | *(empty)* | If non-empty, only these `soldiers:` types may equip it. Fine-grained, applies to people and chassis alike. Mirrors `Armor.units:` exactly (same key, same semantics, same `linkRule` + `sortVectorHave` implementation). |

Resolution order for "may unit U equip item I":

1. U is **not** a geoscape soldier (alien, classic `vehicleUnit` HWP, civilian) → **allowed**.
   Non-soldier units are untouched, so nothing about existing content changes.
2. U is a chassis (`RuleSoldier::isVehicle()`) and `!I.vehicleItem` → **denied**.
3. `I.units` non-empty and U's soldier type not listed → **denied**.
4. Otherwise **allowed**.

This composes cleanly:

```yaml
items:
  - type: STR_RIFLE
    # nothing -> soldiers yes, chassis no. Every existing item, free.

  - type: STR_DX_HWP_ENGINE_S
    vehicleItem: true                       # chassis may install it
    units: [ STR_MEDIUM_TANK, STR_HEAVY_TANK ]   # ...and only these two

  - type: STR_UNIVERSAL_FLARE
    vehicleItem: true                       # both: chassis AND people
```

**Derived layout filtering (option C) is dropped.** Rule 2 achieves the same vehicle-side result
more cheaply and far more explicitly, so there is no reason to infer it from slot geometry.

**Scope: the inventory ground tile + auto-equip/equipment templates.** The craft equipment screen and
the base Stores/Purchase/Sell screens are deliberately *not* filtered — those list what you own and
what you are shipping, not what a given unit straps on.

**Filter *and* refuse.** The equipping lists hide it, and `Inventory::checkSlotRules` rejects the
move, so a saved loadout template, a hand-edited save or a future code path can't sneak a restricted
item onto a unit.

**The ground stays universal.** Only *equipping* is gated; any unit can still pick a dropped item up
off the floor and haul it home, matching how `canBePlacedIntoInventorySection` already exempts
`INV_GROUND`.

## What shipped

| Piece | Where |
|---|---|
| `vehicleItem` + `units:` parsing, `afterLoad` linking, `canBeEquippedBy()` | [RuleItem.h](../src/Mod/RuleItem.h) / [RuleItem.cpp](../src/Mod/RuleItem.cpp) |
| Ground-list display filter | [Inventory.cpp](../src/Battlescape/Inventory.cpp) `arrangeGround` |
| Placement refusal (`STR_CANNOT_EQUIP_ITEM`) | [Inventory.cpp](../src/Battlescape/Inventory.cpp) `checkSlotRules` |
| Auto-equip refusal | [BattleUnit.cpp](../src/Savegame/BattleUnit.cpp) `addItem` |
| Saved-layout refusal | [BattlescapeGenerator.cpp](../src/Battlescape/BattlescapeGenerator.cpp) `placeItemByLayout` |
| Worked example (every vehicle part tagged) | `bin/standard/dx-test/dx-test-vehicles.rul` |

### Notes from implementation

- **The display filter reuses an existing mechanism rather than adding one.** `arrangeGround` already
  parks items it won't lay out off-grid at `slotX = 1000000` (that's how zero-width items are
  handled). Restricted gear takes the same path, so it stays in the tile's inventory — it deploys,
  survives, and is recovered at mission end exactly as before; it is simply not drawn or reachable on
  *that* unit's screen. Nothing is destroyed or moved.
- **Non-soldier units are exempt inside `canBeEquippedBy()` itself**, not at each call site. Aliens,
  civilians and classic `vehicleUnit` HWPs have no `RuleSoldier` to test, and gating them would have
  changed pre-existing content for no benefit.
- **`BattleUnit::addItem` needed the guard placed early** — before the weight/tally bookkeeping, so a
  refused item doesn't perturb the carried-weight accounting on its way to returning false.
- The `units:` list deliberately reuses `Armor`'s exact implementation shape (`loadUnorderedNames` →
  `linkRule` → `sortVector` → `sortVectorHave`), so the two behave identically and a modder who knows
  one knows the other.
