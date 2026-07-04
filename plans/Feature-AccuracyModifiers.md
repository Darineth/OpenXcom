# Feature: Accuracy Modifiers (smoke & exhaustion)

**Status:** **Implemented (Jul 2026)**, pending in-play tuning of the smoke constants. Extends the
aim-cone model ([Feature-AimConeTrajectory.md](Feature-AimConeTrajectory.md)) with two additional
soldier-cone factors. **Scope: cone weapons only** (`baseAccuracy > 0`) — native scatter weapons stay
byte-for-byte vanilla, consistent with the rest of the aim-cone opt-in.

**Implemented where:**
- **Exhaustion** — `BattleUnit::getFiringAccuracy` ([BattleUnit.cpp:2595+](../src/Savegame/BattleUnit.cpp#L2595)),
  gated on `baseAccuracy > 0` + a direct-fire action type. Flows to the shot, both readouts, and
  AI/reaction from one place.
- **Smoke** — `TileEngine::getSmokeAccuracyFactor(originVoxel, targetVoxel)`
  ([TileEngine.cpp](../src/Battlescape/TileEngine.cpp)), applied to `soldierAcc` in
  `Projectile::applyAimCone` (soldier-cone branch, once per volley) and
  `Projectile::calculateHitChancePercent` (hover readout). Not in effective-range (target-independent).

## Motivation

The aim-cone soldier cone folds together everything about the shooter's aim. OXCE already covers
most of it; DX adds the two realism penalties the legacy OpenXcom+ fork had but OXCE lacks:
smoke obscuring the line of fire, and a tired shooter's hands shaking.

## OXCE / OXCE-Plus audit — what `getFiringAccuracy` already folds in

[src/Savegame/BattleUnit.cpp:2550](../src/Savegame/BattleUnit.cpp#L2550). Confirmed present:

- **Shot-type accuracy** — snap/aimed/auto/burst × the Firing-stat `accuracyMultiplier`.
- **Kneel bonus** (`getKneelBonus`), applied when kneeled.
- **One-handed / two-handed penalty** (`getOneHandedPenalty`) when a two-handed weapon is fired
  with the other hand occupied.
- **Health + fatal wounds** via `getAccuracyModifier`
  ([BattleUnit.cpp:2621](../src/Savegame/BattleUnit.cpp#L2621)):
  `max(10, 25*health/maxHealth + 75 − 10*wounds)` (head wounds always; arm wounds if that hand
  holds the weapon).
- **Berserk** — folded in downstream via the `accuracyDivider` (200 instead of 100) in
  `ProjectileFlyBState::createNewProjectile`, which the cone path recovers as a ×0.5 soldier-cone
  widener.

**Confirmed absent from OXCE base** (a grep of `getFiringAccuracy`/`applyAccuracy`/`getAccuracyModifier`
finds no reference to smoke or energy): a **smoke** accuracy penalty and an **exhaustion**
(low-energy) penalty. These are the DX delta. (Legacy also had a ×0.7-vs-sprinting-target penalty;
that is target-state dependent and **deferred** — noted under *Future* below.)

## Target design — the two new factors

Both are **soldier-cone factors**: they scale the effective `soldierAcc` (percent), which the cone
math squares into the soldier cone width, so a penalty widens the shooter's cone (never the weapon
cone). They stack multiplicatively with the existing factors.

### 1. Exhaustion (shooter energy) — target-independent

A tired shooter aims worse. Legacy formula (carried over): below 50% energy the accuracy multiplier
is `0.5 + energyRatio`, where `energyRatio = energy / stamina`; at/above 50% there is no penalty.
Equivalently `factor = min(1.0, 0.5 + energy/stamina)`:

| energy | factor |
|---|---|
| ≥ 50% | 1.00 (no penalty) |
| 25% | 0.75 |
| 0% | 0.50 (hard floor) |

Because it depends only on the shooter (no target), it folds into **`getFiringAccuracy`** — gated on
`baseAccuracy > 0` **and** a direct-fire action type (snap/aimed/auto/burst/launch, not throw/melee).
That way it flows automatically into the shot, the hover hit-chance readout, the **effective-range**
readout, and AI/reaction estimates — all consistently, from one place. Throwing and melee are left
alone.

### 2. Smoke on the line of fire — target-dependent

Shooting through smoke degrades aim. Mirrors the engine's existing *visibility* smoke model
([TileEngine.cpp:1816-1835](../src/Battlescape/TileEngine.cpp#L1816)), which already sums
`step * tile->getSmoke()` along a traced line to shorten sight range. DX reuses the same idea for
accuracy: sum the smoke along the shooter→target tile line and reduce `soldierAcc` by a factor:

```
sumSmoke = Σ getSmoke() over the LOF tiles (TileEngine::calculateLineTile)
smokeFactor = max(SMOKE_ACC_FLOOR, 1.0 − sumSmoke * SMOKE_ACC_PER_UNIT)
```

Provisional constants (**to tune in play**): `SMOKE_ACC_PER_UNIT = 0.01`, `SMOKE_ACC_FLOOR = 0.30`
(so e.g. a path through ~30 total smoke → ~0.70; heavy smoke bottoms out at 0.30 — smoke degrades
but never fully blinds).

Because it depends on the target/path, smoke is applied in the **cone path** where origin+target are
known — analogous to the existing no-LOS penalty (`getNoLOSAccuracyPenaltyFactor`):
- `Projectile::applyAimCone` — computed **once per volley** (in the soldier-cone branch, so shotgun
  pellets share it via the stored true-aim), on the ideal origin→target line.
- `Projectile::calculateHitChancePercent` — computed once on the ideal center line, applied to
  `soldierAcc` before sampling.
- **Not** in `getFiringAccuracy`, and **not** in `calculateEffectiveRange` — effective range is a
  target-independent "clear-air reliable range" property, so it intentionally ignores smoke (it
  already ignores cover/no-LOS for the same reason).

## Implementation approach (delta)

1. **Exhaustion in `getFiringAccuracy`.** Add an energy factor `min(1.0, 0.5 + energy/stamina)`,
   gated on `item->getRules()->getBaseAccuracy() > 0` and a direct-fire action type. Guard against
   zero stamina.
2. **Smoke helper.** Add `TileEngine::getSmokeAccuracyFactor(originVoxel, targetVoxel)` (or an inline
   sum via `calculateLineTile`) returning the `smokeFactor` above.
3. **Apply smoke in the cone path.** In `applyAimCone` (soldier-cone branch) and
   `calculateHitChancePercent`, multiply `soldierAcc` by the smoke factor before computing the cone
   — right alongside the existing no-LOS multiply.
4. **No readout/UI changes.** Both factors flow through the existing hit-chance and effective-range
   readouts automatically (exhaustion into both; smoke into the hover readout only).

## Open questions / tuning

- **Smoke constants** (`SMOKE_ACC_PER_UNIT`, `SMOKE_ACC_FLOOR`) need playtest tuning; capture final
  values here.
- **Exhaustion floor/curve** — legacy `0.5 + energyRatio` is adopted; revisit if 0.5 floor feels too
  harsh/soft for DX stamina ranges.

## Future (deferred)

- **Sprinting-target penalty** (legacy ×0.7 vs a target that ran this turn) — target-state
  dependent; revisit if movement-state tracking is added.
