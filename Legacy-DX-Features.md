# OpenXcom+ — Feature & Change Summary

> **Purpose:** Hand-off reference describing the features and engine changes that
> were layered on top of stock OpenXcom (the "OpenXcom+" / "OXP"
> branch, which incorporates *Xusilak's X-COM Overhaul* / "XXCO"). Intended as a
> checklist for re-implementing these options elsewhere.
>
> **Source:** The original commits for a prior effort at revamping OpenXcom.
>
> ⚠️ **Critical note on `0f412b17`:** Its first line reads "Post-rebase cleanup,"
> but the commit is a **squash of the entire overhaul's development history** —
> ~983 lines of changelog. **The overwhelming majority of OpenXcom+ features below
> come from this single commit.** The other 23 commits are mostly later
> refinements, rebases, and TFTD porting.
>
> ✅ **Code-verified:** Class names, action/enum values, and ruleset (YAML) keys in
> this document were checked against the current source tree (Jun 2026). Verified
> identifiers are shown in `code font`. Where behavior/balance numbers come only
> from the commit prose (not re-read in code), that is noted.
>
> 🆕 **OpenXcomDX era (2017+):** This fork (now branded **OpenXcomDX**, see §20)
> continued well past the original "OpenXcom+" squash. After the milestone commit
> `53af17e8` it gained several large features that are **not** in the original
> changelog: a **turn-based air-combat minigame** (§21), a wave of **OpenXCOM
> Extended+ (OXCE+) integration** — martial training, `refNode` ruleset
> inheritance, in-inventory armor/avatar management, sortable soldier lists, item
> categories, alien inventories, UFO retreat (§22) — and **utility equipment slots**
> (§23). These sections were added/verified Jun 2026 against the current source tree
> **including uncommitted working-tree changes** (the in-progress state at the time
> of writing); where a fact comes only from the working tree that is noted.

---

## Feature Overview

A quick index of what OpenXcom+ adds or changes on top of stock OpenXcom. Each
item links to its detailed section below.

1. **[Firing / trajectory system](#1-firing--trajectory-system)** — Direct fire is
   rebuilt around an aim-cone model where shots fly along a randomly deflected
   vector instead of scattering an aim point, with separate soldier and weapon
   spread cones. Adds a Burst fire mode, shotgun pellet spread,
   kneel/two-handed/exhaustion/smoke accuracy factors, and richer targeting
   readouts (distance, estimated accuracy, tracers).
2. **[Multiple projectiles in flight](#multiple-projectiles-in-flight-at-once)** —
   Many projectiles and explosions can now travel and resolve at the same time,
   enabling simultaneous shotgun pellets and dual-wield fire, and letting automatic
   or burst fire resolve all at once rather than one shot at a time. Previously only
   one bullet existed at a time.
3. **[Overwatch](#2-overwatch-set-and-hold-reaction-fire)** — Units can spend their
   turn entering a held-fire state that reacts to enemies during the enemy turn,
   with on-map indicators and per-weapon overwatch tuning.
4. **[Movement modes](#3-reaction-fire--movement-modes)** — Adds Sprint (fast but
   exhausting and easier to hit) and Sneak (slow but defensively alert) movement,
   plus a reworked split between offensive reaction and defensive evasion scoring.
5. **[Reloading & ammo](#4-reloading--ammo)** — Adds a Quick Reload action and
   weight/slot-based reload costs, lets ammo be bought individually and loaded at
   battle time, and supports grenades as ammo.
6. **[Soldier roles](#5-soldier-roles-system)** — Soldiers can be assigned combat
   roles (Sniper, Medic, Heavy, etc.) that act as equipment templates and show up
   with icons across the soldier, craft, and inventory screens, and the selected
   unit's bobbing battlescape marker is replaced by its role icon.
7. **[Psionics overhaul](#6-psionics-overhaul)** — Mind control becomes an ongoing
   channeled action with backlash damage and AI counter-control, and adds new
   powers (Clairvoyance, Mind Blast), psi-amp ammo, and armor-reducible percentage
   psychic damage.
8. **[Health, wounds & medical](#7-health-wounds--medical)** — Introduces negative
   health and a wound-based bleedout state with battlefield indicators, plus a
   reworked medikit, stabilization, and percentage-based wound recovery.
9. **[Explosions & damage](#8-explosions--damage)** — Explosions run asynchronously
   and scale visuals/sound by blast radius, gain a configurable damage-falloff
   curve, and degrade armor on hits.
10. **[Items, stats & armor rules](#9-items-stats--armor-rulesets)** — Any item can
    grant or modify unit stats, carry directional armor values, and hook into the
    effects system; large damage/armor rebalancing accompanies it.
11. **[Inventory system](#10-inventory-system)** — Per-unit-type inventory layouts,
    slot filtering and move-cost rules, mousewheel scrolling, stat/ammo tooltips,
    and an inventory entry point from the soldier screen.
12. **[Modular vehicles / HWPs](#11-modular-vehicles--hwps)** — HWPs become
    customizable units built from chassis, engines, armor plates, and weapons via
    their own inventory, with a full laser/plasma/artillery vehicle weapon tree.
13. **[Effects framework](#12-effects-framework)** — A data-driven system for
    timed/stacking effects applied by items or ammo, with initial/ongoing/final
    components.
14. **[AI](#13-ai)** — Adds per-weapon AI engagement ranges and target priorities
    so weapons are used at appropriate distances, plus reaction-fire fixes.
15. **[Geoscape & strategy](#14-geoscape--strategy-layer)** — Funding now weights
    local/regional X-COM performance, council increases are linear, the sidebar
    shows score with funds always visible, and interception/globe tweaks.
16. **[Combat log & in-world feedback](#15-combat-log--in-world-feedback)** — A
    floating combat log reports battlefield events, alongside on-map overlays
    (primed grenades, motion readings, hovered unit names) and a fog-of-war view.
17. **[Base / manufacture / purchase / transfer UI](#16-base--manufacture--purchase--transfer-ui)**
    — Surfaces stores/quarters/soldier info and sell prices across base management
    screens and shows space usage on transfers.
18. **[General UI / QoL](#17-general-ui--qol)** — Numeric action hotkeys, maximize
    info screens, craft stat display, debriefing soldier status, and other
    quality-of-life additions.
19. **[Grenades / misc battlescape](#18-grenades--misc-battlescape)** — Instant
    grenade fuse option, reduced grenade accuracy penalty, and kneel/stand pathing
    fixes.
20. **[TFTD compatibility](#19-tftd-xcom2-compatibility--later-commits)** — All
    applicable features were ported to Terror From The Deep with the necessary
    rulesets, sprites, and color configuration.
21. **[Branding](#20-branding)** — The application identifies itself as
    "OpenXcomDX" (earlier "OpenXcom+").
22. **[Air-combat minigame](#21-air-combat-minigame-turn-based)** — A turn-based
    pursuit replacement for stock OpenXcom's real-time dogfight, with positional
    movement, per-action TU/fuel costs, enemy AI (snipe/berserk/escape), armed
    UFOs, escort UFOs, and combat fuel drawn from the craft's normal fuel. Gated
    behind a ruleset flag and off by default.
23. **[OpenXCOM Extended+ (OXCE+) integration](#22-openxcom-extended-oxce-integration)**
    — A large body of features pulled in from OXCE+: martial-training facilities,
    `refNode` ruleset inheritance, in-inventory armor/avatar management, sortable
    soldier lists with stat columns, item categories with category-filtered base
    screens, alien inventories, craft pilots, equipment templates, and UFO mission
    retreat.
24. **[Utility equipment slots](#23-utility-equipment-slots)** — A new
    armor-defined `INV_UTILITY` inventory slot type giving a unit a dedicated
    quick-access equipment slot distinct from hands.
25. **[Geoscape Activity Display](#24-geoscape-activity-display)** — An at-a-glance
    overlay on the left side of the Geoscape showing all ongoing base operations:
    research progress, manufacturing queues, craft maintenance status, and stored
    alien fuel. Idle personnel and crafts needing attention are highlighted with
    reversed colors to draw the player's eye.

---

## 1. Firing / Trajectory System

The core shooting model was rewritten.

### Aim-cone trajectory system

The original OpenXcom direct-fire model picked the target voxel, then on a "miss"
scattered that **target point** by a deviation derived from accuracy (the
"hit-or-wild-miss" / scatter-the-aimpoint approach). OpenXcom+ replaces this for
direct fire with a **3D direction-vector cone** (`Projectile::calculateTrajectory`
in `Projectile.cpp`):

1. Build a normalized **direction vector** from the origin voxel to the target
   voxel (`DVec3`, `direction.normalize()`).
2. **Deflect that vector by two independent random cones** (skipped when `force` /
   `ignoreAccuracy` is set, e.g. scripted/perfect shots):
   - **Soldier cone** — `angle = boxMuller(0, 0.437 / (soldierAcc² / 50) · 1.4826 · 2)`,
     applied via `rotateVectorRandomly(direction, angle)`. Note `soldierAcc` is
     **squared** in the denominator, so high firing skill tightens the cone
     sharply.
   - **Weapon cone** — `angle = boxMuller(0, 0.437 / weaponAcc · 1.4826)`, a
     **separate** deflection driven only by the weapon's `baseAccuracy` (it "isn't
     influenced by anything"). The two cones stack: shooter error + weapon error.
3. Scale the deflected vector (`direction *= 16000`) and trace the shot as a **ray**
   via `TileEngine::calculateLineFromVector(...)` — the bullet flies down the
   deflected line until it hits something, rather than homing on a scattered point.
   `recalculateTrajectoryFromVector()` re-traces the stored vector (used when an
   obstacle breaks mid-flight).

Implementation notes:
- `boxMuller(...)` draws from a **Gaussian/normal** distribution; `1.4826` is the
  MAD→σ constant and `0.437` a tuning constant. `soldierAcc` has a hard **floor of
  20**.
- **`soldierAcc` is accumulated multiplicatively** from many factors (verified in
  code): base `firing` stat (× `kneelModifier`/100 when kneeling, else ×115/100);
  `twoHandedModifier` when a two-handed weapon is fired with both hands occupied;
  ×0.5 when berserk; `getAccuracyModifier(item)`; ×0.7 vs. a sprinting target;
  low-energy exhaustion `×(0.5 + energyRatio)` below 50% stamina; the smoke
  modifier; and the **shot-mode accuracy (snap/aimed/auto/burst/dualfire) is now
  applied to the soldier, not the weapon** (see "Burst fire" below for the new
  mode).
- `_calculatedAccuracy = bu->calculateEffectiveRange(soldierAcc, weaponAcc)` feeds
  the on-screen estimated hit-chance / effective-range display (§1, "Targeting
  display").
- **Throwing still uses the legacy scatter model** — the old target-point
  `applyAccuracy()` deviation path (`Projectile.cpp`) is retained for the parabola
  in `calculateThrow()`; only direct fire moved to the vector cone.
- **Throw reach scales with strength vs. item weight** — the throw arc's
  *curvature* (and therefore how far an item can be lobbed) is
  `max(0.48, 1.73 / ⁴√(strength / itemWeight) + (kneeling ? 0.1 : 0))`
  (`TileEngine::validateThrow`). A stronger thrower or a lighter item flattens the
  arc → **longer range**; a heavy item relative to strength steepens it → **shorter
  range**, and kneeling shortens it further. `validateThrow` then tries increasing
  curvatures up to 5.0 to reach the target tile; if none do, the throw is rejected
  as out of range — so a weak soldier simply *can't* hurl a heavy item as far as a
  strong one. (Arcing-shot weapons use a fixed strength 70 / weight 10.) *(This is
  the standard OpenXcom strength/weight throw model, retained here; weight does
  **not** affect the throw accuracy %, only reach.)*
- **Shotgun pellets** use a single cone deflection
  (`boxMuller(0, 0.437 / shotgunAcc · 1.4826)`) per pellet — see the shotgun
  bullet below.

### Multiple projectiles in flight at once

Stock OpenXcom tracks a **single** in-flight projectile: `Map` held one
`Projectile *_projectile`, and `ProjectileFlyBState` owned the one shot and one
impact result at a time. OpenXcom+ generalizes this to an arbitrary number of
simultaneous projectiles.

**What changed:**

- **`Map` owns a collection** — the old `Projectile *_projectile` is replaced by
  `std::vector<Projectile*> _projectiles` (initially reserved for ~10), with
  `addProjectile()`, `removeProjectile()`, `getProjectiles()`, `deleteProjectiles()`,
  and `hasProjectile()`. Rendering iterates the whole vector each frame
  (`Map::drawTerrain`) instead of drawing one bullet, so any number can be visible
  together. *(Per the changelog, rendering was also reworked into a single pre-pass
  that builds the drawable list before looping over visible tiles.)*
- **Impact is stored per projectile** — each `Projectile` carries its own
  `_impact` / `getImpact()` (changelog: "store calculated impact on the projectile
  object rather than a single member variable in `ProjectileFlyBState`"), so
  resolving one bullet's hit no longer clobbers another's.
- **`ProjectileFlyBState::think()` advances them as a set** — it grabs
  `getProjectiles()` and loops, calling `projectile->move()` on each; when a
  projectile finishes (`!move()`) its outcome (throw drop / hot-grenade explosion /
  launch-waypoint cascade / unit or terrain hit) is resolved and that projectile is
  removed, while the others keep flying. The state ends only once
  `!hasProjectile() && _explosionStates` is empty.
- **Explosions are also concurrent** — resolved impacts push onto
  `std::vector<ExplosionBState*> _explosionStates`, which are ticked and reaped
  independently (this is the "async shot/explosion logic" from the changelog), so
  several detonations can animate at the same time as bullets still travel.
- **A separate targeting projectile** — `Map::_targetingProjectile` handles the
  live aim/trajectory preview (line-of-fire tracer, estimated accuracy) so the
  preview never collides with the live in-flight collection.

**Where the multiple shots come from:**

- **Shotguns** — every pellet is its own `Projectile` added to the collection and
  flying simultaneously (vs. the old single-bullet insta-hit shortcut).
- **Dual fire** (`BA_DUALFIRE`) — `ProjectileFlyBState` spawns a nested
  `_dualState` sub-state (`_subState == true`) that fires the second weapon's
  projectile concurrently with the first.
- **Auto shot / waypoint launches** — sequential shots scheduled via
  `_autoShotTimer` (and waypoint cascades for `BA_LAUNCH`) can overlap in flight
  with explosions still resolving.

**Verified code:** `Map::_projectiles`, `addProjectile`/`removeProjectile`/
`getProjectiles`/`deleteProjectiles`/`hasProjectile` (`Map.{h,cpp}`);
`Map::_targetingProjectile`; `Projectile::_impact`/`getImpact()`;
`ProjectileFlyBState::_dualState`, `_subState`, `_explosionStates`,
`recalculateTrajectoryFromVector()`.

### Burst fire (new shot mode)

OpenXcom+ adds **Burst** as a **fourth firing mode** alongside Snap, Aimed, and
Auto (`BA_BURSTSHOT` in `enum BattleActionType`, `BattlescapeGame.h`). It is a
self-contained shot configuration — `RuleItem::_confBurst` (a `RuleItemAction`,
parallel to `_confSnap` / `_confAuto` / `_confAimed`) — designed to sit **between
snap and auto**: a short controlled volley.

- **Distinct, fully-configurable mode.** Burst has its own accuracy, TU/energy cost,
  flat-cost flag, shot count, and range, loaded from dedicated ruleset keys
  (`accuracyBurst`, `costBurst`/`tuBurst`, `flatBurst`, `burstShots`, `burstRange`;
  plus the generic `confBurst:` action block). It is **only offered when
  `accuracyBurst != 0`**, so weapons opt in.
- **Defaults:** `burstShots = 2` and `burstRange = 8` tiles (verified in
  `RuleItem.cpp` constructor) — i.e. fewer rounds than Auto's default `3` and a
  range band of its own (Snap 15 / Auto 7 / Aimed 200 by default). Cost/accuracy
  fall back to the Aimed values if not set.
- **Fires like Auto, sequentially.** Mechanically burst reuses the auto-shot path:
  it sets `_autoShotsRemaining = shots` and schedules successive rounds on the
  shared `_autoShotTimer` using the weapon's `autoDelay`
  (`ProjectileFlyBState`), each round drawn through the new aim-cone model (§1).
  It is **not** a simultaneous shotgun-style volley.
- **Its own action-menu entry & hotkey** — labeled `STR_BURST_SHOT`, bound to
  `Options::keyActionBurst` (`ActionMenuState`); the menu shows its shot count and
  flags an ammo error if loaded rounds < burst shots.
- **Own experience string** `STR_BURST_SHOT` (granted once per burst, on the first
  round).
- **Usable as the Overwatch shot** — a weapon's `overwatchShot: "burst"` makes
  reaction/overwatch fire (§2) use the burst configuration.

**Verified code:** `BA_BURSTSHOT` (`BattlescapeGame.h`); `RuleItem::_confBurst`,
`getConfigBurst()`, `getAccuracyBurst()`, `getCostBurst()`, `getFlatBurst()`,
`getBurstRange()`, `getBurstShots()`; `ProjectileFlyBState` auto/burst shot loop;
`ActionMenuState` burst entry + `keyActionBurst`.

### Other firing changes

- **Default base weapon accuracy = 75** (`RuleItem::_baseAccuracy` default; later
  commit `d3016e0`).
- **Shotgun / multi-pellet weapons:**
  - Fire all rounds simultaneously (no simplified insta-hit logic).
  - Pellet count pulled from the **ammo** item.
  - `accuracyShotgunSpread` weapon setting controls spread after an accuracy-based
    initial shot; spread follows a roughly **normal distribution**.
  - Shotgun shot count shown in the shot-action menu.
- **`kneelModifier`** (RuleItem) — per-weapon accuracy multiplier when kneeling
  (replaces the hardcoded value). Hostiles granted the modifier if > base 115.
- **`reactionsModifier`** (RuleItem) — per-weapon reaction-fire modifier.
- **`twoHandedModifier`** (RuleItem, default 80) — accuracy applied when a
  two-handed weapon is used while the other hand is occupied / dual-wielding.
- **Smoke-driven accuracy falloff** — smoke between shooter and target reduces hit
  chance.
- **Exhaustion penalty** — accuracy penalty when unit is under 50% energy.
- **Targeting display (live trajectory preview):** while a fire/throw action is
  targeting, the map draws a **predicted trajectory** for the shot, using a
  dedicated preview projectile (`Map::_targetingProjectile`) kept separate from live
  in-flight shots (§1). It is traced with the **ideal trajectory** (accuracy forced
  to perfect / `ignoreAccuracy`) so the line shows the intended path, drawn as
  repeated **tracer sprites** along the route with an **impact sprite** at the end
  (frames from `Projectiles.PCK`). Direct fire uses
  `calculateTrajectoryFromVector`; throws and arcing weapons use `calculateThrow`,
  and the **throwing arc is drawn above the current map Z-level** so a lobbed path
  isn't hidden under higher floors. The preview is rebuilt as the cursor moves and
  cleared when targeting ends.
- **On-hover accuracy readout:** hovering an aim/throw cursor over a tile
  (`CT_AIM` / `CT_THROW`) shows an estimate next to the crosshair in the form
  **`<acc>% (-<cover>%) @ <distance>`** (`Map.cpp`):
  - For **direct fire** the chance comes from `Projectile::calculateChanceToHit`
    (which also returns a **cover** reduction shown as the `(-N%)` term), reflecting
    the aim-cone model and effective range (§1); for **throw** it uses
    `getThrowingAccuracy`, for **melee/`BA_HIT`** `getFiringAccuracy`.
  - It is shown **even without UFOExtender accuracy mode** (the stock gate on
    `Options::battleUFOExtenderAccuracy` is removed), and the percentage is
    **color-graded** — red at 0%, through orange/yellow, to green/bright-green at
    95%+ — for an at-a-glance read.
  - Distance-to-target (tiles) is appended, and the aim cursor flashes red/yellow
    when you lack the TUs to fire.

**Likely code:** `Projectile.cpp`, `ProjectileFlyBState.cpp`, `TileEngine.cpp`,
`Map.cpp`, `RuleItem.{h,cpp}`, `ExplosionBState.cpp`.

---

## 2. Overwatch (Set-and-Hold Reaction Fire)

A deliberate reaction-fire state a unit can enter on its own turn.

- Unit reserves TUs to fire reactively during the enemy turn.
- Cost is a **minimum of 90% of the unit's TU**.
- **Carries over between turns** (configurable behavior; originally cleared each
  turn, later allowed to persist).
- Visual indicators: target-region display, highlight over units on overwatch.
- Movement reaction triggers only "spot to protect from the spotted unit" for
  normal reaction fire — overwatch is exempt from that restriction.
- Many fixes: infinite-fire bug, TU checking, refund when multiple triggers fire.

**Verified code:** action type `BA_OVERWATCH` (`BattlescapeGame.h`); `BattleUnit`
members `_overwatch`, `_overwatchTarget`, `_overwatchWeaponSlot`,
`_overwatchShotsAvailable`; `BattleAction::overwatch` flag;
`getReactionScore(bool checkOverwatch)`. Per-weapon RuleItem rules:
`overwatchModifier` (default 100), `overwatchRadius` (2), `overwatchRange` (20),
`overwatchShot` (default `"snap"`).

---

## 3. Reaction Fire & Movement Modes

- **Sprint** — converts the "Run" advanced-movement option into Sprint: 200%
  energy / 50% TU cost; not interrupted by spotting (later: can be stopped by
  reaction fire); reduces reactions (to 0%, later 20%); imposes ~25–30% accuracy
  penalty on enemies firing at the sprinter; preview path drawn in **blue**.
- **Sneak** (CTRL+ALT) — a slow, maximally-alert "creep" mode (`BAM_SNEAK`), the
  defensive opposite of Sprint. It trades mobility for staying combat-ready:
  - **Costs 2× the normal TU** per tile (`tu *= 2.0` in `UnitWalkBState`), but
    **energy is unchanged** (still `tu/2` of the *base* cost, computed before the
    doubling) — so it's slow, not tiring. Movement is also visually slowed: the
    walk state interval is doubled (`setNormalWalkSpeed` modifier `2.0`), the
    inverse of Sprint's `0.5`.
  - **Defensive alertness is kept at full.** A unit's **evasion** (defensive)
    score normally scales by its remaining-TU ratio (`reactions × TU/maxTU`), and
    Sprint halves it; **Sneak ignores the TU-ratio entirely and uses the full
    reaction stat** (`getEvasionScore`, `BAM_SNEAK` case), so a sneaking soldier
    stays as evasive deep into its turn as at the start. *(A commented-out `×1.25`
    bonus shows an earlier intent to make Sneak actively better-than-normal.)*
  - **Preview path is drawn purple** (vs. Sprint's blue, normal yellow/green),
    and the move grants **`STR_SNEAK` battle experience**.
  - **Restricted to** single-tile (1×1) units with no turret, and is **disabled
    while the unit carries a light-emitting effect** (`EC_DIRECTIONAL_LIGHT` /
    `EC_CIRCULAR_LIGHT`) — you can't creep unseen while glowing. *(activation in
    `Pathfinding::calculate`, CTRL+ALT modifier.)*
  - **Distinct from the `sneakyAI` option:** that older toggle only makes *hostile
    AI* pathfind to avoid visible tiles (doubling the cost of stepping where it can
    be seen); the player Sneak mode does not do tile-visibility avoidance.
- **Reaction/Evasion split** — reaction mechanics split into
  `getReactionScore(bool overwatch)` (offense) and `getEvasionScore()` (defense).
  The offensive score applies the weapon's reaction/overwatch modifier; the
  defensive (evasion) score does **not** — it is pure reactions stat × movement
  factor (full for Sneak, ½ for Sprint, TU-ratio otherwise). During reaction fire
  a would-be reactor only fires if its `reactionScore` beats the mover's
  `getEvasionScore()` (`TileEngine`), so movement mode directly governs whether you
  draw reaction fire.

**Verified code:** `enum MovementAction { MV_WALK, MV_SPRINT, MV_SNEAK,
MV_STRAFE }` (`Pathfinding.h`); `BattleUnit::_movementAction` /
`setMovementAction()` / `getMovementAction()`; `BattleAction::movementAction` and
`targetSprinting` flags.

---

## 4. Reloading & Ammo

- **Quick Reload** action (firearms only; handles swapping an existing magazine).
- **Adjust Reload Costs** option — reload/unload TU cost derived from the
  inventory slot path **plus magazine weight**:
  - Quick Reload = Magazine Weight + Hand→Ground cost.
  - Normal Unload = Magazine Weight + Hand→Hand cost.
- **`battleClipSize`** (RuleItem) — an alternative ammo model for **expensive
  per-shot, but still magazine-fed weapons** (think individually-costed rounds —
  blaster bombs, fusion/artillery shells, psi-orbs, §6 — that you'd never want to
  buy as a "clip of 1"). It decouples how ammo is **stocked** from how it's
  **loaded**:
  - **Stocked, bought, sold, and recovered one round at a time.** With
    `battleClipSize` set, each unit in base stores is a single round (not a clip),
    so purchase/manufacture price and inventory are tracked per shot. Mission
    recovery returns the **raw recovered rounds** to stores (`getBattleClipSize()
    ? rounds : rounds / clipSize` in `DebriefingState`), rather than
    rounds-rounded-down-to-whole-clips.
  - **Loaded as a magazine of `battleClipSize` rounds.** At battle generation the
    loose stored rounds are packed into magazine `BattleItem`s each holding
    `min(remaining, battleClipSize)` rounds (`BattlescapeGenerator`), and in the
    field the weapon reloads/fires/refills exactly like any normal magazine
    (full-clip checks and `spendBullet` honor `battleClipSize`). So a weapon can
    still hold, say, 4 shots per reload while every shot is individually accounted
    for in the economy.
  - **Mutually exclusive with `clipSize`** — setting `battleClipSize` to a non-zero
    value **forces `clipSize` to 0** at load time (`RuleItem::load`), so an ammo
    item uses one model or the other, never both.
  - Pairs with the §6 psi-orb ammo (psi-amps consume per-action rounds) and with
    the "ammo items display their contained count on base screens" change below.
- **Grenades as ammo items** — preliminary support.
- Renamed all "clip" items to "ammo" items; ammo items display the count they
  contain per purchase/manufacture on base screens.
- Reload logic unified into `BattleUnit`; applied to AI; vehicles reload at no TU
  cost.

---

## 5. Soldier Roles System

Assignable combat archetypes that act as **equipment templates** and drive UI.

- Roles: None, Rifleman, Sniper, Scout, Rocketeer, Assault, Heavy, Grenadier,
  Medic, Psionics, Demolitions, Specialist.
- `RuleRole` fields (verified): `name`, `iconSprite`, `smallIconSprite`,
  `isBlank`.
  *(Note: `RuleRole` has no `shortName` field — the changelog's "short role name"
  for lists/combat log appears to be a language string, not a rule field.)*
- **Replaces the selected-unit marker on the battlescape** — the bobbing arrow
  over the currently selected unit is replaced by that unit's role icon (it bobs
  the same way), so the active soldier's role is visible at a glance in the field.
- Manage roles via a roles menu (`RoleMenuState`) and change flow
  (`RoleChangeState`); roles function as **unit equipment templates**.
- Role shown on: Soldier Info screen, craft soldier list (before rank),
  pre-combat inventory soldier list, unit tooltip, combat log.
- Data-driven via `roles.rul` + `interfaces.rul` (no hardcoded colors → TFTD-safe).
- Fixes for roles in New Battle mode and SavedGame load order.

**Verified code:** `RuleRole.{h,cpp}`, `Role.{h,cpp}`, `RoleMenuState.{h,cpp}`,
`RoleChangeState.{h,cpp}`; top-level `roles:` ruleset section; `roles.rul`.

---

## 6. Psionics Overhaul

- **Channeled / ongoing Mind Control** (`BA_MINDCONTROL`) — persists across turns
  with TU upkeep; requires a held PSIAMP; dropped if the amp is lost.
- **Backlash damage** — controller takes psychic damage; on a controlled unit's
  death the controller takes a 20–40 psi hit and −2 psi strength.
- **Counter–mind control** — AI units may attempt to wrest control back.
- Mind-controlled units can't enter panic/berserk; indicator drawn over
  controller and controlled unit.
- **Psi-amp ammo mechanic** — psi-amps now consume ammunition (e.g. "psi orbs")
  like a firearm instead of being free. Each psi action has a **per-use round
  cost** taken from the weapon's `psiCost*` rule (`psiCostPanic`,
  `psiCostMindControl`, `psiCostClairvoyance`, `psiCostMindBlast`); the loaded ammo
  is debited by that many rounds via `spendBullet(psiCost)`, and the amp is
  unloaded when the clip runs out. Using a power with no clip loaded or too few
  rounds fails with `STR_NO_AMMUNITION_LOADED` / `STR_NO_ROUNDS_LEFT`, and the
  action menu flags the cost in red when it exceeds the rounds available. Psi-amps
  support auto-load / reload like other weapons, the ammo is recovered after the
  mission, and psi-orb ammo uses `battleClipSize` so it can be bought individually
  and loaded at battle time (see §4).
- **New psionic powers** (in addition to the classic Panic and Mind Control):
  - **Clairvoyance** (`BA_CLAIRVOYANCE`) — a no-target, area **reveal** power: it
    lights up a circular patch of the map around the chosen tile (marking the tiles
    discovered on every level and exposing any hidden units there), letting a
    psi-strong soldier scout remotely through the fog of war. It requires
    `psiStrength + psiSkill ≥ 100`, and the reveal **radius scales** with that
    combined psi score (roughly: 100 → 0, 110 → 1, 130 → 2, 160 → 3, 200 → 4,
    250 → 5, 310 → 6 tiles). Grants psi-skill experience.
  - **Mind Blast** (`BA_MINDBLAST`) — a direct **psychic-damage** attack
    (`DT_PSYCHIC`). Unlike other psi actions it requires an actual **line of fire /
    squad-sight** to the target (else `STR_NO_LINE_OF_FIRE`). Outcome compares
    attacker `psiStrength + psiSkill` (± a random swing) against the target's, scaled
    by distance (closer hits harder): a large margin is a "decisive" hit that only
    damages the victim, a narrow win damages the victim but also inflicts
    **backlash** on the caster, and an outright failure backlashes the caster
    instead. *(damage formulas verified in `PsiAttackBState::psiAttack`)*
- **Enforced squadsight** for alien psi targeting; prevents psi target selection
  from "remembering" previously spotted targets.
- Psychic damage made **percentage-based**, reducible by armor, applies a morale
  penalty; psychic stun reduced to 25–50%. *(numbers from prose)*

**Verified code:** action types `BA_MINDCONTROL`, `BA_CLAIRVOYANCE`,
`BA_MINDBLAST` (`BattlescapeGame.h`); `PsiAttackBState`; per-weapon RuleItem
costs `psiCostPanic`, `psiCostMindControl`, `psiCostClairvoyance`,
`psiCostMindBlast`.

---

## 7. Health, Wounds & Medical

- **Negative health & bleedout** — in stock OpenXcom a unit's health floors at 0
  (unconscious) and it dies at 0. OpenXcom+ lets **player soldiers** (only:
  `getCanBleedOut()` requires an original-player, non-vehicle geoscape soldier)
  drop into **negative health** and enter a *bleeding out* state instead of dying
  outright:
  - When such a soldier's health would go below 0, `checkStartBleedout()` sets
    `_bleedingOut` and **adds 5 fatal torso wounds** — the "bleedout buffer" that
    keeps the bleed going (this is the changelog's wounds-reduced-10→5 value).
  - The soldier doesn't actually **die until health reaches `getDeathHealth()` =
    −(max health / 2)** — i.e. there's a dying window between 0 and minus-half-max
    HP during which they can still be saved.
  - The mechanic is **wound-based and progressive**: each turn fatal wounds
    subtract from health (`_health -= getFatalWounds()`), so an unstabilized
    bleeding soldier keeps sinking toward the death threshold and will die if not
    treated. `getBleedingOut()` is true while bleeding, health ≤ 0 but above the
    death threshold, and fatal wounds remain.
  - Applies to **player soldiers only** — aliens, civilians, and HWPs/vehicles
    still simply hit 0 (unconscious/dead) as before.
- **Bleeding-out indicators** — per-unit battlefield icon over a dying soldier,
  plus an overall bleedout indicator next to the spotted-enemies list (shown
  regardless of which unit is selected).
- **White cross-marks on the health bar** indicate wounds.
- **Medikit rework / stabilization** — healing a fatal wound on a bleeding-out
  soldier **stops the bleed but cannot revive them in the field**: it applies a
  heavy stun (`_stunlevel = max(1000, …)`) so they can't rejoin the fight and
  forces `healthAmount = 0` (wounds are reduced, but **no** health is restored and
  no wound-recovery bonus is granted). The medikit view also shows all-red when the
  target is bleeding out, with improved damage/stat info. *(An earlier "restore to
  10% HP / triple wound heal" design is present but commented out.)*
- **Wound recovery** — post-mission recovery is now **proportional to health lost**
  rather than a flat value: `woundRecovery = healthLoss × RNG(20–30) / maxHealth`
  days.
- **"Field Surgery" research (`STR_FIELD_SURGERY_UNIT`)** — once researched,
  recovery for **all** soldiers uses a smaller multiplier, `RNG(15–25)` instead of
  `RNG(20–30)`, shortening convalescence (~25% faster in practice; the changelog
  describes the intent as "halving" recovery time). It is a one-time research that
  permanently speeds wound recovery base-wide.
- **Allow zero-HP units.**

**Verified code:** `BattleUnit::_bleedingOut`, `checkStartBleedout()`,
`getCanBleedOut()`, `getBleedingOut()`, `getDeathHealth()`, `heal()`;
wound-recovery + `STR_FIELD_SURGERY_UNIT` branch in `BattleUnit::postMissionProcedures()`.

---

## 8. Explosions & Damage

- **Async shot/explosion logic** — decouples impact/explosion animation from the
  single, globally-ticked battlescape state, which is what allows the concurrent
  projectiles and explosions described in §1.
  - *Stock behavior:* battlescape "states" advance off the **main loop's shared
    state interval**, and the active state sets that interval
    (`setStateInterval`). A bullet's flight and its resulting explosion each
    commandeer the loop one at a time, so only one impact/explosion could play at
    once and it dictated the global tick rate.
  - *OpenXcom+:* `ExplosionBState` owns its **own `Timer _animationTimer`** (built
    in `init()`, default `DEFAULT_ANIM_SPEED/2`, callback `thinkTimer()`). Its
    `think()` just ticks that private timer; `thinkTimer()` advances the explosion
    sprites, applies the damage via `explode()` once they finish, and sets
    `_finished` — all independent of the main loop's interval.
  - When an explosion runs **as a sub-state** (`_subState == true`, e.g. a bullet
    impact spawned by `ProjectileFlyBState`) it **no longer calls
    `setStateInterval()`** (the old call is commented out) — so it can't hijack or
    overwrite the global tick. This is exactly what lets `ProjectileFlyBState` hold
    a `std::vector<ExplosionBState*> _explosionStates` and tick several explosions
    at the same time while bullets are still in flight.
  - The bullet-hit branch derives its own animation interval from the item's
    `explosionSpeed`; psi/mind actions set `_delayExplosion` so the damage step is
    deferred past the cosmetic animation.
- **`blastDropoff`** (RuleItem) — controls how fast an explosion loses damage with
  distance; visual size and sound now derive from **blast radius**, not power.
- **Armor damage model** — armor degrades on hits (10% if base damage > 50% of
  armor; 20% if damage actually lands); smoother, more punishing scaling.
- Numerous crash fixes (power-source explosions, chained explosions causing
  projectile slowdown, blaster launcher not exploding on impact).

**Verified code:** `ExplosionBState::_animationTimer`, `init()`, `think()`,
`thinkTimer()`, `_subState`, `_finished`, `_delayExplosion`;
`ProjectileFlyBState::_explosionStates` (the concurrent explosion list).

---

## 9. Items, Stats & Armor Rulesets

- **`stats` on all items** — any item can grant a `UnitStats` block while equipped
  (RuleItem `_stats`; `_hasStats`).
- **`statModifiers` (percentage) on items and armor** — `node["statModifiers"]` on
  both `RuleItem` and `Armor`; inventory slots can opt to apply stats from items in
  them; Power/Flying Suit bonuses converted to stat modifiers.
- **Directional armor on items:** `frontArmor`, `sideArmor`, `rearArmor`,
  `underArmor` (verified on both `RuleItem` and `Armor`);
  an **Armor inventory slot type** (later reworked into sided slots).
- **Per-item effect hooks:** `hitEffect` and `equippedEffect` (RuleItem) link items
  to the effects framework (see §12).
- **Item slot restriction:** `RuleItem::_validSlots` / `_checkValidSlots`.
- **`BT_EQUIPMENT`** battle type; item appropriate-armor (explosive HP) values.
- Items ruleset data moved into `Items.rul`; large weapon-damage rebalance (tiered
  +33% steps), armor/health rebalance, listorder fills. *(balance from prose)*

---

## 10. Inventory System

- **Inventory-layout ruleset** — different unit types get different inventory
  structures. Top-level `inventoryLayouts:` section → `RuleInventoryLayout` (`id`
  + `invs:` list of slot ids); `RuleSoldier.inventoryLayout` selects one (default
  `STR_STANDARD_INV`).
- **Loadout copy/paste & saved templates** — beyond roles acting as equipment
  templates (§5), the pre-battle inventory screen has a full clipboard-and-library
  system for moving loadouts between soldiers and missions (`InventoryState`):
  - **Create Template (copy) / Apply Template (paste)** — `Create Template`
    snapshots the selected soldier's entire inventory into an in-memory clipboard
    (`_curInventoryTemplate`); `Apply Template` pastes it onto any other soldier,
    pulling the needed items from the ground/base pile. Keys
    `keyInvCreateTemplate` / `keyInvApplyTemplate`; the buttons show empty-vs-filled
    icons and only appear in **pre-battle setup** (non-TU) mode.
  - **Captures the full layout, not just item types** — each entry is an
    `EquipmentLayoutItem` recording the item, **slot + x/y position, loaded ammo,
    and fuse/prime timer**, plus the soldier's **armor color**
    (`_curInventoryTemplateArmorColor`). Applying reports any items it couldn't
    place (missing from stores) rather than silently dropping them.
  - **Named global layout library** — up to **20** persistent equipment templates
    (`SavedGame::MAX_EQUIPMENT_LAYOUT_TEMPLATES`) stored on the savegame with
    editable names, managed via Load/Save screens (`InventoryLoadState` /
    `InventorySaveState`). Number keys **1–9 quick-load** a slot and
    **CTRL+number quick-saves** to it (`btnGlobalEquipmentLayoutClick`); they
    survive across missions and saves, unlike the single copy/paste clipboard.
  - **Craft loadout templates** — a craft-level analogue: up to **10** saved craft
    loadouts (`MAX_CRAFT_LOADOUT_TEMPLATES`, `getGlobalCraftLoadout`) for
    re-equipping a whole craft's stores in one step.
  - **Clear / Auto-equip** — companion one-key actions to strip a soldier
    (`keyInvClear`) or auto-distribute equipment (`keyInvAutoEquip`).
- **Slot filtering / restriction** — `RuleInventory` gained several per-slot rules
  that turn a slot from a generic pocket into a typed, role-specific socket. Each
  is enforced when an item is moved into a slot (`BattleUnit::moveItem` →
  `RuleItem::canBePlacedIntoInventorySection`):
  - **`battleType`** — restricts the slot to one item battle type. If set
    (≠ `BT_NONE`), only items whose own `battleType` matches may be placed there;
    a mismatch is rejected with `STR_INVALID_ITEM_SLOT`. This is what makes
    dedicated **armor slots** (`battleType: BT_ARMOR`, see directional armor in §9),
    ammo-only slots (`BT_AMMO`), etc. Default `BT_NONE` = anything fits.
  - **`allowCombatSwap`** (default true) — when false, the slot is a **loadout-only
    socket**: items can't be moved into or out of it **once a mission is underway**
    (`_inCombat`), failing with `STR_NOT_COMBAT_SWAPPABLE`. Set up before the
    mission, locked during it. (Combat-swappable slots are also tinted differently
    in the slot labels.)
  - **`costs`** (map of `destinationSlotId: TU`) — per-destination move cost
    (`getCost`). It doubles as an **allow-list of in-combat transfers**: a
    destination not listed returns −1, which in combat is rejected as
    `STR_INVALID_TRANSFER` (out of combat the move is free/allowed).
  - **`countStats`** (default false) — only items in `countStats` slots contribute
    their `stats`/`statModifiers` (§9) to the wearer; `updateStats()` walks just
    those slots, so you can have "display/holster" slots that grant no bonuses.
  - **`allowGenericItems`** is **parsed but not currently enforced** — together with
    the item-side `validSlots` / `_checkValidSlots` and the older
    `canBeUsedInSlot()` path, the generic-item gating is **commented out** in the
    live build (the active filter is `battleType` + the item's
    `supportedInventorySections` list). Treat it as a defined-but-dormant key.
- **Item movement UX** — display TU cost of moving items (always, even out of
  combat); highlight valid target slots; reload cost & ammo count shown.
- **Mousewheel scrolling** through oversized ground inventories — the wheel pans
  the ground view **one slot-column at a time** (`changeGroundOffset(±1)`, clamped
  to `[0, xMax-1]`), in contrast to the arrow/page control which jumps a **full
  page width** (`_groundOffset += slotsX`, wrapping to 0). This lets you nudge the
  ground display by a single column to bring a partially-scrolled item fully into
  view instead of being limited to page-sized hops.
- **Inventory tooltip mode** with stat display; improved corpse info; stack /
  ammo indicators repositioned.
- **Soldier-list & Inventory buttons** added to the Soldier Info screen;
  maximize-info-screen fixes.
- New inventory action button sprites (ground/next/prev/ok/unload) + TFTD variants
  (later commits).

---

## 11. Modular Vehicles / HWPs

A large subsystem turning HWPs into customizable, soldier-like units.

- **Modular vehicles as soldier types** — each chassis is a `RuleSoldier` with
  `isVehicle: true` (verified field) and a `buyCost`; vehicles equip engines,
  weapons, armor plates, and accessories via their inventory layout.
- Vehicle-specific inventory layouts; sided armor slots (Hover Tanks get two per
  side); engine bays / accessory rooms.
- **No experience/promotions; no reload TU cost; TU-recovery penalties disabled**
  for overweight vehicles; innate TU boosts (Scout Car +20%, Hover Tank +30%).
- Vehicle **weapon roster:** machine gun, minigun, rocket/missile launchers,
  light/heavy cannons, **HWP laser & plasma** weapons, artillery (fission/fusion
  shells, cluster, smart fusion bomb), with full buy/manufacture/sell/research.
- `STR_VEHICLE` production category; differentiated HWP sprites.

---

## 12. Effects Framework

- **Effects framework (present in code):** `RuleEffect` + `BattleEffect`
  (savegame) + `EffectComponent` (defined in `RuleEffect.h`).
- `RuleEffect` fields (verified): `icon`, `initialComponents`, `ongoingComponents`,
  `finalComponents` (each a `vector<EffectComponent*>`), `duration`, `maxStack`.
- **Ammunition/items apply effects** via RuleItem `hitEffect` / `equippedEffect`
  (see §9); `Game::getGame()` static singleton accessor was added to support it.
- A **Health Damage** effect component was the first implemented example.
  *(Confirm which `EffectComponent` subtypes are fully wired before relying on
  them — the framework was actively evolving in the squash.)*

---

## 13. AI

- **Per-weapon AI range handling** — new RuleItem fields: `aiRangeClose`,
  `aiRangeMid`, `aiRangeLong`, `aiRangeMax`, and
  `aiAttackPriorityClose/Mid/Long/Max`.
- AI forced to use normal TU-reserve logic instead of custom percentages; reserved
  TU fallback handles non-standard shot configs.
- "One in a million" reaction-fire AI fix; counter–mind-control behavior (see §6).

---

## 14. Geoscape & Strategy Layer

- **Funding model rework** — nations weight X-COM's **local** activity (own
  country + region) far more than global activity; council funding **increases are
  linear** (reductions still exponential).
- **Geoscape Score** shown in the sidebar; **Funds always displayed** (old toggle
  obsolete); sidebar layout redesigned. (Commit `63b3774`.)
- **Globe palette setting may be 0** to clear the hardcoded "water" fill.
  (Commit `558da50`.)
- **Interception** — interceptor list sorted by distance to target (uses in-flight
  position); dogfight dialogs accept number-key shortcuts.
- Clear base as retaliation target on successful base defense.

---

## 15. Combat Log & In-World Feedback

- **Floating combat log** displaying battlefield events (damage, reloads,
  psionics, item swaps), with a colorable non-warning message channel and improved
  damage/stun reporting; racial-research and interrogation reveal extra info.
- **Inventory primed-grenade indicator** — in the inventory screen, every **live
  (primed) grenade** is marked with a small **pulsing icon** drawn over its big
  sprite (`Inventory::drawPrimers`; an item counts as live when its fuse timer is
  set, `BattleItem::getGrenadeLive`). It flags armed grenades in a soldier's hands,
  belt, backpack, etc. (and on the ground), so you can see at a glance what's
  primed before committing to a throw. This is the **inventory-view** indicator —
  distinct from the on-**battlescape** primed-grenade indicator (§18).
- **In-world overlays:** hovering icon over armed proximity grenades, motion-detector
  readings shown in-world, hovered unit name on the map, role icon in tooltip.
- **Fog-of-war field-of-view display** — **the key change vs. stock:** base
  OpenXcom draws every *discovered* tile at full brightness and gives no on-map
  indication of which tiles a soldier can actually see *right now*. This fork
  **dims tiles that are not currently in any soldier's line of sight**, so the
  player can read live field-of-view at a glance — currently-seen terrain is bright,
  remembered-but-unobserved terrain is darkened. Mechanically tiles are rendered in
  **three visibility states** keyed off a per-tile *visible count*
  (`Tile::getVisibleCount`):
  - **Currently in sight** (visible count > 0, discovered): full brightness
    (`reShade(tile)`) — same as stock.
  - **Explored but out of current sight** (discovered, visible count 0): **dimmed**
    (`tileShade = 5·tileShade/8 + 6`) — *this is the new behavior; stock leaves
    these tiles at full brightness.* You still see the remembered map layout but can
    tell it's not under live observation, and units standing there are hidden (the
    actual "fog of war"). *(Map drawing, `Map.cpp`.)*
  - **Never discovered:** fully black (`tileShade = 16`), nothing on the tile drawn
    — same as stock.
  - **How the visible count works:** each player unit keeps the set of tiles it can
    currently see (`BattleUnit::_visibleTiles`, built by
    `TileEngine::calculateTilesInFOV`); adding a tile bumps that tile's visible
    count (`changeVisibleCounts(+1, …)`), and recomputing a unit's view clears its
    old set first (`clearVisibleTiles`). So a tile's count is "how many of my
    soldiers can see it right now" — drop to 0 and it fogs over. A **second**
    night-vision count is tracked in parallel (lit tiles are also shaded by the
    night-vision / `fullNightVision` model via `reShade`).
  - **Consequence for events:** projectiles and explosions only force the camera /
    animate when they occur on a tile in current FOV (`_projectileInFOV` /
    `_explosionInFOV` gated on `getVisibleCount()`), so off-screen enemy actions in
    the fog aren't revealed by their animations.
  - The fog is recomputed via `calculateFOV` on movement, terrain changes, doors,
    and unit spotting; a later fix **forces a full FOV recalc at the start of the
    player's turn** to clear stale fog state (commit `35780266`). A `DEBUG_FOV`
    build switch and earlier "display unit FOV" testing commits exist for debugging
    the view computation.
- **Night vision** (OXCE-derived; options under the `STR_OXCE` group) — a toggle
  that brightens the battlefield in darkness so you aren't fighting blind at night:
  - `Map::reShade` caps the shade of dark tiles to an animated `_fadeShade` and
    tints them with a configurable night-vision color, but **only when the global
    shade is dark enough** (past `NIGHT_VISION_THRESHOLD`).
  - Two modes via `Options::fullNightVision`: **full** brightens the whole
    (discovered) map; **local** only brightens tiles within each player unit's
    dark-view radius (`BattleUnit::getMaxViewDistanceAtDark`). `autoNightVision`
    enables it automatically in dark, and it can be toggled/held by key
    (`keyNightVisionToggle` = ScrollLock, `keyNightVisionHold` = Space);
    `nightVisionColor` (default 8) sets the tint.
  - **Per-armor dark sight** — `Armor.visibilityAtDark` (and `visibilityAtDay`) set
    how far a unit sees in the dark/light, feeding the local-night-vision radius and
    spotting ranges, and interact with `camouflage`/`antiCamouflage` armor values.
- **Light / illumination equipment** — units and items can emit light on the
  battlefield through the effects framework (§12): effect components
  **`EC_CIRCULAR_LIGHT`** and **`EC_DIRECTIONAL_LIGHT`** (`RuleEffect.h`), applied
  via `BattleEffect`, trigger a unit-layer lighting recalculation
  (`Tile`/`TileEngine` lighting, `calculateLighting(LL_UNITS)`) so a carried flare/
  torch lights surrounding tiles (on top of the armor's baseline
  `Armor.personalLight`, default 15). Lighting is computed in independent layers
  (ambient / fire / items / units), keeping the brightest per layer. Carrying a
  light source **disqualifies a unit from Sneak movement** (§3) — you can't creep
  unseen while illuminated.
- **Stealth / cloaking armor** — armor can make its wearer **harder to spot**, the
  inverse of the light feature. An armor lists effect ids in
  **`Armor.equippedEffects`** (a `RuleEffect` list applied to the wearer via the
  effects framework, §12; items can likewise grant one via RuleItem
  `equippedEffect`). An effect carrying an **`EC_STEALTH`** component with a
  `magnitude` shrinks the range at which **other units can see the wearer**: when
  spotting checks run, the observer's max-visible-distance to a stealthed target is
  scaled by `(100 − magnitude) / 100`, and a **`magnitude ≥ 100` makes the unit
  effectively invisible** (spot distance 0 — seen only point-blank/adjacent)
  (`TileEngine`, FOV/spotting calc). So e.g. a magnitude-50 cloak halves how far
  enemies can detect you, letting a scout in stealth armor operate close to the
  enemy. Stealthed units are drawn with a translucent **`RecolorStealth`** shader;
  this cloak render is wired in the **inventory paperdoll** but the battlescape
  unit-sprite path (`UnitSprite::drawRecolored`) is **commented out / TODO**, so the
  in-field visual is only partially implemented even though the spotting mechanic
  works. *(A separate `EC_NIGHT_VISION` effect component also exists for
  effect-granted night vision.)*

---

## 16. Base / Manufacture / Purchase / Transfer UI

- Base Info window rearranged with soldiers/stores/quarters info.
- **Manufacture** — changes span all three manufacture screens:
  - *Start/allocate screen* (`ManufactureStartState`): adds a **`STR_SELL_PER_UNIT`
    line** showing the total sell value of everything one production run yields
    (Σ `item->getSellCost() × quantity` over the recipe's produced items), and a
    **`STR_CURRENT_STORES` line** showing how many of the produced item you already
    hold (single-item recipes). *(commits `93793a6a` sell price, `7d5bd9855`
    current stores.)*
  - *Vehicle production category* — `STR_VEHICLE` recipes are gated like craft: a
    vehicle build requires a **free living-quarters slot** (just as `STR_CRAFT`
    requires a free hangar), erroring otherwise. Ties into the modular-vehicle
    production line (§11) and the `STR_VEHICLE` category.
  - *Detail screen* (`ManufactureInfoState`): two production-loop conveniences —
    - **"Build forever" / infinite production** — the units-to-produce count can be
      set to **infinite** (`Production::getInfiniteAmount`/`setInfiniteAmount`):
      **right-click the increase arrow** to flip a normal item to ∞ (the to-do field
      then shows "∞"), so the line keeps producing until you stop it. (Craft/vehicle
      categories right-click to `INT_MAX` rather than true infinite.)
    - **Auto-sell produced items** — a **`STR_SELL_PRODUCTION` toggle** (`_btnSell`,
      labeled with the per-run sell value) flags the production to **automatically
      sell its output** as it's made (`Production::getSellItems`/`setSellItems`);
      hidden for productions that spawn a person. Combined with infinite production
      this gives a hands-off "manufacture-to-sell" money loop.
  - *Current-production list* (`ManufactureState`): engineer count is adjustable
    **inline via per-row left/right arrows** (and mouse-press/middle-click), not
    only inside the detail screen.
  - *Production picker* (`NewManufactureListState`, largely from the OXCE+
    integration, §22): gains a **Category column + category filter combobox**
    (`_cbxCategory`, defaulting to `STR_ALL_ITEMS`), a **status/supply filter
    combobox** (`_cbxFilter`: Default / supplies-OK / no-supplies / facility-
    required / hidden), a **quick-search** box, a **"Show only new"** toggle, and a
    **NEW marker** on never-opened recipes (`getManufactureRuleStatus`). A
    dependencies-tree view (`ManufactureDependenciesTreeState`) is also present.
- Purchase: stores/quarters info; tweaked soldier purchase display.
- Sell: always display base stores info.
- Transfer: lists space used at both local and destination base.
- Removed the "Original X-COM" load button from the savegame list.

---

## 17. General UI / QoL

- **Battlescape action menu hotkeys & layout** (`ActionMenuState`) — every entry
  in the right-click action popup is **bound to a configurable hotkey** that is
  **printed as a prefix on the action label** (e.g. the key name + the action name,
  via `SDL_GetKeyName`), and the action can be fired directly with that key without
  clicking. Each shot mode / action has its own option binding
  (`keyActionSnap`/`Auto`/`Burst`/`Aimed`/`Throw`/`Prime`/`Melee`/`Use`/`DualFire`/
  `Overwatch`/`Psi1`–`Psi4`, …), duplicate keys collapse to "unbound", and the menu
  shows a **header with the selected weapon (and loaded ammo) name** (`_selectedItem`).
  The list was also shrunk and the selected action is surfaced after picking it.
  *(commit `93793a6a`.)*
- **Action menu accuracy / shot readouts** — each action entry shows a reworked
  per-action info line (`ActionMenuItem`), not just a flat hit %:
  - **Direct-fire modes** (Snap / Aimed / Auto / Burst / Overwatch / Dual Fire) use
    the **"new accuracy model"**: instead of a hit-chance percentage they display an
    **effective range** (`STR_RANGE_SHORT`, from
    `BattleUnit::calculateEffectiveRangeForAction`) — the distance at which the
    aim-cone (§1) keeps the shot reliable — which is the meaningful number under the
    vector-cone firing system.
  - **Throw / melee / hit / launch** still show a true **accuracy percentage**
    (`STR_ACCURACY_SHORT`); **psi actions** (panic / mind control / clairvoyance /
    mind blast) show their per-use **round count** (`Nx`).
  - The figure is annotated with **shot count** (`…Nx` for auto/burst/dual fire) and
    **pellets-per-shot** (`…xN` for shotguns) — the "##x\<count\>" display.
  - A **TU line** shows the action's cost against the unit's current TUs
    (`STR_TIME_UNITS_SHORT`), and the readouts **turn red** on insufficient
    ammo/TUs (and warn when a shot would exhaust the unit's TUs) — the
    highlight-in-red feedback from commits `53af17e8` / `04517cb2`.
- **Maximize Info Screens** option (`maximizeInfoScreens`, `STR_MAXIMIZE_INFO_SCREENS`
  under General) — temporarily drops info/detail screens to the original 320×200
  resolution so they render at a legible scale. It is **stack-based**
  (`Screen::pushMaximizeInfoScreen` / `popMaximizeInfoScreen`): nested info screens
  push onto `_maximized` instead of each re-scaling, and closing the last one
  restores the correct **geoscape vs. battlescape** scale — fixing the
  constructor/destructor-order scale confusion of the earlier approach (commit
  `cbfff481f`). Applied to the Geoscape screens as well (commit `036cf0c47`).
- **Craft Info stats** (`CraftInfoState`) — the craft screen now shows core craft
  stats: **Maximum Speed** (`STR_MAXIMUM_SPEED_UC`), **Acceleration**
  (`STR_ACCELERATION`), and **Damage Capacity** (`STR_DAMAGE_CAPACITY_UC`),
  alongside the existing fuel %/repair-time readouts. *(commit `09a66084`.)*
- **Debriefing soldier results** (`DebriefingState`) — the post-mission screen gains
  a **Status column** (`STR_STATUS`) and reports each soldier's outcome including
  **wounded-recovery days**, plus a per-soldier breakdown of gains —
  **experience, kills, rank/level changes, and individual stat increases** (TU,
  stamina, health, bravery, reactions, firing, throwing, melee, strength, psi). The
  detailed soldier info is split into its own view. *(commits `433d2a84`,
  `c5664c1d` "wounded days with status".)*
- **Allow tanks/HWPs to click-open doors** — vehicles can open doors directly in the
  battlescape (stock restricts this to soldier units).
- **Build / infra:** load rulesets from **subdirectories**; the active `Ruleset`/Mod
  is reachable through a static `Game` accessor (`Game::getGame()`, supporting the
  effects framework §12); a **development build configuration** for faster compile/
  test iteration; and VS2010/VS2013 (later VS2017) project files.

---

## 18. Grenades / Misc Battlescape

- **Instant grenade fuse** option (and saving of instaprimed grenades to
  EquipmentLayoutItem).
- Reduced grenade LOS accuracy penalty to 75% max.
- **Kneel/stand pathing recalculation** — toggling kneel/stand changes a unit's
  available TUs (stand-up costs 8 TU, kneel costs 4), and `Pathfinding::previewPath`
  reserves the 8-TU stand-up cost when a kneeled unit plots a move. The fix makes
  the **on-map path preview refresh the moment you kneel or stand** while a path is
  shown: `BattlescapeState::btnKneelClick` re-runs `calculate()` →
  `removePreview()` → `previewPath()` if `isPathPreviewed()`. Without it the preview
  kept stale TU costs / reachability colors after the toggle (a route that looked
  reachable before standing might no longer be, and vice-versa). *(commit `7baca6a`.)*

---

## 19. TFTD (xcom2) Compatibility — later commits

Every applicable OXP feature was ported to Terror From The Deep: `roles.rul` for
TFTD, TFTD `interfaces.rul` colors for OXP stat displays, TFTD inventory-button
sprites, and removal of hardcoded colors/options from Role states that broke in
TFTD. (Commits `fd14930`, `1af11d7`, `66c477e`, `69c5219`, `023b5be`.)

---

## 20. Branding

Application title was first changed from "OpenXcom" to **"OpenXcom+"** (commit
`c68aeee`; also the `OpenXCOM+!!!` marker mid-history in `0f412b17`). In the current
fork it has been rebranded again to **"OpenXcomDX"** *(working tree)*:

- Window title `"OpenXcom "` → **`"OpenXcomDX "`** (`main.cpp` `title <<` line).
- MorphOS `Version[]` `$VER: OpenXCom` → **`OpenXComDX`** (`main.cpp`).
- Options help text `"OpenXcom v"` → **`"OpenXcomDX v"`** (`Options.cpp`).
- Git version suffix `OPENXCOM_VERSION_GIT` `" TD-DEV"` → **`" DX-DEV"`**
  (`version.h:29`).

---

## 21. Air-Combat Minigame (Turn-Based)

A from-scratch replacement for stock OpenXcom's real-time interception dogfight,
developed across `1aa520663` (initial), `bd855acd2` (turn-based rework + AI + armed
UFOs + escorts), and `6bcac4e5e` (crashed-craft explosion, "can escape" tuning,
always-visible minimize button). It is **off by default** in the current tree (see
"Status" below).  Full [design doc](AirCombat-Design.md) available.

### What it is

Instead of the real-time slider dogfight, interception opens a **turn-based pursuit
game** on a positional axis (close ↔ far range columns). Each combatant — player
craft, escort craft, and enemy UFOs — acts from a shared turn queue, spending **TUs
and fuel** on movement and attacks. Multiple craft and multiple enemies can be in
one engagement. *(Mechanics from `AirCombatState`/`AirCombatAI`; specific column
counts and per-turn numbers are implementation detail, not re-confirmed as balance
values.)*

### New classes & enums (verified in source)

- `AirCombatState` (`src/Geoscape/AirCombatState.{h,cpp}`) — the main turn-based
  combat screen: unit list, turn queue, animation pump, UI, minimize.
- `AirCombatAI` (`src/Geoscape/AirCombatAI.{h,cpp}`) — UFO decision logic with modes
  `enum AirAIMode { AAI_NONE, AAI_SNIPE, AAI_BERSERKER, AAI_ESCAPE }`; `canEscape()`
  weighs estimated survival time against pursuit time.
- `AirActionMenuState` (`src/Geoscape/AirActionMenuState.{h,cpp}`) — the pop-up
  action menu; `enum AirCombatActionType` includes `AA_MOVE_FORWARD`,
  `AA_MOVE_BACKWARD`, `AA_MOVE_PURSUE`, `AA_MOVE_RETREAT`, `AA_WAIT`, `AA_HOLD`,
  `AA_EVADE`, `AA_FIRE_WEAPON`, `AA_DUAL_FIRE_WEAPONS`, `AA_SPECIAL`, `AA_DISENGAGE`.
- `AirCombatUnit` (struct) — wraps a `Craft` or `Ufo`, tracking position, health,
  fuel, weapons, turn delay, and AI mode; `getCombatFuel()` / `spendCombatFuel()`.
- A family of animation helpers (`AirCombatMovementAnimation`,
  `AirCombatProjectileAnimation`, `AirCombatExplosionAnimation`,
  `AirCombatMultiShotAnimation`, …) so movement, fire, and detonation animate
  concurrently.

### New ruleset (YAML) keys

- **`RuleUfo`** — `weapons:` (list of CraftWeapon types the UFO carries) and
  `escorts:` (list of UFO types that spawn as escorts); `combatSprite` (air-combat
  sprite index). UFOs can now shoot back. *(verified in `RuleUfo.cpp`; runtime
  `Ufo::_weapons` / `_escorts`.)*
- **`RuleCraft`** — `combatSprite` (air-combat sprite index). An earlier
  `combatFuelMax` (separate combat-fuel pool) was **superseded**: commits
  `9079d7751` / `6c532b24c` made combat fuel derive from the craft's **standard
  fuel**, adding fuel-per-TU helpers on `Craft` for costing actions and computing
  total combat TUs.
- **`enableNewAirCombat`** (top-level Mod option, default **false**) — the runtime
  switch (see Status). *(working tree)*

### Status (off by default)

The feature has been gated three ways over its life, ending **disabled by default**:

1. A compile-time `#define NEW_AIR_COMBAT` in `GeoscapeState.h:39` selects the
   `AirCombatState` path over the classic `DogfightState`.
2. Commit `ca328878d` ("Disable new air combat …") disabled it for the shipped
   `xcomtd` mod by **renaming the data files** — `UFOs.rul` and `CraftWeapons.rul`
   → `*.rul_NewAirCombat`, so they no longer load and UFOs get no weapons/escorts.
   The leftover `bin/standard/xcomtd/Ruleset/CraftWeapons.rul_NewAirCombat` is still
   present in the working tree.
3. **Working tree:** the gate was reworked from the bare `#define` into a **runtime
   ruleset flag** `Mod::_enableNewAirCombat` (`enableNewAirCombat:`, default
   `false`). `GeoscapeState` now keeps **two** dogfight queues — the classic
   `_dogfights`/`_dogfightsToBeStarted` and the new
   `_newDogfights`/`_newDogfightsToBeStarted` — and routes interception to one or
   the other based on the flag.

**Verified code:** `AirCombatState`, `AirCombatAI`, `AirActionMenuState`,
`AirCombatUnit` (`src/Geoscape/`); `RuleUfo` `weapons`/`escorts`/`combatSprite`;
`RuleCraft` `combatSprite`; `Mod::_enableNewAirCombat` + `enableNewAirCombat` key
(working tree); `GeoscapeState` dual dogfight queues (working tree).

---

## 22. OpenXCOM Extended+ (OXCE+) Integration

A large pull-in of features from **OpenXCOM Extended+**, principally commits
`8bd75387f` ("Large effort to load in functionality from OpenXCOMExtended+"),
`1aa520663`, `46471234f`, `d2803555a`, `9639bdb14`, `35780266b`, and `bbca53d10`.

### Martial / basic training

- **Training facilities** improve soldiers' physical stats over time while they sit
  at base. UI states: `TrainingState`, `AllocateTrainingState` (assign soldiers,
  with sorting/stats), `TrainingFinishedState` (completion notice).
- `RuleBaseFacility` gains **`trainingRooms`** (capacity; `getTrainingFacilities()`),
  verified in `RuleBaseFacility.cpp:103`. Example `STR_TRAINING_FACILITY` added to
  the X-COM ruleset.
- `Soldier` gains training state (`setTraining()`/`isInTraining()`,
  `trainPhys(...)`); Mod option **`customTrainingFactor`** (default 100) scales the
  training rate.

### `refNode` ruleset inheritance

- New YAML key **`refNode`** lets a ruleset entry inherit from / extend another
  entry of the same type: the referenced parent is loaded first, then the child's
  fields override it. Cycle-guarded by a depth limit (`refNodeTestDeepth()`,
  ~64-deep). Wired into the generic `loadRule()` path, so it applies across rule
  types (items, crafts, facilities, armor, races, deployments, …). *(verified in
  `Mod.cpp`)*

### In-inventory armor & avatar management

- The **Inventory screen** can now change a soldier's **armor** and **avatar**
  (gender / look / look-variant) directly: `InventoryState::btnArmorClick` and
  variants open `SoldierArmorState` / `SoldierAvatarState`. `SoldierAvatar`
  (`src/Savegame/`) models the gender+look+variant combo; an armor-color combo and
  `modArmorColors` support recolouring. Default keybinds `keyInventoryArmor` /
  `keyInventoryAvatar`.

### Sortable soldier lists with stat columns

- `SoldiersState` and `CraftSoldiersState` gain **clickable sortable columns** and a
  **stats toggle** exposing full per-soldier stats. Sorting is driven by
  `SoldierSortUtil` (`SortFunctor` + per-stat getters: id/name/rank/missions/
  kills/wound-recovery/total plus every primary stat).
- **Stats ↔ Roles view toggle on the crew-selection screen** — the
  `CraftSoldiersState` (assign-soldiers-to-craft) screen has a paired
  **Show Stats / Show Roles** button (`_btnStats` "STR_SHOW_STATS" /
  `_btnRoles` "STR_SHOW_ROLES", `btnToggleStatsClick`, `_showStats`): the **Roles
  view** (default) shows the role + rank + craft-assignment columns, while the
  **Stats view** hides those and swaps in full stat columns (TU, stamina, health,
  bravery, reactions, firing, throwing, melee, …) so you can pick a crew either by
  role loadout or by raw numbers without leaving the screen.

### Item categories

- `RuleItem` gains **`categories`** (list of category ids; `getCategories()`,
  `hasCategory()`); category definitions are `RuleItemCategory`
  (`itemCategories:` → `Mod::_itemCategories`). Sell / purchase / equip screens
  filter by category when custom categories are enabled
  (`Mod::getUseCustomCategories()`), with category buttons in the UI (e.g.
  `SellState`).

### Alien inventories

- `AlienInventoryState` / `AlienInventory` show an alien's held equipment, now
  honoring **custom inventory layouts** (`RuleInventoryLayout`) and per-armor
  inventory sprites (commit `35780266b`, which also forces a full FOV recalc at the
  start of the player's turn and fixes Ufopaedia screen-scale restore from the
  battlescape).

### UFO mission retreat

- UFOs can **abandon a mission and retreat** based on damage taken: `Ufo` gains
  `_retreating` (`getRetreating()` / `setRetreating()`, saved/loaded). Pairs with
  the air-combat `AA_MOVE_RETREAT` action. *(commit `bbca53d10`; initial work.)*

### Other OXCE+ surface area present in the tree

Craft **pilots** (`CraftPilotSelectState`, `CraftPilotsState`), craft **equipment
templates** (`CraftEquipmentLoadState` / `CraftEquipmentSaveState`), a **tech-tree
viewer** (`TechTreeViewerState`, `TechTreeSelectState`), `RuleDamageType`, and
`ModScript` scripting hooks all came in with the OXCE+ merge. *(present in source;
depth not individually re-verified here.)*

**Verified code:** `RuleBaseFacility::_trainingRooms` / `trainingRooms`;
`TrainingState` family; `refNode` in `Mod.cpp`; `SoldierArmorState`,
`SoldierAvatarState`, `SoldierAvatar`; `SoldierSortUtil`; `RuleItem::getCategories`/
`hasCategory`, `RuleItemCategory`; `AlienInventory(State)`; `Ufo::_retreating`.

---

## 23. Utility Equipment Slots

A new inventory **slot type** that gives a unit one dedicated quick-access
equipment slot, separate from the hands. Introduced with the milestone commit
`53af17e8`.

- New enum value **`INV_UTILITY`** in `enum InventoryType { INV_SLOT, INV_HAND, INV_GROUND, INV_UTILITY, INV_EQUIP }` (`RuleInventory.h:35`), sized
  `UTILITY_W = 2 × UTILITY_H = 2`.
- Behaves like a hand slot for fit/cost purposes: `RuleInventory::fitItemInSlot`
  and `checkSlotInPosition` treat `INV_UTILITY` the same family as `INV_HAND`
  (single occupant, always "fits"), and inventory cost / ammo-swap / item-ownership
  logic groups `INV_HAND | INV_UTILITY | INV_EQUIP` together
  (`BattleUnit.cpp`, `BattleItem.cpp`).
- A unit's utility slot is **armor-defined**: `BattleUnit` picks up the first
  `INV_UTILITY` slot from its inventory layout into `_utilitySlot`
  (`getUtilitySlot()`), and `getUtilityItem()` returns whatever occupies it.
- The shipped `xcomtd` inventories define utility layouts (e.g.
  `STR_SCOUT_UTILITY`, `STR_HEAVY_UTILITY`, `type: 3`) with `countStats: true`,
  `allowGenericItems: false`, `allowCombatSwap: false`. *(working data)*

**Verified code:** `INV_UTILITY` / `UTILITY_W` / `UTILITY_H` (`RuleInventory.h`);
`RuleInventory::fitItemInSlot` / `checkSlotInPosition`; `BattleUnit::_utilitySlot`,
`getUtilitySlot()`, `getUtilityItem()`.

---

## 24. Other Working-Tree Changes

Beyond branding (§20) and the air-combat runtime flag (§21), the current
uncommitted working tree carries a handful of smaller behavior fixes worth noting
(these are **not yet committed**):

- **Inventory → Ufopaedia button** — `InventoryState::btnUfopaediaClick` opens the
  Ufopaedia from the inventory screen via the geoscape Ufopaedia hotkey (no-op while
  holding an item); the prior code mis-bound the handler to `GeoscapeState`.
- **Overwatch shot accounting** — `BattleUnit::getOverwatchShotsAvailable()` added;
  overwatch availability now checks `_overwatchShotsAvailable > 0`, and shot count
  is computed from the overwatch cost (max of action TU or 90% base TU) rather than
  raw action TU.
- **Zero-TU actions** — `BattlescapeGame` `haveTU()` returns true for zero-cost
  actions (fixes overwatch/zero-cost shot firing); psi actions set
  `_currentAction.ammo` for consistency, and AI passes the ammo item into
  `BA_PANIC` / `BA_MINDCONTROL` cost checks.
- **Action-menu / projectile fixes** — unprime offered only when the weapon defines
  an unprime action; clicking an in-progress ongoing action cancels targeting;
  waypoint projectiles skip the TU check while sub-firing; a missing action config
  reports `STR_NO_AMMUNITION_CONF`.
- `TileEngine.cpp` shows a very large diff that is **predominantly reformatting**
  (lambda indentation / pointer style) plus reaction-fire debug logging, not new
  behavior.

---

## 25. Stealth Equipment

DX originally also added a stealth feature where armor could grant a stealth effect, which reduced the range
at which a unit could be spotted.  

## Appendix A — New / Notable Ruleset (YAML) Keys

All keys below were verified against the `Ruleset/*.cpp` `load()` parsers.

**`RuleItem` (items / weapons / ammo)**

| Key | Default | Purpose |
|-----|---------|---------|
| `baseAccuracy` | 75 | Base weapon accuracy |
| `accuracyBurst` | 0 | Burst-fire accuracy (>0 enables the Burst mode, §1) |
| `burstShots` | 2 | Rounds fired per Burst |
| `burstRange` | 8 | Burst effective range band |
| `costBurst`/`tuBurst`/`flatBurst`/`confBurst` | — | Burst TU/energy cost, flat-cost flag, full action config |
| `accuracyShotgunSpread` | 0 | Pellet spread after the initial accuracy shot |
| `kneelModifier` | 0 | Per-weapon kneeling accuracy multiplier |
| `reactionsModifier` | 0 | Per-weapon reaction-fire modifier |
| `twoHandedModifier` | 80 | Accuracy when two-handed weapon used one-handed/dual |
| `blastDropoff` | 0 | Explosion damage falloff with distance |
| `battleClipSize` | 0 | Stock/buy/recover ammo per round; pack into magazines of this size at battle time. Non-zero forces `clipSize`→0 (§4) |
| `stats` | — | `UnitStats` granted while equipped |
| `statModifiers` | — | Percentage stat modification while equipped |
| `frontArmor`/`sideArmor`/`rearArmor`/`underArmor` | 0 | Directional armor |
| `aiRangeClose`/`Mid`/`Long`/`Max` | -1 | AI engagement bands |
| `aiAttackPriorityClose`/`Mid`/`Long`/`Max` | [] | AI target priority per band (string lists) |
| `overwatchModifier` | 100 | Overwatch reaction modifier |
| `overwatchRadius` | 2 | Overwatch trigger radius |
| `overwatchRange` | 20 | Overwatch range |
| `overwatchShot` | `"snap"` | Action used for the overwatch shot |
| `psiCostPanic`/`psiCostMindControl`/`psiCostClairvoyance`/`psiCostMindBlast` | 0 | Per-action psi costs |
| `hitEffect` / `equippedEffect` | — | Link to effects framework (§12) |
| `validSlots` (+ `_checkValidSlots`) | — | Restrict item to specific inventory slots |

**`Armor`:** `frontArmor`, `sideArmor`, `rearArmor`, `underArmor`, `statModifiers`,
`visibilityAtDark` / `visibilityAtDay` (sight range; §15 night vision),
`personalLight` (default 15; carried-light radius), `camouflageAtDark` /
`camouflageAtDay` / `antiCamouflageAtDark` / `antiCamouflageAtDay`.

**`RuleInventory` (slot):** `id`, `x`, `y`, `type`, `slots`, `costs`, `listOrder`,
`countStats`, `allowGenericItems` *(parsed but dormant, §10)*,
`allowCombatSwap`, `battleType`.

**`RuleInventoryLayout`:** `id`, `invs` (list of slot ids). Top-level section
`inventoryLayouts:`.

**`RuleSoldier`:** `inventoryLayout` (default `STR_STANDARD_INV`), `isVehicle`,
`buyCost`.

**`RuleRole`:** `name`, `iconSprite`, `smallIconSprite`, `isBlank`. Top-level
section `roles:`.

**`RuleEffect`:** `icon`, `initialComponents`, `ongoingComponents`,
`finalComponents`, `duration`, `maxStack`.

### OpenXcomDX-era keys (air combat, OXCE+, categories)

**`RuleItem`:** `categories` (list of item-category ids; §22), plus `refNode`
(inherit from another item; §22).

**`RuleUfo`:** `weapons` (CraftWeapon types carried), `escorts` (UFO types spawned
as escorts), `combatSprite` — air-combat support (§21).

**`RuleCraft`:** `combatSprite` (air-combat sprite). *(`combatFuelMax` was an
earlier separate-pool key, since superseded by standard fuel — §21.)*

**`RuleBaseFacility`:** `trainingRooms` (martial-training capacity; §22).

**`RuleItemCategory`** (top-level `itemCategories:`): item-category definitions used
by category-filtered base screens (§22).

**Top-level / Mod options:** `enableNewAirCombat` (bool, default false; §21),
`customTrainingFactor` (int, default 100; §22), `refNode` (generic ruleset
inheritance, all rule types; §22).

**Options (night vision, `STR_OXCE` group; §15):** `fullNightVision`,
`autoNightVision`, `nightVisionColor` (default 8), keys `keyNightVisionToggle`
(ScrollLock) / `keyNightVisionHold` (Space).

---

## Appendix B — Key New Classes & Enums (verified in source)

| Symbol | Location | Notes |
|--------|----------|-------|
| `RoleMenuState`, `RoleChangeState` | `src/Battlescape/` | Role UI |
| `RuleRole` / `Role` | `Ruleset/` / `Savegame/` | Role rule + per-save state |
| `RuleEffect` / `BattleEffect` / `EffectComponent` | `Ruleset/` / `Savegame/` | Effects framework |
| `EC_CIRCULAR_LIGHT` / `EC_DIRECTIONAL_LIGHT` | `Mod/RuleEffect.h` | Light-emitting effect components (§15) |
| `Map::_targetingProjectile` | `src/Battlescape/Map` | Live aim/throw trajectory preview (§1) |
| `RuleInventoryLayout` | `Ruleset/` | Per-unit inventory structure |
| `InventoryLoadState` / `InventorySaveState` | `src/Battlescape/` | Named global loadout library UI (§10) |
| `SavedGame` global layouts | `Savegame/SavedGame` | `getGlobalEquipmentLayout`/`Name` (20 slots), `getGlobalCraftLoadout` (10 slots) (§10) |
| `EquipmentLayoutItem` | `Savegame/` | Stored loadout entry (item, slot, pos, ammo, fuse) used by templates/roles (§5, §10) |
| `CombatLog` | `src/Battlescape/` | Floating combat log |
| `Vehicle` | `Savegame/` | Vehicle support (also Ufopaedia vehicle articles) |
| `enum BattleActionType` | `BattlescapeGame.h` | Adds `BA_MINDCONTROL`, `BA_OVERWATCH`, `BA_CLAIRVOYANCE`, `BA_MINDBLAST`, `BA_DUALFIRE`, `BA_BURSTSHOT`, `BA_RELOAD`, `BA_RETHINK` |
| `enum MovementAction` | `Pathfinding.h` | `MV_WALK`, `MV_SPRINT`, `MV_SNEAK`, `MV_STRAFE` |
| `BattleUnit` overwatch members | `Savegame/BattleUnit.h` | `_overwatch`, `_overwatchTarget`, `_overwatchWeaponSlot`, `_overwatchShotsAvailable` |
| `BattleUnit` bleedout members | `Savegame/BattleUnit.h` | `_bleedingOut`, `checkStartBleedout()`, `getCanBleedOut()`, `getBleedingOut()` |
| `BattleUnit` reaction split | `Savegame/BattleUnit.h` | `getReactionScore(bool overwatch)`, `getEvasionScore()` |
| `BattleUnit::recoverTimeUnits()` | `Savegame/BattleUnit.cpp` | TU recovery (encumbrance, leg wounds, overwatch/MC upkeep) |
| `Game::getGame()` | `Engine/Game` | Static singleton accessor (supports effects) |
| `AirCombatState` / `AirCombatAI` / `AirActionMenuState` | `src/Geoscape/` | Turn-based air-combat minigame (§21) |
| `AirCombatUnit` + animation structs | `src/Geoscape/AirCombatState.h` | Combatant wrapper + concurrent fire/move/explosion animations (§21) |
| `enum AirCombatActionType` | `AirActionMenuState.h` | `AA_MOVE_FORWARD/BACKWARD/PURSUE/RETREAT`, `AA_WAIT`, `AA_HOLD`, `AA_EVADE`, `AA_FIRE_WEAPON`, `AA_DUAL_FIRE_WEAPONS`, `AA_SPECIAL`, `AA_DISENGAGE` |
| `enum AirAIMode` | `AirCombatAI.h` | `AAI_NONE`, `AAI_SNIPE`, `AAI_BERSERKER`, `AAI_ESCAPE` |
| `TrainingState` / `AllocateTrainingState` / `TrainingFinishedState` | `src/Geoscape/` | Martial-training UI (§22) |
| `SoldierArmorState` / `SoldierAvatarState` / `SoldierAvatar` | `Basescape/` & `Savegame/` | In-inventory armor & avatar management (§22) |
| `SoldierSortUtil` | `src/Basescape/` | Sortable soldier-list comparators (§22) |
| `RuleItemCategory` | `src/Mod/` | Item categories (§22) |
| `AlienInventoryState` / `AlienInventory` | `src/Battlescape/` | Alien inventory display w/ custom layouts (§22) |
| `INV_UTILITY` (`enum InventoryType`) | `RuleInventory.h` | Utility equipment slot type (§23) |
| `BattleUnit::getUtilitySlot()` / `getUtilityItem()` | `Savegame/BattleUnit` | Per-unit utility slot (§23) |
| `Ufo::_retreating` | `Savegame/Ufo` | UFO mission retreat (§22) |
| `Mod::_enableNewAirCombat` | `src/Mod/Mod` | Runtime air-combat gate (§21, working tree) |

---

## Appendix C — Commit Index

| Date | Commit | Summary |
|------|--------|---------|
| 2015-06-10 | `0f412b1` | **★ Squashed overhaul history — source of §§1–18 (983-line changelog)** |
| 2015-06-10 | `810bffa` | Reorg config/data into new folders |
| 2015-06-10 | `41fcc3f` | Post-rebase fixes |
| 2015-06-10 | `ad3a5bf` | Move XXCO resources into mod directory |
| 2015-06-25 | `769f1e5` | Language and resource fixes |
| 2015-07-23 | `8932d67` | Add missing INVENTORY string |
| 2015-08-10 | `8e217f0` | Move XXCO ruleset files; language fixes |
| 2015-08-14 | `2dafddb` | Rebase fix |
| 2015-08-16 | `63b3774` | Geoscape Score; Funds always on; sidebar; inventory layout & soldier fund cost |
| 2015-08-16 | `d3016e0` | Default weapon accuracy = 75 |
| 2015-08-17 | `7ae1a0a` | Remove `x50` from cannon round strings |
| 2015-08-17 | `387501a` | Migrate to new `isAmmo` functions |
| 2015-08-21 | `1c00224` | Includes/forward-declaration cleanup |
| 2015-08-21 | `93b8c2f` | `recoverTimeUnits()` TU-recovery overhaul (encumbrance, leg wounds, overwatch/MC upkeep) |
| 2015-08-22 | `1af11d7` | OXP sprites in xcom2; TFTD inventory button sprites |
| 2015-08-22 | `fd14930` | TFTD roles config; TFTD OXP stat colors; inventory stacking fix |
| 2015-08-22 | `66c477e` | TFTD interface elements; action menu pos; role before rank; explosion-less projectile fix |
| 2015-08-23 | `7baca6a` | Pathing updated when standing from kneel |
| 2015-08-23 | `69c5219` | Removed explicit colors from Role states (TFTD) |
| 2015-08-23 | `023b5be` | Role menu/role-change interface rulesets; TFTD role-menu fixes |
| 2015-08-24 | `c68aeee` | Title → "OpenXcom+" |
| 2015-08-24 | `1b69afd` | Rebasing cleanup |
| 2015-08-25 | `558da50` | Globe Palette setting may be 0 (clears water fill) |
| 2015-08-26 | `468f983` | Rebase fixes |

### OpenXcomDX era (post-`53af17e8`)

These commits come after the milestone `53af17e8` and are the source of §§21–24. Dates are approximate (2017+).

| Commit | Summary |
|--------|---------|
| `53af17e8` | **★ Milestone — contains all of §§1–18 plus utility slots (§23), shot-menu TU/Acc red highlights, auto-shot count display, wounded-days status** |
| `bbca53d1` | UFOs can abandon/retreat from a mission based on damage (§22) |
| `d793c69e` | Build platform → VS 2017 (xbrz scaler) |
| `1d4bc747` | Quick Reload fix |
| `d2803555` | Integrated **basic/martial training** from OXCE+ (§22) |
| `9639bdb1` | Martial-training language/interface + example training facility (§22) |
| `46471234` | Soldier-list **sorting + stats**, in-inventory **armor/avatar** management, **`refNode`** ruleset loading (§22) |
| `8bd75387` | **Large OXCE+ functionality merge** (§22) |
| `1aa52066` | Source aligned to OXCE+ base; **initial new air-combat minigame** (§21) |
| `bd855acd` | **Turn-based air-combat rework** + enemy AI + armed UFOs + escort UFOs (§21) |
| `6bcac4e5` | Crashed/destroyed-craft explosion; **item categories** (§22); air-combat "can escape" tuning + always-visible minimize button (§21) |
| `35780266` | Alien inventory images; AlienInventory custom layouts; FOV recalc at turn start; Ufopaedia scale fix (§22) |
| `aa4570be` | New Battle mode can save role-equipment changes |
| `9079d775` | Use standard craft fuel as **combat fuel** (§21) |
| `6c532b24` | Combat-fuel cleanup; fuel-per-TU / total-combat-TU helpers on craft (§21) |
| `a915a301` / `ccd3b95d` / `df87bae6` | Re-sync with upstream OpenXcom / SupSuper (through 2017-11-10) |
| `ca328878` | **Disable new air combat** by renaming its ruleset files; minor interface tweaks (§21) |
| `898db5c7` | Globe ruleset color cleanup (current `HEAD`) |
| *(working tree)* | Rebrand → **OpenXcomDX** (§20); air-combat moved to runtime `enableNewAirCombat` flag + dual dogfight queues (§21); inventory→Ufopaedia button; overwatch/zero-TU fixes (§24) |

> **Re-implementation tip:** Because §§1–18 derive from a squashed changelog, treat
> this doc as a *map*. For any feature you port, `git log -p --follow` the relevant
> file (e.g. `BattleUnit.cpp`, `ProjectileFlyBState.cpp`, `RuleItem.cpp`) to read
> the actual implementation rather than the commit prose.

---

## 24. Geoscape Activity Display

An at-a-glance overlay on the left side of the Geoscape screen showing all ongoing
operations across the player's bases, eliminating the need to enter individual base
screens just to check progress.

### Purpose

The Activity Display consolidates research, manufacturing, craft maintenance, and
fuel storage status into a single always-visible panel on the Geoscape. Idle
personnel and crafts requiring attention are highlighted with reversed colors so
the player's eye is drawn to issues that need resolution.

### Location

Displayed on the left side of the Geoscape screen, overlaying the globe view.

### Sections

The display is divided into four sections:

#### Resources

Shows alien fuel quantities stored at each base. Only bases containing fuel are listed.

**Format:** `[Base Number] Amount FuelType`

#### Research

Lists active research projects at each base with their current progress values.
Bases with idle scientists (no research assigned) show a "No Research" alert in
reversed colors.

**Format:** `[Base Number] ProjectName: ProgressValue`

#### Manufacturing

Displays items currently being produced at each base, including production count and
estimated time remaining. Items marked for sale are indicated with a `$` symbol.
Bases with idle engineers show a "No Manufacturing" alert in reversed colors.

**Format:** `[Base Number] ItemName: Produced/Total (TimeRemaining)`

#### Craft

Shows crafts that require maintenance (repairs, refueling, or rearming) with
estimated completion time. Only crafts stationed at bases (not flying) are included.
This section only appears when there are crafts needing attention — highlighted in
reversed colors.

**Format:** `[Base Number] CraftName: Status (TimeRemaining)`

### Visual Indicators

- **Normal text:** Active tasks making progress
- **Reversed colors:** Idle personnel or crafts requiring attention — draws the player's eye to issues that need resolution

### Time Format

Estimated times are shown as:
- Hours (`h`) for durations under one day
- Days (`d`) for longer durations

### Update Behavior

The display refreshes automatically whenever game time advances. Update frequency
varies with the selected time speed setting (ranging from every 5 game seconds to
every game day).

### Base Numbering

Bases are referenced by sequential number (`[1]`, `[2]`, etc.) rather than name to
keep entries concise and readable.

> **Re-implementation tip:** This feature reads data already tracked by the engine
> (`Base` research/manufacturing progress, `Craft` maintenance timers, fuel stores).
> The implementation challenge is UI layout — fitting a multi-section text overlay on
> the Geoscape without obscuring the globe or existing sidebar. Check how
> `GeoscapeState` composes its widgets and whether the left side has sufficient
> clear space across zoom levels.
