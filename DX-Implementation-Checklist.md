# OpenXcom DX Implementation Order

This document proposes an implementation order for the features in
`DX-Implementation-Plan.md`. The sequencing philosophy is **UX-first, then mechanics in
dependency order**:

1. **Front-load improvements that work on the *existing* engine** — UI, feedback, QoL,
   and standalone battlescape/geoscape tweaks that need none of the new combat systems.
   These improve the game immediately and carry low risk.
2. **Then build the new mechanics in dependency order** — data backbone first, then the
   systems that hang off it, so nothing is built against a system that doesn't exist yet.
3. **Audit the OXCE-Plus base before building anything** — DX sits on OXCE-Plus, which
   already ships much of what the legacy "OpenXcom+" fork ported in by hand.

A handful of UI features are genuinely *coupled* to a new mechanic (e.g. the live tracer
preview and the effective-range action-menu readout both need the aim-cone). Those stay
with their mechanic and are called out where they land.

## ⚠️ Phase 0: Audit existing OXCE-Plus functionality (do this first)

*Goal: Don't reimplement what the base engine already provides. Verify, wire up, adjust.*

A source audit (Jun 2026) confirmed these plan items **already exist in OXCE-Plus** —
treat as *verify/configure*, not *implement*:

- [x] **OXCE+ Integration (Plan §16) — confirmed present.** Verified in-tree (Jun 2026):
  martial training (`AllocateTrainingState`, `RuleBaseFacility::trainingRooms`,
  `customTrainingFactor`), `refNode` inheritance (used across all `Rule*::load`),
  item categories (`RuleItemCategory`, `getUseCustomCategories`), tech-tree viewer
  (`TechTreeViewerState`/`TechTreeSelectState`), sortable soldier lists (`SoldierSortUtil`),
  craft pilots (`CraftPilotsState`), in-inventory armor/avatar (`SoldierArmorState`,
  `SoldierAvatarState`), alien inventories (`AlienInventory`/`AlienInventoryState`).
  → No work needed.
- [x] **Manufacture / base UI (Plan §15) — confirmed present.** Verified: infinite "build
  forever" and auto-sell (`Production::getInfiniteAmount`/`getSellItems`), sell readouts
  (`SellState`), vehicle-category gating + picker filters (`NewManufactureListState`),
  dependencies tree (`ManufactureDependenciesTreeState`). → No work needed.
- [x] **Night-vision system (part of Plan §10) — base confirmed present.**
  `Armor.visibilityAtDark`/`visibilityAtDay` and OXCE night-vision options/toggle present
  (`Options`, `OptionsAdvancedState`, `TileEngine`, `Map`, `BattlescapeState`). DX-specific
  remainder stays scheduled: **fog-of-war** (Phase 1) and **light equipment** (Phase 6,
  needs Effects).
- [x] **Load rulesets from subdirectories (Plan §10) — confirmed present.** `FileMap`
  `mapPlainDir` uses recursive `ls_r` + `isRuleset`, so `.rul` files load from any
  subdirectory of a mod. → No work needed.

Everything below is DX-specific work confirmed **absent** from the base.

---

# Part A — UX & QoL on the existing engine (no new mechanics)

## Phase 1: Battlescape UX & Feedback

*Goal: Immediate tactical-layer feedback improvements that need no new combat systems.*

- [x] **Combat log** — floating event log. Infra + emit points (turn, casualties, fire/throw,
  melee, hit/damage, reaction, panic, out-of-ammo) wired; later mechanics can emit as they come
  online. *(design: [plans/Feature-CombatLog.md](plans/Feature-CombatLog.md))*
- [x] **Action menu revamp** - More compact layout (small font), numeric action hotkeys + per-action configurable hotkey
  labels on the popup. *(the effective-range readout is deferred to Phase 5 — it needs the aim-cone)*, display
  when out of TU or Ammo for an action, show shot count for shotgun-mode and burst/auto-fire modes.
  *(design: [plans/Feature-ActionMenuRevamp.md](plans/Feature-ActionMenuRevamp.md))*
- **On-map overlays** — small always-on visual cues drawn onto the Battlescape map.
  *(design: [plans/Feature-MapOverlays.md](plans/Feature-MapOverlays.md))*
  - [x] **Hovered unit name** — knowledge-aware, faction-colored name label over the unit
    under the cursor.
  - [x] **Primed-grenade indicator** — pulsing icon over player-thrown live grenades on
    discovered tiles (red disc for normal, cyan ring for proximity).
  - [ ] **Bleeding indicators** — wound icon over visible units with fatal wounds.
  - [ ] **Motion-detector readings** — motion blips painted in-world.
