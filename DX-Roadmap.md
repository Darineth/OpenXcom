# OpenXcom DX Roadmap

This is the **single source of truth** for OpenXcom DX feature planning: what's done, what's
left, and the order to build it in. It merges the two former docs
(`DX-Implementation-Plan.md`, the flat by-category checklist, and
`DX-Implementation-Checklist.md`, the phase-ordered plan) into one phase-ordered roadmap with
the granular per-feature sub-tasks folded in.

The sequencing philosophy is **UX-first, then mechanics in dependency order**:

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

Each feature gets a design doc in `plans/` before implementation (see CLAUDE.md "Planning
Features"). New features outside the original plan go in the **New Features** section at the
bottom.

## ⚠️ Phase 0: Audit existing OXCE-Plus functionality (do this first)

*Goal: Don't reimplement what the base engine already provides. Verify, wire up, adjust.*

A source audit (Jun 2026) confirmed these plan items **already exist in OXCE-Plus** —
treat as *verify/configure*, not *implement*:

- [x] **OXCE+ Integration — confirmed present.** Verified in-tree (Jun 2026):
  martial training (`AllocateTrainingState`, `RuleBaseFacility::trainingRooms`,
  `customTrainingFactor`), `refNode` inheritance (used across all `Rule*::load`),
  item categories (`RuleItemCategory`, `getUseCustomCategories`), tech-tree viewer
  (`TechTreeViewerState`/`TechTreeSelectState`), sortable soldier lists (`SoldierSortUtil`),
  craft pilots (`CraftPilotsState`), in-inventory armor/avatar (`SoldierArmorState`,
  `SoldierAvatarState`), alien inventories (`AlienInventory`/`AlienInventoryState`).
  → No work needed.
- [x] **Manufacture / base UI — confirmed present.** Verified: infinite "build
  forever" and auto-sell (`Production::getInfiniteAmount`/`getSellItems`), sell readouts
  (`SellState`), vehicle-category gating + picker filters (`NewManufactureListState`),
  dependencies tree (`ManufactureDependenciesTreeState`). → No work needed.
- [x] **Night-vision system — base confirmed present.**
  `Armor.visibilityAtDark`/`visibilityAtDay` and OXCE night-vision options/toggle present
  (`Options`, `OptionsAdvancedState`, `TileEngine`, `Map`, `BattlescapeState`). DX-specific
  remainder stays scheduled: **fog-of-war** (Phase 1) and **light equipment** (Phase 8;
  the Effects framework it once depended on was dropped — see Phase 8).
- [x] **Load rulesets from subdirectories — confirmed present.** `FileMap`
  `mapPlainDir` uses recursive `ls_r` + `isRuleset`, so `.rul` files load from any
  subdirectory of a mod. → No work needed.

**OXCE+ integration — remaining deltas verified (audit Jun 2026):**

- [x] **UFO mission retreat** — *already in OXCE-Plus.* Damage-driven dogfight escape is present:
  a per-UFO `breakOffTime` escape countdown (`RuleUfo`), plus a damage-threshold abort — a UFO past
  `damageMax/3` (>33%) breaks off unless the player craft is more damaged, maximizes speed to flee,
  and hunter-killers revert to their original destination ([DogfightState.cpp:985-1018](src/Geoscape/DogfightState.cpp#L985-L1018)).
  Crash at >50% (`Ufo::isCrashed`) / destroyed at 100% (`Ufo::isDestroyed`) thresholds also apply
  ([Ufo.cpp:590-613](src/Savegame/Ufo.cpp#L590-L613)); `huntBehavior == 1` kamikaze UFOs ignore
  damage escape. No ground-battle UFO takeoff exists, but that was never planned. → No work needed.
- [x] **Craft equipment templates** — *already in OXCE-Plus, same feature.* 10 named per-craft
  loadout slots (`MAX_CRAFT_LOADOUT_TEMPLATES`), each an `ItemContainer` of item type→qty (incl.
  HWPs), saved to the game (`globalCraftLoadout0..9` + names) via `CraftEquipmentSaveState`/
  `CraftEquipmentLoadState` with `keyInvCreateTemplate`/`keyInvApplyTemplate` and numbered quick-slots.
  Identical to the planned feature; not a separate gap. → No work needed.
- [x] **`RuleDamageType` / `ModScript` hooks** — *complete; nothing further needed.* The DX
  **Editable Damage Types** feature (see New Features) added the global `damageTypes:` node covering
  all `RuleDamageType` fields, applied in a load-order-independent pre-pass. The full OXCE ModScript
  engine is present with damage-path hooks (`damageUnit`, `damageUnitAmmo`, `damageSpecialUnit`,
  `healUnit`) plus the broad unit/item/bonus-stat hook groups. Damage types are static-by-design
  (tuned via the ruleset node, not runtime callbacks). → No work needed.

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
  - [ ] **Reduced grenade accuracy penalty** — N/A on current base (OXCE has no throw penalty);
    revisit with the aim-cone firing model (Phase 5).

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
  loadouts** (10 slots) all present. Clear / auto-equip actions present. No work. *(reused later by Soldier Roles.)*
- [x] **Geoscape interface enhancements** — *already in OXCE-Plus.* `showFundsOnGeoscape` and
  `oxceGeoShowScoreInsteadOfFunds` options cover both use cases (sidebar score + always-visible funds,
  persistent info panel). Score display is intentionally not forced (it's a cheat in OXCE's own
  framing). → No DX work needed.

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
- [x] **Armor Degradation** — sustained side-armor wear from heavy blocked or penetrating hits.
  *(design: [plans/Feature-ArmorDegradation.md](plans/Feature-ArmorDegradation.md))*
  - ✅ **Done.** `RuleDamageType` now supports `ToArmorBlocked` and `ToArmorBlockedThreshold`, adding thresholded blocked-hit armor wear without changing the inherited `ToArmorPre` / `ToArmor` stages.

## Phase 4: Items / Armor / Inventory Backbone (the data spine)

*Goal: Ruleset + inventory structures that roles, utility slots, and modular vehicles need.*

- [x] **Item stats & stat-modifiers** — `stats` / `statModifiers` on items & armor.
  *(design: [plans/Feature-ItemStatsModifiers.md](plans/Feature-ItemStatsModifiers.md))*
  - ✅ **Done.** Added `RuleItem.stats`, `RuleItem.statModifiers`, and `Armor.statModifiers` ruleset support, then wired effective stat recomputation in `Soldier::prepareStatsWithBonuses` and `BattleUnit::getBaseStats` so equipped items and armor modifiers contribute at runtime.
- [x] **Inventory stat display revamp** — display all unit stats (TU, reactions, firing, throwing, melee, psi, strength) from the inventory view.
  - ✅ **Done.** Inventory now shows the expanded stat block in the battlescape inventory screen: TU, weight/strength, reactions, firing, throwing, melee, psi skill, and psi strength. The layout keeps the compact right-side stack by adding two extra stat rows and shifting the whole block only when `showMoreStatsInInventoryView` is enabled.
- [x] **Directional armor on items** — `frontArmor`/`sideArmor`/`rearArmor`/`underArmor`
  on `RuleItem`, folded into the wearer's per-side max armor.
  *(per-side armor on units/armor already exists in OXCE and drives the damage model; this adds the
  item-side contribution — design: [plans/Feature-DirectionalArmorOnItems.md](plans/Feature-DirectionalArmorOnItems.md))*
  - ✅ **Done.** Added `frontArmor`/`sideArmor`/`rearArmor`/`underArmor` to `RuleItem`; equipped
    items now contribute to the wearer's per-side max armor via `BattleUnit::recalculateMaxArmor`
    (cached base + item bonuses, with separately-tracked per-side armor damage so removing/
    re-equipping a plate never refunds lost armor). Fields shown in Stats-for-Nerds.
- [x] **Inventory layouts** — `RuleInventoryLayout`, `Armor.inventoryLayout`.
  *(design: [plans/Feature-ConfigurableInventoryLayouts.md](plans/Feature-ConfigurableInventoryLayouts.md))*
  - ✅ **Done.** Per-armor inventory section sets via the `inventoryLayouts:` node (`id:` + `invs:`-list
    schema + `refNode` reuse) and `Armor.inventoryLayout`, resolved/cached per `BattleUnit`. The base
    data defines `STR_STANDARD_INV` (the nine standard slots) as the default for armors with no layout;
    a section restricted to specific layouts is just a global `invs:` entry those layouts list. The
    inventory UI, placement, quick-move, auto-equip, and alien inventory all honor the active layout;
    a synthesized default preserves vanilla behavior. Stranding-protection on template/equipment-layout
    apply. See `DX-Features.md`.
  - [x] **Configurable weapon slots / unload config** — `Inventory::unload` is now layout-aware:
    candidate hands are restricted to the layout's hand sections, and ammo falls back from off-hand →
    best-fit inventory slot → ground. Shipped as the lean, config-free approach (the policy surface in
    the design doc is deferred).
    *(design: [plans/Feature-ConfigurableWeaponSlotsUnload.md](plans/Feature-ConfigurableWeaponSlotsUnload.md))*
- [x] **Typed slots / filtering / move-cost** — `battleType`, `allowCombatSwap`, `costs`,
  `countStats`.
  - ✅ **Done.** Three `RuleInventory` fields added: slot-side `battleType` filter, `allowCombatSwap`
    (combat-locked loadout slots), `countStats` (stat-bonus gating per slot), plus an explicit `-1`
    `costs` entry that forbids an in-combat transfer. (Legacy's `allowGenericItems` was dropped — see
    the design doc for why.)
  *(design: [plans/Feature-TypedInventorySlots.md](plans/Feature-TypedInventorySlots.md))*
- [x] **Utility equipment slots** — `INV_UTILITY`. *(needs typed slots)*
  *(design: [plans/Feature-UtilityEquipmentSlots.md](plans/Feature-UtilityEquipmentSlots.md))*
  - ✅ **Done (plumbing).** Added `INV_UTILITY`/`INV_EQUIP` as append-only `InventoryType` values
    — single-item slots (one occupant, always-fit, single bounding box) keyed off a new
    `RuleInventory::isSingleItem()` predicate, with per-type box-dim getters. Geometry/fit/draw/
    ownership call sites (placement, `occupiesSlot`, move-cost, grid + item draw) switched to
    `isSingleItem()`; wielding/reload paths stay strict `INV_HAND` (utility items are never wielded —
    handedness is a separate property). Per-unit handle: `RuleInventoryLayout` caches the first
    `INV_UTILITY` section; `BattleUnit::getUtilitySlot()`/`getUtilityItem()` expose it. Typed-slot
    rules (`battleType`/`allowCombatSwap`/`countStats`/`costs`) compose for free. No save-format
    change. `INV_EQUIP` ships as a usable sibling type; its per-unit handle is deferred until a
    consumer defines it.
  - ✅ **Use consumer.** A battlescape hotkey (`keyBattleUseUtility`, default `Z`) opens the utility
    item's action menu in place (medikit/scanner/grenade/etc.) without moving it to a hand; combat-locked
    slots stay usable-but-not-swappable. The action menu is already slot-agnostic, so this is just a
    trigger routing `getUtilityItem()` into `handleItemClick`.
    *(design: [plans/Feature-UtilitySlotUse.md](plans/Feature-UtilitySlotUse.md))*
- [x] **Configurable hand slots** — handedness is now a slot property (`hand: right|left`) instead of
  the hard-coded `STR_RIGHT_HAND`/`STR_LEFT_HAND` ids, so renamed/layout-specific hand slots are real
  hands. Legacy id fallback keeps existing mods unchanged; hands validated/resolved **per layout** (the
  same hand section can be reused across layouts); auto-equip and the inventory-screen hand shortcuts
  use the unit's layout hands (re-cached on unit/armor switch, null-guarded for omitted hands);
  active/preferred hand stored as handedness with save migration. Two hands per layout; N-hands deferred.
  *(design: [plans/Feature-ConfigurableHandSlots.md](plans/Feature-ConfigurableHandSlots.md))*

## Phase 5: Firing & Accuracy (the combat loop)

*Goal: The aim-cone shooting model and its shot modes.*

- [x] **Aim-Cone Trajectory Model** — soldier + weapon deflection cones, stacking error.
  Per-weapon opt-in via `baseAccuracy` (0 = native scatter model, unchanged); shotgun volleys
  share one soldier roll; Monte-Carlo calibrated (`reference/aimcone_montecarlo.py`).
  *(design: [plans/Feature-AimConeTrajectory.md](plans/Feature-AimConeTrajectory.md))*
  - [x] 3D direction-vector cone for direct fire
  - [x] Soldier deflection cone (Gaussian/normal distribution)
  - [x] Weapon deflection cone (independent of soldier)
  - [x] Stacking of soldier and weapon error
- [x] **Accuracy Modifiers** — kneel/two-handed/exhaustion/smoke; shot-mode accuracy.
  *(design: [plans/Feature-AccuracyModifiers.md](plans/Feature-AccuracyModifiers.md))*
  - [x] Kneel / two-handed / shot-mode / wounds — already in OXCE `getFiringAccuracy` (feed the soldier cone).
  - [x] Exhaustion (low-energy) + smoke-on-LOF accuracy factors — DX delta, cone weapons only
    (exhaustion in `getFiringAccuracy`; smoke in the cone path). Smoke constants pending tuning.
  - [x] Shot-mode accuracy application (Snap/Aim/Auto/Burst) — feeds the soldier cone via `getFiringAccuracy`.
- [x] **Realistic throwing accuracy** — replaces the native disc-scatter throw deviation with a
  physical launch-error model (short/long along the throw line + lateral, scaling with distance and
  strength-vs-weight strain). The throwing analog of the aim-cone; reach/curvature unchanged. Behind
  the `battleRealisticThrowing` option (default off). Constants pending in-play tuning.
  *(design: [plans/Feature-ThrowAccuracyRealism.md](plans/Feature-ThrowAccuracyRealism.md))*
- [x] **Shotgun Pellet Flight & Spread** — pellets now fly as individual concurrent projectiles
  with spread, each resolving its own impact via the async projectile system.
- [x] **Dual-Fire** — fire both hands' weapons at once (`BA_DUALFIRE`), each hand its own
  weapon/ammo/best-mode (Auto→Burst→Snap→Aimed), concurrent full sequences at the same target.
  Cost = `min(96, round(max(handTU) × 1.1))`; nested off-hand sub-state; per-projectile impact
  resolution. *(design: [plans/Feature-DualFire.md](plans/Feature-DualFire.md))*
- [x] **Live Trajectory Preview** — tracer sprites for the predicted line-of-fire / throw arc while
  aiming (ideal-path preview, shipped independently of the aim-cone). *(design:
  [plans/Feature-LiveTrajectoryPreview.md](plans/Feature-LiveTrajectoryPreview.md))*
- [x] **Hover Accuracy Readout** — color-graded percentage, cover, and distance. *(coupled UI — needs aim-cone)*
  - [x] Physical hit-chance % on the aiming crosshair for cone weapons (deterministic Monte-Carlo
    that voxel-traces each sample, so it's cover-aware; shotgun-aware, no-LOS widening), color-graded
    red→green; shown without UFOExtender; cached per aim.
  - [x] Separately-broken-out cover-reduction term `(-<cover>%)` (unit targets) and `@ <distance>` suffix,
    giving the legacy `<acc>% (-<cover>%) @ <distance>` breakdown.
- [x] **Throw Reach Scaling** — *already provided by stock OpenXcom.* The strength-vs-weight throw
  model (arc curvature in `TileEngine::validateThrow`; max reach in
  `ProjectileFlyBState::getMaxThrowDistance`) is retained as-is — a stronger thrower / lighter item
  throws farther, and out-of-reach throws are rejected. The existing trajectory-preview arc plus the
  cursor percentage already make it clear when a tile can't be reached, so no separate readout is
  needed.
- [x] **Action-menu effective-range readout** — the deferred Phase 1 piece. Cone weapons show each
  direct-fire mode's 50%-hit effective range (tiles) in place of the accuracy %
  (`Projectile::calculateEffectiveRange`, target-independent median-of-per-sample-range; validated
  against `reference/aimcone_montecarlo.py`). Vanilla weapons keep the accuracy %.

## Phase 6: Ammo & Reloading

- [x] **Quick Reload** — OXCE already provides the core (R key → `reloadAmmo`); DX adds a **visible
  Reload item** in the weapon action menu (hotkey R) surfacing it. Partial-magazine swap deferred. See
  [plans/Feature-QuickReloadMenu.md](plans/Feature-QuickReloadMenu.md).
- [x] **Weight/slot-based Reload Costs** — `battleWeightBasedReloadCost` option; base load/unload
  cost becomes `weight*2+5` on top of `tuLoad`/`tuUnload`. (The slot-path half already shipped as
  OXCE's `extendedItemReloadCost`.) See
  [plans/Feature-WeightBasedReloadCost.md](plans/Feature-WeightBasedReloadCost.md).
- [x] **`battleClipSize`** — individual round tracking, magazine packing at battle gen
  (decouples stocked ammo count from loaded round count). See
  [plans/Feature-BattleClipSize.md](plans/Feature-BattleClipSize.md).
- [x] **Base-Screen Ammo Counts** — ammo rows on Buy/Sell/Transfer/Stores/Craft-Equipment show
  rounds-per-clip (`(xN)`) for multi-round clips. See
  [plans/Feature-BaseScreenAmmoCounts.md](plans/Feature-BaseScreenAmmoCounts.md).
- ~~Grenades-as-Ammo~~ — moved to **Maybe / Someday** (see below); a fun toy in the legacy
  fork but not actually useful, so deprioritized.

## Phase 7: Tactical Unit Systems

- [~] **Sprint Mode** — surfaces & polishes OXCE's hidden **Run** (Ctrl) mode. See
  [plans/Feature-SprintSneakModes.md](plans/Feature-SprintSneakModes.md).
  - [~] High TU/energy cost, high speed, high hit chance — *OXCE Run already applies the TU+energy
    cost multipliers; speed added below; "high hit chance" deferred (belongs with Reaction Split)*
  - [x] Blue path-preview color when sprinting
  - [x] Accelerate unit motion when sprinting (~2× animation)
  - [x] Prevent cancelling movement while sprinting (commits to full path; no spot-stop)
- [~] **Sneak Mode** — surfaces & polishes OXCE's hidden **Sneak** (Alt) mode. *(the
  "no creeping while glowing" gate shipped with Phase 8's Light Equipment —
  `sneakDefaults: { maxLight }`)*
  - [~] Low speed, high alertness, maintains evasion — *low speed (move-cost + ~1.5× slower crawl) and
    **evasion** are now done: sneaking raises the mover's defensive reaction-fire evasion (sprint
    lowers it), mod-configurable globally + per-armor. See
    [plans/Feature-MovementModeEvasion.md](plans/Feature-MovementModeEvasion.md). "High alertness"
    (spotting/detection) remains the only open piece. (The AI-only `sneakyAI` visible-tile
    avoidance is unrelated.)*
  - [x] Purple path-preview color when sneaking
  - [x] Slower unit motion when sneaking (~1.5×)
- [~] **Overwatch System** — `BA_OVERWATCH`, held-fire behavior + indicators. **Implemented**: DX uses
  a **cone** (per-weapon full angle + range + optional min-range) instead of the legacy radius, with
  trigger-tile markers; one-enemy-turn commitment, shots from reserved TU (pre-paid, fired free).
  See [plans/Feature-Overwatch.md](plans/Feature-Overwatch.md).
  - [x] Held-fire state for units (`BattleUnit` overwatch state, save/load, per-turn clear)
  - [x] Per-weapon overwatch tuning (cone angle, range, min-range, shot type, modifier)
  - [x] Cone trigger-tile markers (while aiming + when reselected)
  - [ ] On-map per-unit overwatch indicator *(deferred; cone markers cover the selected unit)*
- [x] **Reaction Scoring Split** — offensive `getReactionScore` / defensive `getEvasionScore`; the
  mover is now measured by evasion (driven by a new armor `evasion:` percent), spotters by reaction.
  Enables independent evasion tuning (sneak, stealth armor). See
  [plans/Feature-ReactionScoringSplit.md](plans/Feature-ReactionScoringSplit.md).
- [x] **Bleedout & Indicators** — negative-health/bleedout state plus battlefield UI cues.
  *(design: [plans/Feature-Bleedout.md](plans/Feature-Bleedout.md))*
  - [x] Negative-health state (eligible units survive below 0 down to `getDeathHealth()`)
  - [x] Bleedout state (buffer fatal torso wounds; mod-configurable via `bleedoutDefaults` + armor `canBleedOut`)
  - [x] Battlefield bleeding indicators (map dying-glyph, HP-bar wound cross-marks, visible-unit column cue)
- [x] **Medikit/Stabilization Rework** — a unit that ever dropped into negative health (bleedout) is
  **out for the mission**: it can be healed to survive but won't revive/rejoin (recovered at debriefing).
  Heal itself is unchanged. Mod knob `bleedoutDefaults.lockoutForMission` (default on).
  *(design: [plans/Feature-MedikitStabilization.md](plans/Feature-MedikitStabilization.md))*
  - [x] **Medikit target-state readout** — target name + derived `STATUS>` (Healthy / Injured /
    Unconscious / Bleeding out / Incapacitated); `HP cur/max`; `Stun stun/curHP`; and the healer's
    `TU cur/max`, on the medikit screen.
- [x] **Proportional Wound Recovery + Field Surgery** — recovery scaling and research gate.
  - ✅ **Done.** Opt-in fraction-of-health-lost recovery (`healthLost × RNG(min–max) / maxHealth`,
    default 20–30 days at full loss) replacing OXCE's absolute-loss formula, plus a configurable
    Field Surgery research gate (default `STR_FIELD_SURGERY_UNIT`) that drops the band to 15–25
    base-wide. All config in the `health:` mod-info node; defaults preserve stock behavior. Engine
    hook only — research content left to the ruleset.
  *(design: [plans/Feature-ProportionalWoundRecovery.md](plans/Feature-ProportionalWoundRecovery.md))*
- [x] **Role Definitions & Templates** — **player-authored** roles (`RuleRole` seeds + savegame `Role`)
  each owning its own loadout template; hybrid seed-then-player-editable model (create/rename/abbreviate/
  re-icon/recolor/delete in-game via the management screen), not a fixed mod list. Assignment via
  clickable badges on Soldier Info + the battlescape Inventory; per-role loadout Save Kit / Apply Kit;
  New Battle persistence. *(reused the Phase 2 loadout-template plumbing as planned —
  design: [plans/Feature-SoldierRoles.md](plans/Feature-SoldierRoles.md))*
- [x] **Role UI & Markers** — role badge + "role> rank" line on Soldier Info and the Inventory,
  rank-cell abbreviations (`MRK-Rookie`) in the base/craft soldier lists, and the battlescape
  selected-unit marker (the role's map glyph replaces the down-arrow).
  *(design: [plans/Feature-SoldierRoles.md](plans/Feature-SoldierRoles.md))*
- [x] **Per-Role Armor Colors** — built on OXCE's vacant utile recolor channel with the legacy DX
  recolor semantics (lighten/darken modes); colour picker in the manager, `soldierArmorBaseColors:`
  mod config (UFO + TFTD sets), measured armor accent blocks, battlescape + inventory-paperdoll
  rendering. *(design: [plans/Feature-SoldierRoles.md](plans/Feature-SoldierRoles.md))*

## Phase 8: Lighting & Psionics

**⚠️ Effects framework dropped (audit + decision, Jul 2026).** The legacy DX "Effects Core"
(`RuleEffect`/`BattleEffect`/`EffectComponent` — a generic triggered/ongoing buff-debuff container)
is **not being ported**. It was a reasonable 2015 design when base OpenXcom had no extension surface,
but modern OXCE covers nearly every use it was invented for, via direct fields + the scripting engine:

- **Stealth/cloaking** → `Armor.camouflageAtDay/AtDark`, `antiCamouflage*`, `psiVision`/`psiCamouflage`,
  plus the `visibilityUnit` script hook for arbitrary per-observer visibility math.
- **Custom light emission** → `Armor.personalLightFriend/Hostile/Neutral` (per-faction, which legacy
  never had).
- **Night vision** → native (`visibilityAtDark`, the NV toggle — see the Phase 0 audit).
- **Timed buffs/debuffs** → `newTurnUnit`/`newTurnItem` + `hitUnit`/`damageUnit` script hooks with
  save-persisted per-unit `ScriptValues` tags; `RuleEnviroEffects` (battle-wide conditions);
  `RuleSoldierBonus` (persistent stat layering).
- Not covered: one-off oddities like the legacy grapple-hook *teleport-on-hit* — if ever wanted, that's
  a small dedicated feature, not a framework justification.

A generic effect system must integrate with stats, FOV/lighting, rendering, saves, UI, and AI all at
once — the costliest kind of engine code — and every DX feature that has landed well (bleedout,
overwatch, roles) was a *targeted* system with a small ruleset surface instead. Phase 8 items are
therefore reframed as targeted deltas:

- ~~**Effects Core Framework**~~ — dropped (see above).
- ~~**Item Effect Hooks** (`hitEffect` / `equippedEffect`)~~ — dropped; specific behaviours become
  their own small features if they earn a slot.
- [x] **Light / illumination equipment** — audit found carried circular light **already native**
  (held `BT_FLARE` items light the carrier; power = radius; prime/unprime = on/off); DX shipped the
  true deltas: **directional/cone light** (`glowConeAngle:` on items — facing-based beam, turn sweeps
  it, ground fallback = half-power circle), carried glow counting DX **utility/equip slots**, and the
  **sneak light gate** (`TileEngine::getUnitLightEmission` + `sneakDefaults: { maxLight }`, personal
  light counts — toggle it off to creep). Closes the Phase 7 Sneak "high alertness/light" leftover's
  lighting half.
  *(design: [plans/Feature-LightEquipment.md](plans/Feature-LightEquipment.md))*
- [x] **Stealth / cloaking armor** — ✅ **Done.** **Audit (Jul 2026): the spotting math is already native and
  richer than legacy's.** OXCE `Armor.camouflageAtDay/AtDark` (+ observer-side `antiCamouflage*`) scale
  down the observer's max view distance in the one `TileEngine::visible()` funnel, so AI, reaction fire,
  overwatch, FOV and the visible-unit buttons all honor it for free — a *static* stealth armor is pure
  ruleset content, zero engine work. The two real DX deltas are (a) the **dynamic cloak** (legacy broke
  stealth on walk/sprint/attack/item-use but preserved it while sneaking — absent from OXCE, and it
  composes with DX's sneak/evasion/light-gate work) and (b) the **translucent render** (genuinely
  absent: no alpha anywhere, `Map::drawUnit` is a binary visible-gate; legacy's `RecolorStealth`
  scanline-cull only worked on the inventory paperdoll and was commented out on the battlescape because
  it punched holes in the terrain). Both shipped: a per-armor `cloak: { dynamic, breaksOn }` node (walk/
  run/attack/useItem break it; sneaking and turning keep it; recovers next turn) and a checkerboard
  ghost render for any unit with active camouflage (new `CurrentPixel` shader arg + a `ghost` flag on
  `ScriptWorkerBlit::executeBlit`, leaving the background pixel alone rather than zeroing it).
  *(design + audit: [plans/Feature-StealthArmor.md](plans/Feature-StealthArmor.md);
  legacy: [Legacy-DX-Features.md](Legacy-DX-Features.md) §15)*
- [x] **Channeled Mind Control** — ✅ **Done.** with backlash/counter-control. Per-turn upkeep state on
  `BattleUnit`, implemented directly (the overwatch pattern). **Audit done (Jul 2026):** the delta is
  real and unreachable from mods — `convertToFaction` and `_mindControllerID` are unbound to script and
  the `tryPsiAttack*` hooks are `const`, so a mod cannot end control, identify a controller, or apply
  backlash. Note the audit's surprise: **stock MC is not permanent** — `BattleUnit::prepareNewTurn`
  already reverts the faction at the victim's next turn, so channeling replaces a one-turn expiry rather
  than an infinite one. **Mod opt-in** (`mindControl:` node on the psi-amp); no node = stock behavior.
  *(design + audit: [plans/Feature-ChanneledMindControl.md](plans/Feature-ChanneledMindControl.md))*
- [x] **Mind Blast** — ✅ **Done.** A `BA_MINDBLAST` attack (opt-in per psi-amp, `mindBlast:` node) that
  reuses the shared psi contest and deals damage scaled by the win margin, with a mod-chosen damage type
  and an optional caster backlash on a miss. Replaces legacy's parallel formula, hard-coded three-tier
  constants and armor-ignoring DT_PSYCHIC. *(design: [plans/Feature-MindBlast.md](plans/Feature-MindBlast.md))*
- [x] **Clairvoyance** — ✅ **Done.** A psychic sweep around a target tile (`BA_CLAIRVOYANCE`), opt-in per
  psi-amp (`clairvoyance:` node). Terrain is marked *discovered but not visible*, so DX's fog of war draws
  it remembered-and-dimmed and it stays known; units in the area are marked like motion-detector contacts
  (through walls, cleared at end of turn) rather than spotted. Psi score gates (`minPsiScore`) and scales
  (`scaleWithPsi`) the radius — replacing legacy's hard-coded 100-point threshold and magic square-root
  curve. *(design: [plans/Feature-Clairvoyance.md](plans/Feature-Clairvoyance.md))*
- [x] **Psi-Amp Ammo Mechanics** — ✅ **Done (ammo).** A `psiAmmo:` node (opt-in per psi-amp) makes each
  psi action — panic, mind control, `BA_USE`, clairvoyance, mind blast — draw a configurable number of
  rounds from the amp's loaded clip; spent on the attempt, gated before TU, shown in the action menu.
  Pairs with `battleClipSize`. The bundled *"percentage-based, armor-reducible psychic damage"* is
  effectively covered by Mind Blast (mod-chosen, armor-aware damage type) + the global `damageTypes:`
  node; a true %-of-max-health mode is deferred as a separate small Mind-Blast option if wanted.
  *(design: [plans/Feature-PsiAmpAmmo.md](plans/Feature-PsiAmpAmmo.md))*

## Phase 9: AI

- [ ] **Per-weapon AI targeting** — engagement range bands (`aiRangeClose`/`Mid`/`Long`/`Max`)
  + per-band target priorities (`aiAttackPriority*`). *(needs firing system)*
- [ ] **AI fixes** — normal TU-reserve logic (drop custom percentages), reaction-fire fixes,
  counter-mind-control behavior.

## Phase 10: Strategic Large Systems

- [ ] **Modular Vehicles (HWPs)** — chassis/engine/armor/weapon customization + weapon
  tree. *(needs inventory layouts, directional armor/sided slots, item stats)*
  - *Future idea — piloted vehicles:* explore letting a vehicle carry a **pilot** (a crewing
    soldier) instead of being a fully autonomous unit. A pilot could tie the HWP's effectiveness to
    the soldier's stats/skills, expose it to crew casualties/bail-out, and let it gain experience —
    versus today's self-contained tank units. (Reuses the craft-pilot plumbing conceptually; the
    battlescape unit model would need a rider/occupant concept. Design TBD.)
- [ ] **Air-Combat Minigame** — turn-based pursuit, positional movement + TU/fuel costs, enemy
  AI (snipe/berserk/escape), armed UFOs/escorts; gated behind `enableNewAirCombat` (off by
  default). *(largely independent — design: [plans/Feature-NewAirCombat.md](plans/Feature-NewAirCombat.md))*

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
| Light equipment (P8, direct TileEngine feature — Effects framework dropped) | Sneak light-gating |
| Firing system (P5) | per-weapon AI targeting (P9) |

---

# New Features

*Items added outside the original plan. Each should get a design doc in `plans/` before
implementation (see CLAUDE.md "Planning Features").*

- [x] **Mod-configurable armor move-cost defaults (`moveCostDefaults`)** — a top-level node setting the
  default walk/run/sneak/etc. move costs armors fall back to when they don't specify `moveCost:`, so a
  mod can retune movement (e.g. slow sneaking) game-wide without editing every armor. Per-armor values
  still override; no node = stock. Spun out of Sprint/Sneak (OXCE sneak defaults to walk-equivalent).
  See [plans/Feature-MoveCostDefaults.md](plans/Feature-MoveCostDefaults.md).

- [~] **Targeting visualization (aim-cone spread & throw landing area)** — spatially draw where the
  aim-cone / throw launch-error spread actually goes. *(design: [plans/Feature-TargetingVisualization.md](plans/Feature-TargetingVisualization.md))*
  - [x] **Sampled impact/landing dots** — hold Alt while aiming to swap the ideal tracer line for a
    dot cloud of where shots/throws would land (reuses the cached hit-chance / landing-chance MCs;
    cone-model fire + realistic throwing). Dots coloured **green = hit, yellow = cover-blocked, red = miss**.
  - [ ] Probability tile-heatmap; grenade blast-radius footprint.

- [ ] **Overrush / TU debt** — let a unit spend past 0 into negative TUs during its turn to
  push an extra action ("over rush"), at the cost of starting the next turn with reduced TUs
  (the debt carried over). *(design: TBD)*
  - Open questions: cap on how far negative; whether energy/morale is also taxed; interaction
    with reaction fire and TU reserves; AI usage; whether the debt is a flat carryover or
    scaled. Touches `BattleUnit` TU accounting (`spendTimeUnits`, `prepareNewTurn`/turn
    recovery) and the action-cost checks that gate actions on available TUs.
  - It might be better to do a very different approach, where there's a "base TU" and a
    "bonus TU" pool?  Rather than negative, just clamp the %TU action costs, and also
    keep TU restoration from going beyond that base set.  Needs thought.

- [x] **Editable base damage type properties** — make the built-in damage types' properties
  moddable instead of hard-coded, so mods can tune the base behavior of each damage type.
  *(design: [plans/Feature-EditableDamageTypes.md](plans/Feature-EditableDamageTypes.md))*
  - Implemented via a new top-level `damageTypes:` ruleset node keyed by `ResistType`, which
    overlays any `RuleDamageType` field (the same set `damageAlter` understands) onto the built-in
    base table. Applied in a pre-pass across all files before `items:` load (order-independent),
    so the precedence chain is built-in default -> global `damageTypes:` edit -> per-item
    `damageAlter`. Slot count stays fixed at 20; `ResistType` is the key (a node cannot remap
    itself). Resolved questions: all loadable `RuleDamageType` fields are editable; `ResistType`
    is re-locked after load; out-of-range indices are soft errors; no save format change. Touches
    [src/Mod/Mod.cpp](src/Mod/Mod.cpp) (`loadAll` global pre-pass + `loadEarlyRules`) and
    [src/Mod/Mod.h](src/Mod/Mod.h).

- [x] **Edit soldier inventories from New Battle** — un-gate the per-soldier inventory screen in New
  Battle so loadouts can be arranged without starting the battle (the mechanism already existed;
  it was disabled via `!_isNewBattle` / `months > -1`).
  *(design: [plans/Feature-NewBattleInventoryEditing.md](plans/Feature-NewBattleInventoryEditing.md))*
  - ✅ **Done.** Un-gated the Inventory button in `CraftEquipmentState` (with a New-Battle bottom-row
    relayout so Unload Craft + Inventory coexist) and in `SoldierInfoState` (split out of the
    months-gated button group). Build/persistence path works as-is under the New-Battle save.

- [x] **Contextual inventory info panel** — when hovering an item, the right-side stat panel shows
  item-type info: weapon shot modes + accuracy, medikit charges, and granted unit stats/armor; reverts
  to unit stats on hover-out.
  *(design: [plans/Feature-InventoryContextualInfoPanel.md](plans/Feature-InventoryContextualInfoPanel.md))*
  - ✅ **Done.** `InventoryState::showItemStats` replaces the panel with the item's stat/armor bonuses
    (STAT>VAL, per-stat soldier bar colors) plus shot modes / medikit charges below, and the weight
    line shows the item's own weight. Reverts to unit stats via `updateStats` on hover-out. Reuses the
    stat rows (follows `showMoreStatsInInventoryView`).

- [x] **Inventory ammo-count badges** — show remaining rounds top-right on weapons/clips in the
  inventory, colored by state, plus on the right-side ammo preview (and for hovered clips).
  *(design: [plans/Feature-InventoryAmmoCount.md](plans/Feature-InventoryAmmoCount.md))*
  - ✅ **Done.** Bordered count drawn by `Inventory::drawItems`/`drawAmmoCount`; full/half/low
    coloring via `ammoStateColor` and configurable `ammoFull`/`ammoMid`/`ammoLow` interface
    elements (block-start indices so the bordered glyph stays in-hue). Single-shot ammo (clip ≤ 1)
    is skipped. Preview badge via the public `Inventory::drawAmmoBadge`; the redundant `_txtAmmo`
    rounds text was removed (its medikit-quantities display will return with the hover stats-panel
    feature).

- [x] **Quick stock buttons on craft equipment (New Battle)** — a **Fill** button beside the existing
  **Unload Craft** (clear) button to instantly stock the craft with a generous spread of items, so
  New-Battle loadout setup is fast.
  *(design: [plans/Feature-CraftEquipmentQuickStock.md](plans/Feature-CraftEquipmentQuickStock.md))*
  - ✅ **Done.** New `CraftEquipmentState::btnFillClick` (New Battle only) adds 40 of each
    ammo/grenade/proximity-grenade/flare and 10 of every other recoverable, non-corpse inventory item,
    then refreshes the list. Existing Unload Craft kept as the clear; bottom row re-laid-out (narrower
    filter combo) to fit Inventory + Unload Craft + Fill + OK. New `STR_DX_CRAFT_FILL` string.

- [x] **Visible craft loadout save/load buttons** — surface the craft equipment template save/load,
  which OXCE-Plus shipped only as hidden hotkeys (`keyCraftLoadoutSave`/`keyCraftLoadoutLoad`, default
  F5/F9), as real on-screen buttons on `CraftEquipmentState`. *(DX philosophy: prefer visible,
  discoverable UI over hidden hotkey-only features. Keep the hotkeys too.)*
  *(design: [plans/Feature-CraftLoadoutButtons.md](plans/Feature-CraftLoadoutButtons.md))*
  - ✅ **Done.** Added compact **Save** / **Load** buttons wired to the existing `btnSaveClick`/
    `btnLoadClick` (→ `CraftEquipmentSaveState`/`CraftEquipmentLoadState`, 10 named slots), shown in both
    the geoscape and New-Battle screens at fixed right-aligned slots; the row was rebalanced (narrower
    combo/Inventory, "Unload" button narrowed but text unchanged). Also **un-gated Save in New Battle**
    (`btnSaveClick` no longer `!_isNewBattle`) since New Battle has a live in-memory `SavedGame`, so
    save/load work within the session. F5/F9 hotkeys retained. New `STR_DX_CRAFT_LOADOUT_SAVE`/`_LOAD`
    strings. Also made **New Battle persist its loadout templates** (craft loadouts + soldier equipment
    layouts) in its `.cfg` — factored `SavedGame::saveTemplates`, wired into `NewBattleState` save/load —
    so templates created in New Battle survive a restart (previously discarded).

- [x] **Ufopaedia fallback stats page for unconfigured items** — auto-generate a viewable stats page
  for items/weapons that have no authored `ufopaedia` article, so the player can still inspect their
  stats (accuracy, damage, TU costs, weight, etc.) instead of the entry being unopenable.
  - ✅ **Done.** Scope extended well past items: **armor, craft, craft weapons, base facilities,
    soldier types, alien unit types and UFOs** all get stand-in articles. Synthesized in
    `Mod::generateMissingUfopaediaArticles()` just before `sortLists()`, which means all ~45
    existing middle-click call sites work with **no changes to any of them** — the lookup simply
    succeeds now. Research gating comes free (the generated article carries the rule's own
    requirements; craft weapons inherit their launcher item's). Article style (UFO vs TFTD) is
    chosen from whichever the mod predominantly authored. Two ruleset keys:
    **`generateMissingPediaArticles`** (default **on**) and **`listGeneratedPediaArticles`**
    (default **off** — middle-click works everywhere, but no mod's authored pedia index changes).
    Verified against stock xcom1 + dx-test: 37 articles generated, both flags confirmed parsing.
  - **Research gating did NOT "come free" as planned, and closing that took two passes.** `Unit`
    and `RuleUfo` carry no requirements field at all, so their articles generated with empty
    requirements - and `isResearched({})` is true, exposing every alien's stats from day one (33 of
    them on stock xcom1). Units and UFOs were first cut from scope; that was an overcorrection.
    The gate turns out to live on the *article*, following a derivable convention (article
    `STR_SMALL_SCOUT` requires research `STR_SMALL_SCOUT`), so DX now looks up a research topic
    named after the rule and inherits exactly the gate a modder would have written. Units and UFOs
    are back, correctly gated; for that enemy-side content a derivable gate is **mandatory**, so
    ungatable rules (`MALE_CIVILIAN`, `STR_ZOMBIE`) generate nothing. Player-facing types may still
    be ungated, which for them just means "available from the start".
  - Also filtered on value grounds: unrecoverable fixed weapons (a unit's innate attack, never held
    by the player; recoverable HWP weapons are kept) and `BT_CORPSE` items (vanilla covers those
    with autopsy articles).
  - Also fixed: the prev/next buttons walk `articleList` directly, so filtering only the index let
    navigation wander into unlisted generated articles. Unlisted ones are now marked in
    `articleStatusList` (the engine's existing skip mechanism), which kept middle-click working.
  - Discovered along the way, both hardened as upstream fixes in `DX-OXCE-Fixes.md`:
    `ArticleStateCraft`/`CraftWeapon`/`Unit` **null-deref crashed** on an empty `image_id`
    (`Mod::getRule` returns 0 for empty names rather than throwing), and
    `ArticleCommonState::nextArticle`/`prevArticle` **recursed once per skipped article** with no
    base case when all were hidden — now bounded loops.
  *(design: [plans/Feature-UfopaediaFallbackArticles.md](plans/Feature-UfopaediaFallbackArticles.md))*

- [x] **Psi success chance on hover** — show the hit probability for a psi action while targeting. Psi
  was the only attack with no accuracy feedback at all: the action menu shows TU only, and the sole
  existing readout was an Alt-held *margin range* on the cursor tile that deliberately omits the target's
  `psiDefence`.
  - ✅ **Done.** Cursor readout (`72% @ 8m`), color-graded like the other targeting readouts, computed
    exactly from the default roll: `P = clamp(margin + 55, 0, 56) / 56`. Research-gated (option **C**) —
    exact against own units, civilians and researched hostiles; against an unresearched hostile it assumes
    the baseline defence and prefixes `~`, so a hover can't be used to read off an alien's psi stats.
    Counter-control aware (shows the odds vs the *controller*). Excludes `BA_CLAIRVOYANCE`, which shares
    the psi cursor but is not a contest, and never draws over a unit the player can't see. Supersedes the
    Alt-held min-max indicator. Option **Psi success chance on cursor**
    (`psiChanceIndicatorEnabled`, default on). Known limit: a mod replacing the `tryPsiAttackItem` script
    makes the figure inexact and the engine can't detect that.
    *(design: [plans/Feature-PsiHoverChance.md](plans/Feature-PsiHoverChance.md))*

- [x] **Fix: mind blast ignores the amp's psi accuracy** — `BattleUnit::getPsiAccuracy` had cases for
  `BA_MINDCONTROL` / `BA_PANIC` / `BA_USE` but none for `BA_MINDBLAST`, so a blast got only the
  `accuracyMultiplier` and the flat per-action accuracy term was silently 0 — no ruleset key influenced it.
  A DX bug (DX added the action without extending the function), not an upstream one.
  - ✅ **Done.** New **`accuracy`** key on the `mindBlast:` node, read by a `BA_MINDBLAST` branch in
    `getPsiAccuracy`. Defaults to **0** (matching `accuracyMindControl`/`accuracyUse`), so the default
    reproduces the old behavior rather than silently retuning existing mods — a mod enabling `mindBlast:`
    should now set `accuracy:`. Documented in `docs/Ruleset-Items.md` **[DX]**; `dx-test.rul` sets 20.
    *(design: [plans/Feature-PsiHoverChance.md](plans/Feature-PsiHoverChance.md) — "Known gap")*

- [x] **Battlescape wound indicator on the HP bar** — white tick marks (one per fatal wound) on the
  selected unit's health bar, so wounded/bleeding units are visible at a glance. Delivered as part of
  **Bleedout & Indicators** (Phase 7) via `Bar::setMarks`. *(design: [plans/Feature-Bleedout.md](plans/Feature-Bleedout.md))*

- [x] **Dying-unit camera focus** — the tactical camera never framed deaths, so a kill on an
  off-screen unit was invisible to the player, and an in-flight projectile outranked it. A dying
  unit that is visible but off screen now pulls the camera, and claims it from projectile
  following for the duration of the death animation. Units already on screen are left alone (no
  camera jerk in normal fights). Toggle: **Focus camera on dying units**
  (`battleFocusDyingUnits`, default on).
  *(design: [plans/dying-unit-camera-focus.md](plans/dying-unit-camera-focus.md))*

# Maybe / Someday

*Deprioritized ideas — interesting but not clearly worth building yet. Revisit if a concrete use
case appears; each still needs a design doc before implementation.*

- **Grenades-as-Ammo** — allow grenades to be loaded as weapon ammo (e.g. a grenade launcher firing
  from a magazine of grenade items). Existed as preliminary support in the legacy DX fork
  (Legacy-DX-Features.md §4) but proved to be a novelty toy without a compelling gameplay payoff, so
  it was pulled out of Phase 6. *(design: TBD)*
