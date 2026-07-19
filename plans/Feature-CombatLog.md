# Feature - Combat Log

**Status:** ✅ Implemented. Core infrastructure plus all scoped emit points (#1–#17) are wired and committed, including the psi coverage (#12–#17: cast, contest outcome, mind blast damage, backlash, clairvoyance, and the verbose contest maths). A hidden `combatLogVerbose` option gates extra diagnostic detail (per-side armor damage, the damage-calc breakdown, overwatch evaluation, MC upkeep, and the psi roll). Remaining items are optional/deferred only: pre-fire no-LOF warnings (#8b), finer research-gated text (#9), and configurable options (#10).

## Overview

A floating, centered combat log displayed at the top of the battlescape screen. It shows recent battlefield events as color-coded lines that self-erase over time. Each entry has an outcome tone (NEUTRAL / GOOD / WARNING / BAD) that determines its display color, loaded from interface ruleset theming.

## How it works

The log is a transient store (`CombatLog`) owned by `SavedBattleGame`. Combat code calls `appendToCombatLog(text, outcome)` to push already-localized entries. A display widget (`CombatLogPanel`) reads the entries and draws them centered at the top of the screen, one per line, colored by outcome. Entries older than 8 seconds are pruned; the log caps at 20 visible entries (oldest scroll off first).

The store is **not serialized** — it lives only for the duration of a battle and is cleared on new battle start. Each entry's lifetime is `Options::combatLogDuration` seconds (advanced option, default 8, range 1–60), read directly by `CombatLog::prune()` each frame, so duration changes take effect immediately, even mid-battle.

## Key files

| File | Role |
|------|------|
| `src/Savegame/CombatLog.h/.cpp` | Core store: outcome enum, entry struct, add/prune/clear |
| `src/Battlescape/CombatLogPanel.h/.cpp` | Display widget: reads log, draws centered color-coded lines |
| `src/Savegame/SavedBattleGame.h/.cpp` | Ownership + helpers: emit API (`appendToCombatLog`), naming (`getCombatLogName`, `getCombatLogWeaponName`), tone (`combatLogVictimOutcome`, `combatLogActorOutcome`), and per-event loggers (`logUnitEvent`, `logKillEvent`, `logFireEvent`, `logThrowEvent`, `logMeleeEvent`, `logHitEvent`, `logArmorDamageEvent`, `logDamageCalcEvent`, `logPanicEvent`, `logOutOfAmmoEvent`) |
| `src/Battlescape/BattlescapeState.h/.cpp` | Panel instantiation, interface ruleset theming, visibility toggle |
| `src/Engine/Options.inc.h` / `Options.cpp` | `combatLogEnabled` option registration; `combatLogDuration` (entry lifetime, seconds) advanced option; hidden `combatLogVerbose` option for extra diagnostic detail |
| `bin/standard/xcom1/interfaces.rul` / `xcom2` | `combatLog` element + four outcome color slots (`combatLogNeutral`, `combatLogGood`, `combatLogWarning`, `combatLogBad`) |
| `bin/common/Language/DX/en-US.yml` | DX language strings (`STR_COMBATLOG_*`) |

## Interaction points

- **`SavedBattleGame::appendToCombatLog(text, outcome)`** — the main emit API. Any combat code with a `SavedBattleGame*` can call it.
- **`SavedBattleGame::logUnitEvent(msgId, unit, outcome)`** — convenience helper that resolves a gendered string + knowledge-aware unit name in one call (used for kill/stun events).
- **`SavedBattleGame::getCombatLogName(unit)`** — returns "Sectoid" if researched, "Hostile" if not visible at all. Used by the helpers above.
- **`SavedBattleGame::combatLogVictimOutcome(unit)`** — determines GOOD vs BAD based on whether the victim is friendly or hostile from the player's perspective.
- **`BattlescapeState` constructor** — reads position/size from `interfaces.rul`, wires four outcome colors, gates visibility on `Options::combatLogEnabled`.
- **`CombatLogPanel::think()`** — calls `_log->prune()` and flags redraw when entry count changes. Called as part of the battlescape think loop.

## Localization strings (infrastructure / casualties)

This table covers the original core strings. Strings added when wiring each emit point (fire/throw/shot-type, melee, hit/damage/wounds, reaction variants, panic/berserk, out-of-ammo) are listed in the **Strings:** line of the matching section under *Implementation Notes*.

| Key | Value | Notes |
|-----|-------|-------|
| `STR_COMBATLOG_UNKNOWN_UNIT` | `"Unknown"` | Fallback when unit is not visible at all |
| `STR_COMBATLOG_HOSTILE` | `"Hostile"` | When unit type is unknown but hostile |
| `STR_COMBATLOG_KILLED_MALE` / `_FEMALE` | `"{0} is killed"` | Gendered, used by `logUnitEvent()` |
| `STR_COMBATLOG_KILLED_BY_MALE` / `_FEMALE` | `"{0} was killed by {1}"` | Two-arg kill message with attacker name |
| `STR_COMBATLOG_STUNNED_MALE` / `_FEMALE` | `"{0} is knocked out"` | Gendered stun message |
| `STR_COMBATLOG_NEW_TURN` | `"- Turn {0} -"` | Single arg: turn number, NEUTRAL outcome |

## Wiring status (emit points)

All planned emit points are active:

| # | Event | File | Outcome | Status |
|---|-------|------|---------|--------|
| 1 | New turn | `NextTurnState.cpp:289` | NEUTRAL | ✅ Active |
| 2 | Kill / stun (casualties) | `BattlescapeGame.cpp:951` + `SavedBattleGame.cpp:3530-3539` | GOOD/BAD via faction | ✅ Active |
| 3 | Any unit fires / throws | `ProjectileFlyBState.cpp::createNewProjectile` + `SavedBattleGame::logFireEvent`/`logThrowEvent` | actor-based | ✅ Active |
| 4 | Any unit melee attack | `MeleeAttackBState.cpp::init` + `SavedBattleGame::logMeleeEvent` | actor-based | ✅ Active |
| 5 | Hit / damage on a unit | `TileEngine.cpp::hitUnit` + `SavedBattleGame::logHitEvent` | GOOD/BAD via victim | ✅ Active |
| 6 | Reaction fire | `BattleAction::reaction` flag (set in `TileEngine::tryReaction`) → reaction-variant wording in #3 fire/melee lines | actor-based | ✅ Active |
| 7 | Unit takes damage | — superseded by #4 (same `hitUnit` hook) | GOOD/BAD via victim | ✅ via #4 |
| 8 | Panic / berserk | `BattlescapeGame::handlePanickingUnit` + `SavedBattleGame::logPanicEvent` | GOOD/BAD via unit | ✅ Active |
| 9 | Out-of-ammo (weapon emptied by the shot) | `ProjectileFlyBState::createNewProjectile` + `SavedBattleGame::logOutOfAmmoEvent` | WARNING | ✅ Active |
| 10 | Armor takes damage (verbose only) | `BattleUnit::damage` + `SavedBattleGame::logArmorDamageEvent` | GOOD/BAD via victim | ✅ Active (gated on `combatLogVerbose`) |
| 11 | Damage calculation breakdown (verbose only) | `BattleUnit::damage` + `SavedBattleGame::logDamageCalcEvent` | GOOD/BAD via victim | ✅ Active (gated on `combatLogVerbose`); names the damage type and the stats hit |
| 12 | Psi cast (panic / MC / mind blast) | `PsiAttackBState::init` + `SavedBattleGame::logPsiCastEvent` | actor-based | ✅ Active |
| 13 | Psi contest outcome (took hold / resisted) | `TileEngine::psiAttack` + `::mindBlast` + `SavedBattleGame::logPsiResultEvent` | GOOD/BAD via victim, inverted on a resist | ✅ Active |
| 14 | Mind blast damage | `TileEngine::hitUnit` (BA_MINDBLAST branch) + `SavedBattleGame::logMindBlastEvent` | GOOD/BAD via victim | ✅ Active |
| 15 | Psychic backlash on the caster | `TileEngine::applyPsiBacklash` + `SavedBattleGame::logPsiBacklashEvent` | GOOD/BAD via caster-as-victim | ✅ Active |
| 16 | Clairvoyance sweep | `TileEngine::clairvoyance` + `SavedBattleGame::logClairvoyanceEvent` | actor-based | ✅ Active |
| 17 | Psi contest maths (verbose only) | `TileEngine::psiAttack` + `::mindBlast` + `SavedBattleGame::logPsiRollEvent` | NEUTRAL | ✅ Active (gated on `combatLogVerbose`) |

---

# Implementation Notes

The core infrastructure (store, panel, theming, helpers) and all scoped emit points are complete. Each section below documents how a point was wired (✅ DONE) or why it remains open (deferred/optional).

### 3. Any unit fires/throws ✅ DONE
**File:** `src/Battlescape/ProjectileFlyBState.cpp` (`createNewProjectile`), helpers `SavedBattleGame::logFireEvent` / `logThrowEvent`.
**What:** Emits one entry per action for *any* unit/faction — "{0} fires {1} (Snap Shot)" for shots, "{0} throws {1}" for `BA_THROW`. Wired into the shared projectile path — right beside the existing `appendToHitLog(HITLOG_NEW_SHOT, ...)` call — rather than the player-only `ActionMenuState`, so reaction fire and alien shots are covered too. Gated on `_action.autoShotCounter == 1` so a burst/auto-shot logs once, not once per bullet.
**Shot type:** The fire line appends the shot mode as a parenthetical (Snap/Aimed/Auto/Launch). The label is resolved at the call site from the weapon's `RuleItemAction::name` (`getConfigSnap()/Aimed()/Auto()->name`, so it honors per-weapon custom names; `STR_LAUNCH_MISSILE` for blaster launch) and passed into `logFireEvent`, which wraps it via `STR_COMBATLOG_SHOT_TYPE` ("({0})"). Applies to the reaction variant too.
**Color/outcome:** Actor-based via `combatLogActorOutcome` — our unit acting is GOOD, an enemy acting is BAD, civilian/other NEUTRAL (the inverse of `combatLogVictimOutcome`, which colors harm done *to* a unit). Uses the actor's *current* faction so a mind-controlled unit is colored by whose side it now fights for.
**Melee (3b):** `MeleeAttackBState::init` emits "{0} strikes with {1}" via `logMeleeEvent`, once per melee action. Hooked in `init()` (which runs once) rather than `performMeleeAttack()` (which re-runs for multi-hit AI melee), so a multi-strike attack still logs a single line. Terrain melee (hitting a wall/object, no target unit) is not logged — `init` returns before this point for that case. Same knowledge gating and actor-based color as fire/throw.
**Knowledge gating:** Attacker name via `getCombatLogName` (unresearched hostiles read "Hostile"). Weapon/item name via `getCombatLogWeaponName`, which gates *hostile* gear on its unlocking research (`RuleItem::getRequirements`) — unresearched hostile weapons read "an unknown weapon"; our own gear and researched enemy gear are named plainly.
**Strings:** `STR_COMBATLOG_FIRED`, `STR_COMBATLOG_THROWS`, `STR_COMBATLOG_UNKNOWN_WEAPON`, `STR_COMBATLOG_SHOT_TYPE`.

### 4. Hit / damage on a unit ✅ DONE
**Hook:** `TileEngine::hitUnit` (the single place every attack's damage is applied to a unit — bullets, melee, explosion fragments), routed through `SavedBattleGame::logHitEvent`. `hitUnit` already captured `healthOrig`/`stunLevelOrig`; we now also capture `woundsOrig` and pass `healthDamage` + `woundsInflicted` to the helper. This means it covers all hits (incl. reaction fire and AoE) and **supersedes the old #6** ("unit takes damage"), which targeted the same code.
**Color:** `combatLogVictimOutcome(victim)` — a hostile victim is GOOD, our unit is BAD. (Note: the original spec's parenthetical was reversed; the established semantic — hitting an enemy is good for the player — is what's implemented.)
**Hidden units:** A hostile the player can't currently see (`!getVisible()`) is not logged at all.
**Detail tiers (`logHitEvent`):**
- Own unit/civilian, or a hostile whose **unit type is researched** -> exact damage: "James hits Sectoid Soldier for 3 damage".
- Visible but un-researched hostile -> vague band by `damage * 100 / maxHP`: 0 = "for no damage", ≤50% = "for light damage", >50% = "for heavy damage".
- Wounds: only for fully-known victims and only when new wounds were inflicted -> "(N wound[s])" appended (plural via `_one`/`_other`). Never shown for un-researched enemies.
**Damage = health damage only** (stun isn't counted here; stun knockouts get their own #2 line). A pure-stun hit therefore reads "for no/0 damage" — acceptable since the knockout is reported separately.
**Strings:** `STR_COMBATLOG_HIT`, `STR_COMBATLOG_DAMAGE_EXACT/NONE/LIGHT/HEAVY`, `STR_COMBATLOG_WOUNDS_one/_other`.

### 5. Reaction fire ✅ DONE
**Design:** Rather than a separate "reacts" line, the reaction is *tagged on the action* and the existing #3 fire/melee log line renders a reaction variant. A `bool reaction` flag was added to `BattleAction` (default false) and set `true` in `TileEngine::tryReaction` when it builds the reaction action. `ProjectileFlyBState::createNewProjectile` and `MeleeAttackBState::init` pass `_action.reaction` into `logFireEvent`/`logMeleeEvent`, which pick the reaction-variant string.
**What:** A reaction shot logs "{0} took a reaction shot with {1}" instead of "{0} fires {1}"; a reaction melee logs "{0} took a reaction swing with {1}" instead of "{0} strikes with {1}". The follow-up hit line (#4) is unchanged. No extra line, no double reporting.
**Color:** `combatLogActorOutcome(attacker)` — same actor-based coloring as a normal shot.
**Strings:** `STR_COMBATLOG_FIRED_REACTION`, `STR_COMBATLOG_MELEE_REACTION`. (The earlier `STR_COMBATLOG_REACTION` and `logReactionEvent` were removed.)

### 6. Unit takes damage ✅ via #4 (superseded)
Folded into #4: `TileEngine::hitUnit` is the single damage-application point, so the richer #4 hit line (exact/vague damage + wounds) replaced the originally-planned coarse "took damage" line at the same hook. No separate emit point.

### 7. Panic / berserk ✅ DONE
**Hook:** `BattlescapeGame::handlePanickingUnit` (the single place panic/berserk is handled, beside the existing "has panicked"/"gone berserk" infobox), via `SavedBattleGame::logPanicEvent(unit, berserk)`.
**What:** Logs "{0} panics" or "{0} goes berserk" (`berserk` = `status == STATUS_BERSERK`). A hostile the player can't currently see is not logged (gated inside the helper, like hits). Name is knowledge-aware.
**Color:** `combatLogVictimOutcome(unit)` — our unit losing control is BAD, an enemy losing control is GOOD.
**Strings:** `STR_COMBATLOG_PANIC`, `STR_COMBATLOG_BERSERK`.

### 8. Out-of-ammo (weapon emptied by the shot) ✅ DONE
**Hook:** `ProjectileFlyBState::createNewProjectile`, after ammo is spent, via `SavedBattleGame::logOutOfAmmoEvent`.
**What:** Logs "{0}'s {1} is out of ammo" (WARNING) on the shot that empties the weapon — i.e. a *result* notification, not the pre-fire "tried to fire with no ammo" warning. Detected with `!weapon->getAmmoForAction(type)` after the spend (the same emptiness test the engine uses for `noMoreShotsToShoot`); for an auto/burst it fires exactly once, on the round that runs the clip dry. Self-powered weapons never report (their slot returns the weapon itself), and throws are excluded.
**Launch:** Blaster/launch ammo is spent in `think()` (not `createNewProjectile`), so there's a second check right after that spend. The `createNewProjectile` check excludes `BA_LAUNCH` to avoid a premature read before the spend.
**Player-only:** Gated to `getFaction() == FACTION_PLAYER` inside the helper — an enemy's empty gun isn't an actionable warning for the player (and avoids alien-turn spam).
**Strings:** `STR_COMBATLOG_OUT_OF_AMMO`.

### 8b. No-LOF / other pre-fire warnings (deferred)
**Where:** Existing `warning()` call sites in `src/Battlescape/BattlescapeState.cpp:2610+`.  
**What:** WARNING-outcome entries alongside line-of-fire / pre-fire failure messages. Deliberately *not* done in this pass (user scoped #8 to the out-of-ammo result only).

### 9. Research-gated text helper (optional enhancement)
The current `getCombatLogName()` already handles basic knowledge-aware naming ("Hostile" vs specific alien type). A more granular research gate — e.g. revealing numeric damage values or unit-specific lore when a race is fully researched — can be added later as an enhancement to the name helper and string composition logic.

### 10. Configurable options (optional)
- `combatLogMaxEntries` — override the default of 20 visible entries.
- `combatLogLifetime` — override the default 8-second decay lifetime.
- Both would follow the existing pattern in `Options.inc.h` / `Options.cpp`.

### 11. Localization strings ✅ DONE

### 12. Verbose armor damage ✅ DONE
**Option:** `combatLogVerbose` — a hidden (HIDDEN-category) DX option, default off, registered in `createOptionsDX()`. Not shown in the Advanced Options menu, but settable by mods via `fixedUserOptions`/`recommendedUserOptions`. The bundled `dx-test` mod forces it on so the behavior is easy to observe.
**Hook:** `BattleUnit::damage`, right after the per-side armor reduction (`setValueMax(_currentArmor[side], ...)`). The armor actually lost this hit is computed as the drop in `_currentArmor[side]`, and when `Options::combatLogVerbose` is on and the loss is positive, `SavedBattleGame::logArmorDamageEvent(this, lost, side)` is called.
**What:** Logs "{0}'s {1} armor takes {2} damage" with the unit's knowledge-aware name and the side word (front/left/right/rear/under). Always shows the exact amount — it is a developer-facing diagnostic.
**Color:** `combatLogVictimOutcome(unit)` — our unit's armor loss is BAD, an enemy's is GOOD.
**Strings:** `STR_COMBATLOG_ARMOR_DAMAGE`, `STR_COMBATLOG_ARMOR_SIDE_FRONT/LEFT/RIGHT/REAR/UNDER`.

### 13. Verbose damage calculation ✅ DONE
**Option:** Same hidden `combatLogVerbose` gate as #12.
**Hook:** `BattleUnit::damage`, at the same per-side armor-reduction point. The hook captures `rawDamage` (damage reaching the armor after resistance/scripts), `effectiveArmorUsed` (`getArmor(side) * type->ArmorEffectiveness`, rounded — the value actually subtracted), the penetrating damage left after armor (`std::max(0, damage)`), and the final health damage (`std::get<toHealth>(args.data)`), then calls `SavedBattleGame::logDamageCalcEvent(this, side, incoming, armor, penetrating, health)`. Fires on every hit that deals positive damage (not only when armor is lost), so it always pairs with #5's hit line and optionally #12's armor-degradation line.
**What:** Logs "{0}'s {1}: {2} damage vs {3} armor, {4} through ({5} to health)" so the armor maths are legible at a glance. Always exact — developer-facing diagnostic.
**Color:** `combatLogVictimOutcome(unit)` — damage to our unit is BAD, to an enemy GOOD.
**Strings:** `STR_COMBATLOG_DAMAGE_CALC` (reuses the `STR_COMBATLOG_ARMOR_SIDE_*` side words).
### 14. Psi actions ✅ DONE
**Motivation:** Psi was entirely silent in the log. A mind control, a mind blast or a clairvoyance sweep produced no line at all, so the only feedback was the stock infobox on the player's own successes — nothing for alien casts, nothing for failures, and nothing for the DX-specific psi features (mind blast, backlash, clairvoyance, counter-control).

**Hooks:**
- **Cast (#12):** `PsiAttackBState::init`, immediately after TU and psi-amp rounds are spent, via `logPsiCastEvent(caster, amp, target, type)`. Placed after the commit so an aborted attempt (out of range, no rounds, no TU) logs nothing. Being on the shared cast path, it covers AI casts too. Reads "{0} focuses {1} on {2} ({3})".
- **Outcome (#13):** `TileEngine::psiAttack` and `TileEngine::mindBlast`, via `logPsiResultEvent(caster, victim, type, success)` — "{0}'s {2} takes hold of {1}" / "{1} resists {0}'s {2}". Toned by `combatLogVictimOutcome(victim)`, **inverted on a resist** so our soldier shrugging off an alien probe reads GOOD.
- **Mind blast damage (#14):** emitted from `hitUnit` itself, which branches to `logMindBlastEvent` when `attack.type == BA_MINDBLAST` (flavor wording; identical research gating to `logHitEvent`). Wiring this initially revealed that mind blast was calling `BattleUnit::damage` directly and so bypassed `hitUnit` altogether — see the note below.
- **Backlash (#15):** `TileEngine::applyPsiBacklash`, capturing health/stun before and after so the line reports what actually landed rather than the configured roll range; morale comes straight from the rule. Only the configured components are named: "{0} suffers psychic backlash (3 damage, 5 stun, 10 morale)".
- **Clairvoyance (#16):** logged inside `TileEngine::clairvoyance` rather than at the `BattlescapeGame` call site, because that is where the psi-score-derived radius is known.
- **Counter-control:** freeing one of our own from an enemy's grip is a won `BA_MINDCONTROL` contest, but "his mind control takes hold of our own soldier" reads wrong. That case is detected before the contest resolves (so the ousted controller can still be identified) and logged via the existing `logMindControlBreakEvent` with a new reason `STR_COMBATLOG_MC_BREAK_COUNTER` ("torn free by a rival psi") instead of the generic success line.
- **Verbose contest maths (#17):** `psiAttackCalculate` gained three optional out-params (attack score, defence score, tile distance) so a caller that *resolves* an attack can print the maths without recomputing them. The AI's speculative target scoring (`AIModule`) passes nothing and therefore logs nothing — important, since it evaluates every candidate target every turn.

**Strings:** `STR_COMBATLOG_PSI_CAST`, `STR_COMBATLOG_PSI_SUCCESS_MALE/FEMALE`, `STR_COMBATLOG_PSI_RESIST_MALE/FEMALE`, `STR_COMBATLOG_MIND_BLAST_HIT`, `STR_COMBATLOG_PSI_BACKLASH`, `STR_COMBATLOG_BACKLASH_DAMAGE/STUN/MORALE`, `STR_COMBATLOG_CLAIRVOYANCE`, `STR_COMBATLOG_PSI_ROLL`, `STR_COMBATLOG_PSI_ACTION_*` (action names), `STR_COMBATLOG_MC_BREAK_COUNTER`.

**Follow-up: mind blast now goes through the standard damage path.** Wiring #14 exposed that `TileEngine::mindBlast` called `BattleUnit::damage` directly instead of `TileEngine::hitUnit`. That skipped more than logging — most importantly **murderer attribution** (`setMurdererId` / `setMurdererWeapon`, set only in `hitUnit`), so a blast that killed, or that left the victim to bleed out or burn, credited nobody: no kill statistic and no promotion credit. It also skipped the OXCE hit log and the `isPreview` / already-dead guards.

`mindBlast` now calls `hitUnit`, which needed two small extensions:
- **Side/bodypart overrides.** `hitUnit` gained optional `sideOverride`/`bodypartOverride` params forwarded to `damage()` (which already accepted them). A blast passes `SIDE_FRONT, BODYPART_HEAD` — it has no physical trajectory to derive a facing from, and `relative` of `(0,0,0)` would otherwise be read as an under-the-feet hit.
- **An experience opt-out** — since removed. `awardExperience` had **no `BT_PSIAMP` case**, so a psi-amp in the default training mode fell through to its "FIREARMS and other" branch and would train **firing**. That was first worked around with an `awardExp` flag on `hitUnit`, then fixed at the source: `awardExperience` now returns no award for a psi-amp, leaving the psi paths' own psi-skill award as the only one (see [DX-OXCE-Fixes.md](../DX-OXCE-Fixes.md)). The flag was removed rather than left as a dead parameter.

## Verbose lines: tagging and the damage-calc readout (Jul 2026)

Two problems with the verbose stream, both reported from play.

**1. Verbose lines were indistinguishable from combat events.** They report engine internals (armor
maths, overwatch scoring, psi rolls) but sat in the same stream, in the same styling, as
"James hit the Sectoid" — so it was not clear which lines were diagnostics.

All six verbose loggers now go through `SavedBattleGame::appendToCombatLogVerbose`, which prepends
`STR_COMBATLOG_VERBOSE_PREFIX` (`"[Verbose] "`). Routing it through one helper rather than prefixing at
each call site means a new verbose logger gets the tag by construction. The prefix is a language key, so
it can be shortened to `"[V] "` if the longer lines run off the panel.

The tagged loggers are exactly: `logArmorDamageEvent`, `logDamageCalcEvent`, `logOverwatchEvalEvent`,
`logOverwatchOutcomeEvent`, `logMindControlUpkeepEvent`, `logPsiRollEvent`.

**2. The damage-calc line only ever reported health, and never named the damage type.** It read
`"<unit>'s <side>: 12 damage vs 50 armor, 0 through (0 to health)"`. For anything that does not damage
health — stun, morale, a psi backlash — that "0 to health" read as *the hit did nothing*, when it had
simply landed somewhere the line did not mention. And with a mod-defined damage type in play there was
no way to tell which type a given line was even about.

It now reads `"<unit>'s <side>: <incoming> <TYPE> vs <armor> armor, <through> through -> <effects>"`,
where `<effects>` lists only the components that landed (`"3 health, 6 stun, 5 morale"`), or `"nothing"`.

The effects are reported as *applied*, not as computed: stun is suppressed for a pain-immune target,
wounds for a non-woundable one, and morale is scaled by bravery — the call site mirrors each of those
conditions rather than dumping the raw script output, so the line cannot claim an effect the unit never
took.

Type naming was extracted rather than duplicated: `ArticleState::getDamageTypeText` held a 20-case
switch mapping `ItemDamageType` → `STR_DAMAGE_*`. That moved to
`RuleDamageType::getResistTypeLanguageKey` (a table) and `ArticleState` now delegates to it, so the
Ufopaedia and the combat log name a type identically — which matters most for a mod-claimed spare slot,
whose name then lives in exactly one place.
