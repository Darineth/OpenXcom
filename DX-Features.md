# OpenXcom DX — Features

This document tracks features added in OpenXcom DX on top of OXCE-Plus. Entries will be added
here as features are implemented.

## Configurable Inventory Layouts

Different armors can now grant different inventory **section sets** (slots), instead of every unit
in the game sharing one global grid. In stock OXCE the `invs:` sections are global and armor's only
inventory control is the `allowInv` on/off toggle; DX adds a per-armor layout on top.

Define named layouts with a new top-level `inventoryLayouts:` node and assign one to an armor via
`Armor.inventoryLayout`. A layout is just an `invs:` list of globally-defined `invs` section ids.
A slot that should only appear in certain layouts (not on every unit) is a normal global `invs`
section that those layouts simply list and others don't:

```yaml
invs:
  - id: STR_SATCHEL          # a normal global section, not part of STR_STANDARD_INV
    x: 192
    y: 37
    type: 0                  # 0 = slot, 1 = hand, 2 = ground
    slots: [ [0,0], [1,0], [2,0], [0,1], [1,1], [2,1] ]
    costs: { STR_RIGHT_HAND: 8, STR_BELT: 12, STR_GROUND: 10 }

inventoryLayouts:
  - id: STR_LAYOUT_LIGHT
    invs: [STR_RIGHT_HAND, STR_LEFT_HAND, STR_BELT, STR_SATCHEL, STR_GROUND]

armors:
  - type: STR_HEAVY_SUIT
    inventoryLayout: STR_LAYOUT_LIGHT
```

Details and behavior:

- **Sections are reusable globals.** A layout's `invs:` is an ordered list of global `invs` section
  ids (the same section object can appear in many layouts). There are no inline section definitions —
  define the section in `invs:` and list its id. Layouts support the standard `refNode` parent
  mechanic for reuse between layouts. (This single-object-per-section model means slot identity is
  unambiguous everywhere, including when a soldier's equipment layout is saved and reloaded.)
- **`STR_STANDARD_INV` is the default.** The base game data (`xcom1`/`xcom2` `inventories.rul`) defines
  the standard nine-slot set as the `STR_STANDARD_INV` layout. An armor that sets no `inventoryLayout`
  falls back to it — so a section that isn't listed in `STR_STANDARD_INV` (and isn't in the armor's
  own layout) won't appear on that unit. To add a slot to *everyone*, add it to `STR_STANDARD_INV`;
  to add it to *some* units, make a layout that lists it and assign that to their armor.
- **Keying is armor-only.** Every unit always has an armor (soldiers, aliens, HWPs), so the armor's
  layout fully determines its slots. There is no `RuleSoldier`/`Unit` layout field.
- **Default behavior is unchanged.** Armors with no `inventoryLayout` use `STR_STANDARD_INV` (the same
  nine slots as vanilla). If a mod / total conversion doesn't define `STR_STANDARD_INV` at all, the
  engine falls back to an implicit layout synthesized from the full global `invs` set, so those mods
  behave exactly as before.
- A ground section is always guaranteed (appended if a layout omits one).
- The inventory screen, item placement, quick-move (ctrl+click), start-of-mission auto-equip, and the
  alien inventory all honor the active unit's layout: sections not in the layout are not drawn or used.
- **Stranding protection.** Item placement that would force an item into a section the unit's layout
  lacks (loadout templates, persistent equipment layouts saved under a different armor) instead leaves
  the item on the ground; templates show a warning (`STR_DX_TEMPLATE_SLOT_NOT_IN_LAYOUT`). Loading a
  battlescape save whose item slots became invalid because the layout/armor definition changed *since
  the save* drops those items to the unit's tile (logged), so they stay accessible.
- **Layout-aware unload.** Weapon unload only uses hand sections present in the unit's layout; when no
  off-hand is available, the ejected ammo is best-fit into another inventory slot (ctrl+click style)
  before falling back to the ground.

## Typed Inventory Slots

Inventory sections (`invs:`) can now be turned into **typed, role-specific
sockets** with three new `RuleInventory` fields. In stock OXCE/OXCE-Plus, item-vs-slot restriction
only exists from the *item* side (`RuleItem.supportedInventorySections`); DX adds the complementary
*slot* side plus combat-lock, move gating, and per-slot stat gating.

```yaml
invs:
  - id: STR_ARMOR_SLOT
    battleType: 11          # BT_FLARE — only items of this battle type fit here (0 = anything)
    allowCombatSwap: false  # locked once combat is underway: can't move items in or out
    countStats: false       # items here grant no stat/statModifier bonuses (display/holster slot)
    costs:
      STR_GROUND: -1        # an explicit -1 forbids this transfer while in combat
```

| Field | Default | Meaning |
|---|---|---|
| `battleType` | `0` (BT_NONE) | If set, only items whose `battleType` matches may be placed in the slot. Composes with the item-side `supportedInventorySections` (both must pass). |
| `allowCombatSwap` | `true` | When `false`, items can't be moved **into or out of** the slot once combat is live (the pre-battle equip / base inventory screen is unaffected). |
| `countStats` | `true` | When `false`, items in the slot do **not** contribute their `stats`/`statModifiers` to the wearer — for holster/display slots. Default `true` preserves the shipped *Item Stats Modifiers* behavior. |

Behavior and details:

- **`battleType` is enforced when placing the item *as a slot occupant*** (pre-battle and in-combat),
  at manual drop, ctrl-click quick-equip, `fitItem`, and the auto-place candidate scan. A rejected
  placement shows `STR_INVALID_ITEM_SLOT`. It does **not** block loading ammo into a weapon already in
  the slot — dropping a clip onto the weapon in a firearm-only slot still reloads it.
- **`allowCombatSwap` only bites in combat** (`InventoryState` TU mode), and only on a *move between
  slots*: a locked item can still be **picked up** (so you can unload it) and dropped back into the
  same slot, but it can't be moved to a *different* slot, and nothing can be moved *into* a locked
  slot — rejected with `STR_NOT_COMBAT_SWAPPABLE`. Enforced at the drop / `fitItem` /
  ctrl-click-to-ground paths, so a locked item can't be popped out to the floor either. Pre-battle
  equip / base inventory is unaffected. The battlescape **Throw** action is also withheld for items
  in a combat-locked slot (throwing would move the item out, bypassing the lock).
- **`costs` allow-list (lenient):** unlisted section pairs keep the existing `DEFAULT_MOVE_COST`
  fallback (so partially-specified custom layouts still work); only an **explicit `-1`** cost forbids
  that move while in combat (`STR_INVALID_TRANSFER`).
- **`countStats`** gates the per-item bonus sum in `BattleUnit::computeEffectiveBaseStats`.
- All four fields are also exposed to Y-Script on `RuleInventory` (`getBattleType`,
  `getAllowCombatSwap`, `getCountStats`). No save-format change (rule-side only).

This re-ports the legacy DX typed-slot behavior onto the current OXCE-Plus base and is the
foundation for the follow-on **Utility equipment slots** (`INV_UTILITY`) feature.

## Utility Equipment Slots

A new inventory **slot type**, `INV_UTILITY` (plus a reserved sibling `INV_EQUIP`), that gives a
unit a dedicated single-occupant equipment slot distinct from its hands. It holds one item (no
`slots:` grid to author) and renders as a single bounding box — like a hand — but the item is
**not** wielded: it is never fired, reaction-fired, or treated as the active hand. **Unlike a hand,
it size-checks the item**: only items that fit within the box dimensions are accepted; an oversized
item is rejected (a hand, by contrast, accepts any size). The box size defaults to `2×2` but is
**rule-defined** per section via `width:`/`height:` (in slot cells), so an equip slot for a larger
piece of gear (e.g. `3×2`) is just a ruleset value.

```yaml
invs:
  - id: STR_UTILITY
    type: 3            # 3 = INV_UTILITY (4 = INV_EQUIP); appended to INV_SLOT/HAND/GROUND = 0/1/2
    x: 256
    y: 37
    battleType: 10     # optional typed-slot rules still apply (here: flares only)
    # no `slots:` list needed - single occupant
```

Behavior and details:

- **Single-item geometry, not a hand.** A new `RuleInventory::isSingleItem()` predicate groups
  `INV_HAND`/`INV_UTILITY`/`INV_EQUIP` for fit/placement/move-cost/overlap and for grid + item
  rendering (per-type box dims: utility/equip are `2×2`, hands `2×3`). Wielding and reload logic stays
  keyed strictly on `INV_HAND` and on handedness (`isRightHand()`/`isLeftHand()`), which utility/equip
  slots never set — so a utility item is never held in hand.