- [ ] **Fog-of-war view** — per-tile visible-count rendering (currently-seen vs.
  remembered-but-unobserved vs. undiscovered). *(also a prereq for Clairvoyance later)*
- [ ] **Kneel/stand pathing recalculation** — refresh path preview on kneel/stand toggle.
- [ ] **Tanks/HWPs open doors** — allow vehicles to click-open doors.
- [ ] **Grenade tweaks** — instant-fuse option, reduced grenade LOS accuracy penalty.

## Phase 2: Base / Geoscape / Inventory UX

*Goal: Strategic-layer and inventory UI polish on the existing systems.*

- [ ] **Maximize info screens** — stack-based 320×200 drop for info/detail screens.
- [ ] **Craft stat display** — max speed, acceleration, damage capacity on Craft Info.
- [ ] **Debriefing soldier status** — status column, wounded-recovery days, per-soldier
  gains breakdown.
- [ ] **Inventory UI polish** — mousewheel ground scrolling, tooltip/stat-display mode,
  inventory entry point from the soldier screen.
- [ ] **Loadout templates** — create/apply clipboard, 20-slot named global library with
  number-key quick load/save, 10-slot craft loadouts, clear/auto-equip. *(builds on the
  existing OXCE inventory + `EquipmentLayoutItem`; later reused by Soldier Roles)*
  - ⚠️ **Audit first:** OXCE already provides create/apply-template (clipboard) and craft
    equipment save/load. Verify those, then build only the DX delta — likely the **20-slot
    *named* global library** and **number-key quick load/save** — rather than the whole
    system.
- [ ] **Geoscape enhancements** — local/regional funding weighting, linear council
  increases, sidebar score + always-visible funds.

---

# Part B — New mechanics (dependency order)

## Phase 3: Combat Infrastructure (the foundation)

*Goal: The async core all multi-projectile/explosion combat relies on.*

- [ ] **Async Projectile System** — `Map` projectile collection, per-projectile impact
  tracking, async resolution.
- [ ] **Async Explosion System** — concurrent `ExplosionBState` with private timers.
- [ ] **Advanced Damage Models** — armor degradation, `blastDropoff` falloff,
  visual/sound scaling by blast radius.
- [ ] **Engine plumbing** — `Game::getGame()` accessor (prereq for Effects).

## Phase 4: Items / Armor / Inventory Backbone (the data spine)

*Goal: Ruleset + inventory structures that roles, utility slots, and modular vehicles need.*

- [ ] **Item stats & stat-modifiers** — `stats` / `statModifiers` on items & armor.
- [ ] **Directional armor** — `frontArmor`/`sideArmor`/`rearArmor`/`underArmor`,
  `armorSide`. *(feeds the Phase 3 damage model)*
