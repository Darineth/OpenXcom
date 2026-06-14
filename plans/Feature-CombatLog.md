# Feature - Combat Log

The original brief: **Floating combat log** displaying battlefield events (damage, reloads,
  psionics, item swaps), with a colorable non-warning message channel and improved
  damage/stun reporting; racial-research and interrogation reveal extra info.

## Expected Capabilities

- Display a list of recent combat events, with the most recent at the bottom.
- Each entry should have an associated OUTCOME_* type, for color-coding.
  - Outcomes should be:
    - GOOD
    - NEUTRAL
    - WARNING (open to better names)
    - BAD
  - The outcome dictates the display color.
- Colors should be loaded from interface rules.
- The log should be capped at a certain number of visible entries (e.g. 20), with older entries scrolling off the top.
- Over time, log entries should disappear.
- Log entries are just for combat, and do not need to be saved or persist across battles.
- The log is going to be displayed at the top of the battlescape, and entries should be centered.
- If a unit is named in a log entry, the name should be gendered and reflect the player's current knowledge of that unit (e.g. "Alien is hit" vs. "Sectoid is hit").  If the unit is not visible at all, the log entry should call it Unknown or something similar.

# Implementation Plan

**Phase:** 1 (Battlescape UX & Feedback) — see `DX-Implementation-Checklist.md`.
**Depends on:** nothing (foundational). **Unlocks:** a feedback channel that every later DX
combat mechanic emits into for free.

## 1. Relationship to the existing OXCE `HitLog` (read first)

DX is not starting from zero, but the existing system is **not** what this feature needs, so
we build alongside it rather than extend it.

OXCE already ships a "hit log": `src/Savegame/HitLog.h` accumulates a per-turn **text blob**
in an `ostringstream` with coarse categories (`HITLOG_NEW_TURN`, `HITLOG_PLAYER_FIRING`,
`HITLOG_REACTION_FIRE`, `HITLOG_NEW_SHOT`, `HITLOG_NO_DAMAGE`, `HITLOG_SMALL_DAMAGE`,
`HITLOG_BIG_DAMAGE`). It lives on `SavedBattleGame` (`_hitLog`, `SavedBattleGame.cpp:85`) and
is shown **on demand only** via `Ctrl-H` (`InfoboxState`) / `Ctrl-Alt-H` (`TurnDiaryState`),
`BattlescapeState.cpp:2792-2814`. It is **not serialized** (`load/save` never touch `_hitLog`).

It is the wrong base for our floating log: not always-visible, not per-event (it buckets damage
into small/big rather than reporting an outcome per line), no per-entry color or time-decay.
**Decision:** introduce a separate, transient, structured event channel for the floating log
and **reuse the existing `appendToHitLog` call sites as known-good emit locations** —
instrument those same spots to also emit a structured combat-log entry. The legacy Ctrl-H hit
log is left untouched; the floating log is purely additive.

This matches the brief: *"a colorable non-warning message channel and improved damage/stun
reporting"* — a new channel distinct from both `HitLog` and the transient `WarningMessage`.

## 2. Data model

New files `src/Battlescape/CombatLog.h` / `.cpp` (battlescape-local because, per the brief, it
is **transient and not saved**):

```cpp
enum CombatLogOutcome : int { OUTCOME_GOOD, OUTCOME_NEUTRAL, OUTCOME_WARNING, OUTCOME_BAD };

struct CombatLogEntry {
    std::string text;          // already localized + arg-substituted at emit time
    CombatLogOutcome outcome;  // drives color
    Uint32 bornFrame;          // engine think-tick when added, for time-decay
};

class CombatLog {
    std::deque<CombatLogEntry> _entries;   // newest at back
public:
    void add(const std::string& text, CombatLogOutcome outcome);
    void think();                          // age out expired entries
    void clear();                          // on battle start / new battle
    const std::deque<CombatLogEntry>& entries() const { return _entries; }
};
```

Design notes tied to the stated capabilities:
- **Outcome → color, not faction → color.** The four outcomes map to four colors loaded from
  `interfaces.rul` (§5). Emit sites choose the outcome (e.g. *enemy killed by player* = GOOD,
  *XCOM soldier hit/killed* = BAD, *new turn / info* = NEUTRAL, *out-of-ammo / no-LOF* =
  WARNING).
