# Feature: Targeting Visualization (aim-cone spread & throw landing area)

**Status:** Option **A + D (sampled impact/landing dots) implemented (Jul 2026)**, Alt-triggered;
other options remain candidates. Grew out of the Scratch note "Visualize aim/throw cone/landing
area?". Purely a **DX visualization** feature — it draws what the existing aim-cone and
realistic-throwing models already compute; it changes no game logic.

## Implemented — sampled impact/landing dots (Alt)

Hold **Alt** while aiming to replace the single ideal tracer line with a **dot cloud** showing where
the shots (aim-cone) or throw (launch error) would actually land.
- The hit-chance / landing-chance Monte-Carlos gained an optional out-vector
  (`calculateHitChancePercent(..., outSampleImpacts)`, `calculateThrowLandChancePercent(...,
  outSampleLandings)`) that collects each sampled round's **real voxel impact** (target, wall, ground,
  or — for a flew-past shot — the target-plane crossing). No extra tracing: it's the same MC the
  readout runs.
- Each sample is tagged **on-target vs miss** (`SpreadSample{ pos, onTarget }`): a shot is on-target
  when its traced impact tile is the aimed unit/wall/tile (for throws, the target tile). Dots are
  drawn **green for hits, red for misses**, so you see at a glance how much of the spread connects.
- `Map::updateTargetingPreview` computes the cloud when Alt is held (Alt is part of the rebuild key,
  so toggling re-traces), for **cone-model direct fire** (`baseAccuracy > 0`) and **realistic
  throwing** only — the models that actually have a physical spread. `drawTargetingPreview` blits a
  readable subset (~40) of the samples with the tracer sprite (recoloured per hit/miss via
  `blitRaw`'s `newBaseColor`), **in place of** the line.
- Dual-fire shows the display (cone-preferred) hand's cloud, matching the single tracer line.
- Deterministic (seeded MC) so the cloud is stable per aim; recomputed only on aim/Alt change.

*Note:* Alt already shows the crosshair damage readout, so holding Alt is now a combined
"detailed aim" mode (damage numbers + spread cloud).

Follow-ups from the options below remain open: a distinct third colour for **cover-blocked** shots
(would-have-hit but stopped by terrain, vs. genuine aim misses), the probability tile-heatmap (C/F),
and the grenade blast-radius footprint (E).

## Motivation

DX's firing and throwing are now physical models — a soldier + weapon aim cone
([Feature-AimConeTrajectory.md](Feature-AimConeTrajectory.md)) and a launch-error throw
([Feature-ThrowAccuracyRealism.md](Feature-ThrowAccuracyRealism.md)). Today the player sees only a
**single ideal tracer line** ([Feature-LiveTrajectoryPreview.md](Feature-LiveTrajectoryPreview.md))
plus a **hit-chance / landing-chance %**. Neither conveys *where the spread actually goes* — the
cone's width at this range, or the short/long-vs-lateral bias of a throw. A spatial visualization
would make the model legible: why a far shot is risky, how much cover helps, how strength/weight
skews a throw.

## Existing infrastructure to build on

Everything needed to *compute* the spread already exists and is cached; the work is mostly *drawing*.

- **Preview rendering** — `Map::drawTargetingPreview` ([Map.cpp](../src/Battlescape/Map.cpp))
  already blits tracer sprites along a trajectory's voxels (`_camera->convertVoxelToScreen` +
  `Surface::blitRaw`) and an impact marker (`HIT.PCK`). Sprite is the mod-configurable
  `trajectoryPreviewSprite` constant; depth-correct via `_projectileSet`. Rebuilt only when the aim
  changes (`_targetingProjectile`, `_previewTarget`).
- **The spread models, already sampling** — `Projectile::calculateHitChancePercent` voxel-traces
  hundreds of deflected shots (the exact two-cone model) and `calculateThrowLandChancePercent`
  Monte-Carlos the launch error through the real parabola. Both are **deterministically seeded and
  cached per aim** — so the sample set is stable and free to reuse for drawing.
- **The cone math** — `soldierConeSigma` / `weaponConeSigma` / `deflectVector` / `seededConeAngle`
  give the angular spread directly (for an analytic ellipse instead of samples).
- **Tile markers** — the map already tints tiles for obstacles/waypoints/danger zones, i.e. there's
  a precedent for per-tile overlays if we go the heatmap route.
- **Toggle precedent** — `battleTrajectoryPreview` (DX option) already gates the tracer line; a new
  overlay can hang off the same or a sibling option / hold-key.

## Options — aim cone (direct fire)

### A. Sampled impact dots  *(recommended first cut)*
Draw a scatter of ~20–40 small sprites where sampled shots would land. Have the hit-chance MC
**return the impact voxels it already traces** (instead of only the count); draw them with the
existing tracer-sprite path.
- **Pros:** truthful (literally the shot model, incl. cover — blocked samples land on the wall);
  reuses `drawTargetingPreview`; tightens/widens live with distance/kneel/cover/smoke; cheap (MC is
  already cached, drawing is a blit loop). Lowest risk.
