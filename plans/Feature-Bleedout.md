# Feature — Bleedout & Indicators

**Status:** ✅ Implemented (pending in-game testing). Phase 7, first of the medical trio (Bleedout &
Indicators → Medikit/Stabilization Rework → Proportional Wound Recovery + Field Surgery).

**Implementation summary.** `BattleUnit` gains `_bleedingOut` (save/load), `getCanBleedOut()`,
`getDeathHealth()`, `getBleedingOut()`, `checkStartBleedout()`. A unifying `getDeathHealth()` returns 0
for normal units and `−maxHealth × deathHealthPercent/100` for bleedout-capable ones; the death
decision in `checkForCasualties` and the fall states (`keepFalling`/`instaFalling`) now test `health ≤
getDeathHealth()` instead of `health ≤ 0`, so an eligible unit crossing 0 falls unconscious + bleeding
(buffer torso wounds added) and dies only at the threshold, draining via the existing per-turn wound
logic. Config: per-armor `canBleedOut` (tri-state `_allowBleedOut`) + top-level `bleedoutDefaults:`
(`ArmorBleedoutDefaults`, reset in `Mod::resetGlobalStatics`). Indicators: a bleedout-priority glyph
(red cross, mod art `FloorBleedoutIndicator` + procedural fallback) on the downed body in `Map.cpp`;
white fatal-wound tick marks on the selected unit's HP bar (`Bar::setMarks`); and bleeding-out
soldiers listed first with a distinct tooltip (`STR_CENTER_ON_BLEEDING_FRIEND`) in the visible-unit
column. Combat log: `{unit} is bleeding out!` (WARNING) on entry.

## Motivation

In stock OpenXcom / OXCE a unit's health effectively floors at 0: reach 0 HP and it dies (or falls
unconscious if stun ≥ health). There's no "dying but savable" window. Legacy DX added a **bleedout**
state so a downed soldier drops into *negative* health and keeps bleeding from fatal wounds, giving
the player a tense window to reach them with a medic before they cross a death threshold — plus
battlefield indicators so a bleeding/dying soldier is obvious at a glance. This feature reproduces
that on the current OXCE-Plus base.

This item covers **the bleedout state + the indicators only**. The medikit "stabilize" rework and
proportional wound recovery / Field Surgery research are **separate** Phase 7 items that build on
this one (see `DX-Roadmap.md`).

## Audit — what OXCE-Plus already provides (do not rebuild)

*(From a read-only audit of the current tree.)*

- **Per-part fatal wounds:** `BattleUnit::_fatalWounds[BODYPART_MAX]` (6 parts). Applied in
  `BattleUnit::damage` (`isWoundable()` gates it — soldiers always; others only with
  `Options::alienBleeding` and non-`bleedImmune` armor).
- **Bleeding drain (hard-coded 1 HP/wound-point/turn):** `prepareNewTurn` → `updateUnitStats` →
  `BattleUnit::prepareHealth` does `health -= getFatalWounds();` then clamps
  `setValueMax(_health, health, -OverkillMultipler*maxHealth, maxHealth)`. Fire adds a separate hit.
- **Wound penalties per turn:** leg wounds −10% TU each (`prepareTimeUnits`), torso −10% energy each
  (`prepareEnergy`), head/arm −10% firing accuracy each (`getAccuracyModifier`).
- **Death vs. unconscious:** `BattlescapeGame::checkForCasualties` — `health ≤ 0 ⇒ DEAD`, else
  `stun ≥ health ⇒ UNCONSCIOUS`. Finalized when the fall animation completes
  (`keepFalling`/`instaFalling` set DEAD if `_health ≤ 0` else UNCONSCIOUS).
- **Overkill / negative health floor:** damage can drive health to `-4 × maxHealth`
  (`Unit::OverkillMultipler = 4`); armor `overKill` fraction gives a gib/instakill chance
  (`UnitDieBState` → `instaKill()`). Scripts can set negative health. **But there is no state where a
  unit lives at negative health** — `checkForCasualties` kills at ≤ 0 regardless.
- **Indicators already present:** on-map status glyphs above the head
  (`Options::unitStatusIndicatorEnabled`, `Map.cpp`) for **fire / fatal-wounds / shock
  (negative-regen) / near-KO stun**, with mod art (`FloorWoundIndicator` etc.) + procedural
  fallbacks; the **selected-unit HP bar blinks** when any fatal wound is present
  (`BattlescapeState::blinkHealthBar`); a **visible-unit column** lists wounded allies;
  `hasNegativeHealthRegen()` exists. Per-unit `disableIndicators` opt-out (script).

**Absent (this feature builds):** any state where an eligible unit survives at negative health; a
distinct **death threshold** below 0; a bleedout *trigger*; indicators that specifically read
"**dying / will bleed out**" (vs. merely wounded), including **cross-marks on the HP bar** and an
**overall bleedout indicator**.

## Legacy DX design (the intent to reproduce)