- **Per-unit handle.** Each inventory layout caches its first `INV_UTILITY` section;
  `BattleUnit::getUtilitySlot()` returns it and `getUtilityItem()` returns whatever occupies it, so
  later systems can ask "what's in this unit's utility slot" without knowing the modder's section id.
- **Typed-slot rules compose.** Because a utility slot is just another section in a layout, the
  typed-slot fields (`battleType` filter, `allowCombatSwap` lock, `countStats` gating, `costs`) all
  apply to it with no extra wiring.
- **`INV_EQUIP`** is recognized as a usable single-occupant sibling type (enum + geometry + fit), but
  its dedicated per-unit accessor is deferred until a concrete consumer defines what "equip" means.
- **Usable in battle** (first consumer): a battlescape hotkey (`keyBattleUseUtility`, default `Z`)
  opens the action menu for the utility item **in place** — a utility medikit heals, a scanner
  scans, etc. — without moving it to a hand. Because using an item doesn't rearrange it, a
  combat-locked (`allowCombatSwap: false`) utility slot is still usable, just not swappable. The
  action menu is slot-agnostic, so any utility item's `RuleItem` actions are offered — **except
  Throw**, which is withheld for utility slots (gear is used in place, not thrown) and for any
  combat-locked slot (throwing would move the item out, bypassing the lock).
  *(design: [plans/Feature-UtilitySlotUse.md](plans/Feature-UtilitySlotUse.md))*
- **No save-format change**: the slot type, per-unit handle, and use path are all derived from rules
  at load. Exposed to Y-Script as the `INV_UTILITY`/`INV_EQUIP` consts on `RuleInventory`. Still lays
  plumbing for later features (effects/light equipment, roles, modular vehicles).

*(design: [plans/Feature-UtilityEquipmentSlots.md](plans/Feature-UtilityEquipmentSlots.md))*

## Configurable Hand Slots

A unit's **hand** is no longer hard-wired to the section ids `STR_RIGHT_HAND` / `STR_LEFT_HAND`.
Any `invs:` section can declare its handedness, so a layout can use renamed or specialised hand
slots (e.g. a creature's `STR_MAW`, a one-armed unit) and the engine treats them as real hands for
auto-equip, firing, reactions, the active-hand toggle, and held-item rendering.

```yaml
invs:
  - id: STR_MAW
    hand: right        # right | left | none (default none)
    type: 1
    # ...
```

Behavior and details:

- **`hand:` makes a section a hand.** `hand: right` / `hand: left` flag the slot as that hand;
  `none` (the default) is a normal slot. Handedness is read from this property (`isRightHand()` /
  `isLeftHand()`) instead of the literal ids, so weapon getters, sprites, reactions, and accuracy all
  work on the flagged slot. Auto-equip and firing resolve "this unit's right/left hand" from the
  unit's own **inventory layout**, so a unit with a renamed hand gets weapons placed correctly.
- **Legacy fallback.** When a section sets no `hand:`, the historical ids still apply —
  `STR_RIGHT_HAND` → right, `STR_LEFT_HAND` → left. So the base game and every existing mod behave
  identically with no changes.
- **Two hands per layout (not globally).** Handedness is validated **per inventory layout**: within
  one layout at most one section may be `hand: right` and one `hand: left` (two of a side in the same
  layout is a ruleset error). The same `hand: left` section — or different ones — can appear across
  many layouts freely: a human layout can use `STR_LEFT_HAND` while a creature layout uses a
  `hand: left` `STR_MAW`. (Only if you list *two* left hands in one layout — e.g. the vanilla
  `STR_LEFT_HAND` plus a custom one — does that layout error; drop one from its `invs:`.) True N-hand
  support (more than two hands in one layout) is a separate future feature; all the two-hand logic
  (dual-wield accuracy, arm-wound mapping, left/right sprite placement, the reaction
  active/preferred/disabled-hand UI) is unchanged and keyed off the property.
- **Inventory-screen hand shortcuts** (reload off-hand placement, ctrl-click-to-hand) resolve the
  *selected unit's* hands from its layout, re-cached whenever the inventory switches unit/armor. A
  layout that omits a hand (e.g. a one-handed layout) yields a null hand, which every shortcut guards
  for — the displaced ammo falls back to the ground / best-fit rather than a nonexistent slot.
- **Save migration.** A unit's active hand and preferred-reaction hand are now stored as handedness
  (`right`/`left`) rather than a section id. Old saves carrying `activeHand: STR_RIGHT_HAND` (etc.)
  are migrated on load; new saves write the short form.
- Script API is unchanged (`isRightHand`, `getRightHandWeapon`, `getRuleInventoryRightHand`, …) —
  same names, now backed by the property.

## Inventory Ammo-Count Badges

Weapons and ammo clips in the inventory now show their remaining rounds as a small bordered number
at the **top-right** of the item, so loadouts can be read at a glance without hovering. A weapon
shows its loaded ammo's rounds (or self-ammo charge); a clip shows its own remaining rounds;
single-shot ammo (clip size ≤ 1, e.g. a rocket) shows nothing. The number is colored by state:
**green** when full, **amber** at half-or-better, **red** below half. These colors are configurable
via three new `inventory` interface elements — `ammoFull`, `ammoMid`, `ammoLow`
(see `interfaces.rul`).

The same count badge is drawn on the right-side ammo **preview** box, which now also appears when
hovering a bare ammo clip (previously only loaded weapons previewed). The old "AMMO ROUNDS LEFT"
text was removed as redundant. (See design doc: `plans/Feature-InventoryAmmoCount.md`.)

## Edit Soldier Inventories from New Battle

The New Battle setup now lets you arrange soldier loadouts in the full inventory screen before
starting the fight. The **Inventory** button is available both from **Equip Craft → Equipment** and
from a soldier's **Soldier Info** screen (previously hidden in New Battle). Edited loadouts are saved
and carried into the battle when you start it. (The other base-management actions — Sack, transfer to
Craft, Transformations — remain unavailable in New Battle.)

## Quick Craft Stock Buttons (New Battle)

In New Battle, the **Equip Craft → Equipment** screen gains a **Fill** button next to the existing
**Unload Craft** button, for fast loadout setup:

- **Fill** instantly stocks the craft with a generous spread of every usable item (recoverable,
  non-corpse inventory items): **40** of each ammo / grenade / proximity grenade / flare, **10** of
  everything else (weapons, tools, medikits). Open the inventory afterward to distribute them.
- **Unload Craft** (unchanged) empties the craft.

Both buttons appear only in New Battle, where the loadout is a sandbox — the real geoscape craft
screen, bound by base storage and the economy, is unaffected.

## Craft Loadout Save/Load Buttons

The craft equipment screen's **loadout templates** (10 named slots that store a whole craft's item
loadout) now have visible **Save** and **Load** buttons in the bottom button row, on both the geoscape
**Equip Craft → Equipment** screen and the New Battle version. Previously this feature was reachable
only via hidden hotkeys (`keyCraftLoadoutSave` / `keyCraftLoadoutLoad`, default **F5** / **F9**), which
remain bound.

- **Save** stores the craft's current loadout to a named template slot.
- **Load** applies a saved template to the craft.
- Both work in **New Battle** too. New Battle now **persists** its loadout templates (the craft
  loadouts *and* the soldier equipment layouts) in its `.cfg`, so templates you create there survive
  exiting and restarting New Battle — previously they were discarded. Existing New-Battle configs
  without saved templates fall back to the mod's starting-base defaults.

(DX prefers surfacing features as discoverable on-screen controls rather than hotkey-only. The
"Unload" button was narrowed to make room; its label is unchanged.)

## Contextual Inventory Info Panel

Hovering an item in the inventory replaces the soldier stat panel with information about that item, in
the same `STAT>VAL` style and colors as the soldier stats:

- **Granted unit stats** the item provides — one line per modified stat, flat and/or percentage
  (e.g. `FA>+5`, `RE>+10%`), each in its matching soldier stat color.
- **Directional armor bonuses** the item provides (`F>/L>/R>/B>/U>`), in the armor colors.
- Below the stats, **item-specific info**: weapon shot modes with base accuracy (`Snap>`, `Aimed>`,
  `Auto>`, `Burst>`, `Melee>`, colored like firing/melee) and medikit charges (`Heal>`/`Stim>`/`Pain>`,
  colored like health/energy/morale).
- The **weight line** shows the hovered item's own weight (in the normal weight color).

Items with none of the above keep the soldier stats; moving off the item restores the full soldier
panel. The panel reuses the expanded stat rows, so it follows the `showMoreStatsInInventoryView`
option. (See `plans/Feature-InventoryContextualInfoPanel.md`.)

## Inventory Move TU Costs in Pre-Battle Setup

