# Feature: Reaction Scoring Split (offensive reaction / defensive evasion)

**Status:** **Implemented (Jul 2026)**, builds clean (0 warnings). Phase 7 (Tactical Unit Systems).
Foundation that lets a unit's *defensive* reaction-fire resistance be modified independently of its
*offensive* reaction ability — notably unblocking real Sneak **evasion**
([Feature-SprintSneakModes.md](Feature-SprintSneakModes.md)). Shipped: `getEvasionScore()`, the armor
`evasion:` percent driver (default 100 = no change), the two defensive TileEngine sites rewired, and a
`getEvasionScore` Y-Script binding. dx-test's STR_PERSONAL_ARMOR_UC uses `evasion: 200`.

## Audit — current OXCE reaction scoring

A single `BattleUnit::getReactionScore()` ([BattleUnit.cpp:2789](../src/Savegame/BattleUnit.cpp#L2789))
= `reactions * currentTU / maxTU`. Reaction fire (`TileEngine`) uses it with **two different
meanings**:

- **Defensive (the moving unit / potential victim):**
  - [TileEngine.cpp:2653](../src/Battlescape/TileEngine.cpp#L2653) — `threshold = unit->getReactionScore()`;
    a spotter must beat this to react.
  - [:2775](../src/Battlescape/TileEngine.cpp#L2775) — `unit->getReactionScore() <= best->reactionScore`;
    the best reactor only fires if the mover's score doesn't beat it.
- **Offensive (the spotter / potential reactor):**
  - [:2664](../src/Battlescape/TileEngine.cpp#L2664) — `bu->getReactionScore() >= threshold`.
  - [:2802](../src/Battlescape/TileEngine.cpp#L2802) (`determineReactionType`) — the reactor's stored score.

So the *same number* determines both "how good am I at reacting" and "how hard am I to react
against." There's no way to make a unit harder to react-fire at (evade) without also making it better
at reacting. Precedent for an armor-side defensive modifier already exists: `Armor._meleeDodge`
(a `RuleStatBonus`, [Armor.h:237](../src/Mod/Armor.h#L237)).

## Design

- **New `BattleUnit::getEvasionScore()`** — the *defensive* score. Base = `getReactionScore()` (so
  behaviour is unchanged by default), times the wearer's **armor evasion**:
  `getEvasionScore() = getReactionScore() * armor->getEvasion() / 100`.
- **New `Armor` field `evasion:`** (int **percent, default 100** → no change). >100 makes the unit
  harder to react-fire at (stealth armor); <100 easier. This is the first concrete evasion driver and
  keeps the split from being a pure rename. Default 100 everywhere ⇒ **zero balance change** out of the
  box.
- **Wire the split in `TileEngine`:** the two *defensive/mover* sites (2653, 2775) call
  `getEvasionScore()`; the two *offensive/spotter* sites (2664, 2802) keep `getReactionScore()`.
- **Y-Script:** expose `getEvasionScore` on `BattleUnit` (read), mirroring the existing
  `getReactionScore` binding, so mods/scripts can read/react to it. (A script *modify* hook and other
  drivers — movement mode, `RuleStatBonus` — are follow-ons; see below.)

## Touch points

- `src/Savegame/BattleUnit.h/.cpp` — `getEvasionScore()`; Y-Script binding.
- `src/Mod/Armor.h/.cpp` — `_evasion` field (`evasion:` key, default 100), `getEvasion()`.
- `src/Battlescape/TileEngine.cpp` — swap the two defensive sites to `getEvasionScore()`.
- `bin/standard/dx-test/dx-test.rul` — a test armor with `evasion:` > 100 to verify.
- Docs: `DX-Features.md`, this doc, `DX-Roadmap.md`.

## Testing plan

- dx-test: give a soldier armor `evasion: 200`; confirm enemies react-fire on it noticeably less than
  on a stock-armor soldier making the same move (and that the evasive soldier's *own* reaction fire is
  unchanged — offence untouched).
- `evasion: 100` (or unset) ⇒ reaction fire identical to stock (regression check).

## Follow-ons (not this pass)

- **Movement-mode evasion** — sneak raises / sprint lowers evasion. **Done** (Jul 2026): see
  [Feature-MovementModeEvasion.md](Feature-MovementModeEvasion.md); mod-configurable globally
  (`evasionDefaults:`) and per-armor (`evasionSprint`/`evasionSneak`).
- **`RuleStatBonus` evasion** (like `_meleeDodge`) and a **script modify-hook** for richer, formula-
  and script-driven evasion.