- [ ] **Inventory layouts** — `RuleInventoryLayout`, `RuleSoldier.inventoryLayout`.
  - ⚠️ **Fix:** `Inventory::unload` (and the Unload button / shift-unload path) hardcodes
    `_inventorySlotRightHand`/`_inventorySlotLeftHand` ([src/Battlescape/Inventory.cpp:1354-1381](src/Battlescape/Inventory.cpp#L1354-L1381))
    and forces the weapon/ammo into hand slots. On layouts that lack those hand slots this
    fails (or would dereference missing slots). Make the unload destination layout-aware
    (e.g. any free slot the item fits, falling back to ground) instead of assuming hands exist.
- [ ] **Typed slots / filtering / move-cost** — `battleType`, `allowCombatSwap`, `costs`,
  `countStats`, `armorSide`.
- [ ] **Utility equipment slots** — `INV_UTILITY`. *(needs typed slots)*

## Phase 5: Firing & Accuracy (the combat loop)

*Goal: The aim-cone shooting model and its shot modes.*

- [ ] **Aim-Cone Trajectory Model** — soldier + weapon deflection cones, stacking error.
- [ ] **Accuracy Modifiers** — kneel/two-handed/exhaustion/smoke; shot-mode accuracy.
- [ ] **Burst Fire Mode** — `BA_BURSTSHOT`. *(needs aim-cone)*
- [ ] **Shotgun & Multi-Projectile** — simultaneous pellets, spread, dual-fire. *(needs
  async projectile + aim-cone)*
- [ ] **Targeting feedback** — live tracer preview, hover accuracy readout, throw-reach
  scaling. *(coupled UI — needs aim-cone)*
- [ ] **Action-menu effective-range readout** — the deferred Phase 1 piece. *(needs
  aim-cone)*

## Phase 6: Ammo & Reloading

- [ ] **Advanced Reloading** — Quick Reload, weight/slot reload costs.
- [ ] **`battleClipSize`** — individual round tracking, magazine packing at battle gen.
- [ ] **Ammo overhaul** — grenades-as-ammo, ammo count on base screens.

## Phase 7: Tactical Unit Systems

- [ ] **Movement Modes** — Sprint / Sneak (paths blue/purple). *(Sneak's
  "no creeping while glowing" gate finalizes in Phase 8 with lighting)*
- [ ] **Overwatch & Reaction split** — `BA_OVERWATCH`, held-fire + indicators;
  `getReactionScore`/`getEvasionScore`.
- [ ] **Health & Medical** — bleedout + indicators, medikit/stabilization rework,
  proportional wound recovery, Field Surgery research.
- [ ] **Soldier Roles** — `RuleRole`/`Role`, role-as-template, UI icons, battlescape
  marker, per-role armor colors. *(reuses the Phase 2 loadout-template plumbing + Phase 4 inventory layouts)*

## Phase 8: Effects, Lighting & Psionics

- [ ] **Effects Framework** — `RuleEffect`/`BattleEffect`/`EffectComponent`,
  item `hitEffect`/`equippedEffect`. *(needs `Game::getGame()`)*
- [ ] **Light / illumination equipment** — `EC_CIRCULAR_LIGHT`/`EC_DIRECTIONAL_LIGHT`;
  finalizes Sneak's light gate. *(needs Effects)*
- [ ] **Psionics Overhaul** — channeled Mind Control + backlash/counter-control; Mind
  Blast *(needs damage model)*; Clairvoyance *(needs Phase 1 fog-of-war)*; psi-amp ammo
  *(needs Phase 6 `battleClipSize`)*.

## Phase 9: AI

- [ ] **Per-weapon AI targeting** — `aiRangeClose/Mid/Long/Max`, `aiAttackPriority*`.
  *(needs firing system)*
- [ ] **AI fixes** — normal TU-reserve logic, reaction-fire fixes, counter-mind-control.

## Phase 10: Strategic Large Systems

- [ ] **Modular Vehicles (HWPs)** — chassis/engine/armor/weapon customization + weapon
  tree. *(needs inventory layouts, directional armor/sided slots, item stats)*
- [ ] **Air-Combat Minigame** — turn-based pursuit, enemy AI, armed UFOs/escorts; gated
  behind `enableNewAirCombat` (off by default). *(largely independent)*

---

## Dependency summary (why mechanics land in this order)

| Built in | Unlocks |
|----------|---------|
| Fog-of-war (P1) | Clairvoyance (P8) |
| Combat log (P1) | event reporting for all later mechanics |
| Loadout templates (P2) | Soldier Roles (P7) |
| Async projectile/explosion (P3) | shotgun, dual-fire, burst, concurrent explosions |
| Damage model (P3) | armor degradation, Mind Blast |
| Inventory layouts + typed slots (P4) | utility slots, roles, modular vehicles |
| Directional armor + item stats (P4) | damage model, sided slots, modular vehicles |
| Aim-cone (P5) | accuracy mods, burst, targeting feedback, AI ranges |
| `battleClipSize` (P6) | psi-amp ammo |
| Effects framework (P8) | light equipment, Sneak light-gating |
| Firing system (P5) | per-weapon AI targeting (P9) |

---

# New Features

*Items added outside the original plan. Each should get a design doc in `plans/` before
implementation (see CLAUDE.md "Planning Features").*

- [ ] **Overrush / TU debt** — let a unit spend past 0 into negative TUs during its turn to
  push an extra action ("over rush"), at the cost of starting the next turn with reduced TUs
  (the debt carried over). *(design: TBD)*
  - Open questions: cap on how far negative; whether energy/morale is also taxed; interaction
    with reaction fire and TU reserves; AI usage; whether the debt is a flat carryover or
    scaled. Touches `BattleUnit` TU accounting (`spendTimeUnits`, `prepareNewTurn`/turn
    recovery) and the action-cost checks that gate actions on available TUs.