- **Cap at N visible (default 20).** `add()` pops from the front past the cap so older entries
  scroll off the top. Newest renders at the bottom (brief: "most recent at the bottom").
- **Time-decay.** Each entry stores its birth tick; `think()` drops entries older than a
  configurable lifetime so the log self-empties when combat is quiet. (Reuse the fade idea from
  `WarningMessage`, `src/Battlescape/WarningMessage.*`; an optional alpha ramp in the last
  second is a nice-to-have, not required.) Time, not save state, is the only lifecycle —
  consistent with "do not need to be saved or persist across battles."

## 3. Where the store lives + emit API

The store must be reachable from deep combat code (`TileEngine`, `BattlescapeGame`,
`ProjectileFlyBState`) that has no `BattlescapeState` pointer but does have `SavedBattleGame`.
So mirror the existing `HitLog` ownership: hold a `CombatLog` on `SavedBattleGame`
(`_combatLog`, next to `_hitLog` at `SavedBattleGame.h:131`) with an accessor and a forwarder
`appendToCombatLog(text, outcome)` that parallels the existing
`appendToHitLog` forwarders (`SavedBattleGame.cpp:3394-3415`).

**Critically: do _not_ add it to `SavedBattleGame::load/save`.** It is intentionally transient
(brief §"Log entries ... do not need to be saved"). Just `clear()` it when a new battle starts.

Emit is then a one-liner from any combat path:

```cpp
save->appendToCombatLog(tr("STR_COMBATLOG_KILLED").arg(victimName).arg(killerName),
                        OUTCOME_GOOD);
```

The string is localized and arg-substituted **at emit time** (language fixed at the moment of
the event), so the panel render stays cheap and language-agnostic — matching how the rest of
the battlescape composes messages.

## 4. Display widget (top-of-screen, centered)

Add a `_combatLog` display to `BattlescapeState` (alongside `_warning`,
`src/Battlescape/BattlescapeState.h`). The brief calls for **top of the battlescape, centered
entries**, transient and self-scrolling, which is closer to a stack of centered `Text` lines
than a bordered `TextList`. Two viable approaches:

- **A (recommended): a lightweight custom surface** that draws the live `CombatLog` entries as
  centered, color-coded, word-wrapped lines top-down — full control over per-entry color,
  centering, and fade, like `WarningMessage` but multi-line. Lowest friction for the exact UX.
- **B: reuse `TextList`** (`src/Interface/TextList.*`: `addRow`, `scrollUp/Down`,
  `removeLastRow`, per-row color, wheel scroll). Faster to stand up and gives free scrollback,
  but centering + per-entry fade + top-anchoring fight its design. Reasonable fallback if
  scrollback is later wanted.

Go with **A** for the stated requirements; keep B in mind if "scroll back through history"
becomes a requirement later.

Integration in `BattlescapeState`:
- Construct with position/size/colors from `interfaces.rul` using the same
  `getInterface("battlescape")->getElement(...)` path the constructor already uses for `icons`
  and `warning` (`BattlescapeState.cpp:~115`).
- Drive decay from the existing think/timer path (call `_save->getCombatLog()->think()` and
  redraw); the panel reads straight from the `SavedBattleGame` store.
- Respect a disable option (skip blit when off).

## 5. Interface ruleset / theming (colors from rules)

Per the brief, colors load from interface rules. Add a `combatLog` element to the
`battlescape` interface in `interfaces.rul` (`bin/standard/xcom1/interfaces.rul` `battlescape:`
block, and `xcom2`), themed like the existing `warning` / `messageWindows` / `visibleUnits`
elements:

```yaml
      - id: combatLog
        pos: [0, 8]          # top, full-width; entries centered within
        size: [320, 40]
        color: 15            # OUTCOME_NEUTRAL (default/info)
        color2: 32           # OUTCOME_BAD (XCOM hurt / killed)
        border: 48           # OUTCOME_GOOD (enemy down) — reuse spare fields...
```

Four outcome colors are more than the stock `color`/`color2`/`border` triple, so either add
named DX fields to the `combatLog` element (preferred, e.g. `colorGood/colorNeutral/
colorWarning/colorBad`) or pack them into the available slots. Read them in the
`BattlescapeState` constructor and build an `outcome → Uint8 color` table.

