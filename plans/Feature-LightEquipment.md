# Feature: Light / Illumination Equipment (directional cone light + sneak light gate)

**Status:** Implemented (Jul 2026) — builds clean (Release/Win32, 0 warnings); needs playtest
(dx-test ships a `STR_FLASHLIGHT` item + `sneakDefaults` config with in-file test instructions).
Implementation notes: cone filter lives inside `TileEngine::addLight` (optional `coneDirection`/
`coneAngleDeg` params, so classic + enhanced/occluded lighting both work); unit emission extracted to
`TileEngine::getUnitLightEmission` (struct: circular + strongest cone item), shared by
`calculateUnitLighting` and the sneak gate; carried-glow scan covers hands + DX single-item slots;
`UnitTurnBState` re-runs `LL_UNITS` for cone carriers only; gates in `Pathfinding` (preview/mode
selection) + `BattlescapeGame` (order + warning); config `Armor::sneakDefaults` loaded from the
top-level `sneakDefaults:` node (DX *Defaults pattern).

Roadmap: Phase 8 **"Light / illumination equipment"** (the Effects framework this once depended on was
dropped — see the Phase 8 audit note in `DX-Roadmap.md`). Also delivers the Phase 7 Sneak leftover:
the **"no creeping while glowing"** gate.

## OXCE audit (Jul 2026) — what already exists

The current engine covers far more than the original plan assumed:

