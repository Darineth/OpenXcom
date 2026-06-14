# Feature - Combat Log

**Status:** ✅ Core infrastructure implemented (Phase 1). Partially wired — only 2 of ~8 planned emit points are active. See the TODO section below for remaining integration work.

## Overview

A floating, centered combat log displayed at the top of the battlescape screen. It shows recent battlefield events as color-coded lines that self-erase over time. Each entry has an outcome tone (NEUTRAL / GOOD / WARNING / BAD) that determines its display color, loaded from interface ruleset theming.

## How it works

The log is a transient store (`CombatLog`) owned by `SavedBattleGame`. Combat code calls `appendToCombatLog(text, outcome)` to push already-localized entries. A display widget (`CombatLogPanel`) reads the entries and draws them centered at the top of the screen, one per line, colored by outcome. Entries older than 8 seconds are pruned; the log caps at 20 visible entries (oldest scroll off first).

The store is **not serialized** — it lives only for the duration of a battle and is cleared on new battle start.

## Key files

| File | Role |
|------|------|
| `src/Savegame/CombatLog.h/.cpp` | Core store: outcome enum, entry struct, add/prune/clear |
| `src/Battlescape/CombatLogPanel.h/.cpp` | Display widget: reads log, draws centered color-coded lines |
| `src/Savegame/SavedBattleGame.h/.cpp` | Ownership + helpers (`appendToCombatLog`, `getCombatLogName`, `combatLogVictimOutcome`, `logUnitEvent`) |
| `src/Battlescape/BattlescapeState.h/.cpp` | Panel instantiation, interface ruleset theming, visibility toggle |
| `src/Engine/Options.inc.h` / `Options.cpp` | `combatLogEnabled` option registration |
| `bin/standard/xcom1/interfaces.rul` / `xcom2` | `combatLog` element + four outcome color slots (`combatLogNeutral`, `combatLogGood`, `combatLogWarning`, `combatLogBad`) |
| `bin/common/Language/DX/en-US.yml` | DX language strings (`STR_COMBATLOG_*`) |

## Interaction points

- **`SavedBattleGame::appendToCombatLog(text, outcome)`** — the main emit API. Any combat code with a `SavedBattleGame*` can call it.
- **`SavedBattleGame::logUnitEvent(msgId, unit, outcome)`** — convenience helper that resolves a gendered string + knowledge-aware unit name in one call (used for kill/stun events).
- **`SavedBattleGame::getCombatLogName(unit)`** — returns "Sectoid" if researched, "Hostile" if not visible at all. Used by the helpers above.
- **`SavedBattleGame::combatLogVictimOutcome(unit)`** — determines GOOD vs BAD based on whether the victim is friendly or hostile from the player's perspective.
- **`BattlescapeState` constructor** — reads position/size from `interfaces.rul`, wires four outcome colors, gates visibility on `Options::combatLogEnabled`.
- **`CombatLogPanel::think()`** — calls `_log->prune()` and flags redraw when entry count changes. Called as part of the battlescape think loop.

## Localization strings (existing)

| Key | Value | Notes |
|-----|-------|-------|
| `STR_COMBATLOG_UNKNOWN_UNIT` | `"Unknown"` | Fallback when unit is not visible at all |
| `STR_COMBATLOG_HOSTILE` | `"Hostile"` | When unit type is unknown but hostile |
| `STR_COMBATLOG_KILLED_MALE` / `_FEMALE` | `"{0} is killed"` | Gendered, used by `logUnitEvent()` |
| `STR_COMBATLOG_KILLED_BY_MALE` / `_FEMALE` | `"{0} was killed by {1}"` | Two-arg kill message with attacker name |
| `STR_COMBATLOG_STUNNED_MALE` / `_FEMALE` | `"{0} is knocked out"` | Gendered stun message |
| `STR_COMBATLOG_NEW_TURN` | `"- Turn {0} -"` | Single arg: turn number, NEUTRAL outcome |

## Wiring status (emit points)

Only **2 of ~8 planned emit points** are currently active:

| # | Event | File | Outcome | Status |
|---|-------|------|---------|--------|
| 1 | New turn | `NextTurnState.cpp:289` | NEUTRAL | ✅ Active |
| 2 | Kill / stun (casualties) | `BattlescapeGame.cpp:951` + `SavedBattleGame.cpp:3530-3539` | GOOD/BAD via faction | ✅ Active |
| 3 | Any unit fires weapon | `ProjectileFlyBState.cpp::createNewProjectile` + `SavedBattleGame::logFireEvent` | NEUTRAL | ✅ Active |
| 4 | Shot fired (impact) | `ProjectileFlyBState.cpp:588`, `TileEngine.cpp:4888` | NEUTRAL | 🔲 TODO |
| 5 | Reaction fire | `TileEngine.cpp:2877` | GOOD/BAD via faction | 🔲 TODO |
| 6 | Unit takes damage | `TileEngine.cpp:3161/3165/3170` | GOOD/BAD via faction | 🔲 TODO |
| 7 | Panic | (near casualty hooks) | BAD for XCOM / NEUTRAL for alien | 🔲 TODO |
| 8 | Out-of-ammo / no-LOF | `BattlescapeState.cpp:2610+` warning sites | WARNING | 🔲 TODO |