From `Legacy-DX-Features.md` §7 (verified against the old fork's code):

- **Eligibility:** `getCanBleedOut()` — original-player, non-vehicle geoscape soldier. Aliens /
  civilians / HWPs still just hit 0 (unconscious/dead) as before.
- **Trigger:** when an eligible soldier's health would go below 0, `checkStartBleedout()` sets
  `_bleedingOut` and **adds 5 fatal torso wounds** (the "bleed buffer" that keeps the drain going).
- **Death threshold:** the soldier doesn't die until health reaches `getDeathHealth() = −(maxHealth /
  2)`. Between 0 and −maxHealth/2 they are *bleeding out* and savable.
- **Progressive:** each turn `health -= getFatalWounds()` (already how the engine drains), so an
  unstabilized soldier keeps sinking toward the death threshold and dies if untreated. Healing the
  torso wounds (existing medikit `BMA_HEAL`) stops the drain and thus the bleed.
- **Indicators:** per-unit battlefield icon over a dying soldier; an overall bleedout indicator by
  the spotted-enemies list (shown regardless of selection); **white cross-marks on the HP bar** for
  wounds.

## Proposed DX approach (deltas only)

### 1. Bleedout state on `BattleUnit`
- Add `bool _bleedingOut` (persisted; save/load) + `getBleedingOut()`.
- `getCanBleedOut()` — eligibility gate (see open question Q1).
- `getDeathHealth()` — the death threshold (see Q2), default `−(maxHealth / 2)`.
- `checkStartBleedout()` — when an eligible unit crosses below 0, set `_bleedingOut = true` and add
  the bleed-buffer torso wounds (see Q2). Idempotent (don't re-add wounds each turn).

### 2. Keep an eligible unit alive at negative health
- **`checkForCasualties`:** replace the flat `health ≤ 0 ⇒ DEAD` with: if `getCanBleedOut()` and
  `health > getDeathHealth()`, do **not** register death — call `checkStartBleedout()` and leave the
  unit standing/downed but alive. Only register DEAD once `health ≤ getDeathHealth()`. Aliens/others
  unchanged.
- **Fall/health clamp:** the existing `prepareHealth` clamp already permits negative health (floor
  `-4×maxHealth`), so the per-turn drain naturally carries a bleeding unit downward; the death
  threshold is enforced in the casualty check, not the clamp. Verify `keepFalling`/`instaFalling`
  and the `_deathRegistered` one-shot don't kill a bleeding unit prematurely.
- Reconcile with **overkill/instakill**: a big enough single hit (or armor `overKill` gib) should
  still kill outright — bleedout only catches units that *cross* 0 within the dying window, not those
  blown past the death threshold in one hit.

### 3. Indicators
- **HP-bar wound cross-marks** *(the roadmap's "wound indicator on HP bar")*: draw small white
  cross-marks on `_barHealth` for fatal wounds, and render the bleedout/dying state distinctly (e.g.
  the negative-health portion styled differently). Coexists with / supersedes the existing blink.
- **Per-unit "dying" glyph:** a bleedout-specific marker distinct from the generic wound glyph, in
  the existing `Map.cpp` status-indicator stack (mod art + procedural fallback, same pattern).
- **Overall bleedout indicator:** a screen indicator by the spotted-enemies list that lights when any
  friendly is bleeding out, regardless of the selected unit (like the visible-unit column, but a
  dedicated "someone is dying" cue).
- All gated so they respect the existing `unitStatusIndicatorEnabled` / `indicatorsAreEnabled()` and
  DX's "prefer visible UI" philosophy.

### 4. Mod configurability (DX pattern)
Follow the established DX mod-default pattern (sentinel + global default node) rather than hard-coding
legacy constants — extent TBD by Q1/Q2. Candidates: who can bleed out, the death-threshold formula,
the bleed-buffer wound count.

## Localization
New `STR_*` keys for any player-facing text (indicator tooltips / combat-log lines if we log
"{unit} is bleeding out"), in `Language/DX/`.

## Out of scope (separate roadmap items)
- **Medikit / stabilization rework** — "stabilize stops the bleed but can't revive in the field"
  (heavy stun, no HP restored). For now the existing medikit `BMA_HEAL` (removing torso wounds) is
  the way to stop a bleed.
- **Proportional wound recovery + Field Surgery research.**

## Resolved decisions
- **Eligibility (Q1) — mod-configurable, legacy default.** Per-armor `canBleedOut` (tri-state:
  unset = legacy rule, i.e. an original-player, non-vehicle geoscape soldier; `false` = never;
  `true` = any unit wearing this armor). Aliens/civilians/HWPs keep the stock hit-0 behavior unless a
  mod opts their armor in.
- **Thresholds (Q2) — mod-configurable, legacy defaults.** A top-level `bleedoutDefaults:` node
  (overwatch-style static + `Mod::resetGlobalStatics`): `deathHealthPercent` (default **50** →
  death at `−maxHealth × 50/100`) and `bufferWounds` (default **5** torso wounds added on entering
  bleedout).
- **Indicators (Q3) — all three:** HP-bar wound cross-marks, a per-unit "dying" map glyph distinct
  from the generic wound marker, and an overall "someone is bleeding out" indicator by the
  spotted-enemies list.
- **Combat log (Q4) — yes:** entering bleedout logs `{unit} is bleeding out` (WARNING). Death from
  bleedout already logs via the normal kill path.
