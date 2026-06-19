# OpenXcom DX Reimplementation Plan

This document serves as a high-level checklist of features from the original OpenXcom+ project to be re-implemented on top of the OXCE+ engine.

## 1. Firing & Trajectory System
- [ ] **Aim-Cone Trajectory Model**
    - [ ] Implement 3D direction-vector cone for direct fire
    - [ ] Implement Soldier deflection cone (Gaussian/Normal distribution)
    - [ ] Implement Weapon deflection cone (independent of soldier)
    - [ ] Implement stacking of soldier and weapon error
- [ ] **Burst Fire Mode**
    - [ ] Implement `BA_BURSTSHOT` action type
    - [ ] Implement configurable burst settings (shots, range, accuracy, cost)
    - [ ] Implement sequential firing logic for burst volleys
- [ ] **Shotgun & Multi-Projectile Logic**
    - [ ] Implement simultaneous pellet flight (multiple projectiles in flight)
    - [ ] Implement shotgun pellet spread (normal distribution)
    - [ ] Implement dual-fire (simultaneous projectile spawning)
- [ ] **Trajectory & Targeting Feedback**
    - [ ] Implement live trajectory preview (tracer sprites)
    - [ ] Implement on-hover accuracy readout (color-graded percentage + distance)
    - [ ] Implement throw reach scaling (strength vs weight)
- [ ] **Accuracy Modifiers**
    - [ ] Implement kneel/two-handed/exhaustion/smoke accuracy factors
    - [ ] Implement shot-mode accuracy application (Snap/Aim/Auto/Burst)

## 2. Projectile & Explosion Management
- [ ] **Asynchronous Projectile System**
    - [ ] Implement `Map` collection for multiple in-flight projectiles
    - [ ] Implement per-projectile impact/outcome tracking
    - [ ] Implement asynchronous resolution of projectile impacts
- [ ] **Asynchronous Explosion System**
    - [ ] Implement concurrent explosion states (`ExplosionBState`)
    - [ ] Implement explosion damage falloff curves
    - [ ] Implement visual/sound scaling by blast radius

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

## 5. Reloading & Ammo Mechanics
- [ ] **Advanced Reloading**
    - [ ] Implement Quick Reload action
    - [ ] Implement weight/slot-based reload costs
    - [ ] Implement `battleClipSize` (decoupling stock vs load)
- [ ] **Ammo Item Overhaul**
    - [ ] Implement individual round tracking for ammo items
    - [ ] Implement grenades as ammo items
    - [ ] Implement ammo count display on base screens

## 6. Soldier Roles System
- [ ] **Combat Archetypes**
    - [ ] Implement Role definitions (Sniper, Medic, etc.)
    - [ ] Implement Role equipment templates
    - [ ] Implement Role UI icons (Soldier, Craft, Inventory, Battlescape marker)
    - [ ] Implement per-Role armor colors

## 7. Psionics Overhaul
- [ ] **Advanced Psionic Powers**
    - [ ] Implement Channeled Mind Control (with backlash/counter-control)
    - [ ] Implement Clairvoyance (area reveal power)
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
- [ ] **Advanced Damage Models**
    - [ ] Implement armor degradation on hits
    - [ ] Implement directional armor values
    - [ ] Implement item-based stat modification rules

## 10. Inventory & UI Improvements
- [ ] **Inventory System**
    - [ ] Implement per-unit-type inventory layouts
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
    - [ ] Implement night-vision modes (full/local), auto/toggle/hold keys, night-vision color
    - [ ] Implement per-armor sight ranges (`visibilityAtDark`/`visibilityAtDay`) + camouflage values
    - [ ] Implement light/illumination equipment (carried light sources, layered lighting)
    - [ ] Implement stealth/cloaking armor (`Armor.equippedEffects` → `EC_STEALTH` scales enemy spot range, ≥100 = invisible; `RecolorStealth` render) — inverse of light equipment, needs Effects (§13)
