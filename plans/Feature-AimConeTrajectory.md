# Feature: Aim-Cone Trajectory Model

**Status:** **Implemented (Jul 2026)** — core model shipped per the *Resolved design decisions*
below; pending in-game validation. The supporting UI calculators (effective-range readout, hover
hit-chance) are separate Phase 5 items and are **not** implemented yet.

**Implementation notes (what shipped, and where):**
- `RuleItem::_baseAccuracy` (`baseAccuracy` ruleset key, default `0`) — the opt-in sentinel;
  shown in Stats for Nerds ([RuleItem.cpp](../src/Mod/RuleItem.cpp)).
- `RNG::boxMuller(mean, stddev)` — Box–Muller Gaussian sampler on the seeded battle stream
  ([RNG.cpp](../src/Engine/RNG.cpp)).
- `Projectile::useAimCone()` / `applyAimCone()` — the model itself; tuning constants with full
  provenance comments at the top of [Projectile.cpp](../src/Battlescape/Projectile.cpp), the
  deflection helpers (`sampleConeAngle`, `rotateVectorRandomly`, `AimVector`) alongside.
  `calculateTrajectory` branches to it; the deflected ray's max-range endpoint is stored in
  `_targetVoxel` so `recalculateImpact()` works unchanged. The no-LOS check was factored into
  `getNoLOSAccuracyPenaltyFactor()`, shared by both paths (native behavior preserved
  bit-identically via a `!= 100` guard).
- Shotgun volley sharing via `Projectile::set/getConeTrueAim()`; the pellet loop in
  [ProjectileFlyBState.cpp](../src/Battlescape/ProjectileFlyBState.cpp) presets each pellet with
  the lead's soldier-cone roll (gated on `hasConeTrueAim()`, so scatter-model shotguns keep the
  behaviorType/choke logic untouched).
- No new files; no language strings needed (no UI surface yet).

## Motivation

DX wants direct fire to feel like *aiming a weapon down a line*, not *rolling a hit/miss and then
teleporting a miss somewhere nearby*. The native OpenXcom/OXCE model computes a single scatter
radius from accuracy and jitters the **target point**; the bullet then flies at that displaced
point. This has two consequences DX considers undesirable:

1. **Non-physical misses.** A "miss" picks a new aim voxel and the round flies dead-straight at
   *that*, so misses cluster in a flat disc around the target rather than fanning out from the
   muzzle. Two soldiers with the same accuracy firing from 2 tiles and 20 tiles away scatter over
   the same voxel radius at the target (before range dropoff), which reads oddly.
2. **No separation of shooter vs. weapon error.** A single accuracy number folds soldier skill and
   weapon precision together, so a laser-accurate weapon in shaky hands and a sloppy weapon in
   expert hands are indistinguishable.

The **aim-cone** model (ported conceptually from the OpenXcom+ lineage — see
`Legacy-DX-Features.md` §1) replaces direct fire with a **3D direction-vector cone**: build the
ideal muzzle→target ray, then deflect that *ray* by two independent random angular cones (soldier
error + weapon error) and trace the shot down the deflected line until it hits something. Error now
grows naturally with distance, and the two cones let a weapon's intrinsic precision be tuned
independently of the shooter. It is also the prerequisite for the rest of Phase 5 (accuracy
modifiers, live tracer preview, hover accuracy readout, effective-range action-menu readout) and
for the Phase 9 per-weapon AI engagement bands.

## OXCE / OXCE-Plus audit — the native accuracy logic (for comparison)

This is what ships today. Understanding it precisely matters because the aim-cone is a **replacement
for one specific stage** (the direct-fire target deviation), and everything around that stage —
accuracy computation, range dropoff, LOS penalty, throwing — must keep working.