- **Cons:** a live spray of dots can look busy; discrete samples read as "pattern" not "boundary".
- **Effort:** low–medium. Mostly: thread the sample points out of the MC + a draw loop + a toggle.

### B. Cone edge rays / wedge
Draw a few tracer lines deflected to the cone's ±Nσ edges (e.g. ±2σ soldier+weapon), forming a
visible fan from the muzzle.
- **Pros:** shows the cone *shape* from the muzzle; reuses the tracer-line trace directly.
- **Cons:** a 2D wedge in isometric 3D reads ambiguously (is that width or depth?); picking "the
  edge" of a Gaussian is arbitrary; doesn't show cover.
- **Effort:** low.

### C. Probability tile-heatmap
Tint the target tile and its neighbours by how often a shot lands on each (bin the MC samples by
landing tile, colour by frequency).
- **Pros:** most *informative* and cleanest-looking — a real "where will it go" heat cloud; shows
  cover naturally; no dot clutter.
- **Cons:** rendering a smooth per-tile heat overlay in iso space is the fiddliest (tile tinting,
  blending, depth ordering); needs a colour ramp that stays readable over varied terrain.
- **Effort:** medium–high.

## Options — thrown ground targets

### D. Landing scatter dots  *(recommended first cut, pairs with A)*
Same mechanism as A, driven by `calculateThrowLandChancePercent`'s sampled landing points. Because
the launch error is short/long-dominant, the cloud comes out **elongated along the throw line**,
visually teaching "you'll miss long/short, not sideways," and it widens with distance / strength
strain.
- **Pros:** truthful; reuses the same draw path as A; the elongation communicates the throw model
  for free.
- **Cons:** same dot-clutter caveat.
- **Effort:** low–medium (shared with A).

### E. Blast-radius footprint
For grenades/AoE, draw the explosion radius circle at the aim tile — and optionally at the
short/long extremes of the scatter — so the player sees whether a near-miss still catches the target.
- **Pros:** answers the real tactical question for grenades; ties into the deferred "landing chance
  should count the blast radius, not just the exact tile" TODO
  ([Feature-ThrowAccuracyRealism.md](Feature-ThrowAccuracyRealism.md)).
- **Cons:** only meaningful for AoE items; iso circle rendering; interacts with the exact-tile-vs-
  blast landing-chance question (probably do them together).
- **Effort:** medium.

### F. Landing tile-heatmap
The throw analogue of C — tint tiles by landing frequency. Same pros/cons/effort as C.

## Cross-cutting concerns

- **Toggle & clutter.** Overlays should be opt-in: a DX option (default off?) and/or a **hold-key**
  to show-while-held, so the normal aim stays clean. The single tracer line likely stays as-is.
- **Performance.** The MCs are already cached per aim, so drawing reuses cached samples — a per-frame
  blit loop only. The heatmap needs a cached per-tile bin array (rebuilt on aim change).
- **Truthful vs analytic.** Dots/heatmap from the *traced* MC include cover and the real silhouette.
  An analytic ellipse (from the σ helpers) is cheaper and smoother but ignores cover — so the
  sample-based approaches are preferred for honesty.
- **Depth / iso ordering.** Dots are drawn at voxel positions via `convertVoxelToScreen` (already
  depth-aware in the preview). A tile-heat overlay needs to respect the map's tile draw order.
- **Sample count / stability.** Reusing the MC's deterministic seed keeps the dots stable per aim
  (no flicker). ~20–40 dots reads well without turning to mush; the heatmap can use the full sample
  set (hundreds) since it bins.
- **Colour.** Dots could be a single neutral tracer, or coloured by "hit vs blocked-by-cover vs
  missed" to also show cover losses at a glance.

## Recommendation

Do **A + D (sampled impact/landing dots)** first: one shared mechanism (surface the MC sample points,
draw them with the existing sprite path), truthful to the model, cheap because the MC is cached, and
low-risk. Gate behind a DX option and/or hold-key. If it lands well, layer **C/F (tile-heatmap)** for
a cleaner look, and pursue **E (blast footprint)** alongside the blast-radius landing-chance upgrade.
Treat **B (edge wedge)** as optional flavour, not the primary.

## Open questions

- **Trigger:** always-on option, hold-key, or both? Default on or off?
- **Sample count** for the dot cloud (readability vs. representativeness).
- **Sprite:** reuse `trajectoryPreviewSprite`, a smaller dedicated dot, or colour-coded dots
  (hit / cover-blocked / miss)?
- **Do the dots replace or augment** the current single ideal tracer line?
- **Blast footprint** — build with, or separately from, the exact-tile-vs-blast landing-chance change?