## 6. Emit points (Phase 1 wiring)

Reuse the verified locations that already call `appendToHitLog`, plus add the casualty hook —
the one that most directly satisfies "improved damage/stun reporting":

| Event | Where (verified) | Outcome (depends on faction) |
|-------|------------------|------------------------------|
| New turn | `NextTurnState.cpp:281,285` | NEUTRAL |
| Player fires weapon | `ActionMenuState.cpp:525` | NEUTRAL |
| Shot fired | `ProjectileFlyBState.cpp:588`, `TileEngine.cpp:4888` | NEUTRAL |
| Reaction fire | `TileEngine.cpp:2877` | GOOD if XCOM reacts, BAD if alien reacts |
| Unit takes damage | `TileEngine.cpp:3161/3165/3170` | GOOD vs enemy / BAD vs XCOM |
| Kill / stun / panic | `BattlescapeGame::checkForCasualties` (`BattlescapeGame.cpp:716`) | GOOD vs enemy / BAD vs XCOM |
| Out-of-ammo, no line-of-fire, etc. | existing `warning()` sites in `BattlescapeState.cpp:2610+` | WARNING |

`checkForCasualties` (`BattlescapeGame.cpp:716`) is the key new hook: it already detects
death/stun, builds kill stats, and pushes the `STR_HAS_BEEN_KILLED` infobox
(`BattlescapeGame.cpp:~926,3090`). Emit a structured kill/stun line there with attacker +
victim names and the right outcome.

The `TileEngine` damage hooks currently only know small/big/no-damage buckets. For Phase 1
emit a "hit / took damage" line at the right outcome; the **firing-model phase (Phase 5)** then
upgrades these to exact damage/stun numbers once that data is plumbed through. This is the
"improved damage/stun reporting" path — coarse now, exact later — and a deliberate seam.

**Research-gated detail (brief: "racial-research and interrogation reveal extra info").** When
composing enemy-facing lines, gate the *richness* of the text on what the player has
researched about that unit's race/type (the same knowledge check the Ufopaedia/auto-sell and
stat-reveal systems use). Unknown alien → generic "Alien is hit"; researched/interrogated →
named unit and (Phase 5) numeric damage. Implement as a helper that picks the string/args from
the unit + `SavedGame` research state; wire the real numbers in Phase 5.

## 7. Localization

New `STR_COMBATLOG_*` keys live in the DX language folder (`bin/common/Language/DX/en-US.yml`),
loaded as a dedicated VFS slice alongside the OXCE folder in `Game::loadLanguages`. They are composed
with `.arg()` and gendered `getString(id, gender)` where a unit is the subject (matching
`STR_HAS_BEEN_KILLED`, `BattlescapeGame.cpp:3090`). Examples:

- `STR_COMBATLOG_NEW_TURN: "— Turn {0} —"`
- `STR_COMBATLOG_FIRED: "{0} fires {1}"`
- `STR_COMBATLOG_HIT: "{0} hits {1}"`
- `STR_COMBATLOG_TOOK_DAMAGE: "{0} is hit"`   (Phase 5 → "{0} takes {1} damage")
- `STR_COMBATLOG_KILLED: "{0} kills {1}"`
- `STR_COMBATLOG_STUNNED: "{0} is knocked out"`
- `STR_COMBATLOG_PANIC: "{0} panics!"`
- `STR_COMBATLOG_REACTION_FIRE: "{0} fires on reaction"`

## 8. Options & input

- DX option `combatLogEnabled` (default on); optionally `combatLogMaxEntries` (default 20) and
  `combatLogLifetime` (decay seconds). Register like `oxceDisableHitLog`
  (`Options.cpp:404`, `Options.inc.h:152`).
- Optional hotkey to toggle the floating panel (independent of the legacy `Ctrl-H` popup,
  which is unchanged).

## 9. Files

**New:** `src/Battlescape/CombatLog.h` / `.cpp` (entry struct, outcome enum, store + decay).
Optionally split the widget into `CombatLogPanel.*`; inline in `BattlescapeState` is fine for
Phase 1.