### Where accuracy comes from — `BattleUnit::getFiringAccuracy`
[src/Savegame/BattleUnit.cpp:2550](../src/Savegame/BattleUnit.cpp#L2550)

A single integer percentage is produced per shot:

```
result = accuracyMultiplier(attack) * accuracy<Mode>() / 100     // Snap/Aimed/Auto/Burst/Melee/Throw/CQB
       * (kneeling ? kneelBonus/100 : 1)                          // e.g. 1.15
       * (twoHanded && other hand occupied ? oneHandedPenalty/100 : 1)   // e.g. 0.8
       * accuracyModifier / 100                                   // health + fatal-wound penalty
```

- `getAccuracyMultiplier(attack)` ([RuleItem]) is the **soldier-skill term**: by default it scales
  with the firer's Firing stat (via `RuleStatBonus`), but it is fully mod-scriptable. This is the
  number the aim-cone will treat as `soldierAcc`.
- `accuracySnap` / `accuracyAimed` / `accuracyAuto` / `accuracyBurst` are the **per-mode weapon
  percentages** (the classic "60% snap" figures).
- `getAccuracyModifier` ([BattleUnit.cpp:2621](../src/Savegame/BattleUnit.cpp#L2621)) folds in
  health and fatal wounds: `max(10, 25*health/maxHealth + 75 - 10*wounds)` where head wounds always
  count and arm wounds count if the relevant hand holds the weapon.

The result is divided by an `accuracyDivider` (100.0) at the call sites in `ProjectileFlyBState`
([ProjectileFlyBState.cpp:473,506,542,546](../src/Battlescape/ProjectileFlyBState.cpp#L473)) to
produce the `double accuracy` in `[0,1+]` passed into the projectile.

### The direct-fire deviation — `Projectile::applyAccuracy`
[src/Battlescape/Projectile.cpp:375](../src/Battlescape/Projectile.cpp#L375)

This is **the stage the aim-cone replaces for direct fire.** Called from
`Projectile::calculateTrajectory` ([Projectile.cpp:201](../src/Battlescape/Projectile.cpp#L201))
after an initial line-of-fire sanity trace. The model:

1. **Range dropoff.** Compute tile distance and apply `calculateLimits` (below): accuracy is
   reduced by `dropoff * (distance − upperLimit)/100` beyond effective range (or below `minRange`).
   `BA_HIT` melee is exempt.
2. **LOS penalty.** If the weapon defines `noLOSAccuracyPenalty != -1` and the target tile isn't in
   line of sight, `accuracy *= noLOSAccuracyPenalty/100`. (Aimed-shot penalty in vanilla; DX will
   need to re-derive an equivalent for the cone model.)
3. **Scatter radius from accuracy** — the "hit-or-wild-miss" core:
   ```
   deviation = RNG(0,100) - accuracy*100;
   deviation += (deviation >= 0) ? 50 : 10;      // "miss cloud" gets a big bump
   deviation = max(1, zShift * deviation / 200); // scaled by a range/axis ratio
   ```
   A roll under the accuracy threshold yields a tight deviation (≈ +10 → radius 1); a roll over it
   snaps to a wide "miss cloud" (+50). `zShift`/`xyShift` are axis-weighted range terms
   ([Projectile.cpp:403-428](../src/Battlescape/Projectile.cpp#L403)) with two variants selected by
   `Options::oxceUniformShootingSpread` (uniform-disc vs. the classic non-uniform "Commandment"
   spread).
4. **Displace the target voxel.** `target->x/y/z += RNG(±deviation)`. With
   `oxceUniformShootingSpread`, the offset is re-rolled up to 10× to land inside a disc (rejecting
   square corners); otherwise it's a raw box.
5. **Extend the line.** If `extendLine`, recompute rotation/tilt from origin→displaced-target and
   project the endpoint out to `maxRange` (16000 voxels) so the shot continues past the target on a
   miss. The trajectory is then traced by `TileEngine::calculateLineVoxel`
   ([Projectile.cpp:204](../src/Battlescape/Projectile.cpp#L204)).

**Key characteristic:** the *direction* is always origin→(displaced target). The round flies
straight at a jittered point. There is no angular cone; deviation is a target-space radius.

### Range dropoff — `RuleItem::calculateLimits`
[src/Mod/RuleItem.cpp:2443](../src/Mod/RuleItem.cpp#L2443)

`upperLimit`/`lowerLimit` default to `aimRange`/`minRange`. With `Options::battleUFOExtenderAccuracy`
on, snap uses `snapRange`, auto uses `autoRange`, throw uses `throwDropoffRange`. Returns the
`dropoff` value (per-tile accuracy loss). This range machinery is orthogonal to the deviation model
and should be **reused** by the aim-cone (feeding the effective-range readout).

### Special cases that must survive the swap

- **Throwing (`BA_THROW`) and arcing shots** — use `Projectile::calculateThrow`
  ([Projectile.cpp:255](../src/Battlescape/Projectile.cpp#L255)), a parabola with its own
  `applyAccuracy(..., keepRange=true)` deviation and a strength/weight curvature model in
  `TileEngine::validateThrow`. **The aim-cone does not touch throwing** — it stays on the scatter
  model. (Legacy-DX-Features.md §1 confirms throwing was intentionally left on the legacy path.)
- **`BA_LAUNCH` guided missiles** — hard-code accuracy 0.55/0.60 by faction and drift via the same
  `applyAccuracy`. **Resolved: launch stays on the scatter model** — waypoint legs fire with
  `extendLine = false` and homing semantics don't fit a fly-down-a-ray model. Revisit only if it
  feels wrong in play.
- **Shotgun pellets** — DX already flies pellets as individual concurrent projectiles
  ([ProjectileFlyBState.cpp:649-659](../src/Battlescape/ProjectileFlyBState.cpp#L649)), but each
  pellet currently calls `calculateTrajectory` with a **reduced per-pellet accuracy** feeding the
  *native scatter model* — not a boxMuller cone. Under the aim-cone this converges on a single
  per-pellet weapon-cone deflection (the *Weapon cone* formula in *Mechanics*), replacing the
  diminishing-accuracy trick. **Resolved: the per-pellet cone is scaled by the ammo's
  `shotgunSpread`** (buckshot vs. slug from the same gun still patterns differently; exact mapping
  settled during tuning), while **`shotgunChoke` is subsumed by `baseAccuracy`** — choke is already
  a flat per-weapon pattern-tightness multiplier ([ProjectileFlyBState.cpp:574-583](../src/Battlescape/ProjectileFlyBState.cpp#L574)),
  i.e. the same axis as intrinsic weapon precision, so an opted-in tight-choked shotgun just sets a
  higher `baseAccuracy`. The cone path ignores choke (document this on the field); scatter-model
  shotguns (`baseAccuracy: 0`) keep both fields working exactly as today.
- **`recalculateImpact`** ([Projectile.cpp:251](../src/Battlescape/Projectile.cpp#L251)) re-traces a
  *stored* trajectory against changed terrain. It re-runs `calculateLineVoxel` from the stored
  origin to `_targetVoxel`. For the cone model the stored `_targetVoxel` must be the **already
  deflected, max-range-extended** endpoint of the ray — then the existing `recalculateImpact`
  works **unchanged** (no port of the legacy `recalculateTrajectoryFromVector` needed).
- **Live trajectory preview** (`Projectile::calculatePreviewTrajectory`,
  [Projectile.cpp:215](../src/Battlescape/Projectile.cpp#L215)) — already traces the ideal
  undeviated muzzle→target ray, which *is* the cone's central axis. **No change needed** under the
  cone model.

### What is NOT present yet (confirmed)

A grep for `boxMuller`, `rotateVectorRandomly`, `calculateLineFromVector`, `calculateEffectiveRange`,
and `calculateChanceToHit` finds them **only** in `Legacy-DX-Features.md` — none exist in `src/`.
So the aim-cone is a genuine port/re-implementation, not a wire-up of dormant code. The
`DVec3`/vector-rotation helpers, the effective-range calculators, and the chance-to-hit estimator
all need to be (re)written.

## Target design — the aim-cone model

> **Naming note:** the function/helper names below (`boxMuller`, `rotateVectorRandomly`,
> `calculateLineFromVector`, `calculateEffectiveRange`, `calculateChanceToHit`, `DVec3`, …) are
> carried over from the legacy OpenXcom+ fork purely to describe the *behavior*. The DX
> implementation is free to name these whatever fits the current codebase's conventions — treat
> the legacy names as descriptive, not prescriptive.

### Conceptual model

The system assumes **two latent sources of inaccuracy** that combine per projectile:

1. **Soldier aim (the "true" trajectory).** The shooter first picks an intended line. This is the
   *soldier cone*: a deflection off the ideal muzzle→target ray driven by an **effective soldier
   accuracy**. Everything that describes the *person's* aim folds into this one number:
   - **Soldier Firing Accuracy** (base skill),
   - **Shot-Type Accuracy** — the Snap/Aimed/Auto/Burst factor *modifies the soldier accuracy* (a
     hurried snap or full-auto spray degrades the shooter's aim; a deliberate aimed shot improves
     it),
   - **Kneeling** bonus,
   - **One-handed penalty** (firing a two-handed weapon with the other hand occupied),
   - **Fatal wounds / health** penalty (modifies soldier accuracy).
2. **Weapon deflection.** Off that intended line, each *projectile* then deviates by the *weapon
   cone*, driven **only** by the weapon's intrinsic accuracy — independent of the shooter, the shot
   type, and everything in stage 1.

**Per shot-mode behavior** (this is the important part):

- **Single shot** — one soldier deflection, then one weapon deflection.
- **Shotgun** — the soldier picks **one** true aim line for the whole volley (a single soldier
  deflection), then **each pellet independently deviates by the weapon cone** off that line. So all
  pellets share the shooter's aim error but scatter individually by weapon spread.
- **Auto / Burst** — **each round re-rolls the full stack**: its own soldier deflection *and* its
  own weapon deflection. Every round is independently subject to everything (shot-type factor,
  kneel, wounds, etc.), so a burst walks around the target rather than flying as a rigid group.

### Mechanics

Replaces steps 3–5 of `applyAccuracy` **for direct fire only**. Conceptually
(`Projectile::calculateTrajectory`):

1. **Direction vector.** Build `DVec3 direction = normalize(targetVoxel − originVoxel)`.
2. **Two independent angular deflections** (skipped when `force`/`ignoreAccuracy`, e.g. scripted
   perfect shots):
   - **Soldier cone** — `angle = boxMuller(0, 0.437 / (soldierAcc² / 50) · 1.4826 · 2)`, applied via
     `rotateVectorRandomly(direction, angle)`. `soldierAcc` is the *effective soldier accuracy*
     above (firing skill × shot-type × kneel × one-handed × wounds). It is **squared** in the
     denominator, so high effective aim tightens the cone sharply. **Applied once per round** — for
     shotguns, once for the whole volley.
   - **Weapon cone** — `angle = boxMuller(0, 0.437 / (weaponAcc² / 75) · 1.4826)`, driven only by
     the weapon's intrinsic accuracy (`baseAccuracy`) and *nothing else*. **Applied per projectile**
     (per pellet for shotguns). The two cones **stack**: shooter error then weapon error.
     **DX deviation from legacy:** the legacy fork scaled this cone *linearly*
     (`0.437 / weaponAcc · 1.4826`); DX squares `weaponAcc` and normalizes at 75, so
     `baseAccuracy: 75` (the legacy default) behaves identically to legacy while values away from
     75 spread much harder. See decision 11 for the calibration data behind this.
   - `boxMuller(mean, stddev)` = a Gaussian/normal sample (Box–Muller transform); `1.4826` is the
     MAD→σ consistency constant; `0.437` is the tuning constant carried over from the legacy fork.
   - **Percent scale:** both accuracies enter these formulas as *percent-scale* integers (60, not
     0.60). The existing call sites divide by `accuracyDivider` (100) before passing accuracy into
     the projectile — the cone path must receive the **undivided** values, or `soldierAcc²/50`
     silently produces a ~180-radian stddev.
   - **Floors:** clamp effective `soldierAcc` to **≥ 20** (legacy's hard floor — the constants were
     calibrated against it; at 20 the soldier-cone stddev is ~3.7°) *after* all multipliers,
     including the no-LOS penalty. Guard `weaponAcc` to **≥ 1**.
   - **Tail clamp:** each sampled deflection angle is clamped at **3σ of its own cone** — scale-free
     (good shooters stay tight, bad shooters stay wild), keeps 99.7% of the distribution untouched,
     and prevents freak outliers from exiting sideways/backwards.
   - **RNG determinism:** the `boxMuller` sampler must draw from the battle `RNG` stream (not a
     separate generator) so seeded saves stay reproducible.
   - **Range dropoff: none.** The cone path applies **no linear range dropoff** — distance falloff
     comes purely from cone geometry (a fixed angular error covers more voxels the further it
     travels). The `aimRange`/`snapRange`/`autoRange`/`minRange`/`dropoff` fields feed only the
     effective-range readout for opted-in weapons. (With default field values and
     `battleUFOExtenderAccuracy` off, dropoff never triggers today anyway — `aimRange` defaults to
     200 tiles, larger than any map — so this matches default-settings vanilla behavior.)
   - **No-LOS penalty:** `noLOSAccuracyPenalty` multiplies into effective `soldierAcc` (before the
     floor-20 clamp) — "can't see it, your aim suffers"; weapon precision is untouched. The
     existing ruleset field is reused unchanged.
3. **Trace as a ray.** `direction *= 16000`, then
   `TileEngine::calculateLineFromVector(origin, direction, ...)` flies the bullet down the deflected
   line until impact — no target-point homing. `recalculateTrajectoryFromVector()` re-traces the
   stored vector when a mid-flight obstacle is removed.

### Supporting calculators (needed by Phase 5 UI, build alongside)

- `Projectile::calculateEffectiveRange(soldierAcc, baseAccuracy, shotgunSpread)` — **implemented
  (Jul 2026)**, feeds the action-menu **effective-range** readout. Returns the distance (tiles) at
  which the combined soldier+weapon cone lands a shot on a standard standing target **half the time,
  in the open** (the legacy 50%-hit-rate definition). It is a property of shooter+weapon+shot-mode
  only — target/terrain-independent, no voxel tracing — so it can be shown before a target is picked.
  See *Effective-range readout* below.
- `Projectile::calculateChanceToHit(...)` — estimated hit chance **plus** a cover-reduction term,
  for the hover readout `<acc>% (-<cover>%) @ <distance>` (shown even without UFOExtender mode,
  color-graded red→green). See `Feature-ActionMenuRevamp.md` (effective-range readout was deferred
  here) and the Phase 5 "Hover Accuracy Readout" item.
  **Implemented (Jul 2026)** as `Projectile::calculateHitChancePercent(attack, mod, distanceTiles,
  targetUnit, hasLOS)` — the crosshair hit-chance half. See *Hover hit-chance readout* below. The
  cover-reduction term and the `calculateEffectiveRange` 50%-range readout remain TODO.

### Hover hit-chance readout — implemented (Jul 2026)

For cone-model weapons the aiming crosshair now shows the **physical probability the shot lands on
the target**, replacing the native folded-accuracy number (which for cone weapons is only the
soldier-cone *input*, and whose dropoff/range terms don't even apply on the cone path).

- **Method — Monte-Carlo that voxel-traces each sample against real terrain (cover-aware).**
  `calculateHitChancePercent` stacks the *same two cones the real shot uses* (via the shared
  `soldierConeSigma` / `weaponConeSigma` helpers), and for each sampled shot it deflects the ideal
  muzzle→target ray, extends it to max range, and runs the **real** `TileEngine::calculateLineVoxel`
  trace. A trial counts as a hit using the **engine's own line-of-fire test**, mirrored exactly from
  `Projectile::calculateTrajectory` / `TileEngine::canTargetUnit`: the impact voxel is `trajectory[0]`
  (traced with `storeTrajectory=false`); a `V_UNIT` impact whose tile has no unit is dropped one tile
  (tall/floating units); and it's a hit iff that impact **tile equals the aimed-at tile**
  (`action->target`). (An earlier attempt read `trajectory.back()` and compared unit *pointers*,
  which misfired — reading ~0% on makeable shots — whenever the cursor tile's unit was null or on a
  different tile than the impact. Matching the engine's tile comparison fixed it.) This means:
  - **Cover is exact, not approximated.** A wall/object between shooter and target blocks shots just
    as in play (fixing the earlier bug where a target behind a wall still read a high %), and
    *partial* cover — only the target's head exposed, say — reduces the estimate because the rays
    toward the covered part hit terrain first.
  - **The silhouette is the unit's real voxel model,** not a rectangle. (An earlier version sampled a
    stance/size rectangle in the 2D tangent plane; the voxel trace supersedes it and also handles
    obstruction, so the rectangle approximation was dropped.)
  - Origin and aim voxels come from the same `getOriginVoxel` / `resolveFireTargetVoxel` resolution
    the actual shot uses, so the traced rays start and point where real fire would.
  - A closed-form isotropic-Gaussian approximation was tried and **rejected**: each cone's tangent
    offset is `θ·(cosφ,sinφ)` with `θ` a *half-normal* radius (not a 2D Gaussian), so the true
    distribution is much more centrally peaked and the Gaussian formula under-read the hit chance by
    up to ~10 points in wide-cone cases.
- **Shotgun-aware.** A volley shares one soldier roll, then each pellet rolls its own weapon cone and
  traces independently; the readout counts a trial as a hit if **any** pellet reaches the target (so
  buckshot reads higher than a single slug up close).
- **Deterministic & side-effect-free.** The sampler uses a private `RNG::RandomState` seeded from the
  quantized aim geometry + cones, so the number is stable for a given aim (no frame flicker) and
  **never draws from the game RNG stream** (no effect on actual shots or save determinism). A copy of
  the action is used for voxel resolution so the live action is never mutated.
- **Performance.** Tracing is far heavier than arithmetic, so the result is **cached in `Map`** and
  recomputed only when the aim changes (cursor tile / ctrl / weapon / action type), not every frame.
  A shared trace budget (~600 traces) keeps a shotgun volley roughly as cheap as a single shot
  (fewer volley-trials, `pellets` traces each) rather than costing `pelletCount`× more.
- **Inputs mirror the shot.** Effective soldier accuracy = `getFiringAccuracy`; no line of sight
  widens the soldier cone (same `noLOSAccuracyPenalty` factor); out-of-range reads `0%`.
- **UI.** Rendered by the existing crosshair `_txtAccuracy` text in `Map.cpp`, color-graded
  red (<35%) → yellow (35–64%) → green (≥65%). Shown even when `battleUFOExtenderAccuracy` is off,
  as long as the player hasn't disabled crosshair info (`oxceShowAccuracyOnCrosshair != 0`).
- **Not yet done:** the *explicit* cover-reduction term `(-<cover>%)` (cover is folded into the
  single % today rather than shown separately) and the `@ <distance>` suffix.

### Effective-range readout — implemented (Jul 2026)

The battlescape **action menu** now shows each cone-weapon shot mode's **50%-hit effective range**
(in tiles) in place of the accuracy `%`. For a cone weapon the per-mode "60% snap / 110% aimed"
figure is only the *soldier-cone input*, not a hit chance, so it's replaced by the number that
actually means something: how far that mode stays reliable. (Vanilla `baseAccuracy: 0` weapons keep
the accuracy `%` unchanged.)

- **Calculator — `Projectile::calculateEffectiveRange(soldierAcc, baseAccuracy, shotgunSpread)`.**
  Target/terrain-independent (no tracing): it samples the same two cones (`soldierConeSigma` /
  `weaponConeSigma`) against the model's standard standing-soldier silhouette and returns the 50%-hit
  distance. It avoids any search via a clean identity: for one sampled shot with small-angle tangent
  offset `(ax, az)` radians, the shot stays on the `W×H` silhouette out to `dMax = min(halfW/|ax|,
  halfH/|az|)`; a shot hits at distance `d` iff `dMax ≥ d`, so `P(hit at d)` crosses 0.5 exactly at
  the **median of the per-sample `dMax` values**. So the effective range is just that median — no
  bisection, monotonic by construction. Validated against `reference/aimcone_montecarlo.py`'s
  effective-range table to within ~0.5 tile (e.g. Rookie snap ≈ 6, Average snap ≈ 13, Veteran aimed
  ≈ 49, Elite aimed ≈ 63). Deterministically seeded, so the readout is stable per shooter/weapon/mode.
- **UI.** In `ActionMenuState::addItem`, cone direct-fire modes (snap/aimed/auto/burst) set the
  accuracy slot to `STR_EFFECTIVE_RANGE_SHORT` (`"Rng:{N}"`) instead of `STR_ACCURACY_SHORT`. Uses
  the per-mode `getFiringAccuracy` as `soldierAcc` (so each mode's kneel/one-hand/wound/shot-type
  factors are reflected) and the ammo's `shotgunSpread` for multi-pellet weapons.

## Vanilla weapon compatibility & code-path swap

**Both firing models coexist; the model is selected per weapon by the new `baseAccuracy` field, so
nothing changes until a weapon opts in.** This is the primary migration mechanism — a global option
(step 7) is secondary.

### The swap rule

- **`baseAccuracy == 0` (default) → native scatter model, unchanged.** The weapon runs the exact
  current path (`Projectile::calculateTrajectory` → `applyAccuracy`). Byte-for-byte identical
  behavior; no calibration, no balance drift. **All vanilla / existing-mod weapons are in this bucket
  by default** and need zero re-authoring.
- **`baseAccuracy > 0` → aim-cone model.** The weapon runs the two-cone path, with `baseAccuracy`
  driving the weapon cone. Modders opt weapons in one at a time.

`Projectile::calculateTrajectory` branches on the weapon's `baseAccuracy` (0 → today's code, >0 →
the cone path). Both paths share the LOS sanity pre-trace, then diverge: the native path keeps its
full `applyAccuracy` (range dropoff, LOS penalty, scatter), while the cone path applies **no linear
range dropoff** (falloff is geometric) and folds the no-LOS penalty into `soldierAcc` instead
(see *Mechanics*). Keeping the native path fully intact
also de-risks the rollout: the cone can be developed and tuned against a handful of test weapons
while the campaign content stays on the proven model.

### How the inputs map for opted-in weapons

The split falls along a natural seam in the current ruleset, so opting a weapon in needs only the one
new field:

- **Soldier cone — fully covered by existing config.** Its input is exactly today's
  `getFiringAccuracy`: `_accuracyMulti` (Firing-stat scaling) × the per-mode `RuleItemAction.accuracy`
  (snap 60 / aimed 110 / auto 40 / burst) × kneel × one-handed × wounds. Every weapon already
  expresses "aim quality" through these fields, so the soldier cone works untouched. The range fields
  (`aimRange`/`snapRange`/`autoRange`/`minRange`/`dropoff`) feed only the **effective-range readout**
  for opted-in weapons — the cone applies no linear dropoff (see *Mechanics*).
- **Weapon cone — driven by `baseAccuracy`.** This is the genuinely new axis: no intrinsic per-weapon
  *precision/spread* field existed before (`RuleItemAction.accuracy` is the aim/mode number, not a
  spread number). A higher `baseAccuracy` → tighter weapon cone.
- **Do not derive the weapon cone from a per-mode `%`.** Those percentages already feed the *soldier*
  cone via the shot-type factor; reusing them for the weapon cone double-counts the same number and
  couples the two axes that are supposed to be independent.

Because opted-in behavior only appears when a modder sets `baseAccuracy`, the calibration burden is
scoped to *those* weapons rather than being a global re-balance — pick `baseAccuracy` values (and the
boxMuller constants) so an opted-in weapon at a representative shot (e.g. Firing 60 / snap 60) lands a
sensible hit rate. (`battleUFOExtenderAccuracy` is **ignored mechanically** for opted-in weapons —
the cone produces range falloff intrinsically; its per-mode ranges surface only through the
effective-range readout.)

## Implementation approach (delta)

1. **Vector math.** Add `DVec3` (or reuse `fmath.h` vector helpers), `normalize`,
   `rotateVectorRandomly(dir, angle)`, and a `boxMuller` sampler on `RNG`. Confirm whether existing
   `VectNormalize`/`VectCrossProduct` (used by vapor clouds in `Projectile.cpp`) can be reused.
2. **TileEngine ray trace.** Add `calculateLineFromVector(origin, dirVoxels, storeTrajectory,
   trajectory, excludeUnit)` — a thin wrapper over the existing Bresenham voxel walker
   (`calculateLineVoxel`) that takes a direction instead of an endpoint.
3. **`Projectile::calculateTrajectory`.** Branch on the weapon's `baseAccuracy`: `0` keeps the
   current native path untouched; `> 0` takes the cone path. Both keep the shared LOS sanity
   pre-trace; the cone path applies **no range dropoff** and folds the no-LOS penalty into
   `soldierAcc` (see *Mechanics*). Store the deflected, max-range-extended ray endpoint in
   `_targetVoxel` so the existing `recalculateImpact` works unchanged. Leave `calculateThrow`
   untouched.
4. **Feed both accuracies separately.** Split today's single folded `getFiringAccuracy` into two
   numbers threaded into the projectile:
   - **`soldierAcc`** = effective soldier accuracy = Firing skill (`getAccuracyMultiplier`) ×
     **shot-type factor** (`accuracySnap`/`Aimed`/`Auto`/`Burst`) × kneel bonus × one-handed penalty
     × wound/health modifier (`getAccuracyModifier`). This is essentially the existing
     `getFiringAccuracy` result — reuse it as the soldier cone input.
   - **`weaponAcc`** = the weapon's intrinsic accuracy, from the *new* per-weapon `RuleItem` field
     **`baseAccuracy` (default `0`)**, independent of the shooter and shot type. **`baseAccuracy == 0`
     is the opt-out sentinel: the weapon uses the current native scatter mechanics unchanged** (see
     *Vanilla weapon compatibility* / *Code-path swap* below). A value `> 0` opts the weapon into the
     aim-cone model and drives its weapon cone.
   Decide the exact mapping so total lethality stays balanced against vanilla.
5. **Shotgun convergence.** Soldier cone once for the volley (shared true aim line); then per-pellet
   weapon cone. Replace the current diminishing-per-pellet-accuracy trick
   ([ProjectileFlyBState.cpp:649-659](../src/Battlescape/ProjectileFlyBState.cpp#L649)) with the
   independent per-pellet weapon deflection, scaled by the ammo's `shotgunSpread`; `shotgunChoke`
   is ignored on the cone path (subsumed by `baseAccuracy`).
6. **UI calculators.** Implement `calculateEffectiveRange` / `calculateChanceToHit`; wire the
   deferred action-menu effective-range readout and the hover readout (separate Phase 5 items but
   depend on this).
7. **Option gate (secondary).** The per-weapon `baseAccuracy == 0` sentinel is the primary opt-in;
   a global `Options` toggle to force *all* weapons back to the native scatter model (ignoring
   `baseAccuracy`) is optional insurance, not the main switch.

## Resolved design decisions (design review, Jul 2026)

All open questions were walked through and resolved; details live in the sections above.

1. **Range dropoff — geometry only.** Opted-in weapons apply no linear dropoff; distance falloff
   comes purely from cone geometry. `aimRange`/`snapRange`/`autoRange`/`minRange`/`dropoff` feed
   only the effective-range readout; `battleUFOExtenderAccuracy` is ignored mechanically. (With
   default field values dropoff never triggers today anyway.)
2. **Accuracy floors.** Effective `soldierAcc` clamped to **≥ 20** (legacy's floor, applied after
   all multipliers including the no-LOS penalty); `weaponAcc` guarded to **≥ 1**. Both enter the
   cone formulas at *percent scale* (60, not 0.60).
3. **Gaussian tail clamp.** Each sampled deflection angle is clamped at **3σ of its own cone** —
   scale-free, keeps 99.7% of the distribution, no freak sideways/backwards shots.
4. **Shotguns.** Per-pellet weapon cone scaled by the ammo's `shotgunSpread` (buckshot vs. slug
   stays meaningful); `shotgunChoke` is subsumed by `baseAccuracy` and ignored on the cone path.
5. **`BA_LAUNCH` stays on scatter.** Waypoint homing doesn't fit a ray model; revisit only if it
   plays badly.
6. **No-LOS penalty widens the soldier cone.** `noLOSAccuracyPenalty` multiplies into effective
   `soldierAcc` before the floor clamp; the ruleset field is reused unchanged.
7. **Split of `getFiringAccuracy`.** All shooter-side terms — Firing skill, shot-type factor
   (`accuracySnap`/`Aimed`/`Auto`/`Burst`), kneel bonus, one-handed penalty, wound/health modifier —
   feed the **soldier** cone. The **weapon** cone is driven *only* by the new per-weapon
   `baseAccuracy`, independent of all of the above.
8. **Model selection.** New `RuleItem` field **`baseAccuracy`, default `0`**. `0` = native scatter
   mechanics unchanged (all existing content); `> 0` = opt into the aim-cone and drive the weapon
   cone. Selected per weapon in `Projectile::calculateTrajectory`.
9. **Tuning validation — Monte-Carlo first.** Before hardcoding constants (`0.437`, `acc²/50`, the
   `·2`), validate with the simulation script **`reference/aimcone_montecarlo.py`** (kept in the
   repo for future balance passes; run with plain `python`, no dependencies). It replicates the
   native `applyAccuracy` scatter exactly (classic spread, integer math) and the cone model with
   decisions 1–3 baked in, printing hit-probability vs. distance tables plus the 50%-hit effective
   ranges. First results (Jul 2026, legacy constants, 40k shots/cell, standing-soldier target):
   - The models cross at ~8–15 tiles: the cone is more generous up close, much harsher at range
     (Average F60 snap @40 tiles: 38% native → 15% cone; the native model's flat long-range tail is
     gone). Effective ranges: Rookie snap ≈ 6 tiles, Average snap ≈ 13, Veteran aimed ≈ 49.
   - The initial run exposed a tuning problem: with the legacy *linear* weapon cone, `baseAccuracy`
     was nearly invisible for average shooters — sweeping it 40→300 at F60/snap moved the 40-tile
     hit rate only 12%→18%, because the soldier cone (σ≈2.9°) dwarfed the weapon cone (σ≈0.5°).
     **Resolved by decision 11.**
10. **RNG determinism.** `boxMuller` draws from the battle `RNG` stream so seeded saves reproduce.
11. **Weapon-cone scaling — quadratic, normalized at 75 ("V3").** A follow-up variant sweep
    (Jul 2026) compared the two candidate fixes for the weak `baseAccuracy` knob: tightening the
    soldier `·2` multiplier vs. steepening how σ_w scales with `baseAccuracy`. Measured at snap
    60%, `baseAccuracy` 75 unless noted, 40k shots/cell:

    | Variant | Global balance (F60 hit% @10 tiles) | Knob spread (baseAcc 40→150, @15 tiles) | Eff. range F60 snap |
    |---|---|---|---|
    | V0 legacy (soldier ×2.0, weapon linear) | 61.5% | +4.8 pts | 12.9 tiles |
    | V1 soldier ×1.5, weapon linear | 73.0% | +6.8 pts | 16.9 tiles |
    | V2 soldier ×1.0, weapon linear | 87.1% | +9.2 pts | 23.5 tiles |
    | **V3 soldier ×2.0, weapon quad@75 — adopted** | 60.9% (≈V0) | **+12.3 pts** | 12.9 tiles |
    | V4 soldier ×1.5, weapon quad@75 | 72.9% | +16.6 pts | 16.9 tiles |

    Tightening the soldier multiplier (V1/V2) mostly re-tunes *global lethality* — everyone hits
    far more, effective ranges balloon — while only weakly strengthening the knob. Squaring the
    weapon term instead (V3) is pinned to legacy behavior at `baseAccuracy: 75`, so the global
    balance and effective ranges are untouched, while the knob's reach more than doubles. The
    spread also works mostly *downward*: `baseAccuracy: 40` (σ_w 1.74°) drops F60's 15-tile hit
    rate 44%→33%, while values above 75 give diminishing returns (45.1%→45.4% up to 150) because
    the soldier cone dominates once the weapon is tight. That preserves the design philosophy: a
    modder can make a weapon meaningfully *bad*, but a laser-precise weapon cannot fix a mediocre
    shooter — precision only shines in skilled, deliberate hands.

    **Concepts (for future balance passes):** the soldier `·2` multiplier is the *global lethality*
    dial; the weapon exponent/normalization is the *knob strength* dial. They are orthogonal —
    re-tune one without disturbing the other, using `reference/aimcone_montecarlo.py`.

    **Final adopted constants:** tuning `0.437`; MAD→σ `1.4826`;
    soldier σ = `0.437 / (soldierAcc²/50) · 1.4826 · 2` (floor 20);
    weapon σ = `0.437 / (weaponAcc²/75) · 1.4826` (floor 1); each sample clamped at 3σ.

## Remaining open items

- **In-game validation of the adopted constants** (decision 11) during implementation playtesting —
  the Monte-Carlo tables predict hit rates against an idealized standing target; real maps add
  cover, elevation, and unit-size variety.
- **Reaction/AI paths.** `AIModule` and reaction fire also call `getFiringAccuracy`
  ([ProjectileFlyBState.cpp:1048](../src/Battlescape/ProjectileFlyBState.cpp#L1048)); confirm the AI
  hit-chance estimate uses the same cone-aware calculator so AI target selection stays coherent
  (this is the Phase 9 dependency).