- [ ] **General UI/QoL**
    - [ ] Implement numeric action hotkeys
    - [ ] Implement action-menu per-action hotkey labels + accuracy/effective-range readouts
    - [ ] Implement maximize info screens
    - [ ] Implement craft stat display
    - [ ] Implement debriefing soldier status
    - [x] Implement allow tanks/HWPs to click-open doors — *already in OXCE-Plus* (`unitOpensDoor` loops all size tiles, no size gate)
    - [ ] Implement loading rulesets from subdirectories

## 11. Strategic & Geoscape Features
- [ ] **Geoscape Enhancements**
    - [ ] Implement funding weighting (local/regional performance)
    - [ ] Implement linear council increases
    - [ ] Implement sidebar score/funds visibility
- [ ] **Modular Vehicles (HWPs)**
    - [ ] Implement customizable HWP chassis/engines/armor/weapons
    - [ ] Implement vehicle weapon trees

## 12. Miscellaneous Battlescape Features
- [ ] **Air-Combat Minigame**
    - [ ] Implement turn-based pursuit replacement
    - [ ] Implement positional movement and TU/fuel costs
    - [ ] Implement enemy AI (snipe/berserk/escape)
    - [ ] Implement armed UFOs and escorts
- [ ] **Utility Equipment Slots**
    - [ ] Implement `INV_UTILITY` slot type (dedicated quick-access slot)
- [ ] **Grenade Improvements**
    - [x] Implement instant grenade fuse option — *already in OXCE-Plus* (`Options::battleInstantGrenade` / per-item `fuseType: -2`)
    - [ ] Implement reduced grenade accuracy penalty — N/A on current base (OXCE has no throw penalty); revisit with aim-cone firing (§5)
    - [x] Implement kneel/stand pathing recalculation (path preview refresh on toggle) — *already in OXCE-Plus* (`btnKneelClick` → `refreshPath`)

## 13. Effects Framework
- [ ] **Effects System**
    - [ ] Implement `RuleEffect` / `BattleEffect` / `EffectComponent` (initial/ongoing/final, duration, maxStack)
    - [ ] Implement item effect hooks (`hitEffect` / `equippedEffect`)
    - [ ] Implement light-emitting effect components (`EC_CIRCULAR_LIGHT` / `EC_DIRECTIONAL_LIGHT`)
    - [ ] Implement stealth / visibility effect components (`EC_STEALTH`, `EC_NIGHT_VISION`)

## 14. AI Enhancements
- [ ] **Per-Weapon AI Targeting**
    - [ ] Implement AI engagement range bands (`aiRangeClose`/`Mid`/`Long`/`Max`)
    - [ ] Implement AI target priorities per band (`aiAttackPriority*`)
- [ ] **AI Fixes**
    - [ ] Implement normal TU-reserve logic for AI (drop custom percentages)
    - [ ] Implement reaction-fire fixes and counter-mind-control behavior

## 15. Base / Manufacture / Purchase / Transfer UI
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

## 16. OXCE+ Integration
- [ ] **Martial / Basic Training**
    - [ ] Implement training facilities (`trainingRooms`, `customTrainingFactor`)
    - [ ] Implement training UI (assign/allocate/finished states)
- [ ] **Ruleset Inheritance**
    - [ ] Implement `refNode` inheritance across rule types (cycle-guarded)
- [ ] **In-Inventory Armor & Avatar Management**
    - [ ] Implement armor change from inventory screen
    - [ ] Implement avatar (gender/look/variant) management + armor recolor
- [ ] **Sortable Soldier Lists**
    - [ ] Implement clickable sortable columns + full stat columns
    - [ ] Implement Stats ↔ Roles view toggle on crew-selection screen
- [ ] **Item Categories**
    - [ ] Implement `RuleItemCategory` + category-filtered base screens
- [ ] **Alien Inventories**
    - [ ] Implement alien inventory display with custom layouts
- [ ] **UFO Mission Retreat**
    - [ ] Implement UFO abandon/retreat based on damage taken
- [ ] **Other OXCE+ Surface**
    - [ ] Implement craft pilots
    - [ ] Implement craft equipment templates
    - [ ] Implement tech-tree viewer
    - [ ] Implement `RuleDamageType` / `ModScript` hooks