**Modified:**
- `src/Savegame/SavedBattleGame.h/.cpp` — own `CombatLog`, accessor, `appendToCombatLog`
  forwarder, `clear()` on battle start. **No load/save.**
- `src/Battlescape/BattlescapeState.h/.cpp` — `_combatLog` display, construct from
  `interfaces.rul`, blit + `think()` decay, toggle hotkey, disable guard.
- `src/Battlescape/BattlescapeGame.cpp` — emit kill/stun/panic in `checkForCasualties`.
- `src/Battlescape/TileEngine.cpp`, `ProjectileFlyBState.cpp`, `ActionMenuState.cpp`,
  `NextTurnState.cpp` — `appendToCombatLog` next to existing `appendToHitLog` calls.
- `src/Engine/Options.inc.h` / `Options.cpp` — new options.
- `bin/standard/xcom1/interfaces.rul` (+ `xcom2`) — `combatLog` element with four outcome colors.
- DX language YAML — `STR_COMBATLOG_*` keys.
- **Build registration (required, no glob):** add the new `.cpp` to `src/CMakeLists.txt`
  (`*_src` lists) **and** `src/OpenXcom.2010.vcxproj` + `.filters` (per `CLAUDE.md`).

## 10. Implementation order

1. `CombatLog` store + `CombatLogEntry`/`CombatLogOutcome`; hang on `SavedBattleGame` with
   accessor, `appendToCombatLog`, and `clear()` on battle start. Build (no behavior change).
2. Top-anchored, centered, color-coded display in `BattlescapeState`, themed from
   `interfaces.rul`; wire `think()` decay and the cap; disable option.
3. Emit Phase-1 events at the §6 sites — start with new-turn / fire / reaction, then
   kill/stun/panic in `checkForCasualties`; choose outcome by faction.
4. Add `STR_COMBATLOG_*` strings (gendered where a unit is subject).
5. Research-gated text helper for enemy-facing lines.
6. Manual verification (§11).

## 11. Testing / verification

No unit-test framework (per `CLAUDE.md`); verify in a live tactical battle:
- Fire (snap/auto), trigger reaction fire, kill + stun units, cause panic, end a turn — each
  produces a correctly-worded, correctly-**colored** centered line at the top.
- Confirm the 20-entry cap scrolls old lines off the top and entries **decay/disappear** when
  combat goes quiet.
- Confirm WARNING-outcome lines appear for out-of-ammo / no-line-of-fire.
- Confirm an unresearched alien shows generic text and a researched/interrogated one shows
  richer text.
- Save/reload mid-battle — confirm the log is (intentionally) empty afterward and nothing
  crashes (it is not serialized).
- Build with `-DFATAL_WARNING=ON` as the correctness gate.

## 12. Future integration (later phases emit into this)

The reason this is Phase-1 infrastructure: later mechanics get feedback for free via
`save->appendToCombatLog(text, outcome)`:
- **Phase 3 async projectile/explosion** — per-projectile impacts, concurrent blasts.
- **Phase 5 firing model** — upgrade Phase-1 damage lines to exact damage/stun numbers,
  crit/graze, shot mode (fulfils "improved damage/stun reporting").
- **Phase 6 reloading / ammo** — reload + out-of-ammo lines (brief mentions reloads, item swaps).
- **Phase 7 health/medical** — bleedout, stabilization, wound recovery.
- **Phase 8 effects/psionics** — effect applied/expired, mind control, mind blast (brief
  mentions psionics).

## 13. Open questions

- **Four outcome colors vs. the stock `color/color2/border` triple** in `interfaces.rul` —
  add named DX fields to the `combatLog` element (preferred) or repurpose existing slots?
- **Decay timing & cap** (lifetime seconds, 20 visible) — tune in-game; keep both as options.
- **Top placement vs. existing top-row HUD** (visible-unit indicators) at 320×200 — verify no
  overlap; keep position data-driven so it is trivial to move.
- **Research-gate granularity** — per-race vs. per-unit-type knowledge; reuse whichever check
  the Ufopaedia stat-reveal uses for consistency.
- **Custom surface (A) vs. `TextList` (B)** for the widget — A chosen for centered/decay/top
  UX; revisit if scrollback history is later requested.