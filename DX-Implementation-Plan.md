# OpenXcom DX Reimplementation Plan

This document serves as a high-level checklist of features from the original OpenXcom+ project to be re-implemented on top of the OXCE+ engine.

## 1. Firing & Trajectory System
- [ ] **Aim-Cone Trajectory Model**
    - [ ] Implement 3D direction-vector cone for direct fire
    - [ ] Implement Soldier deflection cone (Gaussian/Normal distribution)
    - [ ] Implement Weapon deflection cone (independent of soldier)
    - [ ] Implement stacking of soldier and weapon error
- [x] **Burst Fire Mode**
    - [x] Implement `BA_BURSTSHOT` action type
    - [x] Implement configurable burst settings (shots, range, accuracy, cost)
    - [x] Implement sequential firing logic for burst volleys
    - [x] Add AI support for burst fire selection/use
- [x] **Shotgun Pellet Flight & Spread**
    - [x] Implement simultaneous pellet flight (multiple projectiles in flight)
    - [x] Implement shotgun pellet spread (normal distribution)
- [ ] **Dual-Fire**
    - [ ] Implement simultaneous projectile spawning
- [ ] **Live Trajectory Preview**
    - [ ] Implement live trajectory preview (tracer sprites)
- [ ] **Hover Accuracy Readout**
    - [ ] Implement on-hover accuracy readout (color-graded percentage + distance)
- [ ] **Throw Reach Scaling**
    - [ ] Implement throw reach scaling (strength vs weight)
- [ ] **Accuracy Modifiers**
    - [ ] Implement kneel/two-handed/exhaustion/smoke accuracy factors
    - [ ] Implement shot-mode accuracy application (Snap/Aim/Auto/Burst)

## 2. Projectile & Explosion Management
- [x] **Asynchronous Projectile System**
    - [x] Implement `Map` collection for multiple in-flight projectiles
    - [x] Implement per-projectile impact/outcome tracking
    - [x] Implement asynchronous resolution of projectile impacts
- [x] **Asynchronous Explosion System**
    - [x] Implement concurrent explosion states (`ExplosionBState`)

## 3. Overwatch & Reaction Fire
- [ ] **Overwatch System**
    - [ ] Implement "Held-fire" state for units
    - [ ] Implement per-weapon overwatch tuning (radius, range, shot type)
    - [ ] Implement on-map overwatch indicators
- [ ] **Reaction Fire Improvements**
    - [ ] Implement offensive vs. defensive reaction/evasion scoring split

## 4. Movement Modes
- [ ] **Sprint Mode**
    - [ ] Implement Sprint (high TU/energy cost, high speed, high hit chance)
    - [ ] Implement Sprint visual indicator (blue path)
- [ ] **Sneak Mode**
    - [ ] Implement Sneak (low speed, high alertness, maintains evasion)
    - [ ] Implement Sneak visual indicator (purple path)
- [ ] **Movement feel/polish**
    - [ ] Change the color of the path preview when sprinting or sneaking
    - [ ] Accelerate unit motion when sprinting
    - [ ] Prevent cancelling movement while sprinting

## 5. Reloading & Ammo Mechanics
- [ ] **Quick Reload**
    - [ ] Implement Quick Reload action
- [ ] **Weight/slot-based Reload Costs**
    - [ ] Implement weight/slot-based reload costs
- [ ] **`battleClipSize` (decoupling stock vs load)**
    - [ ] Implement `battleClipSize` (decoupling stock vs load)
- [ ] **Ammo Item Tracking Overhaul**
    - [ ] Implement individual round tracking for ammo items
- [ ] **Grenades-as-Ammo**
    - [ ] Implement grenades as ammo items
- [ ] **Base-Screen Ammo Counts**
    - [ ] Implement ammo count display on base screens

## 6. Soldier Roles System
- [ ] **Role Definitions & Templates**
    - [ ] Implement Role definitions (Sniper, Medic, etc.)
    - [ ] Implement Role equipment templates
- [ ] **Role UI & Markers**
    - [ ] Implement Role UI icons (Soldier, Craft, Inventory, Battlescape marker)
- [ ] **Per-Role Armor Colors**
    - [ ] Implement per-Role armor colors

## 7. Psionics Overhaul
- [ ] **Channeled Mind Control**
    - [ ] Implement Channeled Mind Control (with backlash/counter-control)
- [ ] **Clairvoyance**
    - [ ] Implement Clairvoyance (area reveal power)