---

# TODO — Remaining Integration Work

The core infrastructure (store, panel, theming, helpers) is complete and builds cleanly. The following emit points need to be wired so the log actually fires during combat:

### 3. Any unit fires/throws ✅ DONE
**File:** `src/Battlescape/ProjectileFlyBState.cpp` (`createNewProjectile`), helpers `SavedBattleGame::logFireEvent` / `logThrowEvent`.
**What:** Emits one entry per action for *any* unit/faction — "{0} fires {1}" for shots, "{0} throws {1}" for `BA_THROW`. Wired into the shared projectile path — right beside the existing `appendToHitLog(HITLOG_NEW_SHOT, ...)` call — rather than the player-only `ActionMenuState`, so reaction fire and alien shots are covered too. Gated on `_action.autoShotCounter == 1` so a burst/auto-shot logs once, not once per bullet.
**Color/outcome:** Actor-based via `combatLogActorOutcome` — our unit acting is GOOD, an enemy acting is BAD, civilian/other NEUTRAL (the inverse of `combatLogVictimOutcome`, which colors harm done *to* a unit). Uses the actor's *current* faction so a mind-controlled unit is colored by whose side it now fights for.
**Knowledge gating:** Attacker name via `getCombatLogName` (unresearched hostiles read "Hostile"). Weapon/item name via `getCombatLogWeaponName`, which gates *hostile* gear on its unlocking research (`RuleItem::getRequirements`) — unresearched hostile weapons read "an unknown weapon"; our own gear and researched enemy gear are named plainly.
**Strings:** `STR_COMBATLOG_FIRED`, `STR_COMBATLOG_THROWS`, `STR_COMBATLOG_UNKNOWN_WEAPON`.

### 4. Shot fired / projectile impact
**Files:** `src/Battlescape/ProjectileFlyBState.cpp:588`, `src/Battlescape/TileEngine.cpp:4888`  
**What:** Emit a "hit" entry when a projectile connects with its target tile/unit. Call `appendToCombatLog(...)` with outcome NEUTRAL. Add `STR_COMBATLOG_HIT` to DX YAML.

### 5. Reaction fire
**File:** `src/Battlescape/TileEngine.cpp:2877`  
**What:** Emit a reaction-fire entry when a unit fires on reaction to detection. Use `combatLogVictimOutcome()` for the victim's side (GOOD if XCOM reacts, BAD if alien reacts).

### 6. Unit takes damage
**Files:** `src/Battlescape/TileEngine.cpp:3161/3165/3170`  
**What:** Emit a "took damage" entry at the damage application hooks. GOOD for enemy, BAD for XCOM soldier. Phase 5 (firing model) will upgrade these to exact numeric damage values; for now a coarse line is sufficient.

### 7. Panic
**Where:** Near the casualty hooks in `BattlescapeGame` (around `checkForCasualties`).  
**What:** Emit a panic entry when a unit fails morale and panics. BAD for XCOM soldier, NEUTRAL or GOOD if alien panics.

### 8. Out-of-ammo / no-LOF
**Where:** Existing `warning()` call sites in `src/Battlescape/BattlescapeState.cpp:2610+`.  
**What:** Emit a WARNING-outcome entry alongside the existing warning message for ammo shortages, line-of-fire failures, etc.

### 9. Research-gated text helper (optional enhancement)
The current `getCombatLogName()` already handles basic knowledge-aware naming ("Hostile" vs specific alien type). A more granular research gate — e.g. revealing numeric damage values or unit-specific lore when a race is fully researched — can be added later as an enhancement to the name helper and string composition logic.

### 10. Configurable options (optional)
- `combatLogMaxEntries` — override the default of 20 visible entries.
- `combatLogLifetime` — override the default 8-second decay lifetime.
- Both would follow the existing pattern in `Options.inc.h` / `Options.cpp`.

### 11. Localization strings to add (DX YAML)
The following keys are referenced by planned emit points but may not yet exist:
- `STR_COMBATLOG_FIRED` — "{0} fires {1}"
- `STR_COMBATLOG_HIT` — "{0} hits {1}"

Add these to `bin/common/Language/DX/en-US.yml`.