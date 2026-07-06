# Feature — Medikit / Stabilization Rework

**Status:** ✅ Implemented (pending in-game testing). Phase 7, second of the medical trio (Bleedout &
Indicators ✅ → **Medikit/Stabilization Rework** → Proportional Wound Recovery + Field Surgery). Builds
directly on [Feature-Bleedout.md](Feature-Bleedout.md).

**Implementation summary.** No change to Heal (still cures wounds per part + restores HP). New rule:
`BattleUnit::_incapacitated` (save/loaded) is set in `checkStartBleedout()` when a unit crosses into
negative health (gated on the mod knob), and `SavedBattleGame::reviveUnconsciousUnits` gains
`&& !bu->isIncapacitated()` so a once-bled-out unit never revives/rejoins — it survives to be recovered
at debriefing. Knob: `bleedoutDefaults.lockoutForMission` (default true). Medikit readout: `MedikitState`
gains a target block (name; derived `STATUS>` word; `HP cur/max`; `Stun stun/curHP`) and a healer
`TU cur/max` line, refreshed in `update()`; new `STR_MEDIKIT_*` strings.

## Motivation

Bleedout gives a downed soldier a dying-but-savable window, but right now the only way to save them is
the stock medikit **Heal**, which clears the wound *and* restores HP — so a medic can fully un-down a
bleeding-out soldier mid-fight, which is too generous and undercuts the tension. This rework makes
field aid a **stabilize**: a medic can stop the bleed so the soldier survives to be recovered at
mission end, but **cannot bring them back into the fight** on the spot. It also surfaces the target's
condition on the medikit screen so the player can see what they're treating.

## Audit — what OXCE-Plus already provides

- **Medikit item fields** (`RuleItem`): `woundRecovery`, `healthRecovery`, `stunRecovery`,
  `energyRecovery`, `manaRecovery`, `moraleRecovery`, painkiller strength; action bitmask
  `BMA_HEAL / BMA_STIMULANT / BMA_PAINKILLER`; options `allowSelfHeal`, `medikitType` (0 UI / 1-3
  fast), `isConsumable`, custom background/sounds (`Extended.txt`).
- **Apply path:** `TileEngine::medikitUse(action, target, mask, part)` runs a `ModScript::HealUnit`
  hook (can adjust every recovery value), then dispatches to `BattleUnit::painKillers` /
  `stimulant` / `heal`.
- **`BattleUnit::heal(part, woundAmount, healthAmount)`:** removes fatal wounds on **one** body part
  and adds HP, floored at `min(_health, 1)` ("first do no harm"). Reviving an unconscious unit is
  detected here. DX bleedout already: `heal()` clears `_bleedingOut` if HP recovers above 0.
- **Medikit UI** (`MedikitState` + `MedikitView`): a compact overlay showing the body-part picker,
  selected-part + wound count, and three action buttons (heal/stim/painkiller) with charge counts and
  an end button. **No target vitals are shown** (HP / stun / status), and **no healer TU** is shown.
- **Absent:** any "stabilize" / stop-bleed-without-revive concept, and any on-screen target condition
  readout. `bleedImmune` armor exists (from bleedout) but is unrelated to treatment.

## Legacy DX design (intent to reproduce)

From `Legacy-DX-Features.md` §7:
- **Stabilization:** healing a fatal wound on a *bleeding-out* soldier **stops the bleed but cannot
  revive them in the field** — it applies a **heavy stun** (`_stunlevel = max(1000, …)`) so they stay
  down, and forces **`healthAmount = 0`** (wounds reduced, but no HP restored and no wound-recovery
  bonus). A non-bleeding unit heals normally.
- **Readout:** the medikit view shows **all-red** when the target is bleeding out, with improved
  damage/stat info.

## DX target-state readout (roadmap sub-item)

Add a condition readout to `MedikitState`:
- **Top (target):** unit name; `STATUS>` a derived word (e.g. HEALTHY / INJURED / BLEEDING OUT /
  UNCONSCIOUS, from HP / wounds / consciousness); `HP>cur/max`; `STUN>stun/curHP` (stun over
  **current** HP, since a unit drops unconscious once stun exceeds current health — matches the DX
  combat-log convention).
- **Bottom (healer):** `TU>cur/max` from the **acting unit** (the medic), so the player sees whether
  another treatment is affordable.
- Bleeding-out target styled distinctly (red), per legacy.

## Resolved model (scoping answers)

Simpler than the legacy "special stabilize heal": **the Heal button is unchanged** — it cures wounds
per body part and restores HP exactly as stock (that is what "stabilizes" a dying soldier: curing the
fatal wounds stops the HP drain). The one new rule is a **mission lockout**:

> A unit that ever dropped into **negative health** (entered bleedout) is **out for the rest of the
> mission** — even healed back to full HP with low stun it does **not** revive; it stays unconscious
> and is recovered at debriefing.

So there is **no** forced-0-HP, no heavy stun, no dedicated Stabilize action — just a persistent
"was incapacitated" flag plus a revival gate.

## Proposed DX approach (deltas only)

### 1. Mission incapacitation lockout (`BattleUnit` + `SavedBattleGame::reviveUnconsciousUnits`)
- Add a persistent `BattleUnit` flag (e.g. `_incapacitated`) set true in `checkStartBleedout()` (i.e.
  the moment the unit crosses into negative health / bleedout). It is **not** cleared by healing during
  the mission; it is save/loaded like `_bleedingOut`.
- Gate revival: `reviveUnconsciousUnits` currently wakes a unit when
  `getStatus() == STATUS_UNCONSCIOUS && !isOutThresholdExceed()` (HP > 0, stun < HP). Add
  `&& !bu->isIncapacitated()` so a once-bled-out unit stays down even after a full heal.
- **Heal is untouched** — it still cures wounds and restores HP (which clears the *active* `_bleedingOut`
  dying state, per bleedout). The unit simply won't stand back up. Painkiller/stimulant unchanged.

### 2. Target-state readout (`MedikitState`)
- Add Text widgets: a **target** block (name; `STATUS>` derived word — HEALTHY / INJURED / BLEEDING OUT
  / UNCONSCIOUS from HP / wounds / consciousness; `HP>cur/max`; `STUN>stun/curHP`) and a **healer** line
  (`TU>cur/max` from the acting unit). Refresh in `update()`. Style the target red when bleeding out.

### 3. Mod configurability (DX pattern)
- One knob, defaulting to legacy behavior: whether entering bleedout locks the unit out for the mission
  (`true` = legacy; `false` = a bled-out unit can be healed and revived normally). Added to the existing
  `bleedoutDefaults` node.

## Localization
New `STR_*` for the readout labels + status words, in `Language/DX/`.

## Out of scope
- Proportional wound recovery + Field Surgery research (the next Phase 7 item).

## Resolved decisions
- **Heal unchanged, per-part** (stock behavior); no special stabilize heal, no dedicated button.
- **Negative health = out for the mission** — enforced via a persistent flag + revival gate.
- **Mod-configurable, legacy defaults** — the lockout is a `bleedoutDefaults` knob, default on.
- **Readout included** — target STATUS/HP/STUN + healer TU on the medikit screen.
