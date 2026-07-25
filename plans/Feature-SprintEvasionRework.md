# Feature: Sprint Evasion Rework — make sprinting's reaction interaction interesting

**Status:** Design — idea catalog below; **idea C chosen for implementation** (§C). Not yet built.
**Roadmap:** Phase 7 follow-up. Sprint/Sneak modes and the base evasion mechanic already shipped
([Feature-MovementModeEvasion.md](Feature-MovementModeEvasion.md)); this reworks only the *sprint*
side of that.

## Context

Movement Mode Evasion made a mover's reaction-fire evasion depend on how it moves. Sneak keeps full
evasion with no TU penalty (hard to reaction-fire against even when depleted); sprint just **lowers**
evasion (`statPercent: 60`), so a sprinter is a flat "easier target." That works but is flavorless —
sprint is only "a smaller number."

The reaction contest, for reference: a reactor fires when
`mover.getEvasionScore(moverMove) <= reactor.reactionScore`, so **higher mover evasion = fewer
reactors qualify = harder to be reaction-fired at**. The normal (walk) score is TU-limited:
`reactions × (currentTU/maxTU) × armorEvasion%`. See
[BattleUnit::getEvasionScore](../src/Savegame/BattleUnit.cpp#L2907).

Goal: give sprint a *memorable* reaction identity rather than a scalar penalty.

---

## Idea catalog

Four directions were considered. All are recorded here so the reasoning survives; the chosen one is C.

### A. "Shot at often, but hard to hit" — degrade the reaction *shot*, not the threshold
**Idea:** flip the model. A sprinter is *easy to shoot at* (more reactors qualify) but each reaction
shot is *less accurate* — a fast-crossing target adds spread to the reactor's aim-cone. Net feel: a
sprinter runs through a storm of near-misses, versus sneak's few-but-accurate shots.
**Grounding:** reuses the shipped aim-cone directly.
**Verdict:** **Interesting, not now.** We have no existing mechanic that modifies an incoming shot's
accuracy from the *target's* state — it would be new machinery in the reaction/aim-cone path. Parked
as a good idea for when such a hook exists.

### B. Directional evasion
**Idea:** evasion scales with the angle between the sprinter's movement vector and the reactor's
sightline — crossing a defender's line laterally is hard to track; running straight at/away is easy.
Rewards flanking runs, punishes frontal charges.
**Grounding:** DX has the directional geometry (positions, facings, directional armor).
**Verdict:** **Interesting, not now.** Tactically the richest, but the most code (per-reactor angle
math) and **hard to make legible** — the player can't easily see *why* a given run was safe or not,
so the depth would read as randomness. Parked.

### C. Momentum — evasion from tiles moved  ← **CHOSEN**
**Idea:** a sprinter's evasion is driven by **how many tiles it has moved in the current run**, not by
its stats. The first step out of cover is exposed (low evasion, easy to react to); after several tiles
at speed you're a blur (high evasion, hard to react to). Momentum resets when the run ends.
**Grounding:** the reaction check already runs per-step as the mover enters each tile, so the
tiles-so-far count is naturally available; sprint already "commits to the full path," which pairs with
a build-up mechanic.
**User steer:** *"it could just make the evasion score be just based on tiles moved"*, later refined to
*"the evasion per tile should be a percent of the unit's reaction score still."* So the sprint score is
a **momentum ramp** — each tile adds a percent of the unit's evasion score — rather than a flat cut:
skill still scales it, but momentum (tiles) gates it.
**Verdict:** **Chosen.** Simple, legible ("keep running to get safer"), and a genuinely different curve
from both walk (TU-scaled) and sneak (flat-full). Detailed design in §C.

### D. Energy-coupled evasion
**Idea:** sprint evasion scales with remaining **energy** instead of TU — a fresh sprinter dodges, an
exhausted one stumbles into fire.
**Grounding:** the exhaustion system already gates sprinting.
**Verdict:** **Interesting, but weak in practice.** As the user noted, a sprinting unit's **energy
falls off at almost exactly the rate its TU would while walking**, so an energy-scaled curve ends up
nearly identical to the TU-scaled walk curve — it produces little new dynamic. Parked.

---

## C. Momentum evasion — design

**Status:** designed, ready to implement.

### The model

For a **sprinting** mover, replace the flat stat cut with a momentum **ramp** — each tile moved in the
current run adds a percent of the unit's own evasion score:

```
evasion = getEvasionScore() × min(tilesMovedThisMove, maxMomentumTiles) × evasionPercentPerTile / 100
```

where `getEvasionScore()` is the standard `reactions × (currentTU/maxTU) × armorEvasion%` score.

- **Scaled by the reaction score** (per the user's refinement) — momentum is a *percent of your own
  evasion*, so a high-reactions veteran still out-dodges a rookie at the same speed; skill matters,
  it's just gated by momentum instead of applied flat.
- **A ramp, not a constant.** Standing start = 0 tiles = **0 evasion** (fully exposed, any reactor
  fires); each tile adds `evasionPercentPerTile`% of the base score, so after a few tiles at speed the
  sprinter exceeds their normal evasion and few reactors qualify. Distinct from walk (flat TU-scaled)
  and sneak (flat-full), and legible: *keep running and you get safer; the danger is the first steps.*
- Pairs with the shipped "sprint commits to its full path" rule — you can't bail mid-run, and the run
  is what earns the evasion.

Recall the reaction contest is `moverEvasion <= reactorReactionScore` → fire. With the dx-test config
(`evasionPercentPerTile: 20, maxMomentumTiles: 6`) the momentum factor runs 0 → 1.2, so a sprinter is
0% of their evasion at the first step and 120% once wound up — the early tiles are the exposure window.
(Note the base score still carries the `currentTU/maxTU` term, so a sprinter draining TU as it runs
sees the momentum ramp partially offset by exhaustion — a mild, realistic secondary effect.)

### Config

Extend `EvasionModeConfig` (already used by `evasionSprint`/`evasionSneak` + `evasionDefaults`) with
two optional fields. **If `evasionPercentPerTile > 0`, the mode uses the momentum formula; otherwise it
keeps the existing stat/TU formula** — so this is fully backward compatible and opt-in.

| Field | Default | Meaning |
|---|---|---|
| `evasionPercentPerTile` | 0 | **[DX]** Percent of the unit's evasion score gained per tile of momentum. `0` = disabled (use the stat/TU formula). |
| `maxMomentumTiles` | 0 | **[DX]** Cap on counted momentum tiles (`0` = uncapped). |

Engine defaults leave `evasionPercentPerTile: 0`, so stock and existing evasion configs are unchanged.
`dx-test.rul` opts sprint into momentum, e.g.:

```yaml
evasionDefaults:
  sprint: { evasionPercentPerTile: 20, maxMomentumTiles: 6 }   # 0% at standing start -> 120% at full wind-up
  sneak:  { statPercent: 90, tuPenaltyPercent: 0 }             # unchanged (stat-based)
```

### Plumbing — the tiles-moved counter

- New transient `int _tilesMovedThisMove` on `BattleUnit` (default 0, **not** serialized — it's only
  meaningful mid-move, and saves never land mid-move).
- **Increment** once per completed step in `UnitWalkBState::think`, at the top of the
  `if (getStatus() == STATUS_STANDING)` block — which sits *inside* the `STATUS_WALKING` branch, so it
  runs only when `keepWalking` just finished a step. This fires before that step's reaction check, so
  the count includes the tile just entered. **Do not** put it in the "unit moved from one tile to the
  other" (`_pos != _lastPos`) block: `keepWalking` flips `_pos` to the destination at the animation's
  *middle* phase but leaves `_lastPos` at the pre-move tile until the next step, so that condition stays
  true for every remaining animation frame (4 orthogonal / 8 diagonal) — incrementing there over-counts
  by that many per tile (the camera centering there is idempotent, which is why it tolerates it).
- **Reset** to 0 at move start (`UnitWalkBState::init`) so each run starts from a standing count, and
  in `BattleUnit::prepareNewTurn` as belt-and-braces against cross-turn leakage. No other reset is
  needed: `getEvasionScore(BAM_RUN)` is only ever called from `checkReactionFire` during an active
  sprint move, so the counter is always fresh-for-this-run when read.

### Where it applies

Only `BAM_RUN` reads the momentum counter (via `getEvasionScore(BAM_RUN)`), so this is a sprint-only
behavior in practice; sneak and normal are untouched. Faction-agnostic (any unit sprinting gets it),
though in practice only the player uses `BAM_RUN`.

### Implementation checklist

| File | Change |
|---|---|
| `src/Mod/Armor.h` | add `evasionPercentPerTile` / `maxMomentumTiles` to `EvasionModeConfig` + `load`; **extend the per-armor `_evasionSprint`/`_evasionSneak` sentinel init to `{ -1, -1, -1, -1 }`** so the new fields start "unset" |
| `src/Mod/Armor.cpp` (`afterLoad`) | **add per-field resolution lines for the two new fields** (`if (< 0) = evasionDefaults...`), or an armor never inherits the global momentum config — the stat fields' resolution alone is not enough |
| `src/Savegame/BattleUnit.h/.cpp` | `_tilesMovedThisMove` + accessors; momentum branch in `getEvasionScore(bam)`; reset in `prepareNewTurn` |
| `src/Battlescape/UnitWalkBState.cpp` | reset in `init`, increment in the per-tile block |
| `bin/standard/dx-test/dx-test.rul` | sprint momentum config + "what to check" comment |
| Docs | `DX-Features.md`, this doc, `docs/Ruleset-Armors.md` + `docs/Ruleset-Globals.md` (the two **[DX]** keys) |

### Testing

1. **Momentum ramp** — sprint a soldier past a reactor: a short dart (1–2 tiles into view) draws
   reaction fire; a long run that's already at speed crossing the same tile draws less. The danger is
   the first tiles out of cover.
2. **Scales with skill, gated by momentum** — a high-reactions soldier is harder to reaction-fire
   against than a low-reactions one at the *same* momentum, but both start at 0 evasion on the first
   step (the ramp, not the stat, decides the early exposure).
3. **Reset** — after a sprint, a fresh move starts exposed again (counter reset); walking/sneaking are
   unaffected (stat-based).
4. **Backward compatible** — an `evasionSprint` config without `evasionPercentPerTile` keeps the old
   stat-based behavior; no config at all = stock.

**Verbose diagnostic.** With the hidden `combatLogVerbose` option on, every reaction contest against
the top reactor now logs a line — e.g. *"Sectoid reaction vs Ramirez [sprint 4t]: evasion 58 vs
reactions 66 -> fired"* — showing the mover's move mode (and sprint momentum tiles), its move-adjusted
evasion, the reactor's score, and whether it fired or was evaded. This is the direct way to watch the
momentum ramp and tune `evasionPercentPerTile` / `maxMomentumTiles`. Emitted from
`TileEngine::getReactor` via `SavedBattleGame::logReactionEvalEvent`.

---

## Future / open options (not built — momentum-themed sprint follow-ups)

These extend the momentum idea from *evasion* into *movement commitment*. They pair with idea C (both
lean on the tiles-moved counter and the "sprint commits to its full path" rule) but each needs its own
design pass and playtesting before implementation. Logged here so they aren't lost.

### F1. Minimum sprint distance

A sprint should probably require a **minimum number of tiles** — a one-tile "sprint" is meaningless
(0 momentum → 0 evasion anyway) and reads as an exploit/typo. Options:
- Reject `BAM_RUN` for a path shorter than N tiles (fall back to a normal walk, maybe with UI feedback).
- Or let it run but only *engage* sprint speed/mechanics past tile N.

Open: what N; refund-vs-convert-to-walk when the ordered path is too short; how it interacts with a
sprint that gets cut short by terrain, reaction fire, or a spotted enemy mid-path.

### F2. Momentum carry — you can't stop on a dime when fired upon

Today, taking reaction fire **cancels the move immediately**:
[UnitWalkBState.cpp:247-251](../src/Battlescape/UnitWalkBState.cpp#L247) calls `cancelCurentMove()`
the instant `checkReactionFire` returns true. (The shipped "sprint commits to its full path" rule only
skips the *spotting* halt at [:272](../src/Battlescape/UnitWalkBState.cpp#L272); being *shot at* still
stops the run.)

The idea: a sprinter has **momentum** and shouldn't halt instantly. When reaction fire lands, let the
unit carry forward **one or two more tiles before it can stop** — plausibly **momentum-dependent** (a
unit deep into a fast run carries farther than one just starting). This composes with idea C: a
wound-up sprinter is both hard to hit *and* hard to stop, which is a coherent, readable identity.

Design questions to settle first:
- **Carry distance:** fixed 1–2 tiles, or scaled by the tiles-moved counter (e.g. `carry =
  clamp(tilesMoved / K, 1, maxCarry)`)? Reuses the momentum counter already added for idea C.
- **Reaction fire during the carry:** do reactors keep getting shots on each carried tile (momentum
  evasion still applies, so a fast mover is still hard to hit), or is the carry "free"?
- **Hard stops:** the carry must still respect impassable tiles, TU exhaustion, death/stun (a killed
  or knocked-out sprinter obviously stops), and doors/edges — the carry is "can't *choose* to stop,"
  not "ignores the map."
- **Stronger variant:** fully commit — reaction fire never stops a sprint at all (it just runs the
  whole path). The carry-then-stop version is the softer, probably better-feeling middle ground.
- **AI + player parity, and UI:** how (or whether) to telegraph that a sprint can't be halted yet.

Touch points would be `UnitWalkBState`'s `cancelCurentMove` / the reaction-fire branch, plus the
tiles-moved counter on `BattleUnit` (already present from idea C).
