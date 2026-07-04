# Feature: Realistic Throwing Accuracy

**Status:** **Implemented (Jul 2026)**, pending in-play tuning of the constants. Behind the
`battleRealisticThrowing` option (default off) — `Projectile::computeThrowLaunchError` computes the
launch-error offset and `calculateThrow` uses it in place of the scatter deviation for `BA_THROW`.
The throwing analog of the aim-cone
([Feature-AimConeTrajectory.md](Feature-AimConeTrajectory.md)): replace the "scatter the landing
point into a disc" deviation with a physical **launch-error** model, so a thrown item mostly errs
**short/long along the throw line** (with some lateral spread), scaling with distance and with how
hard the throw strains the thrower's strength.

## Motivation

DX wants a throw to feel like *lobbing an object* — where the hard part is judging the force
(distance), not the direction — rather than *rolling accuracy and teleporting the landing spot into
a flat disc*. The native model reuses the direct-fire scatter wholesale, which reads oddly for
throws: a "miss" scatters the item uniformly in x/y/z around the aim tile, so grenades miss
sideways as readily as short/long, and the throw arc's geometry is ignored.

## OXCE / OXCE-Plus audit — how vanilla throw accuracy works

(Full trace in the review; summary here.)

- **Accuracy number** — `getFiringAccuracy(BA_THROW)` = `throwMultiplier × accuracyThrow / 100`,
  where `throwMultiplier` (`_throwMulti`, default `setThrowing()`) scales with the **Throwing** stat
  and `accuracyThrow` defaults to `100`. Throws get **no kneel bonus, no one-handed penalty, and no
  LOS penalty** (all shot-only). ([BattleUnit.cpp:2579](../src/Savegame/BattleUnit.cpp#L2579))
- **Deviation** — `Projectile::calculateThrow` calls the **same** `applyAccuracy` scatter used for
  bullets (`keepRange=true`, `extendLine=false`): `deviation = RNG(0,100) − accuracy·100` (+50 miss
  / +10 hit, scaled by the axis-weighted range terms), jittering the target voxel by `±deviation` in
  **x, y, and z**; that offset becomes the parabola's landing deviation.
  ([Projectile.cpp:375](../src/Battlescape/Projectile.cpp#L375))
- **Range dropoff** — `throwDropoff` (5/tile) past `throwDropoffRange`, but that defaults to **99
  tiles** (larger than any throw) so it's effectively **off** unless `battleUFOExtenderAccuracy`
  swaps in a shorter band.
- **Reach / arc** — `TileEngine::validateThrow` curvature `= max(0.48, 1.73/⁴√(strength/weight) +
  kneel 0.1)`, and `getMaxThrowDistance(weight, strength, zd)` bounds range. **This is the separate
  "Throw Reach Scaling" feature and is *not* changed here** — realism throwing only replaces the
  *accuracy deviation*, reusing the existing arc/reach machinery.

**Confirmed:** no launch-error / arc-aware throw deviation exists; the deviation is a symmetric
target-space disc. This feature is a genuine new model for the deviation stage only.

## Target design — the launch-error model

Replace the target-disc jitter with an error in the **throw's release**, decomposed in the
horizontal plane relative to the throw direction:

1. **Range error (short/long)** — a Gaussian offset **along** the horizontal throw direction. This
   is the dominant term: judging force is the hard part, so most misses fall short or long.
2. **Lateral error (left/right)** — a smaller Gaussian offset **perpendicular** (horizontal) to the
   throw direction.
3. **No independent vertical jitter** — the parabola + terrain already determine the landing height;
   the item lands where the (range/lateral-perturbed) arc meets the ground.

Both stddevs scale with:
- **Distance** — error grows with throw distance (a far lob is harder to place).
- **Throw accuracy** — `getFiringAccuracy(BA_THROW)` (Throwing stat × `accuracyThrow`); higher →
  tighter. Reused as the throw's "soldierAcc" analog.
- **Strain (strength vs. weight)** — a throw near the thrower's *maximum* range for that item's
  weight is less controllable. Use `strain = realDistance / getMaxThrowDistance(weight, strength,
  zd)` (0 = easy lob, →1 = at the limit); higher strain widens the error, especially the range term.

Proposed form (constants **to calibrate**):

```
throwAcc   = getFiringAccuracy(BA_THROW)           // percent, floored at 20
strain     = clamp(horizDistVox / getMaxThrowDistance(weight, strength, zd), 0, 1)
sigmaRange = horizDistVox * (K_RANGE / throwAcc) * (1 + STRAIN_RANGE * strain)   // short/long
sigmaLat   = horizDistVox * (K_LAT   / throwAcc) * (1 + STRAIN_LAT   * strain)   // K_LAT < K_RANGE
delta      = Position( boxMuller(0, sigmaLat),  boxMuller(0, sigmaRange),  0 )   // (x=lateral, y=range)
```

**Important — the injection frame (learned from the code):** `delta` is *not* a landing offset. In
`calculateParabolaHelper` ([TileEngine.cpp:163](../src/Battlescape/TileEngine.cpp#L163)) `delta.x`
perturbs the throw **azimuth** (left/right) and `delta.y`+`delta.z` perturb the **elevation** (which
lands the throw **short/long**). So the lateral error goes in `delta.x` and the range error in
`delta.y` **directly** — *not* a world-axis projection of the throw line (which would swap the two
for axis-aligned throws; an early draft had that bug). Because `delta` is divided by distance inside
the helper, scaling the sigmas by `horizDist` keeps the *angular* spread ~constant, so the *landing*
spread grows with range — the desired behavior. `delta` is passed to `calculateParabolaVoxel` in the
same slot the native scatter offset uses today.

### Scope / opt-in (decision needed)

Throwing is a **universal** action and thrown items (grenades) have no per-weapon precision field
like firearms' `baseAccuracy`, so the aim-cone's per-weapon opt-in doesn't map cleanly. Candidates:
- **Global option** `battleRealisticThrowing` (default **off**) — one switch, preserves vanilla by
  default. *(recommended — matches "universal mechanic" and keeps vanilla intact.)*
- **Per-item field** — most flexible but heavy authoring for every grenade; little benefit.
- **Always on** — simplest mentally, but changes vanilla throw balance for everyone.

### Error-shape emphasis (decision needed)

How strongly should range error dominate lateral? Options: strongly range-dominant (realistic,
`K_LAT` ≈ 0.4·`K_RANGE`), mild, or roughly symmetric (closest to today).

## Implementation approach (delta)

1. **Branch in `calculateThrow`** on the chosen opt-in. Native path unchanged (`applyAccuracy`). New
   path computes the launch-error `offset` and passes it as the parabola deviation.
2. **Reuse** `RNG::boxMuller`, the throw accuracy from `getFiringAccuracy(BA_THROW)`,
   `getMaxThrowDistance` for the strain factor, and the existing `calculateParabolaVoxel` /
   `validateThrow` arc machinery. No change to reach/curvature.
3. **Calibrate** the constants with a small Monte-Carlo (mirror `reference/aimcone_montecarlo.py`):
   land-on-tile probability vs. distance for representative Throwing/strength/weight combos; keep
   grenade lethality vs. vanilla sane. Capture final constants here.
4. **UI (optional, later)** — a throw hit-chance readout could reuse the crosshair machinery, but is
   out of scope for the first cut.

## Resolved decisions (Jul 2026)

- **Opt-in: global option `battleRealisticThrowing`, default off.** Throwing is universal and grenades
  have no per-weapon precision field, so one switch fits; vanilla is preserved unless enabled.
- **Error shape: range-dominant.** Short/long error clearly larger than lateral (`K_LAT ≈ 0.4·K_RANGE`).
- **Strain: yes, mild.** Error widens as the throw approaches the thrower's max range for the item's
  weight (`strain = distance / getMaxThrowDistance`), moderately.

**Provisional constants (tune in play; Monte-Carlo/refine later):**
`K_RANGE = 3.5`, `K_LAT = 1.4` (0.4×), `STRAIN_RANGE = 0.6`, `STRAIN_LAT = 0.3`, throwAcc floor `20`.
These target: expert thrower (Throwing ~90) lands tight (<0.4 tile σ) at ~8 tiles; an average
thrower ~0.6 tile σ; a weak thrower near their range limit spreads ~1 tile. Capture final values here.

## Landing-chance readout — implemented (Jul 2026)

With realistic throwing on, the **throw cursor** now shows the estimated chance the item lands on the
**exact target tile** (`X% @ Zm`, color-graded red→green), replacing the abstract throw-accuracy
stat — which was never a landing probability (it just drove the spread). `Projectile::
calculateThrowLandChancePercent` finds the reaching arc via `validateThrow`, then Monte-Carlos the
launch error through the **real** `calculateParabolaVoxel` (the delta→landing mapping isn't linear,
so it must run the arc), counting exact-tile landings. Deterministic seedless RNG (stable per hover,
no game-RNG draws); cached in `Map` like the aim-cone hit-chance. The action-menu throw entry still
shows the accuracy stat (it's pre-target, so no landing tile to evaluate).

**TODO (requested):** broaden "hit" from the *exact* tile to the **blast radius / adjacent tiles** —
for a grenade, landing one tile off is usually still effective, so the exact-tile number understates
practical usefulness. Exact-tile is the deliberate first cut; the "something more" is this.

## Still open

- **No-LOS / blind throws** — add a penalty for throwing at an unseen tile? Deferred (vanilla has none).
- **Calibration** — re-tune the constants now that the injection frame is correct (azimuth/elevation),
  using the live landing-chance readout to judge.