The per-slot inventory move TU costs (shown on the slot labels while an item is held) now also
appear during the pre-battle equipment setup phase, not only during a mission. These are the same
rule-based costs that will apply once the battle starts, so loadouts can be planned around them;
no TUs are actually spent in the setup phase. The display still honors the existing
`oxceDisableInventoryTuCost` option — turning it off hides the costs in both phases.

**Reload readout when holding ammo:** while a piece of **ammo** is held, any inventory section that
contains a weapon the ammo can load shows that **weapon's name and the reload TU cost** (e.g.
`RIFLE:11`) in place of the generic `SLOT:<move-cost>` label. The reload cost is computed by the same
helper (`Inventory::getReloadTuCost`) the drag-to-load path charges, so the shown and actual costs
match (including the DX weight-based reload term when `battleWeightBasedReloadCost` is on).

**Unload cost on the Unload button:** while holding a loaded weapon, hovering the **Unload** button
shows `Unload <weapon>: <TU>` on the bottom item line (replacing the normal item hover text), via
`Inventory::getUnloadTuCost` — the weapon's `tuUnload` plus the weight-based term when the option is
on. With nothing held, the button keeps its standard "Unload weapon" tooltip.

## Armor Degradation

Battlescape damage types can now wear armor even on a hit that fails to penetrate, as long as the
hit was strong enough to meaningfully challenge that side's armor, and can wear extra armor on a
hit that smashes well past it. This is controlled per `damageAlter` / `RuleDamageType` via four
new knobs:

- `ToArmorBlocked` — the blocked-hit armor wear multiplier, applied only to fully blocked hits.
- `ToArmorBlockedThreshold` — the minimum fraction of the struck side's effective armor block the
  rolled damage must reach before blocked-hit wear begins.
- `ToArmorOverPen` — the over-penetration extra-wear multiplier, applied only to hits that punch
  through the struck side's armor.
- `ToArmorOverPenThreshold` — the multiple of the struck side's effective armor the rolled damage
  must exceed before over-penetration wear begins.

The existing fork behavior is preserved around it: `ToArmorPre` still applies unconditional
pre-armor wear, and `ToArmor` still applies only from post-armor penetrating damage. DX adds two
further stages: one for "nearly penetrated but was stopped" hits (`ToArmorBlocked`, defaults `0.0`
/ `0.5`) and one for "smashed clean through" hits (`ToArmorOverPen`, defaults `0.0` / `2.0`). All
defaults are no-ops, so existing mods keep their prior behavior until they opt in.

## Editable Base Damage Types

The built-in damage types (`DT_AP`, `DT_IN`, `DT_HE`, `DT_LASER`, `DT_PLASMA`, `DT_STUN`,
`DT_MELEE`, `DT_ACID`, `DT_SMOKE`, plus the ten spare slots `DT_10`..`DT_19`) can now be retuned
globally with a new top-level `damageTypes:` ruleset node, instead of overriding `damageAlter` on
every weapon. Each entry selects a slot via `ResistType` and overlays any field that per-item
`damageAlter` already understands (the `To*` multipliers, `Random*`/`Ignore*` flags,
`ArmorEffectiveness`, `RadiusEffectiveness`, `TileDamageMethod`, thresholds, etc.):

```yaml
damageTypes:
  - ResistType: 1        # DT_AP — retune armor-piercing globally
    ToArmor: 0.2
    ArmorEffectiveness: 1.1
  - ResistType: 10       # DT_10 — give a spare slot real behavior (e.g. EMP/cold/sonic)
    RandomType: 1
    ToHealth: 1.0
    ToStun: 0.5
    IgnoreDirection: true
```

Behavior and compatibility:

- All `damageTypes:` nodes are applied in a global pre-pass across **every mod's** rulesets
  **before** any `items:` load, so it does not matter which file or which mod defines the override;
  a later mod can retune a damage type that an earlier-loaded mod's (e.g. the master's) items
  inherit. Mods are swept in load order (last-wins per field). This is required because `RuleItem`
  copies the base damage type by value at load time.
- Precedence chain: built-in engine default → global `damageTypes:` edit → per-item `damageAlter`.
- `ResistType` is the lookup key only and is re-locked after load — a node cannot remap itself to a
  different slot. Out-of-range indices are reported as soft errors.
- The slot count remains fixed at 20 (`DT_NONE`..`DT_19`); adding more types is not supported.
- This is mod data, not savegame state, so there is no save-format change. Retuning base damage
  does shift balance for existing saves.
- The first cut reuses the existing `STR_DAMAGE_1x` strings for spare-slot display names; mods can
  localize those keys.

## Item Stats & Stat Modifiers

Items and armors can now apply generic stat effects using ruleset fields:

- `stats` (flat additive bonuses) on both `RuleItem` and `Armor`.
- `statModifiers` (percent deltas, where `0` means unchanged) on both `RuleItem` and `Armor`.

Runtime behavior:

- Soldier bonus preparation now applies armor `statModifiers` in addition to existing flat armor
  stat bonuses.
- Battle-unit effective stats now include equipped inventory item `stats` and `statModifiers`
  (for items in actual inventory slots), stacked with the existing soldier+armor stat baseline.
- Item `statModifiers` stack additively as percentage deltas around `0` before application.

Compatibility:

- Existing mods keep prior behavior by default; missing keys are no-op.
- Slot-level filtering (`countStats`) is intentionally deferred to the later typed-slot feature.

## Directional Armor on Items

Equipped items can now grant per-side armor to their wearer, complementing the per-side armor
already defined on `Armor`. New `RuleItem` ruleset fields:

- `frontArmor` — added to the wearer's front armor.
- `sideArmor` — added to **both** the left and right armor (matching how `Armor`'s single
  `sideArmor` expands to both sides).
- `rearArmor` — added to the wearer's rear armor.
- `underArmor` — added to the wearer's under armor.

These are distinct from the existing scalar `armor` field, which remains the item's own structural
HP / explosive resistance and is unchanged.

Runtime behavior:

- A unit's per-side max armor is `base armor (armor rule + soldier bonuses) + the directional
  armor of every equipped inventory item that occupies a slot`, recomputed whenever the inventory
  changes (alongside the effective-stat refresh).
- Accumulated per-side armor damage is tracked separately, so changing the max never refunds lost
  armor: equipping a plate while undamaged fills the new capacity, but removing and re-equipping a
  plate after taking damage restores only the plate's own armor, not the damage taken. On reload
  the saved (absolute) current armor is authoritative and the damage is derived from it.
- The fields are shown per-item in the Stats-for-Nerds screen.

Compatibility:

- Existing mods keep prior behavior by default; an item with none of the new keys is a no-op.
- No new save data: per-side max armor is derived from rules + the equipped loadout and recomputed
  on load, exactly like the existing soldier-bonus armor.

## Combat Log

A floating, centered event log across the top of the Battlescape that reports combat events
as they happen, color-coded by outcome (good/neutral/warning/bad from the player's
perspective). Reported events: new turn; weapon fire and throws (with shot mode, e.g. "fires
Rifle (Snap Shot)"); melee attacks; reaction shots/swings (tagged as such); hits, with
research-gated damage detail (exact damage and fatal wounds for your units and researched
enemies, a vague light/heavy band for un-researched enemies); kills and knockouts; panic and
berserk; and a player-only warning when a shot empties a weapon. Entries fade out over time and
the list is capped, so it self-empties when combat is quiet. Unit and weapon names respect the
player's knowledge — own soldiers and researched enemies are named, while unseen/un-researched
hostiles show as "Hostile"/"Unknown" and their gear as "an unknown weapon". The log is
transient (never serialized) and distinct from the on-demand OXCE hit log (Ctrl-H).

- Toggle: advanced option **Combat log** (`combatLogEnabled`, default on), under Battlescape.
- Duration: advanced option **Combat log duration (seconds)** (`combatLogDuration`, default 8,
  range 1–60), under Battlescape — controls how long each entry stays before fading off. Changes
  take effect immediately, even mid-battle.
- Theming: the `combatLog` element in the `battlescape` interface ruleset sets the panel's
  position and size; `combatLogNeutral` / `combatLogGood` / `combatLogWarning` / `combatLogBad`
  set the four outcome colors.

## Action Menu Revamp

The battlescape action popup (Throw / Snap / Aimed / Auto / Hit / …) is more compact and more
informative. Rows use a small font in ~25px boxes, and each row now shows its configurable
hotkey at the left. Multi-shot modes show a shot count (e.g. `x3`) and shotgun ammo shows pellets
(`x3 (9 pellets)`). Affordability is flagged at a glance: an action you can't perform — not
enough TU, or no usable ammo — turns the whole row (text + border) red with a "No TU" / "No Ammo"
tag, while an action with *some* ammo but not enough for a full burst turns amber. Flagged rows
stay clickable (the usual warning still fires).

- A sixth configurable action hotkey (`keyBattleActionItem6`, default `6`) was added so the
  six-slot popup and the skill menu are fully keyboard-reachable.
- Theming: `actionMenuDisabled` (red) and `actionMenuWarning` (amber) elements in the
  `battlescape` interface ruleset color the unaffordable / partial-ammo states.
- DX options now live under their own **DX** tab in Options → Advanced and Options → Controls
  (a dedicated `OPTION_DX` owner), separate from the OXC/OXCE tabs.

## On-Map Overlays

A family of small, always-on visual cues drawn directly onto the Battlescape map, each with its
own toggle so they can be enabled independently. *(design:
[plans/Feature-MapOverlays.md](plans/Feature-MapOverlays.md))*

- **Hovered unit name** — a knowledge-aware, faction-colored floating name label over the unit
  under the cursor. Toggle: **Hovered unit name** (`hoveredUnitNameEnabled`, default on).
- **Primed-grenade indicator** — a pulsing marker hovering over each player-thrown primed
  grenade lying on a discovered tile (a brightness-pulsed red disc for normal grenades, an
  animated red "wifi ping" for proximity grenades). Enemy grenades are never shown. Toggle:
  **Primed grenade indicator** (`grenadeIndicatorEnabled`, default on).
- **Unit status indicators** — status glyphs hovering over each of the player's own *living*
  units, one per active condition: **bleeding** (fatal wounds), **on fire**, **shock** (losing
  HP each turn), and a **near-knockout** stun warning when accumulated stun is close to dropping
  the unit. All active conditions stack side-by-side above the head. Uses the same glyphs the
  engine already shows over unconscious bodies. Toggle: **Unit status indicators**
  (`unitStatusIndicatorEnabled`, default on).

  The four OXCE status indicators (`Floor{Wound,Burn,Shock,Stun}Indicator`) are mod-supplied and
  absent from base data, so they used to render nothing out of the box. DX now builds a small
  **procedural fallback** for all four in `Map::init()` (red blood drop / flame / lightning bolt /
  "Zzz" sleep glyph, palette-safe and shape-distinct), preferred only when no mod art is present — so both the
  living-unit overlay and the long-dormant unconscious-body indicators now work by default. A mod
  can still override any of them with an `extraSprites` `Floor*Indicator` (`singleImage: true`).
- **Motion-detector readings** — a pulsing amber target reticle painted on the floor of each
  enemy/neutral unit detected this turn by a motion scanner (a path-preview-style floor decal),
  brighter the more the unit moved. It shows passively (no key held) and is drawn under the unit/
  wall sprites, so a visible enemy occludes its own marker and the cue is left for fog tiles —
  replacing OXCE's hard-to-see Alt-held bobbing arrow. Detection is unchanged from OXCE — a unit
  only lights up once actually scanned, so the overlay grants no intel a scanner wouldn't. Toggle:
  **Motion detector readings on map** (`motionDetectorOverlayEnabled`, default on).

## Fog of War

Base OpenXcom draws every *discovered* tile at full brightness, with no on-map sign of what a
soldier can actually see right now. DX **dims discovered tiles that no player unit currently has in
line of sight**, so remembered terrain reads as out of live observation while in-sight terrain stays
bright (undiscovered tiles remain black). The dim layers on top of the existing light/night-vision
shading and updates live — tiles re-fog as soon as soldiers move or turn. *(design:
[plans/Feature-FogOfWar.md](plans/Feature-FogOfWar.md))*

Implemented as a renderer change keyed off the engine's existing per-tile player-LOS count
(`Tile::getVisible()`); also fixes a pre-existing double-increment in
`TileEngine::calculateTilesInFOV` that kept that count from returning to zero (so tiles never
re-fogged). Toggle: **Fog of war (dim unseen tiles)** (`fogOfWarEnabled`, default on).

## Maximize Info Screens (all screens)

OXCE's **Maximize Info Screens** option (`maximizeInfoScreens`) drops a screen to 320×200 so its
UI fills the window instead of sitting small in a corner at a high base resolution — but stock
OXCE only honored it for 6 Battlescape popups. DX makes it apply to **every** menu, detail, and
dialog screen (main menu, options, all of Basescape, Ufopaedia, briefings/debriefings, geoscape
dialogs, …). The only screens left at the gameplay resolution are the primary views themselves —
the Geoscape, the Battlescape, and dogfights — plus the self-managing loading/cutscene/intro
screens.

This is driven centrally rather than per-screen: each `State` declares a `ScaleContext`
(`UI` / `Geoscape` / `Battlescape` / `SelfManaged`), and `Game` applies the matching base
resolution once whenever the top of the state stack changes (only resetting the display when the
resolution actually changes, so the common case is free). When the option is **off**, behavior is
unchanged from before — info screens that overlay the Battlescape (the unit info / scanner /
minimap / medikit / inventory popups) keep the battlescape scale, so there's no open/close display
thrash. No new option; this extends the existing `maximizeInfoScreens` toggle. *(design:
[plans/Feature-MaximizeAllScreens.md](plans/Feature-MaximizeAllScreens.md))*

## Craft Stat Display

The Basescape craft info screen now shows the craft's core performance numbers alongside its
status readouts. The stats are laid out in two compact columns so they fit cleanly at 320×200:
damage capacity, maximum speed, and acceleration on the left; current damage, shield, and fuel on
the right. This mirrors the numbers already shown in the Ufopaedia craft article, but makes them
available directly from Craft Info where base management happens.

## Inventory UI Improvements

The inventory ground area now supports mouse-wheel scrolling anywhere over the ground slots
(`WHEEL UP` for backward, `WHEEL DOWN` for forward), alongside the existing ground-scroll button
and keyboard controls. Scrolling moves one column at a time (not page-by-page), and the view
starts fully scrolled left when a unit is selected. Multi-slot items that extend past the left
edge of the viewport are rendered with their visible portion showing.

## Soldier Info Layout Rework

The Soldier Info screen now follows the Legacy DX-style two-row action layout (excluding Level/EXP):
top row `<<`, `OK`, `>>`, `DIARY`, `ARMOR`, `SACK`; second row with a craft button and
`INVENTORY`. The craft line is now a clickable assign/unassign toggle for the base's first craft
slot (using the same placement validation rules as craft soldier assignment), and `INVENTORY`
opens base inventory setup with the current soldier pre-selected.

## Debriefing Soldier Results

The Battlescape debriefing now keeps each soldier in the mission-results table and shows a compact
outcome code for them: `KIA`, `MIA`, `WND`, or `OK`. Wounded soldiers also show recovery time in
days, while the existing per-soldier stat-gain breakdown remains on the same view. Bad outcomes and
the recovery-day value are highlighted in the list's secondary color.

## Geoscape Activity Display

A persistent text panel on the left side of the Geoscape showing real-time activity across all
bases. Updates on every game-time tick (5 s, 10 min, 30 min, 1 h, 1 day).

**Global section (top, shown before per-base data):**
- Economy warnings (reversed-color alert text): negative current balance ("In the red"), negative
  monthly net (income − maintenance = "Monthly deficit"), and a maintenance spike flag when the
  live maintenance exceeds last month's committed value by ≥ §100 k and ≥ 20%.

**Per-base sections (each base shown with its name as header):**
- Alien resource storage count.
- Research projects: translated name with (spent/cost) progress.
- Idle scientist alert (reversed color).
- Manufacturing queue: item name with (produced/total) or (inf), plus a `[sell]` tag when
  auto-sell is active.
- Idle engineer alert (reversed color).
- Training capacity: Martial Training (in-training/slots, plus queued count) and Psi Lab
  Training (in-training/slots) — shown only when training facilities exist.
- Facilities under construction: translated name and remaining days.
- Craft maintenance: repairs with ETA highlighted (reversed color), or refuelling/rearming.
- Active crafts (STR_OUT): craft name and detailed mission status (patrolling, intercepting UFO,
  returning, low fuel, mission complete, tailing, or destination name).
- Store warnings (reversed color): "Stores full" when items exceed capacity, "Stores near full"
  when craft/transfer items would push stores over.
- Base defense readiness (reversed color): count of disabled defense facilities, count of
  defense facilities lacking required ammo.
- Incoming transfers: count with nearest-arriving item name and ETA.
- Wounded soldiers: total count, severe-wound subset (reversed), and longest recovery time in days.

- Toggle: advanced option **Activity display** (`activityDisplayEnabled`, default on), under DX.

## Concurrent Projectile Flight

The engine can now render multiple projectiles in flight simultaneously. This is the
foundation for shotgun pellets flying as individual visible trajectories, burst-fire rounds
overlapping in the air, and dual-fire weapons firing both barrels at once. Each projectile
travels and renders independently, carries its own impact result so overlapping shots resolve
correctly, and the camera follows the centroid (average position) of all visible bullets.

### Timer-based firing cadence

Multi-shot actions (auto/spray bursts) no longer wait for the previous round to impact before
launching the next. Instead, follow-up shots fire on a timer, so several rounds can be airborne
at once. The cadence is controlled by a new per-item ruleset attribute:

- **`fireInterval`** (RuleItem, milliseconds, default `150`) — how long to wait between
  consecutive shots of a burst/spray. This is a largely cosmetic pacing knob; lower values make
  a weapon "spray" faster with more rounds visible simultaneously, higher values space the shots
  out. The interval is converted internally into think-cycles (the state ticks at ~60 Hz) with a
  minimum of one cycle between shots.

### Shotgun pellets as real projectiles

Shotgun spreads no longer "teleport" their extra pellets to instantly-traced impact points.
When a shotgun shot is fired, the full pellet count is launched as individual, concurrently
flying projectiles (using the concurrent projectile system above). Each pellet:

- spreads from the muzzle using the existing `shotgunSpread` / `shotgunChoke` /
  `shotgunBehaviorType` ruleset values (the spread math is unchanged from before),
- flies along its own visible trajectory, and
- resolves its own impact independently — applying damage and range-based power falloff over its
  own travelled distance through the normal projectile/explosion path.

Because each pellet is now a regular projectile, a pellet that lands on an enemy awards firing
experience like any other hit; the previous artificial per-shot experience cap (which existed
only because the old extra pellets were resolved instantly in a single synchronous burst) no
longer applies.

### Live impact recalculation

Because a projectile's trajectory and impact point are computed once when it is fired, an early
hit in a volley can destroy the wall or object that a later, still-flying round was going to hit.
Those later rounds now re-trace their path against the current map when they reach their old
impact point: if the obstacle is gone, the round keeps flying to the next real obstruction (or the
map edge) instead of detonating in mid-air on a wall that no longer exists. This also covers a
target killed by an earlier round — its corpse no longer blocks the tile. (Thrown and arcing shots
are unaffected; only straight shots recalculate.)

### Non-blocking impact explosions

Previously the entire battle froze for the duration of every impact's explosion/hit animation,
which made overlapping volleys stutter on each hit. Impact explosions from gunfire now animate
**concurrently** with the rest of the volley: the remaining rounds keep flying and the weapon
keeps firing while each hit's explosion plays out. Explosion **damage** is still applied the
instant the round lands (so the live-recalculation above sees destroyed terrain immediately); only
the animation and its end-of-animation casualty resolution run alongside continued fire. Thrown
grenades and blaster-launcher waypoint shots remain single, blocking actions.

### Burst fire mode

Adds **Burst** as a fourth firing mode alongside Snap, Auto, and Aimed (`BA_BURSTSHOT`). It is a
short, controlled volley designed to sit between snap and auto: its own accuracy, TU/energy cost,
shot count, and effective range, all loaded from dedicated ruleset keys on the weapon. Burst is
**opt-in** — a weapon only offers it when `tuBurst` (its TU cost) is set, exactly like Auto opts
in via `tuAuto`; weapons without it behave exactly as before.

- **Ruleset keys** (per weapon, parallel to the snap/auto/aimed keys):
  - `tuBurst` / `costBurst:` — TU/energy cost; **setting the TU cost enables the mode**.
  - `accuracyBurst` — burst accuracy.
  - `burstShots` — rounds per burst (default `2`).
  - `burstRange` — effective range band (default `10` tiles, between snap's 15 and auto's 7).
  - `flatBurst:` — flat-cost flag (falls back to aimed).
  - `confBurst:` — the generic action block (custom `name`/`shortName`, `ammoSlot`,
    `spendPerShot`, etc.). The mode's default display name is `STR_BURST_SHOT` ("Burst Shot").
- **Fires like auto.** Burst reuses the existing async multi-shot path: it launches its rounds
  sequentially on the shared `fireInterval` cadence (so several burst rounds can be airborne at
  once), rather than as a simultaneous shotgun-style volley.
- **AI support.** Battlescape AI now includes Burst in both its vanilla and extended fire-mode
  selection logic, scoring it by the same accuracy/TU heuristic as the other firearm modes.
- **Own action-menu entry & hotkey** — labeled from `confBurst.name`, bound to
  `keyBattleActionItem6` (the DX-added 6th key; `keyBattleActionItem5` is taken by Throw on
  throwable firearms); the menu shows its shot count and flags an ammo warning when the loaded
  clip holds fewer rounds than the burst needs.

## Live Trajectory Preview

While a fire or throw action is being aimed, DX draws the **predicted physical path** of the shot
directly on the battlescape — a straight line-of-fire for direct fire, a parabolic arc for throws and
arcing weapons — so the player can see where the round will go (and what it will hit) before spending
TUs. It complements the on-cursor accuracy readout the engine already shows. *(design:
[plans/Feature-LiveTrajectoryPreview.md](plans/Feature-LiveTrajectoryPreview.md))*

- **Ideal path.** The preview traces the *intended* line with accuracy deviation disabled, so it is
  deterministic and stable as the cursor moves. It is independent of the aim-cone model and works for
  every weapon. `Projectile::calculatePreviewTrajectory()` traces the straight case; `calculateThrow`
  gained an `ignoreAccuracy` argument for the ideal arc.
- **Same aim voxel as the real shot.** The direct-fire aim-voxel resolution (unit-center → object →
  walls → floor priority) that lived inline in `ProjectileFlyBState` is factored into
  `TileEngine::resolveFireTargetVoxel()` and shared by both the real shot and the preview, so the
  drawn line matches what will actually be fired. The path stops at the first obstacle it hits.
- **Rendering.** A dedicated preview `Projectile` (kept out of the in-flight collection) is rebuilt
  only when the aim target/action/actor changes. It is drawn as spaced tracer sprites along the route
  with an impact marker at the end, in a top pass — so a lobbed arc that rises above the current view
  level is not hidden under higher floors. A single fixed tracer sprite is used for every preview
  (direct fire and throws alike). The tracer sprite is **mod-configurable** via the `constants`
  ruleset key `trajectoryPreviewSprite` (a Projectiles frame index, default `35` — the rifle-type
  base bullet; standard xcom1 sets `35`, xcom2 sets `36`); the impact marker uses `HIT.PCK`.
- Toggle: **Aim trajectory preview** (`battleTrajectoryPreview`, default on).
- **Spread dot cloud (hold Alt).** While aiming, holding **Alt** replaces the single ideal tracer
  line with a **cloud of sampled impact dots** showing where the shots would actually land — the
  aim-cone spread for cone-model direct fire, and the launch-error scatter for realistic throwing
  (elongated short/long along the throw line). Dots are coloured by outcome — **green** where the
  round lands on the target (its unit/wall/tile, or the target tile for throws), **yellow** where it
  was aimed on target but stopped by cover, and **red** for a genuine miss — so you can see how much
  of the spread connects and how much cover is eating. It reuses the already-cached hit-chance / landing-chance
  Monte-Carlos (each sampled round's real voxel impact, cover included), so it's truthful to the
  model and updates live with distance/kneel/cover. Combines with the existing Alt damage readout as
  a "detailed aim" mode. *(design:
  [plans/Feature-TargetingVisualization.md](plans/Feature-TargetingVisualization.md))*

## Blast Radius Dropoff

- **Blast radius dropoff** (`RuleItem.blastDropoff`, default `0.0`): explosion power can now taper
  from center to edge inside the AoE radius. `0.0` keeps vanilla flat behavior, while higher
  values increasingly weight damage toward ground zero.

## Explosion VFX/Sound Radius Scaling

- Explosion presentation for area-of-effect blasts now scales from blast radius rather than
  damage power.
- Explosion sprite density/spread are driven by the resolved blast radius, so wide-radius effects
  no longer look underpowered just because their damage value is low.
- Big vs small explosion sound selection is now keyed to radius as well, keeping audio behavior
  aligned with visual blast size.


## Aim-Cone Trajectory Model

Direct fire can now use a **3D direction-vector cone** model instead of the native
scatter-the-aimpoint deviation: the ideal muzzle→target ray is deflected by two independent,
Gaussian-sampled angular errors and the shot flies down the deflected line until it hits
something. Misses fan out from the muzzle and error grows naturally with distance. *(design +
full math/constants provenance: [plans/Feature-AimConeTrajectory.md](plans/Feature-AimConeTrajectory.md);
calibration: [reference/aimcone_montecarlo.py](reference/aimcone_montecarlo.py))*

- **Per-weapon opt-in:** new `RuleItem` key **`baseAccuracy`** (default `0`). `0` keeps the weapon
  on the native scatter model, byte-for-byte unchanged — all existing content is unaffected.
  `> 0` opts the weapon into the cone model and sets its intrinsic precision (higher = tighter;
  `75` is the calibration reference point). Shown in Stats for Nerds.
- **Two stacking cones** (all constants documented in `Projectile.cpp` and the design doc):
  - **Soldier cone** — everything about the shooter's aim (Firing skill × shot-mode × kneel ×
    one-handed × wounds × berserk, i.e. the folded `getFiringAccuracy` result), with
    σ = `0.437 / (soldierAcc²/50) · 1.4826 · 2` radians, accuracy floored at 20. Rolled once
    per round.
  - **Weapon cone** — driven only by `baseAccuracy`, σ = `0.437 / (baseAccuracy²/75) · 1.4826`
    radians. Rolled per projectile. The quadratic-in-`baseAccuracy` shape is a DX change from the
    legacy linear scaling (Monte-Carlo calibrated so `75` matches legacy exactly while the knob
    has real reach).
  - Every sampled deflection is clamped at 3σ of its own cone (no freak backwards shots).
- **No linear range dropoff on the cone path** — distance falloff is purely geometric. For
  opted-in weapons the `aimRange`/`snapRange`/`autoRange`/`minRange`/`dropoff` fields and
  `battleUFOExtenderAccuracy` no longer alter the shot (they will feed UI readouts).
- **No-LOS penalty** (`noLOSAccuracyPenalty`) widens the *soldier* cone (applied before the
  accuracy floor); weapon precision is unaffected.
- **Shotguns:** the whole volley shares one soldier-cone roll (the shooter's "true aim" line);
  each pellet then rolls its own weapon-cone deflection, scaled by the ammo's `shotgunSpread`
  (`100` = neutral). `shotgunBehaviorType` and `shotgunChoke` intentionally don't apply on the
  cone path — choke's pattern-tightness role is subsumed by `baseAccuracy`.
- **Auto/burst:** every round re-rolls both cones, so a burst walks around the target.
- **Unchanged by design:** throwing and arcing shots (parabola + scatter model), `BA_LAUNCH`
  guided missiles (faction-based drift), melee. The live trajectory preview already draws the
  cone's central axis and needed no changes.
- Gaussian sampling via a new `RNG::boxMuller()` on the seeded battle stream (deterministic
  for a given seed).
- **Physical hit-chance crosshair readout.** For cone-model weapons the aiming crosshair shows the
  estimated *physical odds of hitting the target* (color-graded red→yellow→green), replacing the
  native folded-accuracy number. It stacks the same two cones the shot uses and, via a deterministic
  Monte-Carlo, **voxel-traces each sampled shot against the real terrain** — counting only rays that
  actually reach the target's own voxel silhouette. So it's fully **cover-aware**: a wall or object
  between shooter and target drops the reading toward zero (a target behind a wall no longer reads a
  false high %), and partial cover reduces it proportionally, all on top of the natural distance
  falloff and weapon/skill differences. Shotgun-aware (any pellet reaching the target counts),
  no-LOS widens the estimate, and it's shown even without UFO Extender accuracy. The estimator is
  deterministic per aim, cached in `Map` (recomputed only when the aim changes, since tracing is
  expensive), and never perturbs the game RNG. The readout shows the full
  `<acc>% (-<cover>%) @ <distance>m` breakdown on one line (e.g. `45% (-20%) @ 12m`), centered just
  above the aiming crosshair with the hovered-unit name stacked above it: `<acc>%` is the real
  post-cover chance, `(-<cover>%)` is the informational reduction from intervening terrain (unit
  targets only; open chance = acc + cover), and `@<distance>m` is the range in tiles.
- **Effective-range action-menu readout.** For cone-model weapons, the battlescape action menu shows
  each direct-fire mode's **50%-hit effective range** (in tiles, `Rng:{N}`) in place of the accuracy
  `%` — because a cone weapon's per-mode `%` is only the soldier-cone input, not a hit chance, so the
  reliable-range number is the meaningful one. It's computed from the same two cones against a
  standard target in the open (`Projectile::calculateEffectiveRange`, target/terrain-independent, no
  tracing), reflects the mode's kneel/one-hand/wound/shot-type accuracy, and accounts for shotgun
  ammo spread. Vanilla (`baseAccuracy: 0`) weapons keep the classic accuracy `%` unchanged.
- **Extra accuracy modifiers (exhaustion & smoke).** Cone weapons add two soldier-cone penalties
  OXCE lacks (native scatter weapons are unaffected). *(design:
  [plans/Feature-AccuracyModifiers.md](plans/Feature-AccuracyModifiers.md))*
  - **Exhaustion** — below 50% energy a tired shooter's aim widens: multiplier `0.5 + energy/stamina`
    (1.0 at 50% energy, floored at 0.5 when spent). Direct fire only; folded into `getFiringAccuracy`
    so it flows through the shot, both readouts, and AI/reaction.
  - **Smoke on the line of fire** — smoke between shooter and target widens the soldier cone,
    summed along the LOF (mirroring the visibility smoke model), floored so heavy smoke degrades but
    never fully blinds. Applied on the shot and the hover hit-chance readout (not the effective-range
    readout, which is a clear-air property). Penalty constants are provisional pending tuning.

## Dual-Fire

A soldier holding a firearm in **each hand** can fire **both at once** with a new **Dual Fire**
action (`BA_DUALFIRE`, action-menu hotkey `7`). Each hand fires its own weapon, ammo, and **best
available fire mode** (first of Auto → Burst → Snap → Aimed it has), firing that mode's **full**
sequence — so two auto weapons spray simultaneously — both aimed at the same target, each with its
own aim-cone. *(design: [plans/Feature-DualFire.md](plans/Feature-DualFire.md))*

- **Shown when eligible:** offered whenever the unit holds two loaded, fire-capable firearms
  (`canDualFire()`); no option/flag — it's a player action.
- **Cost:** the higher of the two hands' chosen-mode TU × 1.1 (they fire simultaneously), capped at
  96 — `min(96, round(max(handTU_L, handTU_R) × 1.1))`.
- **Accuracy:** each hand uses its chosen mode's accuracy (the low auto/hip-fire accuracy is the
  dual-wield tradeoff); the two-handed occupancy penalty still applies.
- **Concurrent & independent:** both hands' full sequences fly at once via the async projectile
  system; each round resolves with its **own** weapon's damage, spends its own ammo, and awards its
  own firing experience. If one hand has no line of fire it simply sits out; the other still fires.
- Built on the async projectile system and reuses the standard per-mode firing pipeline (a nested
  off-hand sub-state driven by the primary).

## Realistic Throwing Accuracy

Optional physical throw-error model (the throwing analog of the aim-cone), replacing the native
"scatter the landing point into a symmetric disc" deviation. A thrown item now errs mostly in
**force** — short/long *along* the throw line — with a smaller **lateral** (left/right) error, and
the spread grows with distance and with **strain** (how close the throw is to the thrower's maximum
range for the item's weight, from `getMaxThrowDistance`). It reuses the existing parabola/reach and
`getFiringAccuracy(BA_THROW)` (Throwing stat × `accuracyThrow`); only the deviation stage changes,
so throw *range* is unchanged. *(design:
[plans/Feature-ThrowAccuracyRealism.md](plans/Feature-ThrowAccuracyRealism.md))*

- **Option:** `battleRealisticThrowing` (**default off**) — a DX battlescape option. When off,
  throwing is byte-for-byte vanilla (the native scatter deviation).
- **Model:** Gaussian offset along the throw direction (dominant) + a ~0.4× lateral Gaussian, each
  scaled by distance, throw accuracy (floored), and a mild strain factor; clamped at 3σ. Vertical aim
  is left true (the arc + terrain set landing height). Tuning constants are provisional.

## Per-Round Ammo Economy (`battleClipSize`)

A `RuleItem` field that **decouples how ammo is stocked from how it's loaded**, for weapons that fire
*expensive per-shot* rounds but are still *magazine-fed* (blaster bombs, fusion/artillery shells,
and — later — psi-orbs). With `battleClipSize` set, the ammo is **stocked, bought, sold, and
recovered one round at a time** (each base-store unit is a single round, so purchase/sell price and
inventory are tracked per shot), yet at battle generation the loose rounds are **packed into
magazines of up to `battleClipSize` rounds each**, and in the field the weapon loads / fires /
refills exactly like any normal magazine. So a weapon can hold, say, 4 shots per reload while every
shot is individually accounted for in the economy. *(design:
[plans/Feature-BattleClipSize.md](plans/Feature-BattleClipSize.md))*

- **`battleClipSize`** (RuleItem, default `0`) — non-zero enables the per-round model. It is
  **mutually exclusive with `clipSize`**: setting `battleClipSize > 0` forces `clipSize` to `0` at
  load time, so an ammo item uses one model or the other, never both.
- **Field logic** routes through a new `getBattleMagazineSize()` (= `battleClipSize` if set, else
  `clipSize`): initial magazine fill, the spend/refill gate, the loaded-ammo bar and inventory
  ammo-count badges, and the Ufopaedia clip-size line all show/behave against the magazine capacity,
  while `getClipSize()` stays the economy / base-store unit (`0` under this model, so stores stay
  per-round).
- **Battle generation** packs stored rounds into magazine `BattleItem`s
  (`BattlescapeGenerator::createStoredItemsForTile`) — `min(remaining, battleClipSize)` per magazine,
  so a stock that isn't a multiple of `battleClipSize` yields one partial final magazine.
- **Recovery** returns the raw recovered rounds to stores (rather than rounding down to whole clips),
  which also avoids dividing by the forced-to-zero `clipSize`.
- Intended for **hand-carried** magazine-fed ammo, not vehicle/HWP fixed ammo (which keeps the
  whole-clip economy).

## Weight-based Reload Cost (`battleWeightBasedReloadCost`)

An optional DX battlescape option that makes the **base** time to load or unload a magazine scale
with the magazine's **weight** — heavier magazines take longer to swap — instead of the flat
inventory-slot move cost. *(design:
[plans/Feature-WeightBasedReloadCost.md](plans/Feature-WeightBasedReloadCost.md))*

- **Option:** `battleWeightBasedReloadCost` (**default off**). When off, reload cost is byte-for-byte
  the stock behavior.
- **Formula:** the weight term is `magazineWeight * 2` (2 TU per unit of magazine weight), exposed as
  `BattleItem::getReloadWeightCost()`. On top of it sits the weapon's `tuLoad`/`tuUnload`, whose
  **default drops from 15/8 to 5** while this option is on (a weapon that sets `tuLoad`/`tuUnload`
  explicitly keeps its own value as the basis). So a typical reload ≈ `weight*2 + 5` — mirroring the
  legacy design, which *replaced* the flat 15 base with a weight-based cost rather than adding to it.
- **Model:** the weight term **replaces the base handling cost** — i.e. the OXCE
  `extendedItemReloadCost` inventory-slot move term where that applies — so the two don't stack.
  Incidental moves (bringing the weapon body into a hand) are unchanged. The reduced default base is
  applied in `RuleItem::getTULoad`/`getTUUnload` (an unset value now means "use the model's default").
- **Applied everywhere reloads are costed:** inventory drag-load and quick-swap, inventory unload,
  the battlescape mid-turn auto-reload, and the AI's reload-cost estimate (so the AI budgets reloads
  correctly). The OXCE slot-path option (`extendedItemReloadCost`) remains available and independent;
  when the DX option is on it takes precedence.

## Quick Reload action-menu item

OXCE already provides a quick-reload on the **R** key (`keyBattleReload` → `BattleUnit::reloadAmmo`,
which loads the cheapest compatible clip into a hand weapon's empty slot). DX **surfaces it as a
visible action-menu row** — matching the DX preference for discoverable UI over hidden hotkeys.
*(design: [plans/Feature-QuickReloadMenu.md](plans/Feature-QuickReloadMenu.md))*

- A **Reload** row (`BA_RELOAD`, reusing the localized `STR_RELOAD`) appears in a firearm's action
  menu when the weapon uses external clips and has an empty ammo slot. It shows the reload **TU cost**
  (weight-based when `battleWeightBasedReloadCost` is on) and is flagged red **No Ammo** (no
  compatible clip carried) or **No TU**, like the fire-mode rows. Its hotkey is `keyBattleReload` (R),
  so R still works with the menu open.
- Selecting it reloads *that* weapon, plays the reload sound, and refreshes the ammo readout.
- Shared logic: `reloadAmmo()` (the R key) and the menu item both route through a new
  `BattleUnit::reloadWeapon` / `getReloadCost` (built on a shared `findReloadAmmo` helper), so the
  keyboard and menu paths behave identically. Partial-magazine swap (ejecting a half-spent clip) is
  **not** included — the empty-slot behavior matches OXCE's R key.

## Base-Screen Ammo Counts

On the base item screens — **Buy, Sell, Transfer, Stores, and Craft Equipment** — an ammo row now
shows how many **rounds** each clip holds, appended to its name as `(xN)` (e.g. `Rifle Clip (x20)`). Only multi-round
clips (`clipSize > 1`) get the suffix; single-shot ammo (rockets) and per-round `battleClipSize` ammo
show nothing, so the readout reflects what a purchase / stockpile actually contains.
*(design: [plans/Feature-BaseScreenAmmoCounts.md](plans/Feature-BaseScreenAmmoCounts.md))*

- Reuses each screen's existing ammo classification (`BT_AMMO || (BT_NONE && clipSize>0)`); the format
  is the shared, translatable `STR_DX_AMMO_ROUND_COUNT` (`"{0} (x{1})"`). Manufacture screens are not
  covered (deferred).

## Sprint & Sneak Movement Modes

OXCE already has hidden **Run** (hold Ctrl) and **Sneak** (hold Alt) movement modes — armor-gated,
with per-mode TU *and* energy cost multipliers. Run is a full mechanic (cheaper TU / more energy);
sneak, by contrast, is nearly a stub in stock OXCE (its cost defaults to walk-equivalent, it forces
walking, and it has no built-in stealth/detection effect). DX **surfaces** Run as a visible sprint now
and adds sneak's colour; real sneak mechanics (evasion, the light gate) come with later Phase 7/8 work.
*(design: [plans/Feature-SprintSneakModes.md](plans/Feature-SprintSneakModes.md))*

- **Path-preview colours** — the move preview now colours tiles by mode: **blue** while sprinting
  (Run), **purple** while sneaking, keeping **red** for an unaffordable step and the normal
  green/yellow for a walk. Colours are **configurable** via a `pathfindingDX` interface element
  (`color` = sprint/blue, `color2` = sneak/purple, palette block indices like the base `pathfinding`
  element), shipped for both UFO and TFTD; the engine falls back to built-in defaults if unset.
- **Movement-mode animation pace** — a running unit animates ~2× faster and a sneaking unit ~1.5×
  slower (frame interval scaled in `UnitWalkBState::setNormalWalkSpeed`), so sprint reads as a fast
  dash and sneak as a slow, careful crawl. Both factors are tunable constants.
- **Sprint commits to its full path** — a sprinting unit no longer auto-stops when it spots a new
  enemy (the enemy is still revealed; you just can't halt mid-sprint to react). Normal walking still
  stops to react. The existing desperate/charging exemptions are unchanged.
- Deferred: the roadmap's "high hit chance" (sprint) and "high alertness / maintains evasion" (sneak).
  Sprint's cost side is covered by OXCE's run multipliers, but sneak has **no** stealth/evasion effect
  yet — that's genuine new work landing with the Phase 7 Reaction Scoring Split (evasion) and the
  Phase 8 lighting gate.

## Mod-configurable Armor Move-Cost Defaults (`moveCostDefaults`)

A top-level `moveCostDefaults:` ruleset node that sets the **default** movement costs armors fall back
to when they don't specify their own `moveCost:`. In stock OXCE those defaults are hardcoded in C++
(and notably `sneakPercent` defaults to `[100, 50]` — identical to walking), so making, say, "sneaking
is slow" game-wide meant editing every armor. Now a mod tunes the baseline once.
*(design: [plans/Feature-MoveCostDefaults.md](plans/Feature-MoveCostDefaults.md))*

```yaml
moveCostDefaults:
  sneakPercent: [200, 50]   # [time%, energy%] per tile - sneak: double TU, same energy
  runPercent:   [50, 100]   # sprint: half TU, double energy