- [ ] **Mind Blast**
    - [ ] Implement Mind Blast (direct psychic damage)
- [ ] **Psi-Amp Mechanics**
    - [ ] Implement Psi-amp ammo consumption (per-use round cost)
    - [ ] Implement psychic damage properties (percentage-based, armor-reducible)

## 8. Health & Medical System
- [ ] **Wound & Bleedout System**
    - [ ] Implement negative health state
    - [ ] Implement Bleedout state (with fatal torso wounds)
    - [ ] Implement battlefield bleeding indicators
- [ ] **Medical Rework**
    - [ ] Implement reworked medikit/stabilization/wound recovery logic

## 9. Damage & Armor Rules
- [x] **Blast Radius Dropoff**
    - [x] Implement explosion damage falloff inside AoE radius
- [ ] **Explosion VFX/Sound Radius Scaling**
    - [ ] Implement visual/sound scaling by blast radius
- [x] **Armor Degradation**
    - [x] Implement thresholded blocked-hit and penetrating-hit armor degradation via `RuleDamageType`
- [x] **Directional Armor Values**
    - [x] Implement directional armor values
- [ ] **Item-based Stat Modification Rules**
    - [ ] Implement item-based stat modification rules

## 10. Inventory & UI Improvements
- [ ] **Inventory System**
    - [ ] Implement per-unit-type inventory layouts
    - [ ] Implement configurable weapon slots and unload destination policies
    - [ ] Implement slot filtering and move-cost rules
    - [ ] Implement inventory entry point from soldier screen
    - [ ] Implement mousewheel ground scrolling (one column at a time)
    - [ ] Implement inventory tooltip / stat-display mode
- [ ] **Loadout Templates**
    - [ ] Implement Create/Apply Template clipboard (full layout: slot, pos, ammo, fuse, armor color)
    - [ ] Implement named global layout library (20 slots) with number-key quick load/save
    - [ ] Implement craft loadout templates (10 slots)
    - [ ] Implement Clear / Auto-equip actions
- [ ] **Combat Feedback**
    - [ ] Implement floating combat log
    - [ ] Implement on-map overlays
        - [x] Implement hovered unit name overlay
        - [x] Implement primed-grenade indicator overlay
        - [x] Implement unit status indicators overlay (bleeding / fire / shock / near-knockout)
        - [x] Implement motion-detector readings overlay (DETBLOB tile blips, passive, replaces Alt-arrow)
    - [x] Implement fog-of-war view
- [ ] **Night Vision & Lighting**
    - [x] Implement night-vision modes (full/local), auto/toggle/hold keys, night-vision color
    - [x] Implement per-armor sight ranges (`visibilityAtDark`/`visibilityAtDay`) + camouflage values
    - [ ] Implement light/illumination equipment (carried light sources, layered lighting)
    - [ ] Implement stealth/cloaking armor (`Armor.equippedEffects` → `EC_STEALTH` scales enemy spot range, ≥100 = invisible; `RecolorStealth` render) — inverse of light equipment, needs Effects (§13)
- [ ] **General UI/QoL**
    - [x] Implement numeric action hotkeys
    - [ ] Implement action-menu per-action hotkey labels + accuracy/effective-range readouts
    - [x] Implement maximize info screens
    - [x] Implement craft stat display
    - [x] Implement debriefing soldier status
    - [x] Implement allow tanks/HWPs to click-open doors — *already in OXCE-Plus* (`unitOpensDoor` loops all size tiles, no size gate)
    - [x] Implement loading rulesets from subdirectories

## 11. Geoscape Interface Enhancements
- [x] **Geoscape UI Polish**
    - [x] Implement sidebar score + always-visible funds display
    - [x] Implement persistent geoscape info panel (not toggle-based)

## 12. Modular Vehicles (HWPs)
- [ ] **Vehicle Customization**
    - [ ] Implement customizable HWP chassis/engines/armor/weapons
    - [ ] Implement vehicle weapon trees

## 13. Miscellaneous Battlescape Features
- [ ] **Air-Combat Minigame**
    - [ ] Implement turn-based pursuit replacement
    - [ ] Implement positional movement and TU/fuel costs
    - [ ] Implement enemy AI (snipe/berserk/escape)
    - [ ] Implement armed UFOs and escorts
- [x] **Utility Equipment Slots** — *plumbing done* ([design](plans/Feature-UtilityEquipmentSlots.md))
    - [x] Implement `INV_UTILITY` slot type (dedicated quick-access slot) — single-occupant
      hand-family slot + reserved `INV_EQUIP`; per-unit `getUtilitySlot()`/`getUtilityItem()`;
      typed-slot rules compose; no consumer wired yet.
