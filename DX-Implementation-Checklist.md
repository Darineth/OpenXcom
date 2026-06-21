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
  - [x] **Unit status indicators** — bleeding / fire / shock / near-knockout glyphs hovering over
    the player's own living units (reuses the engine's `Floor*Indicator` surfaces, with procedural
    fallbacks so they work without mod art).
  - [x] **Motion-detector readings** — authentic `DETBLOB` scanner blips painted on the tiles of
    units scanned this turn (intensity by motion points); passive, tile-level, replaces OXCE's
    Alt-held arrow. Detection gating unchanged (still requires using a scanner).
- [x] **Fog-of-war view** — per-tile visible-count rendering (currently-seen vs.
  remembered-but-unobserved vs. undiscovered). *(also a prereq for Clairvoyance later)*
  *(design: [plans/Feature-FogOfWar.md](plans/Feature-FogOfWar.md))* — renderer dims discovered tiles
  with `getVisible() == 0`; also fixed a double-count bug in `calculateTilesInFOV` so tiles re-fog on
  move/turn.
- [x] **Kneel/stand pathing recalculation** — *already in OXCE-Plus.* `btnKneelClick` calls
  `Pathfinding::refreshPath()` on toggle ([BattlescapeState.cpp:1257](src/Battlescape/BattlescapeState.cpp#L1257)). No work needed.
- [x] **Tanks/HWPs open doors** — *already in OXCE-Plus.* `TileEngine::unitOpensDoor` loops over all
  `armor->getSize()` tiles with no size gate ([TileEngine.cpp:4074](src/Battlescape/TileEngine.cpp#L4074)), so big units open doors. No work needed.
- **Grenade tweaks**
  - [x] **Instant-fuse option** — *already in OXCE-Plus.* `Options::battleInstantGrenade`
    ([BattleItem.cpp:344](src/Savegame/BattleItem.cpp#L344)) makes thrown grenades detonate without the prime dialog; per-item
    `fuseType: -2` (`BFT_INSTANT`) does the same per grenade. No work needed.

## Phase 2: Base / Geoscape / Inventory UX

*Goal: Strategic-layer and inventory UI polish on the existing systems.*

*(Phase 2 audit, Jun 2026 — much of this is already in OXCE-Plus; only the deltas below are DX work.)*

- [x] **Maximize info screens** — stack-based 320×200 drop for info/detail screens.
  - ✅ **Done (central rewrite).** Replaced the per-screen save/restore hack with a central
    `State::ScaleContext` + `Game::applyDisplayScale()` rule applied on every top-of-stack change
    ([State.h](src/Engine/State.h), [Game.cpp](src/Engine/Game.cpp)). With `maximizeInfoScreens`
    on, **every** UI/detail/dialog screen now drops to 320×200; only the primary gameplay views
    (`GeoscapeState`, `BattlescapeState`, `DogfightState`) and self-managed display states
    (`StartState`, `CutsceneState`/`SlideshowState`/`VideoState`, `TestState`) are exempt. The 6
    Battlescape popups + inventory keep the battlescape scale when the option is off (no thrash).
    See [plans/Feature-MaximizeAllScreens.md](plans/Feature-MaximizeAllScreens.md).
- [x] **Craft stat display** — max speed, acceleration, damage capacity on Craft Info.
  - ✅ **Done.** Crafted a compact 2-column layout in CraftInfoState: damage capacity, max speed, and acceleration on the left; damage, shield, and fuel on the right. Values are populated in init() from RuleCraft getters.
- [x] **Debriefing soldier status** — status column, wounded-recovery days, per-soldier
  gains breakdown. *(design: [plans/Feature-DebriefingSoldierStatus.md](plans/Feature-DebriefingSoldierStatus.md))*
- [x] **Inventory UI polish** — mousewheel ground scrolling, one-column-at-a-time step, partial item visibility at edges.
  - ✅ **Done.** Mousewheel ground scrolling anywhere over the ground inventory area (`SDL_BUTTON_WHEELUP`/`SDL_BUTTON_WHEELDOWN`), scrolled by one column per notch (not page-by-page). Ground starts fully scrolled left on unit select. Multi-slot items partially visible at the left edge render correctly. Button/keyboard ground scroll and hover stat tooltips (`showMoreStatsInInventoryView`) already existed in OXCE-Plus.
- [x] **Soldier Info Equipment button** — Inventory entry point from `SoldierInfoState`, full screen layout rearrangement to match Legacy DX reference.
  - ✅ **Done (without Level/EXP):** SoldierInfoState now uses a two-row button layout matching the Legacy DX direction: top row `<< / OK / >> / DIARY / ARMOR / SACK`, second row with craft-as-button and `INVENTORY`. Craft is now a clickable **assign/unassign toggle** for the base's first craft slot (and shows standard craft-capacity/group errors). `INVENTORY` opens base inventory setup with the current soldier preselected. **Level/EXP remains deferred** to a future feature.
- [x] **Loadout templates** — *already in OXCE-Plus.* Clipboard create/apply
  (`keyInvCreateTemplate`/`keyInvApplyTemplate`), a **named global equipment library** (50 slots,
  `InventoryLoadState`/`InventorySaveState`, number-key quick load / Ctrl+number save), and **craft
  loadouts** (10 slots) all present. No work. *(reused later by Soldier Roles.)*
- [x] **Geoscape interface enhancements** — *already in OXCE-Plus.* `showFundsOnGeoscape` and
  `oxceGeoShowScoreInsteadOfFunds` options cover both use cases. Score display is intentionally
  not forced (it's a cheat in OXCE's own framing). → No DX work needed.

---

# Part B — New mechanics (dependency order)

## Phase 3: Combat Infrastructure (the foundation)

*Goal: The async core all multi-projectile/explosion combat relies on.*
*(index: [plans/Phase-3-CombatInfrastructure.md](plans/Phase-3-CombatInfrastructure.md))*

- [x] **Async Projectile System** — `Map` projectile collection, per-projectile impact
  tracking, centroid camera follow, timer-based firing (`fireInterval`), async resolution.
  *(design: [plans/Feature-AsyncProjectileSystem.md](plans/Feature-AsyncProjectileSystem.md))*
- [x] **Async Explosion System** — impact explosions animate concurrently (non-blocking)
  alongside the still-flying volley; per-state sprite ownership + wall-clock pacing.
  *(design: [plans/Feature-AsyncProjectileSystem.md](plans/Feature-AsyncProjectileSystem.md))*
- [x] **Burst Fire Mode** — `BA_BURSTSHOT`, a fourth firing mode (own accuracy/TU/shots/range,
  opt-in via `tuBurst`), sequential rounds reusing the async auto-shot path, now also wired into
  Battlescape AI fire-mode selection.
  *(design: [plans/Feature-BurstFire.md](plans/Feature-BurstFire.md))*
- [x] **Blast Radius Dropoff** — `RuleItem.blastDropoff` falloff inside AoE radius.
  - ✅ **Done.** Center-weighted explosion power scaling is implemented; `0.0` preserves flat
    vanilla behavior.
- [x] **Explosion VFX/Sound Radius Scaling** — presentation controls by blast radius.
  - ✅ **Done.** Explosion presentation now scales from blast radius (not damage power):
    sprite density/spread are radius-driven, and the small/large explosion sound threshold is
    keyed off radius as well.
- [ ] **Armor Degradation** — sustained side-armor wear from heavy blocked or penetrating hits.
  *(design: [plans/Feature-ArmorDegradation.md](plans/Feature-ArmorDegradation.md))*
  - Deferred for a follow-up design.
  - Proposed rule change note: let strong blocked hits dent armor once they reach at least 50% of the side's effective armor block, while preserving the fork's existing `ToArmorPre` / `ToArmor` semantics.

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
- [ ] **Throw-accuracy tuning (if needed)** — OXCE currently applies no throw LOS penalty
  (`accuracyThrow` default 100; `_noLOSAccuracyPenalty` is aimed-shot-only). Only add a
  reduced throw penalty if the aim-cone firing model introduces one that needs softening.
- [x] **Shotgun Pellet Flight & Spread** — pellets now fly as individual concurrent projectiles
  with spread, each resolving its own impact via the async projectile system.
- [ ] **Dual-Fire** — simultaneous projectile spawning from both hands. *(needs
  async projectile + aim-cone; shotgun pellet flight/spread already implemented)*
- [ ] **Live Trajectory Preview** — tracer sprites. *(coupled UI — needs aim-cone)*
- [ ] **Hover Accuracy Readout** — color-graded percentage + distance. *(coupled UI — needs aim-cone)*
- [ ] **Throw Reach Scaling** — strength vs weight readout/scaling. *(coupled UI — needs aim-cone)*
- [ ] **Action-menu effective-range readout** — the deferred Phase 1 piece. *(needs
  aim-cone)*

## Phase 6: Ammo & Reloading

- [ ] **Quick Reload**.
- [ ] **Weight/slot-based Reload Costs**.
- [ ] **`battleClipSize`** — individual round tracking, magazine packing at battle gen.
- [ ] **Grenades-as-Ammo**.
- [ ] **Base-Screen Ammo Counts**.

## Phase 7: Tactical Unit Systems

- [ ] **Sprint Mode** — fast movement mode with blue pathing.
- [ ] **Sneak Mode** — low-profile movement mode with purple pathing. *(final
  "no creeping while glowing" gate lands in Phase 8 with lighting)*
- [ ] **Overwatch System** — `BA_OVERWATCH`, held-fire behavior + indicators.
- [ ] **Reaction Scoring Split** — `getReactionScore` / `getEvasionScore`.
- [ ] **Bleedout & Indicators** — negative-health/bleedout state plus battlefield UI cues.
- [ ] **Medikit/Stabilization Rework** — revised field treatment flow.
- [ ] **Proportional Wound Recovery + Field Surgery** — recovery scaling and research gate.
- [ ] **Role Definitions & Templates** — `RuleRole`/`Role` + template loadouts.
- [ ] **Role UI & Markers** — soldier/craft UI icons and battlescape marker.
- [ ] **Per-Role Armor Colors**. *(reuses Phase 2 loadout-template plumbing + Phase 4 inventory layouts)*

## Phase 8: Effects, Lighting & Psionics

- [ ] **Effects Core Framework** — `RuleEffect` / `BattleEffect` / `EffectComponent`.
- [ ] **Item Effect Hooks** — `hitEffect` / `equippedEffect`.
- [ ] **Light / illumination equipment** — `EC_CIRCULAR_LIGHT`/`EC_DIRECTIONAL_LIGHT`;
  finalizes Sneak's light gate. *(needs Effects)*
- [ ] **Stealth / cloaking armor** — `Armor.equippedEffects` → `EC_STEALTH` magnitude scales
  down enemy spot range (≥100 = effectively invisible); translucent `RecolorStealth` render
  (inventory paperdoll wired, `UnitSprite::drawRecolored` still TODO). *(needs Effects; the
  inverse of light equipment)* *(legacy: [Legacy-DX-Features.md](Legacy-DX-Features.md) §15)*
- [ ] **Channeled Mind Control** — with backlash/counter-control.
- [ ] **Mind Blast** *(needs damage-model pieces).* 
- [ ] **Clairvoyance** *(needs Phase 1 fog-of-war).* 
- [ ] **Psi-Amp Ammo Mechanics** *(needs Phase 6 `battleClipSize`).*

## Phase 9: AI

- [ ] **Per-weapon AI targeting** — `aiRangeClose/Mid/Long/Max`, `aiAttackPriority*`.
  *(needs firing system)*
- [ ] **AI fixes** — normal TU-reserve logic, reaction-fire fixes, counter-mind-control.

## Phase 10: Strategic Large Systems

- [ ] **Modular Vehicles (HWPs)** — chassis/engine/armor/weapon customization + weapon
  tree. *(needs inventory layouts, directional armor/sided slots, item stats)*
- [ ] **Air-Combat Minigame** — turn-based pursuit, enemy AI, armed UFOs/escorts; gated
  behind `enableNewAirCombat` (off by default). *(largely independent)*

## Phase 11: Strategic Balance & Economy (new mechanic)

*Goal: Introduce new strategic-layer mechanics that change how the game's economy and
council scoring work. This is not a UX polish — it adds a new resource-management dimension
and changes core progression curves.*

- [ ] **Funding weighting** — replace flat monthly income with local/regional performance
  scoring: countries weight contributions based on nearby craft coverage, alien threats
  neutralized, and base reputation. *(new mechanic — changes `Country::newMonth` from a
  flat lookup to a performance-weighted formula; balance-sensitive)*
- [ ] **Linear council increases** — replace the exponential council score growth with
  linear (or configurable) per-month increases. *(new mechanic — `Country::newMonth` is
  exponential today; changing the curve affects mission pacing and difficulty scaling)*
- [ ] **Economy tuning hooks** — ruleset-exposed parameters for funding weights, council
  curves, and regional performance factors so mods can tune without code changes.

---

## Dependency summary (why mechanics land in this order)

| Built in | Unlocks |
|----------|---------|
| Fog-of-war (P1) | Clairvoyance (P8) |
| Combat log (P1) | event reporting for all later mechanics |
| Loadout templates (P2) | Soldier Roles (P7) |
| Async projectile/explosion (P3) | shotgun, dual-fire, burst, concurrent explosions |
| Blast dropoff + armor degradation (P3) | Mind Blast balancing hooks |
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
