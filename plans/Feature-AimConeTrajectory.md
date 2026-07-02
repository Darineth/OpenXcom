# Feature: Aim-Cone Trajectory Model

**Status:** Planned (Phase 5 — Firing & Accuracy). Not yet implemented; the current engine still
uses the native OXCE scatter-the-aimpoint model described below.

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
  `applyAccuracy`. Decide whether launch stays scatter-based or joins the cone.
- **Shotgun pellets** — DX already flies pellets as individual concurrent projectiles
  ([ProjectileFlyBState.cpp:649-659](../src/Battlescape/ProjectileFlyBState.cpp#L649)), but each
  pellet currently calls `calculateTrajectory` with a **reduced per-pellet accuracy** feeding the
  *native scatter model* — not a boxMuller cone. Under the aim-cone this should converge on a single
  per-pellet cone deflection (`boxMuller(0, 0.437 / shotgunAcc · 1.4826)` per the legacy design),
  replacing the diminishing-accuracy trick.
- **`recalculateImpact`** ([Projectile.cpp:219](../src/Battlescape/Projectile.cpp#L219)) re-traces a
  *stored* trajectory against changed terrain. It re-runs `calculateLineVoxel` from the stored
  origin to `_targetVoxel`. For the cone model the stored `_targetVoxel` must be the **already
  deflected** endpoint so the re-trace stays deterministic — same requirement as today.

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
   - **Weapon cone** — `angle = boxMuller(0, 0.437 / weaponAcc · 1.4826)`, driven only by the
     weapon's intrinsic accuracy and *nothing else*. **Applied per projectile** (per pellet for
     shotguns). The two cones **stack**: shooter error then weapon error.
   - `boxMuller(mean, stddev)` = a Gaussian/normal sample (Box–Muller transform); `1.4826` is the
     MAD→σ consistency constant; `0.437` is the tuning constant carried over from the legacy fork.
3. **Trace as a ray.** `direction *= 16000`, then
   `TileEngine::calculateLineFromVector(origin, direction, ...)` flies the bullet down the deflected
   line until impact — no target-point homing. `recalculateTrajectoryFromVector()` re-traces the
   stored vector when a mid-flight obstacle is removed.

### Supporting calculators (needed by Phase 5 UI, build alongside)

- `BattleUnit::calculateEffectiveRange(soldierAcc, weaponAcc)` — the distance at which the combined
  cone keeps a shot reliable; feeds the action-menu **effective-range** readout and
  `calculateEffectiveRangeForAction`. **Legacy DX defined a weapon's "effective range" as the
  distance at which the aim-cone yields a 50% hit chance** — i.e. the range where the combined
  soldier+weapon deflection puts the shot on target half the time. Use that same 50%-hit-rate
  definition as the readout's meaning. This is "fancy math" over **both** accuracies: the combined
  cone half-angle (soldier + weapon) vs. the angular size a target subtends at distance `d`; solve
  for the `d` where the on-target probability crosses 50%.
- `Projectile::calculateChanceToHit(...)` — estimated hit chance **plus** a cover-reduction term,
  for the hover readout `<acc>% (-<cover>%) @ <distance>` (shown even without UFOExtender mode,
  color-graded red→green). See `Feature-ActionMenuRevamp.md` (effective-range readout was deferred
  here) and the Phase 5 "Hover Accuracy Readout" item.

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
the cone path). Both paths share the up-front pieces — the LOS sanity pre-trace, range dropoff
(`calculateLimits`), and the LOS penalty — and diverge only at the deviation stage
(`applyAccuracy`'s steps 3–5 vs. the direction-vector cone). Keeping the native path fully intact
also de-risks the rollout: the cone can be developed and tuned against a handful of test weapons
while the campaign content stays on the proven model.

### How the inputs map for opted-in weapons

The split falls along a natural seam in the current ruleset, so opting a weapon in needs only the one
new field:

- **Soldier cone — fully covered by existing config.** Its input is exactly today's
  `getFiringAccuracy`: `_accuracyMulti` (Firing-stat scaling) × the per-mode `RuleItemAction.accuracy`
  (snap 60 / aimed 110 / auto 40 / burst) × kneel × one-handed × wounds. Every weapon already
  expresses "aim quality" through these fields, so the soldier cone works untouched. The range fields
  (`aimRange`/`snapRange`/`autoRange`/`minRange`/`dropoff`) carry straight into the dropoff and
  effective-range math.
- **Weapon cone — driven by `baseAccuracy`.** This is the genuinely new axis: no intrinsic per-weapon
  *precision/spread* field existed before (`RuleItemAction.accuracy` is the aim/mode number, not a
  spread number). A higher `baseAccuracy` → tighter weapon cone.
- **Do not derive the weapon cone from a per-mode `%`.** Those percentages already feed the *soldier*
  cone via the shot-type factor; reusing them for the weapon cone double-counts the same number and
  couples the two axes that are supposed to be independent.

Because opted-in behavior only appears when a modder sets `baseAccuracy`, the calibration burden is
scoped to *those* weapons rather than being a global re-balance — pick `baseAccuracy` values (and the
boxMuller constants) so an opted-in weapon at a representative shot (e.g. Firing 60 / snap 60) lands a
sensible hit rate. (`battleUFOExtenderAccuracy` becomes largely redundant for opted-in weapons since
the cone produces range falloff intrinsically — decide whether to honor its per-mode ranges or ignore
the option under the cone model.)

## Implementation approach (delta)

1. **Vector math.** Add `DVec3` (or reuse `fmath.h` vector helpers), `normalize`,
   `rotateVectorRandomly(dir, angle)`, and a `boxMuller` sampler on `RNG`. Confirm whether existing
   `VectNormalize`/`VectCrossProduct` (used by vapor clouds in `Projectile.cpp`) can be reused.
2. **TileEngine ray trace.** Add `calculateLineFromVector(origin, dirVoxels, storeTrajectory,
   trajectory, excludeUnit)` — a thin wrapper over the existing Bresenham voxel walker
   (`calculateLineVoxel`) that takes a direction instead of an endpoint.
3. **`Projectile::calculateTrajectory`.** Branch on the weapon's `baseAccuracy`: `0` keeps the
   current native path untouched; `> 0` takes the cone path. Both keep the shared LOS sanity
   pre-trace, range-dropoff (`calculateLimits`), and LOS penalty; they diverge only at the deviation
   stage. Store the deflected `_targetVoxel` (endpoint of the traced ray) so `recalculateImpact`
   stays deterministic. Leave `calculateThrow` untouched.
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
   independent per-pellet weapon deflection.
6. **UI calculators.** Implement `calculateEffectiveRange` / `calculateChanceToHit`; wire the
   deferred action-menu effective-range readout and the hover readout (separate Phase 5 items but
   depend on this).
7. **Option gate (secondary).** The per-weapon `baseAccuracy == 0` sentinel is the primary opt-in;
   a global `Options` toggle to force *all* weapons back to the native scatter model (ignoring
   `baseAccuracy`) is optional insurance, not the main switch.

## Open questions

- **Balance vs. vanilla.** The tuning constants (`0.437`, the `soldierAcc²/50` shaping, the `·2` on
  the soldier cone) were tuned for the legacy fork's stat ranges. Re-validate against DX's current
  weapon accuracy values so mods don't silently rebalance. Capture final constants here once tuned.
- **Split of `getFiringAccuracy` — resolved (design).** All shooter-side terms — Firing skill,
  **shot-type factor** (`accuracySnap`/`Aimed`/`Auto`/`Burst`), kneel bonus, one-handed penalty, and
  the wound/health modifier — feed the **soldier** cone (they modify the shooter's aim). The
  **weapon** cone is driven *only* by the new intrinsic per-weapon `baseAccuracy`, independent of all
  of the above.
- **Model selection — resolved (design).** New `RuleItem` field **`baseAccuracy`, default `0`**.
  `0` = keep the native scatter mechanics for that weapon (no behavior change; all existing content
  stays here); `> 0` = opt into the aim-cone model and use the value as the weapon cone input. Both
  code paths coexist and are selected per weapon in `Projectile::calculateTrajectory`. This scopes
  balancing to opted-in weapons rather than a global re-tune.
- **Launch/guided.** Keep `BA_LAUNCH` on the scatter drift model or move it to a (wide) cone?
- **LOS penalty form.** The native penalty multiplies accuracy (which shrinks the scatter radius).
  Under a cone, decide whether no-LOS *widens the cone* or reduces effective range.
- **Reaction/AI paths.** `AIModule` and reaction fire also call `getFiringAccuracy`
  ([ProjectileFlyBState.cpp:1048](../src/Battlescape/ProjectileFlyBState.cpp#L1048)); confirm the AI
  hit-chance estimate uses the same cone-aware calculator so AI target selection stays coherent
  (this is the Phase 9 dependency).