- [ ] **Grenade Improvements**
    - [x] Implement instant grenade fuse option — *already in OXCE-Plus* (`Options::battleInstantGrenade` / per-item `fuseType: -2`)
    - [ ] Implement reduced grenade accuracy penalty — N/A on current base (OXCE has no throw penalty); revisit with aim-cone firing (§5)
    - [x] Implement kneel/stand pathing recalculation (path preview refresh on toggle) — *already in OXCE-Plus* (`btnKneelClick` → `refreshPath`)

## 14. Effects Framework
- [ ] **Effects System**
    - [ ] Implement `RuleEffect` / `BattleEffect` / `EffectComponent` (initial/ongoing/final, duration, maxStack)
    - [ ] Implement item effect hooks (`hitEffect` / `equippedEffect`)
    - [ ] Implement light-emitting effect components (`EC_CIRCULAR_LIGHT` / `EC_DIRECTIONAL_LIGHT`)
    - [ ] Implement stealth / visibility effect components (`EC_STEALTH`, `EC_NIGHT_VISION`)

## 15. AI Enhancements
- [ ] **Per-Weapon AI Targeting**
    - [ ] Implement AI engagement range bands (`aiRangeClose`/`Mid`/`Long`/`Max`)
    - [ ] Implement AI target priorities per band (`aiAttackPriority*`)
- [ ] **AI Fixes**
    - [ ] Implement normal TU-reserve logic for AI (drop custom percentages)
    - [ ] Implement reaction-fire fixes and counter-mind-control behavior

## 16. Base / Manufacture / Purchase / Transfer UI
- [ ] **Manufacture Enhancements**
    - [ ] Implement sell-per-unit and current-stores readouts
    - [ ] Implement vehicle-category production gating (free living-quarters slot)
    - [ ] Implement infinite "build forever" production
    - [ ] Implement auto-sell produced items toggle
    - [ ] Implement inline engineer-count adjustment on the current-production list
    - [ ] Implement production-picker category filter, status filter, quick-search, "show only new" + NEW marker
    - [ ] Implement manufacture dependencies-tree view
- [ ] **Purchase / Sell / Transfer Info**
    - [ ] Implement stores/quarters + sell-price info panels
    - [ ] Implement space-used display at both source and destination base on transfers

## 17. OXCE+ Integration
- [x] **Martial / Basic Training**
    - [x] Implement training facilities (`trainingRooms`, `customTrainingFactor`)
    - [x] Implement training UI (assign/allocate/finished states)
- [x] **Ruleset Inheritance**
    - [x] Implement `refNode` inheritance across rule types (cycle-guarded)
- [x] **In-Inventory Armor & Avatar Management**
    - [x] Implement armor change from inventory screen
    - [x] Implement avatar (gender/look/variant) management + armor recolor
- [x] **Sortable Soldier Lists**
    - [x] Implement clickable sortable columns + full stat columns
    - [x] Implement Stats ↔ Roles view toggle on crew-selection screen
- [x] **Item Categories**
    - [x] Implement `RuleItemCategory` + category-filtered base screens
- [x] **Alien Inventories**
    - [x] Implement alien inventory display with custom layouts
- [ ] **UFO Mission Retreat**
    - [ ] Implement UFO abandon/retreat based on damage taken
- [ ] **Other OXCE+ Surface**
    - [x] Implement craft pilots
    - [ ] Implement craft equipment templates
    - [x] Implement tech-tree viewer
    - [ ] Implement `RuleDamageType` / `ModScript` hooks

## 18. Strategic Balance & Economy (new mechanic)

*Not a UX polish — introduces new strategic-layer mechanics that change core progression.*

- [ ] **Funding Weighting**
    - [ ] Implement local/regional performance-based funding (countries weight contributions by nearby craft coverage, alien threats neutralized, base reputation)
    - [ ] Replace flat monthly income with performance-weighted formula (balance-sensitive — `Country::newMonth` is exponential today)
- [ ] **Linear Council Increases**
    - [ ] Replace exponential council score growth with linear (or configurable) per-month increases
    - [ ] Configurable curve affects mission pacing and difficulty scaling
- [ ] **Economy Tuning Hooks**
    - [ ] Expose ruleset parameters for funding weights, council curves, and regional performance factors so mods can tune without code changes
