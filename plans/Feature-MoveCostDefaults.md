# Feature: Mod-configurable armor move-cost defaults (`moveCostDefaults`)

**Status:** **Implemented (Jul 2026)**, builds clean (0 warnings). New Feature (spun out of the
Sprint/Sneak work — see [Feature-SprintSneakModes.md](Feature-SprintSneakModes.md)).

## Motivation

Each armor's movement costs (`moveCost: { walkPercent, runPercent, sneakPercent, … }`) fall back to
**hardcoded C++ defaults** when omitted — and those defaults make sneaking cost exactly the same as
walking (`sneakPercent` defaults to `[100, 50]`, identical to `walkPercent`). A mod that wants, say,
"sneaking is slow game-wide" would have to add a `moveCost:` block to **every armor**. There was no
way to change the *default*. This adds a top-level `moveCostDefaults:` node so a mod can retune the
baseline once, and per-armor `moveCost:` still overrides it.

## Audit — the delta

- The `{100,50}`/`{75,75}` values are `ArmorMoveCost` struct initializers in `Armor.h`; an armor's
  `moveCost:` node is only read when present ([Armor.cpp:134](../src/Mod/Armor.cpp#L134)), so an omitted
  key keeps the hardcoded default. No engine-level mod-provided default existed; YAML anchors /
  `refNode` let a mod *share* a block but not change the fallback for armors that specify nothing.

## Design

- New top-level ruleset node **`moveCostDefaults:`** with the same keys as an armor's `moveCost:`
  (`walkPercent`, `runPercent`, `sneakPercent`, `strafePercent`, the `fly*`/`climb*`/`base*` variants,
  `gravLiftPercent`). Each is an `[time%, energy%]` pair.
- Stored in `Armor::moveCostDefaults` (a static `ArmorMoveCostDefaults` whose fields default to the
  **stock values**, so behaviour is unchanged when unset — same static-reset-in-`Mod` pattern as the
  Pathfinding path colours). Reset to stock in `Mod::resetGlobalStatics`, loaded from the node in
  `Mod::loadFile`.
- **Merge-safe sentinel resolution:** each armor's `_moveCost*` field initialises to the sentinel
  `{ -1, -1 }` ("not set by this armor"). `ArmorMoveCost::load` no-ops on an absent key, so across
  mod merges only the keys an armor actually specifies get real values; the rest stay sentinel.
  `Armor::afterLoad` (which runs once after all files/merges) replaces any remaining sentinel with the
  corresponding `Armor::moveCostDefaults` value. An explicit per-armor value always wins.
- Load order is correct by construction: `Game::loadMods` → `resetGlobalStatics` (defaults→stock) →
  `loadAll`/`loadFile` (node→defaults) → `afterLoad` (defaults→armors).

Example (dx-test uses this to make sneak actually slow, since OXCE sneak is otherwise a stub):

```yaml
moveCostDefaults:
  sneakPercent: [200, 50]   # sneak: double TU per tile (slow), same energy as walking
  runPercent:   [50, 100]   # sprint: half TU (fast, covers ground), double walking's energy
```

## Touch points

- `src/Mod/Armor.h` — `ArmorMoveCostDefaults` struct + `Armor::moveCostDefaults` static; the 16
  `_moveCost*` initialisers changed to the `{ -1, -1 }` sentinel.
- `src/Mod/Armor.cpp` — static definition; `afterLoad` sentinel resolution.
- `src/Mod/Mod.cpp` — reset in `resetGlobalStatics`; load `moveCostDefaults:` in `loadFile`.
- `bin/standard/dx-test/dx-test.rul` — example making sneak slow / run ground-covering.
- Docs: `DX-Features.md`, this doc, `DX-Roadmap.md` (New Features).

## Testing plan

- dx-test: with the `moveCostDefaults` above, confirm a soldier in a stock armor (no `moveCost:`)
  now spends visibly **more** TU per tile while sneaking (Alt) and slightly fewer while sprinting
  (Ctrl) — verifying the default reaches armors that set nothing.
- An armor that *does* set `moveCost.sneakPercent` keeps its own value (override still wins).
- No `moveCostDefaults:` node ⇒ byte-for-byte stock movement (defaults equal the old hardcoded ones).