- **Carried light is native.** A `BT_FLARE` item **glows** when lit (`BattleItem::getGlow`,
  [BattleItem.cpp:1263](../src/Savegame/BattleItem.cpp#L1263)) — either always-on (no fuse) or when
  primed (fuse set); its **power is its light radius** (`getGlowRange` → `getPowerBonus`, so scripts can
  scale it). Ground flares light via the `LL_ITEMS` layer
  ([TileEngine.cpp:1026](../src/Battlescape/TileEngine.cpp#L1026)); **held flares light the carrier** —
  `calculateUnitLighting` checks both hands ([TileEngine.cpp:1086](../src/Battlescape/TileEngine.cpp#L1086)).
  A torch/lantern is therefore just a ruleset item today (BT_FLARE + power + fuse for on/off, unprime to
  switch off). **No engine work needed for circular carried light.**
- **Armor light is native, per-faction** (`personalLightFriend/Hostile/Neutral`), with the in-battle
  personal-light toggle persisted on the save.
- **The lighting engine** ([TileEngine::calculateLighting](../src/Battlescape/TileEngine.cpp#L1124)) is
  layered (`LL_AMBIENT/FIRE/ITEMS/UNITS`, max-combined per tile) and **incremental** (dirty-region
  flood bounded by `maxDynamicLightDistance`); light spread is `power − distance` circular
  (`TileEngine::addLight`), with optional "enhanced lighting" raycasting (walls/smoke/height occlusion)
  per layer. Recomputing only `LL_UNITS` is the cheap path.
- **Directionality precedent:** the DX overwatch cone's membership test
  (`isInOverwatchCone`, [TileEngine.cpp:2600](../src/Battlescape/TileEngine.cpp#L2600)) — a cosine
  bearing test. Unit facing is `getDirection()` (0–7) → `Pathfinding::directionToVector`.
- **Gaps confirmed:** no directional/cone light anywhere; **turning does not retrigger lighting**
  (`UnitTurnBState` only recalculates FOV); no per-unit "how much light am I emitting" accessor (the
  logic lives inline in `calculateUnitLighting`); only **hand** slots contribute held-item glow.
- **Legacy check:** the legacy fork's `addDirectionalLight` (90°-wedge, `power − distance`, no
  occlusion, all z-levels) exists in its tree but has **zero callers** — legacy light equipment was
  never actually wired. DX builds this fresh; the wedge is only a shape reference (our version should
  reuse the engine's occlusion-aware spread instead).

## Design — true deltas only

### 1. Directional (cone) light — "flashlight"

- **Ruleset (per item):** two new `RuleItem` fields —
  - `glowConeAngle:` (degrees, full angle; 0 = default = circular glow, i.e. feature off)
  - the existing `power` remains the range/brightness, `getGlow()` gating (fuse/prime) remains the
    on/off switch. A flashlight is a `BT_FLARE` with `glowConeAngle: 60` and a fuse (prime = on,
    unprime = off — native UX).
- **Engine:**
  - New cone-aware spread: extend `addLight` with an optional cone filter (facing vector + half-angle
    cosine test, same math as `isInOverwatchCone`) so both classic and enhanced (occlusion) modes work
    unchanged; tiles outside the cone are skipped.
  - `calculateUnitLighting`: a held glowing item with `glowConeAngle > 0` emits the cone in the
    **carrier's facing** (`getDirection()` → direction vector) instead of a circle.
  - **Turning re-triggers lighting** for cone carriers: `UnitTurnBState` gains a
    `calculateLighting(LL_UNITS, pos, radius)` call, gated to units that actually carry a glowing cone
    item (so ordinary turns stay free).
  - On the **ground**, a lit cone item has no facing → falls back to a circular glow at the same power.
- **AI/visibility:** none needed — light already feeds spotting via tile shade.

### 2. Sneak light gate — "no creeping while glowing"

- Extract the per-unit emission computation from `calculateUnitLighting` into a reusable
  `TileEngine::getUnitEmittedLight(const BattleUnit*)` (armor personal light [with toggle], held glow
  items, on-fire) and have both callers share it.
- Gate sneak where the mode is selected ([Pathfinding.cpp:1261](../src/Battlescape/Pathfinding.cpp#L1261),
  the preview/`bam` choice) and where the action's sneak flag is set
  ([BattlescapeGame.cpp:2141](../src/Battlescape/BattlescapeGame.cpp#L2141)): if
  `getUnitEmittedLight(unit) > threshold`, sneak silently degrades to a normal walk (no purple preview),
  with a one-shot warning message when the player explicitly orders a sneak move.
- **The personal-light toggle becomes a tactical control:** switching your light off (existing hotkey)
  is how a glowing soldier becomes sneak-capable. On-fire units can never sneak (light 15).
- **Config:** mod-wide `sneakMaxLight` (threshold; sneak allowed when emitted light ≤ value) in the
  existing DX global-defaults style. Default TBD (see Q2) — note the stock friend personal light is 15,
  so any threshold below 15 means "toggle your light off to sneak", which is the intended trade-off.

### 3. Non-hand glow slots (small DX-consistent extension)

OXCE only counts **hand** items for carried glow. DX's utility/equipment slots (`INV_UTILITY`/`INV_EQUIP`,
single-item "worn gear" sockets) are a natural home for a shoulder lamp — extend the carried-glow scan
to include them (a lit lantern clipped to your webbing glows; one stowed in a backpack does not).

## Out of scope

- Armor-mounted cone light (headlamp) — could reuse the same spread later via an armor field; not now.
- Any generic effect system (see the Phase 8 audit).
- Night-vision items (native armor fields / later concern).

## Resolved decisions (Jul 2026)

1. **Cone shape** — as proposed: `glowConeAngle` = full angle in degrees (0 = off/circular), axis =
  carrier facing, `power − distance` falloff inside the cone, z-spread as circular.
2. **Sneak threshold** — personal light **counts** (toggle it off to sneak — the tactical trade-off);
  mod-wide `sneakDefaults: { maxLight: 5 }` default (small glows tolerated; lit flares/torches and the
  stock personal light 15 block sneaking; on-fire units can never sneak).
3. **Utility-slot glow** — **yes**: carried glow counts hands + DX single-item slots
  (`INV_UTILITY`/`INV_EQUIP`).
4. **Ground cone items** — circular fallback at **reduced (half) power** (directional optics don't
  spread well).
