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
  remainder stays scheduled: **fog-of-war** (Phase 1) and **light equipment** (Phase 8,
  needs Effects).
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
- [ ] **Dual-Fire** — simultaneous projectile spawning from both hands. *(needs
  async projectile + aim-cone; shotgun pellet flight/spread already implemented)*
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

- [ ] **Quick Reload**.
- [ ] **Weight/slot-based Reload Costs**.
- [ ] **`battleClipSize`** — individual round tracking, magazine packing at battle gen
  (decouples stocked ammo count from loaded round count).
- [ ] **Grenades-as-Ammo**.
- [ ] **Base-Screen Ammo Counts**.

## Phase 7: Tactical Unit Systems

- [ ] **Sprint Mode** — fast movement mode with blue pathing.
  - [ ] High TU/energy cost, high speed, high hit chance
  - [ ] Blue path-preview color when sprinting
  - [ ] Accelerate unit motion when sprinting
  - [ ] Prevent cancelling movement while sprinting
- [ ] **Sneak Mode** — low-profile movement mode with purple pathing. *(final
  "no creeping while glowing" gate lands in Phase 8 with lighting)*
  - [ ] Low speed, high alertness, maintains evasion
  - [ ] Purple path-preview color when sneaking
- [ ] **Overwatch System** — `BA_OVERWATCH`, held-fire behavior + indicators.
  - [ ] Held-fire state for units
  - [ ] Per-weapon overwatch tuning (radius, range, shot type)
  - [ ] On-map overwatch indicators
- [ ] **Reaction Scoring Split** — offensive `getReactionScore` / defensive `getEvasionScore`.
- [ ] **Bleedout & Indicators** — negative-health/bleedout state plus battlefield UI cues.
  - [ ] Negative-health state
  - [ ] Bleedout state (with fatal torso wounds)
  - [ ] Battlefield bleeding indicators
- [ ] **Medikit/Stabilization Rework** — revised field treatment flow.
- [ ] **Proportional Wound Recovery + Field Surgery** — recovery scaling and research gate.
- [ ] **Role Definitions & Templates** — `RuleRole`/`Role` + template loadouts. *(reuses Phase 2
  loadout-template plumbing)*
- [ ] **Role UI & Markers** — soldier/craft/inventory UI icons and battlescape marker.
- [ ] **Per-Role Armor Colors**. *(reuses Phase 2 loadout-template plumbing + Phase 4 inventory layouts)*

## Phase 8: Effects, Lighting & Psionics

- [ ] **Effects Core Framework** — `RuleEffect` / `BattleEffect` / `EffectComponent`
  (initial/ongoing/final, duration, maxStack).
- [ ] **Item Effect Hooks** — `hitEffect` / `equippedEffect`.
- [ ] **Light / illumination equipment** — `EC_CIRCULAR_LIGHT`/`EC_DIRECTIONAL_LIGHT`;
  finalizes Sneak's light gate. *(needs Effects)*
- [ ] **Stealth / cloaking armor** — `Armor.equippedEffects` → `EC_STEALTH` magnitude scales
  down enemy spot range (≥100 = effectively invisible); translucent `RecolorStealth` render
  (inventory paperdoll wired, `UnitSprite::drawRecolored` still TODO). *(needs Effects; the
  inverse of light equipment)* *(legacy: [Legacy-DX-Features.md](Legacy-DX-Features.md) §15)*
- [ ] **Channeled Mind Control** — with backlash/counter-control.
- [ ] **Mind Blast** — direct psychic damage *(needs damage-model pieces).*
- [ ] **Clairvoyance** — area reveal power *(needs Phase 1 fog-of-war).*
- [ ] **Psi-Amp Ammo Mechanics** — per-use round cost; percentage-based, armor-reducible
  psychic damage *(needs Phase 6 `battleClipSize`).*

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