```

- Same keys as an armor's `moveCost:` block (`walkPercent`/`runPercent`/`sneakPercent`/`strafePercent`,
  the `fly*`/`climb*`/`base*` variants, `gravLiftPercent`); each an `[time%, energy%]` pair.
- A **per-armor `moveCost:` value always overrides** the default. Merge-safe: each armor field carries
  a `{ -1, -1 }` "unset" sentinel that `Armor::afterLoad` resolves from `Armor::moveCostDefaults`, so
  only keys an armor actually sets win, across mod merges.
- Defaults equal the old hardcoded values, so **no `moveCostDefaults:` node ⇒ movement is unchanged**.

## Reaction Scoring Split (offensive reaction / defensive evasion)

Stock reaction fire uses one `getReactionScore()` (`reactions × currentTU/maxTU`) for two opposite
roles: how good a unit is at *reacting* (offence) **and** how hard it is to *be reacted against*
(defence). DX splits them so a unit's defence can be tuned without touching its offence.
*(design: [plans/Feature-ReactionScoringSplit.md](plans/Feature-ReactionScoringSplit.md))*

- New **`BattleUnit::getEvasionScore()`** (defensive) = `getReactionScore()` × the wearer's armor
  **`evasion`** percent. The reaction-fire check now measures the **moving unit** by its evasion score
  and each **spotter** by its (unchanged) reaction score.
- New **`Armor.evasion`** field (percent, **default 100** = no change). `> 100` makes a unit harder to
  react-fire against (stealth armor); `< 100` easier. Its *own* reaction fire is unaffected.
- Both scores are exposed to Y-Script (`getReactionScore`, `getEvasionScore`).
- Default 100 everywhere ⇒ **reaction fire is byte-for-byte stock**. This is the foundation for
  independent evasion drivers still to come (sneak-mode evasion, `RuleStatBonus`/script hooks).

## Movement-mode Evasion (sprint/sneak alter reaction-fire evasion)

Building on the reaction split, a moving unit's **defensive evasion** now varies with *how* it moves.
The base score is `reactions × (currentTU/maxTU)`; a sprint/sneak config reshapes its **two parts
independently** — the **reactions stat** (`statPercent`) and the **TU/maxTU penalty**
(`tuPenaltyPercent`, where `tuFactor = 1 - (1 - currentTU/maxTU) × tuPenaltyPercent/100`). So you can
express e.g. *sprint = half reactions but only half the TU penalty*, or *sneak = full reactions at all
times*. Mod-configurable **globally and per-armor**.
*(design: [plans/Feature-MovementModeEvasion.md](plans/Feature-MovementModeEvasion.md))*

- `BattleUnit::getEvasionScore(BattleActionMove)` recomputes the score with the mode's `statPercent`
  /`tuPenaltyPercent` (× the armor's flat base `evasion`); reaction fire threads the mover's
  `getMoveType()` through `getSpottingUnits`/`getReactor` so the mover is judged by its *move-adjusted*
  evasion (spotters' offensive scores are untouched).
- **Global:** `evasionDefaults: { sprint: {statPercent, tuPenaltyPercent}, sneak: {...} }`.
- **Per-armor:** `Armor.evasionSprint` / `Armor.evasionSneak` sub-maps override the global **per field**.
- Non-move reactions (shooting, turning) report `BAM_NORMAL` ⇒ plain score; `{100,100}` everywhere ⇒
  reaction fire byte-for-byte unchanged (reproduces `reactions × TU/maxTU`).

## Overwatch (set-and-hold reaction fire, cone-based)

A deliberate held-fire state a unit enters on its own turn to react during the enemy turn — DX's take
on the legacy radius overwatch, rebuilt around a **directional cone**.
*(design: [plans/Feature-Overwatch.md](plans/Feature-Overwatch.md))*

- **Cone, not radius.** Pick **Overwatch** from a firearm's action menu (hotkey `keyBattleActionItem8`,
  default **O**), then click a tile to aim; the watched area is a cone from the unit toward that tile,
  defined per-weapon by `overwatchConeAngle` (full cone width, widens with range), `overwatchRange`, and an
  optional `overwatchMinRange` near dead zone. `overwatchShot` picks the fire mode (snap/burst/auto/
  aimed) and `overwatchModifier` scales the offensive reaction score.
- **Confirm the target like a shot.** Because arming spends TU and locks a facing, targeting takes a
  confirmation click: the first click on a tile locks the aim (the cone freezes there and a target
  reticle appears), and a second click on that same tile arms overwatch. Right-clicking backs out of
  the pending target without leaving overwatch mode.
- **Reaches its full cone range on shared sight.** Ordinary reaction fire is capped at the engine's
  max view distance (~20 tiles) and needs the reactor's own line of sight. Overwatch is exempt: it can
  trigger anywhere inside its cone up to `overwatchRange`, and it fires as covering fire when *any
  teammate* is spotting the mover — the watcher itself doesn't need line of sight, only a valid line of
  fire. This lets a long-range sniper overwatch cover ground a spotter reveals.
- **Opt-in + mod-configurable defaults.** `overwatchRange` defaults to **0** = overwatch off (the menu
  option is hidden), so a weapon opts in by setting `overwatchRange > 0`. All the per-weapon defaults
  are overridable mod-wide via a top-level `overwatchDefaults:` node (same keys), so a mod can enable
  overwatch broadly or change the default shot/angle without touching each item.
- **Trigger-tile markers.** While aiming (and when a unit already on overwatch is reselected), every
  tile inside the cone gets a **tile-level marker** (the dithered Pathfinding target-reticle sprite,
  like the path preview's tile markers), so the watched area reads as a translucent filled region.
- **Feedback.** Arming plays a reload/ready sound and logs `{unit} is on overwatch` to the floating
  combat log. When the overwatch fires, it reuses the shared fire line but renders the **overwatch**
  variant (`{unit} took an overwatch shot at {target} with {weapon}`) instead of the reaction wording.
  The target name is research-gated like every other combat-log unit name. Entries use the actor's
  faction color (green for the player, red for hostiles).
- **One reserved shot, then TU.** Arming reserves a single **free** shot for the upcoming enemy turn
  (its TU paid up front); every further overwatch shot spends the unit's TU like a normal reaction
  (ammo is consumed either way). An overwatch shot uses the unit's **full reactions stat** ×
  `overwatchModifier` (not the TU-depleted score), so a committed watcher reacts reliably.
- **Needs ammo.** Overwatch can't be armed on an empty weapon (the menu row flags *No Ammo* and arming
  warns *No Rounds left!*), and running dry cancels it: the shot that empties the weapon drops overwatch
  immediately, and any lingering empty-weapon overwatch is cleared the next time reaction fire is checked.
- **Cone-exclusive trigger.** Folded into `TileEngine::checkReactionFire`: an overwatching enemy fires
  when the mover enters its cone (with line of fire), **exempt** from the normal reaction threshold and
  the spot-to-protect restriction — but conversely it takes **no** ordinary reaction fire at anything
  **outside** the cone.
- **Persists across turns.** Overwatch stays until cancelled; only the reserved free shot is a
  one-turn thing (it expires at the owner's next turn, after which shots just spend TU). State survives
  save/load. It **auto-cancels** when the unit is commanded to move or fire a normal shot, and can be
  toggled off by re-selecting **Overwatch**.
- Deferred: a dedicated on-map per-unit overwatch glyph, per-tile line-of-fire filtering of the
  markers, and AI use of overwatch.

## Bleedout & Indicators (negative-health dying state)

In stock OXCE a unit dies the instant its health hits 0. DX lets eligible units drop into **negative
health** and *bleed out* — a dying-but-savable window — with battlefield cues so a dying soldier is
obvious. *(design: [plans/Feature-Bleedout.md](plans/Feature-Bleedout.md))*

- **Bleed out instead of dying.** When an eligible unit's health crosses 0, instead of dying it enters
  **bleedout**: it falls (unconscious) but stays alive, and a buffer of fatal torso wounds keeps its
  health draining each turn (the engine's existing 1 HP/wound/turn). It only actually dies once health
  reaches the death threshold, so a medic has a window to reach it. Healing its wounds stops the bleed;
  recovering above 0 HP ends the bleedout.
- **Mod-configurable, legacy defaults.** Eligibility is per-armor `canBleedOut` (a tri-state: unset =
  the legacy rule — an original-player, non-vehicle geoscape soldier; `true`/`false` force it on/off).
  A top-level **`bleedoutDefaults:`** node sets `deathHealthPercent` (default **50** → death at
  `−maxHealth/2`) and `bufferWounds` (default **5** torso wounds on entry). Aliens/civilians/HWPs keep
  the stock hit-0 behaviour unless a mod opts their armor in.
- **Indicators (three).** A bleedout-priority **red-cross glyph** on the downed body (mod art
  `FloorBleedoutIndicator`, else a built-in procedural cross); **white tick marks** — one per fatal
  wound — on the selected unit's HP bar (`Bar::setMarks`, complementing the existing wound blink); and
  bleeding-out soldiers listed **first** in the visible-unit column with a distinct *"Center on
  bleeding-out soldier"* tooltip (a click-to-jump cue shown regardless of the selected unit).
- **Combat log.** Entering bleedout logs `{unit} is bleeding out!` (WARNING); death from bleedout logs
  via the normal kill path.
- **Not yet:** the medikit "stabilize" rework (stabilise-without-revive) and proportional wound
  recovery / Field Surgery research are separate Phase 7 items that build on this.
