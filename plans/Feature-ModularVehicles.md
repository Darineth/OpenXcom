# Feature: Modular Vehicles (HWPs)

**Status:** Implemented (Jul 2026). Builds clean (Release/Win32); the demonstrator mod loads with zero
ruleset errors. Phase 10 roadmap item. Shipped as the six engine deltas D1–D6 below plus a working
`dx-test` chassis; the legacy content roster is deliberately left as mod content.

Checklist item: *Phase 10 — **Modular Vehicles (HWPs)** — chassis/engine/armor/weapon
customization + weapon tree. (needs inventory layouts, directional armor/sided slots, item stats)*

All three prerequisites shipped in Phase 4, so this feature is unblocked.

## Motivation

Vanilla (and OXCE) sell four monolithic HWP products — Tank/Cannon, Tank/Rocket Launcher,
Tank/Laser Cannon, Hovertank/Plasma — each an indivisible purchase with a fixed weapon, fixed
ammo, fixed armor, and no progression beyond "buy the next tier". An HWP is an *item* that the
engine converts into a unit at mission start; the player never interacts with its loadout.

Legacy DX replaced that with a **chassis + loadout build system**: the player buys a chassis,
then fits an engine (which sets mobility and carry capacity), bolts armor plates onto individual
facings, mounts one or two turret weapons, and slots accessory modules — all against a weight
budget the engine's `strength` has to cover. Four SKUs become a combinatorial build space with
its own research tree. See
[reference/Legacy-DX-Content/Vehicles-HWPs.md](../reference/Legacy-DX-Content/Vehicles-HWPs.md)
for the full legacy content dump (4 chassis × 16 engines × ~15 turret weapons × 10 modules ×
4 plate types) and [Legacy-DX-Features.md §11](../Legacy-DX-Features.md#L679).

## OXCE / OXCE-Plus + DX audit (Jul 2026)

**Headline result: the large majority of this system is already expressible in rulesets today.**
Modern OXCE grew purchasable/salaried soldier types, and DX's Phase 4 shipped the inventory and
item-stat plumbing. What legacy DX needed a bespoke `isVehicle:` subsystem for is now mostly
composition of existing features. This section records what exists so the implementation only
builds true deltas.

### Already present upstream (OXCE) — no DX work needed

| Legacy need | Provided today by |
|---|---|
| Chassis is a soldier-like unit | `soldiers:` root — `RuleSoldier` ([RuleSoldier.h:63](../src/Mod/RuleSoldier.h#L63)) |
| Buy a chassis with cash | `costBuy` (`RuleSoldier::getBuyCost()`) |
| Monthly upkeep per vehicle (legacy `costVehicle`) | `costSalary` (`RuleSoldier::getSalaryCost()`) |
| Manufacture a chassis | `RuleManufacture::spawnedSoldier` ([RuleManufacture.h:53](../src/Mod/RuleManufacture.h#L53)) |
| Research-gated availability | `RuleSoldier::requires` / `requiresBuyBaseFunc` / `requiresBuyCountry` |
| No promotions / no ranks | `allowPromotion: false` |
| Fixed stats, no crew variance | `minStats == maxStats`, `femaleFrequency: 0` |
| No stat growth from missions | `statCaps` == base stats (growth is capped out) |
| 2×2 unit that still has an inventory | `Armor.size: 2` + `allowInv: true`; `BattleUnit::hasInventory()` is just `_armor->hasInventory()` ([BattleUnit.cpp:5876](../src/Savegame/BattleUnit.cpp#L5876)) |
| 2×2 soldiers actually spawn in battle | `BattlescapeGenerator` has a dedicated 2×2-soldier pass ([BattlescapeGenerator.cpp:1135](../src/Battlescape/BattlescapeGenerator.cpp#L1135)) |
| A 2×2 soldier costs 4 craft slots | `Craft::getSpaceAvailable()` already uses `soldier->getArmor()->getTotalSize()` ([Craft.cpp:1452](../src/Savegame/Craft.cpp#L1452)) |
| Loadout survives between missions, consumes stores | Standard soldier equipment layout (`EquipmentLayoutItem`) |
| Weight budget vs. carry capacity | Existing `strength` vs. `getCarriedWeight()` encumbrance ([BattleUnit.cpp:3013](../src/Savegame/BattleUnit.cpp#L3013)) |
| Chassis turret sprite exists as a concept | `BattleUnit::_turretType`, `UnitSprite` turret draw ([UnitSprite.cpp:805](../src/Battlescape/UnitSprite.cpp#L805)) |
| Turret index on an item | `RuleItem::turretType` ([RuleItem.h:772](../src/Mod/RuleItem.h#L772)) |

`Extended.txt` has **no** modular-vehicle entries — OXCE never built this system; it only
generalized the soldier type enough that the strategic half falls out for free.

### Already present in DX (Phase 4) — no work needed

| Legacy need | Shipped DX feature |
|---|---|
| Per-chassis hardpoint layout instead of belt/backpack | **Inventory layouts** — `inventoryLayouts:` + `Armor.inventoryLayout` ([Feature-ConfigurableInventoryLayouts.md](Feature-ConfigurableInventoryLayouts.md)) |
| Slots that only accept engines / plates / ammo | **Typed slots** — `RuleInventory.battleType` filter ([Feature-TypedInventorySlots.md](Feature-TypedInventorySlots.md)) |
| Hardpoints you cannot re-fit mid-battle | **Typed slots** — `allowCombatSwap: false`, `costs: -1` |
| Only some slots grant their items' stats | **Typed slots** — `countStats` |
| Free reload from the ammo rack | **Typed slots** — `costs: { STR_AMMO_RACK: 0 }` |
| Engines/modules granting `tu`, `strength`, `health`, … | **Item stats & stat-modifiers** — `RuleItem.stats` / `statModifiers` ([Feature-ItemStatsModifiers.md](Feature-ItemStatsModifiers.md)) |
| Chassis-wide TU bonus (Scout Car +20, Hover +30) | **Item stats** — `Armor.statModifiers` |
| Equipped items adding armor | **Directional armor on items** — `RuleItem.frontArmor`/`sideArmor`/`rearArmor`/`underArmor` folded into per-side max armor with separately-tracked per-side damage ([Feature-DirectionalArmorOnItems.md](Feature-DirectionalArmorOnItems.md)) |
| Turret 1 / Turret 2 as real, fireable mounts | **Configurable hand slots** — `hand: right\|left` on any layout section ([Feature-ConfigurableHandSlots.md](Feature-ConfigurableHandSlots.md)) |
| Headlight / night-vision vehicle equipment | **Utility slots** + Light Equipment ([Feature-UtilityEquipmentSlots.md](Feature-UtilityEquipmentSlots.md), [Feature-LightEquipment.md](Feature-LightEquipment.md)) |
| Burst/auto turret fire, overwatch, shotgun shells, arcing artillery, `battleClipSize`, `baseAccuracy` | Shipped Phase 5/6 features — these are **item** features and apply to turret weapons with no vehicle-specific work |

### True deltas — what this feature actually has to build

Everything above composes into a working modular vehicle *except* the following. These are the
scope of the implementation.

**D1 — Chassis behavior flag on `RuleSoldier`.**
No `isVehicle` exists anywhere in the tree today (grep: zero hits outside unrelated
`CraftEquipmentState` locals). Even with `allowPromotion: false` and capped stats, a chassis is
still a *person* to several subsystems. Needs an audit-and-gate pass over: mission stat
improvement / experience posting (`Soldier::postMissionProcedures`), psi training and gym
training (`Soldier::trainPhys`, psi lab), wound recovery by days vs. "repair", name-pool naming,
`allowPiloting`, memorial/statistics wording, and `SoldierDiary`. The flag itself is trivial; the
work is enumerating which of these should change and localizing the vehicle-flavored strings.

**D2 — New non-wieldable item classes.**
`enum BattleType` is `BT_NONE … BT_CORPSE` (0–11, [RuleItem.h:35](../src/Mod/RuleItem.h#L35)).
Armor plates and engine/accessory modules need their own types so typed slots can filter them and
so they are never wieldable, throwable, primeable, or usable. Legacy used `battleType: 12` and
`13`, which are exactly the next two ordinals — appending `BT_ARMOR_PLATE = 12` and
`BT_EQUIPMENT = 13` is source- and save-compatible (same append-only rule the
`INV_UTILITY`/`INV_EQUIP` work followed).

*Revised during implementation:* the second class was first named `BT_VEHICLE_MODULE`, but nothing
in the engine keys off vehicle-ness — the only rule beyond slot filtering is "installed gear is
never wielded", which is equally true of a soldier's night-vision module or exoskeleton servos. It
is now the generic **`BT_EQUIPMENT`**, pairing with the `INV_EQUIP` slot type DX already ships.
`BT_ARMOR_PLATE` was already generic (a soldier's plate carrier is the same idea as a tank's
hardpoints). Modders needing "engines only in engine bays" use the item-side
`supportedInventorySections`, which is the right layer for that.

**D3 — Slot-declared armor facing.**
DX's directional armor is **item-declared**: a plate carries its own front/side/rear/under
values. Legacy plates are **slot-declared** — one `STR_HWP_LIGHT_ARMOR_PLATE` item type stacks
into `..._FRONT`, `..._LEFT`, `..._REAR`, `..._UNDER` sections and contributes to whichever facing
it occupies. Without this, a modder needs four near-identical item types per plate tier (and the
player has to buy the right-facing variant), which is exactly the SKU sprawl this feature exists
to remove. Delta: an `armorSide:` property on the inventory section, honored by
`BattleUnit::recalculateMaxArmor` ([BattleUnit.h:727](../src/Savegame/BattleUnit.h#L727)).

**D4 — Turret sprite driven by the mounted weapon.**
`_turretType` is set exactly twice — from the HWP item at spawn
([BattlescapeGenerator.cpp:1469](../src/Battlescape/BattlescapeGenerator.cpp#L1469)) and for
units spawned from a weapon ([BattlescapeGame.cpp:2633](../src/Battlescape/BattlescapeGame.cpp#L2633)).
A soldier-chassis has no turret at all, so it would render as a turretless hull regardless of what
is mounted. Delta: derive the turret type from the item in the chassis' turret mount, and
recompute when that item changes. This also gates `strafe`, melee/CQC exemption, and the
turn-turret-not-hull behavior, all of which key off `getTurretType() != -1` — so it is not
cosmetic.

**D5 — Geoscape stat preview.**
`Soldier::prepareStatsWithBonuses` folds in armor stats and soldier bonuses but **not** equipment
layout items ([Soldier.cpp:2142-2155](../src/Savegame/Soldier.cpp#L2142)); item stats are applied
only at battlescape level via `BattleUnit::getBaseStats`. For a soldier that is fine (a rifle
doesn't change your TU). For a chassis whose *entire* mobility and carry capacity come from an
installed engine, the base-screen stat display would show TU 0 and strength 0. Delta: make the
geoscape stat preview account for `countStats` layout items.

**D6 — Vehicle-flavored combat tuning.**
Legacy disabled the overweight TU-recovery penalty for vehicles and made reloading free. The
free-reload-from-rack half is already covered by typed-slot `costs`; the encumbrance-penalty half
([BattleUnit.cpp:3013](../src/Savegame/BattleUnit.cpp#L3013)) needs a knob. Small, but it changes
how the weight budget plays: without it, an over-weighted vehicle is slow *and* degrades, which
may double-punish.

### Explicitly out of scope for the engine work

The legacy content set — 4 chassis, 16 engines, 15 turret weapons + munition families, 10 modules,
4 plate tiers, and the ~20-node `STR_MODULAR_HWP_UPGRADES` research tree — is **mod content**, not
engine code. Once D1–D6 land, all of it is authorable in a ruleset. This doc plans the engine
support plus a minimal demonstrator chassis in `dx-test`; shipping the full legacy roster is a
separate content effort.

## Design sketch (pending the decisions below)

A chassis is a `RuleSoldier` + a 2×2 `Armor` + a `RuleInventoryLayout`:

```yaml
soldiers:
  - type: STR_MEDIUM_TANK
    costBuy: 300000
    costSalary: 20000
    allowPromotion: false
    allowPiloting: false
    vehicle: true                 # D1
    femaleFrequency: 0
    armor: STR_MEDIUM_TANK_ARMOR
    minStats:  { health: 70, reactions: 40, firing: 50, strength: 0, tu: 0, stamina: 100 }
    maxStats:  { ... same ... }
    statCaps:  { ... same ... }

armors:
  - type: STR_MEDIUM_TANK_ARMOR
    size: 2
    allowInv: true
    inventoryLayout: STR_MEDIUM_TANK_INV
    frontArmor: 30
    sideArmor: 25
    rearArmor: 18
    underArmor: 18

inventoryLayouts:
  - id: STR_MEDIUM_TANK_INV
    invs: [ STR_TURRET, STR_TURRET_2, STR_AMMO_RACK, STR_ENGINE_MEDIUM,
            STR_ARMOR_MEDIUM_FRONT, STR_ARMOR_MEDIUM_LEFT, ... ]

invs:
  - id: STR_TURRET
    hand: right                   # turret mounts are the layout's "hands"
    battleType: 1                 # BT_FIREARM only
    allowCombatSwap: false
  - id: STR_ENGINE_MEDIUM
    battleType: 13                # BT_EQUIPMENT  (D2)  -- generic installed gear, not vehicle-only
    countStats: true
    allowCombatSwap: false
  - id: STR_ARMOR_MEDIUM_FRONT
    battleType: 12                # BT_ARMOR_PLATE     (D2)
    armorSide: front              # (D3)
    allowCombatSwap: false
```

**Turret mounts are the layout's two hands.** This is the load-bearing design choice: it means
firing, reaction fire, dual-fire, reloading, the action menu, ammo handling, and TU costs all work
on turret weapons with **zero** new code, because DX's configurable hand slots already decoupled
handedness from the `STR_RIGHT_HAND`/`STR_LEFT_HAND` ids. A chassis with one mount (Scout Car)
just declares one hand; the layout hand resolution is already null-guarded for an omitted hand.

## Decisions (confirmed with user, Jul 2026)

1. **Additive, opt-in.** OXCE's item-based `vehicleUnit` HWPs are untouched and keep working;
   chassis-as-soldier is a parallel path a mod chooses. No save or mod breakage.
2. **Slot-declared armor facing (D3) ships.** One plate item type serves every hardpoint.
3. **Engine deltas + a `dx-test` demonstrator.** Grown on request into a full flat port of the
   vanilla HWP line — see *Demonstrator content* below. The **legacy** roster (16 engines, artillery,
   the research tree) remains a separate effort.
4. **Piloted vehicles deferred.** Needs a rider/occupant concept on `BattleUnit` plus crew
   casualty/bail-out rules — its own feature and design doc.

## What shipped

| Delta | Ruleset surface | Where |
|---|---|---|
| D1 chassis flag | `soldiers:` → `vehicle: true` | [RuleSoldier.h](../src/Mod/RuleSoldier.h), [Mod.cpp](../src/Mod/Mod.cpp) `genSoldier`, [Soldier.cpp](../src/Savegame/Soldier.cpp) ctor, both Allocate*TrainingState |
| D2 hardware item classes | `items:` → `battleType: 12` / `13` | [RuleItem.h](../src/Mod/RuleItem.h) enum, [Inventory.cpp](../src/Battlescape/Inventory.cpp) `checkSlotRules` |
| D3 slot-declared facing | `invs:` → `armorSide:` | [RuleInventory.cpp](../src/Mod/RuleInventory.cpp), [BattleUnit.cpp](../src/Savegame/BattleUnit.cpp) `recalculateMaxArmor` |
| D4 turret from weapon | `armors:` → `turretFromWeapon: true` | [BattleUnit.cpp](../src/Savegame/BattleUnit.cpp) `refreshTurretType` |
| D5 geoscape stat preview | *(none — behavioral)* | [Soldier.cpp](../src/Savegame/Soldier.cpp) `prepareStatsWithBonuses`, [SoldierInfoState.cpp](../src/Basescape/SoldierInfoState.cpp) |
| D6 encumbrance knob | `armors:` → `ignoresEncumbrance: true` | [BattleUnit.cpp](../src/Savegame/BattleUnit.cpp) `prepareTimeUnits` |

### Notes from implementation

- **D1 came out much smaller than the audit predicted.** Stat growth, gym training and psi training
  are *already* inert for a chassis whose `statCaps` equal its base stats — no gating needed. The
  flag's real jobs turned out to be naming (the "John Doe" fallback is genuinely wrong for hardware),
  hiding it from the training screens, and suppressing the missing-name-pool load error.
- **Naming needed a `Language`.** `Mod::genSoldier` gained an optional trailing `const Language*` so
  the designation can be localized; all six call sites pass one (`Production::step` already carried a
  `Language*`, which set the precedent). A null falls back to the raw type id.
- **Training screens are gated by click, not by filtering.** Row indices in `AllocateTrainingState` /
  `AllocatePsiTrainingState` are coupled to `_base->getSoldiers()` order in ~10 places each (arrow
  reordering, `SoldierInfoState(_base, _sel)`); filtering rows would have required remapping all of
  them. The first pass therefore listed chassis with an `N/A` status and blocked the click.
  **Revised on review:** an ineligible row is still clutter, so vehicles are now filtered out
  entirely. Each state gained a `_rowIndex` map (visible row -> base-soldier index) that every
  consumer goes through — the click handler, `SoldierInfoState(_base, _rowIndex[_sel])`, the
  arrow/wheel bounds, and the reorder helpers, which now swap with the previous/next *visible*
  soldier so a hidden chassis in between stays put. The assign/remove-all handlers skip vehicles so
  their row counters stay aligned, and `initList` defensively releases a training slot a chassis
  should never have held. `STR_NO_VEHICLE` is gone.
- **D5 is a separate stat cache on purpose.** `BattleUnit::computeEffectiveBaseStats` starts from
  `getStatsWithAllBonuses()` and then applies the live inventory itself, so folding equipment into
  that cache would double-count every item in battle. `getStatsWithEquipment()` is display-only.
- **Armor selection is refused at all three doors plus the sink.** `SoldierArmorState` is reachable
  from `SoldierInfoState`, `InventoryState` and `CraftArmorState`; each now declines to open it for a
  chassis, and `SoldierArmorState::lstArmorClick` refuses as a backstop so the invariant survives a
  future entry point. Two easy misses: `CraftArmorState`'s **right-click** is a quick-swap-to-last-armor
  shortcut (also an armor change — blocked), and its **ctrl-click** is craft assign/unassign, which had
  to keep working. On `SoldierInfoState` the ARMOR button stays *visible* because it doubles as the
  armor-name readout (it shows the chassis type); only the click is inert, matching how that handler
  already treats a deployed craft.
- **The rank column reports the chassis type.** `Soldier::getRankString()` returns
  `_rules->getType()` for a chassis instead of `STR_RANK_NONE`, so every list that shows a rank
  (`SoldiersState`, `CraftSoldiersState`, soldier info, inventory, memorial, `UnitInfoState`) reads
  "DX Tank" rather than a bare "-". One change covers them all because every one of them goes
  through `tr(getRankString())`. Modder override is preserved: a vehicle type that defines
  `rankStrings` falls through to the normal path.
- **Promotion is locked at the rule, not the call site.** `Soldier::promoteRank()`/`setRank()`
  already early-return on `!getAllowPromotion()`, so the only real gap was that a chassis relied on
  the modder remembering `allowPromotion: false`. `RuleSoldier::afterLoad` now forces it off when
  `vehicle` is set (same pattern as `RuleItem` forcing `psiRequired` for psi-amps). Doing it there
  rather than guarding `promoteRank()` covers every consumer in one line — auto/manual promotion,
  `getRankString()` (`STR_RANK_NONE` instead of "Rookie"), transformation demotion, the manual
  promotion UI, and `RankCount`'s `_totalSoldiers`, which would otherwise have counted chassis
  toward promotion openings and handed the player extra senior ranks for owning tanks.
- **New Battle needed a fix, not just a feature.** Its random-soldier roll picks from *all*
  `soldiers:` types and then runs `promoteRank()` plus random per-stat bumps — which would silently
  break a chassis' fixed-stats/no-rank invariant as soon as a mod defined one. Chassis are now
  excluded from that roll and stocked deterministically by `NewBattleState::stockVehicles()`
  (4 per type, top-up semantics so it also repairs an older `battle.cfg`). They are deliberately
  **not** auto-assigned to the craft: a 2×2 chassis costs 4 of the craft's 14 space points, so
  filling the large-unit cap would leave room for ~2 soldiers.
- **The demonstrator caught a real bug.** Loading it logged
  `STR_DX_TEST_TANK: total soldier name pool weight is invalid` — correct for a soldier, wrong for a
  chassis. `RuleSoldier::afterLoad` now skips that check when `vehicle` is set.
- **`ignoresEncumbrance` ships default-off** and the demo leaves it off, because the weight-vs-engine
  budget *is* the interesting mechanic — an under-powered engine should make a sluggish tank. The
  knob exists for mods that would rather enforce the budget purely by what an engine can mount.

## Demonstrator content (`bin/standard/dx-test/dx-test-vehicles.rul`)

A flat port of all five vanilla HWPs into the modular system, so the feature is playable without
authoring anything:

| Vanilla product | Becomes |
|---|---|
| Tank/Cannon, Tank/Rocket Launcher, Tank/Laser Cannon | **DX Tank** chassis + the matching turret |
| Hovertank/Plasma, Hovertank/Launcher | **DX Hovertank** chassis + the matching turret |

- **2 chassis** — tank (ground) and hover (`movementType: 1`, `constantAnimation`, float height 6),
  each with its own layout: the tank stacks **2** plates per facing, the hover **1**, mirroring how
  the vanilla Hovertank trades depth for flight and an all-round hull.
- **5 turrets** — cannon/rocket/laser/plasma/launcher, `turretType` 0–4 so each draws its own turret
  sprite. Power, accuracy, TU costs, sprites, sounds and `compatibleAmmo` are verbatim from xcom1;
  the laser/plasma/launcher keep their vanilla `requires:` research gates.
- **2 engines** — basic (55 TU / 150 str) and advanced (75 TU / 280 str). Deliberately a *choice*:
  the basic engine cannot haul a heavy turret **and** a full plate set (tank + cannon + 8 plates =
  190 vs 150 str → overweight, losing TU every turn), the advanced one can (215 vs 280). That trade
  is the core mechanic and is the reason `ignoresEncumbrance` is left **off** here.
- **Plates + targeting module**, and the three vanilla HWP ammo types merged with `vehicleItem: true`
  so a chassis can load them (no `units:` on ammo — a crewman may carry spare rounds).

Two deliberate deviations from vanilla, both to make the system visible rather than cosmetic:

1. **TU and strength are 0 on the chassis** (vanilla units have 70–100 TU, 60 strength). All of it
   comes from the engine, so an unfitted hull genuinely cannot move.
2. **Hull armor is below vanilla and plates make up the difference** — tank hull 66/55/44/44 plus
   2×12 per facing ≈ vanilla's 90/75/60/60. An unplated chassis is meaningfully softer, so the plate
   economy is worth engaging with.

## Not built (and why)

- **The legacy content roster** — 4 chassis, 16 engines, ~15 turret weapons and munition families,
  10 modules, 4 plate tiers, the ~20-node `STR_MODULAR_HWP_UPGRADES` research tree. All of it is
  authorable in a ruleset now; none of it is engine work.
- **Piloted vehicles** — deferred, see decision 4.
- **A dedicated vehicles UI tab.** Chassis appear in the ordinary soldier lists. OXCE's
  `RuleSoldier::group` can already separate them; a bespoke screen is not needed to play the feature.

## TODO: audit the remaining soldier-list screens

**Only the two training screens have been reviewed for chassis so far.** Every other screen that
lists or acts on soldiers still treats a chassis as an ordinary person, and each needs a decision:
*correct as-is*, *hide the chassis*, or *reword for hardware*. Nothing here is known to be broken —
this is an unreviewed surface, not a bug list.

Note the pattern established in `AllocateTrainingState` / `AllocatePsiTrainingState`: hiding rows
requires a `_rowIndex` (visible row → base-soldier index) because row indices are otherwise passed
straight to `SoldierInfoState(_base, index)` and to the reorder helpers. Any screen that filters
must do the same remap, or it will open/move the wrong soldier.

| Screen | Question |
|---|---|
| `SoldiersState` | The main roster. Chassis probably *should* appear — but check sorting, statStrings, and the rank column for a rankless unit. |
| `CraftSoldiersState` | Assigning a chassis to a craft is intended and must keep working (2×2 costs 4 slots). Verify the space readout and "no space" messaging read sensibly. |
| `SoldierTransformationListState` / `SoldierTransformationState` / `SoldierTransformState` | Transforming a tank into a psi-trooper is nonsense. Likely hide, or let mods gate it via existing transformation requirements. |
| `SoldierMemorialState` | A destroyed chassis in a memorial with cause-of-death wording. Decide: list it, or keep the memorial for people. |
| `SoldierDiary*` (Overview/Performance/Mission/Light) | Commendations and kill diaries for hardware. Harmless but odd. |
| `SoldierRankState` | Promotions are already off via `allowPromotion: false`; confirm the screen can't be reached or does nothing for a chassis. |
| ~~`SoldierArmorState`~~ | ✅ **Done.** Refused from all three entry points and in the picker itself. |
| `SoldierAvatarState` | Avatar/look picking for a tank — almost certainly hide. |
| `SoldierBonusState` | Read-only bonus list; probably fine, verify it renders. |
| `SackSoldierState` | Sacking a chassis should read as scrapping/selling hardware, not dismissing a person. |
| `CraftPilotSelectState` / `CraftPilotsState` | Already excluded via `allowPiloting: false`; confirm no chassis can be picked. |
| Base personnel counts / `MonthlyCostsState` | A chassis draws `costSalary`; check whether it should be counted under soldier headcount or shown separately. |
