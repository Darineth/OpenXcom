# Feature: Movement-mode evasion (sprint/sneak affect reaction-fire evasion)

**Status:** **Implemented (Jul 2026)**, builds clean (0 warnings). Phase 7 follow-on to
[Feature-ReactionScoringSplit.md](Feature-ReactionScoringSplit.md) (which this depends on) and the
payoff for [Feature-SprintSneakModes.md](Feature-SprintSneakModes.md)'s sneak. Mod-configurable
**globally and per-armor**.

## Motivation

The reaction split gave a unit a *defensive* `getEvasionScore()` separate from its offensive reaction
score, but nothing yet varied it by **how the unit is moving**. Sneaking should make you harder to
react-fire against (low profile), sprinting easier (exposed). This wires that in, and — per request —
lets mods tune the amounts globally and override per-armor.

## Design

The base reaction/evasion score is `reactions × (currentTU / maxTU)` — two parts: the **reactions
stat** and the **TU/maxTU penalty**. A movement mode reshapes *both* independently, so the config is a
pair per mode (`EvasionModeConfig`):

- **`statPercent`** — percent of the reactions stat used (100 = full).
- **`tuPenaltyPercent`** — how strongly low TU penalises the score:
  `tuFactor = 1 - (1 - currentTU/maxTU) × tuPenaltyPercent/100`. So 100 = the vanilla TU/maxTU term,
  0 = no penalty (full score regardless of spent TU).

This expresses the target cases exactly: **Sprint = `{statPercent:50, tuPenaltyPercent:50}`** (half
reactions, only half the TU penalty), **Sneak = `{statPercent:100, tuPenaltyPercent:0}`** (full
reactions at all times).

- **`BattleUnit::getEvasionScore(BattleActionMove bam)`** recomputes the score with the mode's
  `statPercent`/`tuPenaltyPercent` (then × the armor's flat base `evasion`). `BAM_RUN`/`BAM_SNEAK` use
  the sprint/sneak config; normal/strafe use the plain `getEvasionScore()`.
- **Threaded through reaction fire:** `TileEngine::checkReactionFire` reads `originalAction.getMoveType()`
  and passes it to `getSpottingUnits` / `getReactor`, whose two *defensive* (mover) sites call
  `unit->getEvasionScore(moverMove)`. Spotters' offensive scores are untouched; non-move reactions
  report `BAM_NORMAL` ⇒ plain score.
- **Per-armor:** `Armor.evasionSprint` / `evasionSneak` sub-maps. Each field defaults to `-1`
  ("use the mod-wide default"), resolved **per field** in `Armor::afterLoad` (merge-safe).
- **Global:** top-level `evasionDefaults:` (`sprint:`/`sneak:` sub-maps) → static
  `Armor::evasionDefaults`, reset in `Mod::resetGlobalStatics`, loaded in `Mod::loadFile`.
- **`{100,100}` for both modes ⇒ no behaviour change** (that reproduces `reactions × TU/maxTU`).

```yaml
evasionDefaults:                                    # mod-wide
  sprint: { statPercent: 50,  tuPenaltyPercent: 50 }
  sneak:  { statPercent: 100, tuPenaltyPercent: 0 }
armors:
  - type: STR_NIMBLE_SUIT
    evasionSprint: { statPercent: 75, tuPenaltyPercent: 25 }   # per-field override of the global sprint
```

## Touch points

- `src/Savegame/BattleUnit.h/.cpp` — `getEvasionScore(BattleActionMove)` overload (+ forward-decl of
  `BattleActionMove` in the header).
- `src/Mod/Armor.h/.cpp` — `EvasionModeConfig` + `ArmorEvasionDefaults` structs + static;
  `_evasionSprint`/`_evasionSneak` configs, getters, load, per-field afterLoad resolution.
- `src/Mod/Mod.cpp` — reset + load `evasionDefaults:`.
- `src/Battlescape/TileEngine.h/.cpp` — `BattleActionMove` param on `getSpottingUnits`/`getReactor`;
  `checkReactionFire` threads `originalAction.getMoveType()`.
- `bin/standard/dx-test/dx-test.rul` — global `evasionDefaults` + a per-armor `evasionSneak` override.
- Docs: `DX-Features.md`, this doc, `DX-Roadmap.md`.

## Testing plan

- dx-test (global sprint `{50,50}`, sneak `{100,0}`): a soldier drawing enemy overwatch takes **more**
  reaction fire while **sprinting** (Ctrl) at full TU and **less** while **sneaking** (Alt) than
  walking; a **TU-depleted** sneaker stays fully evasive (no TU penalty), and a depleted sprinter is
  less punished than a walker.
- `STR_PERSONAL_ARMOR_UC` (`evasionSprint: {75,25}`) sprints more evasively than a stock-armor soldier —
  verifying the per-field per-armor override beats the global default.
- No `evasionDefaults`/armor fields ⇒ reaction fire identical to the reaction-split baseline
  (`{100,100}` reproduces `reactions × TU/maxTU`).

## Follow-ons

- Strafe currently has no evasion modifier (only run/sneak). Could add `evasionStrafe` if wanted.
- A `RuleStatBonus`/script-hook evasion driver (richer, formula-based) remains open.
