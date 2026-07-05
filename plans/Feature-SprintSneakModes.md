# Feature: Sprint & Sneak — surface & polish OXCE movement modes

**Status:** **Implemented (Jul 2026)**, builds clean (0 warnings). Phase 7 (Tactical Unit Systems),
first item. **Not** building movement modes — OXCE already has them; this surfaces and polishes them
(DX "prefer visible over hidden" philosophy, [[dx-surface-hidden-features]]). Shipped the three deltas
below (blue/purple path colours, ~2× sprint animation, sprint commits to its full path); accuracy /
evasion / alertness tuning deferred to Reaction Scoring Split. Palette colour blocks
(`blue=8`, `purple=13`) are provisional — confirm visually.

## Audit — OXCE-Plus already has the movement-mode core

- `enum BattleActionMove { BAM_NORMAL, BAM_RUN, BAM_STRAFE, BAM_SNEAK, BAM_MISSILE }`
  ([BattlescapeGame.h:43](../src/Battlescape/BattlescapeGame.h#L43)).
- **Run** (Ctrl) and **Sneak** (Alt) are wired in [Pathfinding.cpp:1255-1259](../src/Battlescape/Pathfinding.cpp#L1255),
  armor-gated via `allowsRunning`/`allowsSneaking` (default on).
- Per-mode **TU *and* energy** cost multipliers — `getMoveCostRun`/`getMoveCostSneak` applied in
  `Pathfinding::getTUCost` ([:719,:741](../src/Battlescape/Pathfinding.cpp#L719)) to both the time
  and energy of every step.
- **Sneak (`BAM_SNEAK`, Alt) is nearly a stub in stock OXCE.** It applies the armor's `sneakPercent`
  move cost — but that **defaults to `{100, 50}`, identical to walking** — forces `MT_WALK`
  ([:811](../src/Battlescape/Pathfinding.cpp#L811)), and exposes `move_sneak` to Y-Script. It has **no
  built-in stealth / detection / reaction effect**. (The "avoid visible tiles" 2× cost at
  [:94](../src/Battlescape/Pathfinding.cpp#L94)/[:228](../src/Battlescape/Pathfinding.cpp#L228) is a
  *different*, AI-only mechanic — `Options::sneakyAI` for hostile units — not the player sneak mode.)
- AI already runs to escape/close ([AIModule.cpp:1601](../src/Battlescape/AIModule.cpp#L1601));
  modes exposed to scripts (`move_run`, `move_sneak`).

**So for Run the mechanics/costs exist and just need surfacing.** Sneak, by contrast, exists as a
*mode* (Alt, armor-gated, walk-forced) but is mechanically hollow — it has no stealth payoff yet. The
DX delta is therefore: **surface Run** (visible sprint) now, and give **Sneak real mechanics** later
(evasion via Reaction Scoring Split, the light gate in Phase 8); this pass only adds sneak's colour.

## Deltas (this feature)

1. **Path-preview colors** — the aim/move preview colours each tile green/yellow/red by affordability
   only ([Pathfinding.cpp:1303](../src/Battlescape/Pathfinding.cpp#L1303)). Key it on the already-computed
   `bam`: **blue** when running (sprint), **purple** when sneaking, keeping **red** for an unaffordable
   step. New `Pathfinding::blue`/`purple` static colours (palette block indices, like `green=4`/etc.),
   **configurable** via a `pathfindingDX` interface element (`color` = sprint/blue, `color2` =
   sneak/purple) with engine-default fallback — mirroring how the base `pathfinding` element sets
   green/yellow/red, and necessary because the block indices differ per palette (TFTD's `pathfinding`
   uses 6/2/12 vs UFO's 4/10/3). Shipped in both `xcom1`/`xcom2` `interfaces.rul`.
2. **Movement-mode animation pace** — the walk animation speed matches the mode. `setNormalWalkSpeed`
   ([UnitWalkBState.cpp:519](../src/Battlescape/UnitWalkBState.cpp#L519)) sets the frame interval to
   `battleXcomSpeed`/`battleAlienSpeed`; for `BAM_RUN` scale it down (~50% → ~2× faster) and for
   `BAM_SNEAK` scale it up (~150% → a slower, careful crawl), both tunable. (Sneak's slower crawl was a
   legacy-DX touch.)
3. **Sprint cancel-prevention** — a moving unit auto-stops when it spots a new enemy (the "Company!"
   halt at [:265](../src/Battlescape/UnitWalkBState.cpp#L265) and the turn-reveal halt at
   [:425](../src/Battlescape/UnitWalkBState.cpp#L425)). For `BAM_RUN`, skip that halt so a sprint
   **commits to its full path** — fast but you can't stop to react to what you run into. (Player and AI
   both; the desperate/charging exemptions already there stay.)

## Deferred (not this pass)

- **Accuracy / evasion / alertness tuning** — the roadmap's "high hit chance" (sprint) and "high
  alertness, maintains evasion" (sneak). The cost multipliers + sneak-detection already carry much of
  this, and explicit evasion belongs with **Reaction Scoring Split** (`getEvasionScore`). Revisit
  after that lands. ("high hit chance" for a *sprinting* unit is also semantically unclear — clarify
  then.)

## Touch points

- `src/Battlescape/Pathfinding.h/.cpp` — add `blue`/`purple` statics + defaults; colour the preview
  by `bam` in `previewPath`.
- `src/Mod/Mod.cpp` — reset `blue`/`purple` alongside the other pathing colours.
- `src/Battlescape/BattlescapeState.cpp` — load `blue`/`purple` from the optional `pathfindingDX`
  interface element (INT_MAX fallback), next to the existing `pathfinding` load.
- `bin/standard/xcom1/interfaces.rul`, `bin/standard/xcom2/interfaces.rul` — `pathfindingDX` element.
- `src/Battlescape/UnitWalkBState.cpp` — `setNormalWalkSpeed` run acceleration; skip the two
  spot-halts for `BAM_RUN`.
- Docs: `DX-Features.md`, this doc, `DX-Roadmap.md` (tick Sprint + Sneak sub-items covered).

## Open questions

1. **Cancel-prevention scope** — confirm "prevent cancelling while sprinting" = *don't auto-stop on
   spotting an enemy mid-sprint* (commit to the full run). Recommended. Alternative: leave the
   spot-halt and only prevent some other cancel.
2. **Sprint speed factor** — how much faster the run animation is (default ~2×, i.e. half the frame
   interval). Tunable.
3. **Blue/purple palette blocks** — pick defaults, confirm visually in-game.
