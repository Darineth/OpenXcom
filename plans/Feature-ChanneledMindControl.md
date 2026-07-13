# Feature: Channeled Mind Control

**Status:** Audit complete; design decided (upkeep + resist are mod-configurable, counter-control is a contest). Ready to implement.
**Roadmap:** Phase 8 (Lighting & Psionics).
**Legacy reference:** `Legacy-DX-Features.md` §psionics; legacy source at `D:\Code\Projects\OpenXcomDX-Legacy`.
**Hard requirement:** **mod opt-in.** With no ruleset opt-in, mind control behaves exactly as stock
OXCE does today — byte for byte.

## Motivation

Stock mind control is fire-and-forget: you roll once, you own the unit, and it costs nothing to keep.
DX wants MC to be a *sustained* act — the psi operative is holding the leash, it costs something every
turn, and it can go wrong. That turns MC from a "win button" into a commitment with a price.

## Audit (Jul 2026)

### What OXCE actually does today — and the surprise

The critical finding, which changes the framing of the whole feature:

> **Mind control in OXCE is *not* permanent. It already expires after one turn.**

[`BattleUnit::prepareNewTurn`](../src/Savegame/BattleUnit.cpp) silently reverts any unit whose
`_faction != _originalFaction` back to its original faction at the start of *its own* side's turn, and
returns early so the unit gets no TU that turn. So a mind-controlled unit is yours for the remainder of
your turn, is inert for the enemy turn, and snaps back when its original side comes round. There is no
upkeep, no roll, no cost — and no way for a mod to change any of it.

The rest of the picture:

- **Success roll** ([`TileEngine::psiAttackCalculate`](../src/Battlescape/TileEngine.cpp)):
  `attack = accuracyMindControl + accuracyMultiplier` (default `psi: 0.02`, i.e.
  `0.02 × psiSkill × psiStrength`) versus `defense = 30 + Armor.psiDefence` (default
  `psiStrength + 0.2 × psiSkill`), minus a distance falloff, with the roll itself living in a **default
  Y-Script** on the `tryPsiAttackItem` hook (`random 0..55`).
- **On success** ([`TileEngine::psiAttack`](../src/Battlescape/TileEngine.cpp)): faction swap via
  `convertToFaction`, victim gets **full TU restored**, `setMindControllerId(attacker)`.
- **`_mindControllerID` is a dead end.** It exists, it's saved — but it is write-only bookkeeping for
  *kill credit* (one consumer, in `checkForCasualties`). It is never cleared, panic sets it too, there
  is no reverse link (controller → thralls), and **nothing reads it to decide whether control persists**.
  A channeled system can reuse the idea, but not the field's (absent) semantics.
- **On failure**: nothing at all happens to the attacker — the else-branch only grants the victim
  psi-strength XP.
- **Scripts cannot do this.** `convertToFaction` is **not bound** to script, `_mindControllerID` is not
  bound, and the `tryPsiAttack*` hooks take everything as `const`. So a mod **cannot** end control,
  identify a controller, or apply backlash today. This is engine work or it is nothing.
- **`RuleSkill` can already declare a costed psi ability** (`targetMode: BA_MINDCONTROL` passes the
  `BA_CQB` clamp), but it only contributes *cost* — the semantics remain the engine's, and a skill
  cannot introduce ongoing state.

### What the legacy fork did (and what to avoid)

Legacy's "channeled" MC was **not** an Effects-framework thing (psi never used `BattleEffect` at all) —
it was a pair of pointers on `BattleUnit` plus a tick in the turn prep. Worth copying in shape:

- `_controlling` / `_controller` pointer pair, **one thrall maximum** (MC'ing a second silently released
  the first), with `_loadedControllingId` for save relinking.
- **Upkeep = the controller recovers only 10% of its normal TU regeneration** while channeling, and the
  control breaks outright if the controller is no longer **holding a psi-amp** at turn start.
- **Cast cost**: on success the caster is dumped to **5 TU**; the thrall gets a **full fresh TU bar**.
- **Breaks on**: controller death, dropping the amp, MC'ing someone else, the controller *himself* being
  MC'd, thrall death, or a **free (0 TU) cancel** from the action menu.
- **Backlash fires only when the thrall dies**: `RNG(20,40)` psychic damage to the controller plus a
  *permanent* psi-stat loss.
- **UI**: a bobbing `IconMindControl` sprite over both ends of the link when either is selected, and the
  psi-amp action menu collapses to a single "Cancel Mind Control" entry while channeling.
- **No per-turn resistance roll by the victim, and no real counter-control** — the "free your own MC'd
  ally" branch exists but the AI provably can never reach it.

Legacy bugs *not* to reproduce: two rival psi resolution engines running simultaneously (an AI MC could
resolve twice and produce an *unbreakable* control with no link and no upkeep); backlash writing
`psiStrength` from `psiSkill`; kill credit broken on the DX path; a cancel entry bound to the overwatch
hotkey.

### Conclusion

Everything about channeling is **absent** and unreachable from mods. The delta is real, and it is engine
work: a controller↔thrall link with a lifetime, an upkeep tick, a backlash path, and a cancel action.

## Design

### 1. Opt-in (the hard requirement)

Channeling is enabled **per psi-amp**, by a `mindControl:` node on the item — named for the mechanic it
configures, not for the general idea of channeling, since nothing else in DX channels. **No node (or
`channeled: false`) ⇒ stock OXCE behavior**, including the existing one-turn expiry. Nothing in `bin/standard/` gets the node by default;
it ships in `dx-test` only, so stock UFO/TFTD play is untouched.

```yaml
items:
  - type: STR_PSI_AMP
    mindControl:
      channeled: true          # opt in. Absent/false = stock one-turn MC, unchanged.
      maxThralls: 1            # how many units this amp can hold at once (stacking upkeep per thrall)
      requiresWeapon: true     # control breaks if the controller isn't holding the amp at turn start

      # --- Upkeep: paid by the CONTROLLER at the start of its turn, PER THRALL.
      # Two independent kinds, both moddable, both default 0 (= free, i.e. nothing is taken):
      upkeep:
        # (a) flat costs, deducted outright, same resource set as any RuleItemUseCost
        time: 0
        energy: 0
        mana: 0
        morale: 0
        health: 0
        stun: 0
        # (b) recovery penalties: percent of the controller's normal per-turn REGEN withheld.
        #     100 = that resource does not regenerate at all while channeling.
        #     Legacy's model is `timeRecoveryPercent: 90` (recover only 10% of your TU).
        timeRecoveryPercent: 0
        energyRecoveryPercent: 0
        manaRecoveryPercent: 0
        moraleRecoveryPercent: 0

      # --- Can't pay? The link breaks and the thrall reverts exactly as it does in stock.

      backlashOnThrallDeath:   # when a thrall dies while controlled
        damage: [0, 0]         # random psychic damage to the controller
        stun: [0, 0]
        morale: 0
      backlashOnFailure:       # when the MC attempt FAILS (stock does nothing here, so this is additive)
        damage: [0, 0]
        stun: [0, 0]
        morale: 0

      # --- Whether the thrall gets to struggle. Off = control is stable while the upkeep is paid.
      resist:
        perTurn: false         # true = the thrall re-rolls the psi contest at the start of each of its turns
        modifier: 0            # added to the thrall's defence on that re-roll (negative = easier to hold)

      thrallRecoversTimeUnits: true   # stock restores the victim's full TU bar on capture; mods can drop it
```

Every sub-field defaults to **0 / off / stock**, so `channeled: true` on its own gives you an indefinite
control that costs nothing — and a mod builds the flavour it wants from there. The two upkeep kinds are
deliberately separate and stackable: **flat costs** bite immediately and can be afforded or not, while
**recovery penalties** throttle the controller's regeneration (legacy's whole model was a single one of
these, `timeRecoveryPercent: 90`). A mod can tax time units, energy, mana, or morale — or any mix —
without DX picking a house style. `dx-test` will ship one concrete configuration as an example, not as
a statement.

### 2. Engine state (the overwatch pattern)

On `BattleUnit`, save-persisted, resolved by **id, not pointer** (the trick the overwatch weapon already
uses, because pointers don't survive save/load):

- `_mindControlledBy` (int unit id, −1 = none) — on the thrall.
- `_thralls` (small vector of unit ids) — on the controller. Bounded by `maxThralls`.

Both are cleared on: controller death/unconsciousness, thrall death, cancel, upkeep failure, and mission
stage change. This is the link `_mindControllerID` never was; the existing field stays exactly as it is
for kill credit, untouched.

### 3. The upkeep tick

The one delicate spot. `BattleUnit::prepareNewTurn` currently reverts the faction and **`return`s early**.
The channeling check must be interposed *before* that revert, and the revert becomes conditional:

```
at the controller's turn start, for each thrall:
    still able to channel?  (alive, conscious, not panicking; holding the amp if requiresWeapon)
    can it afford the FLAT upkeep costs?
    -> no  : the link breaks; the thrall reverts exactly as it does in stock
    -> yes : deduct the flat costs, and apply the recovery penalties to the controller's regen

at the THRALL's turn start (only when resist.perTurn):
    re-roll the psi contest, with resist.modifier on the thrall's defence
    -> thrall wins : the link breaks; it reverts
    -> else        : it stays controlled and does NOT revert
```

A unit that is *not* channeled reverts exactly as today, so the stock path is preserved by construction.
Recovery penalties have to be applied where the regen is computed (`BattleUnit::recoverStats` /
`prepareNewTurn`'s stat recovery), not as a post-hoc subtraction, so that "recover nothing" really means
nothing rather than going negative.

### 4. Backlash

Two hooks, both off by default: on a **failed attempt** (stock does nothing, so this is purely additive)
and on **thrall death** while controlled. Costs are `[min,max]` ranges applied to the controller. Unlike
legacy, **no permanent stat loss** — that was a buggy, punishing outlier, and DX has no other mechanic
that permanently damages a soldier's stats mid-mission.

### 5. Counter-control — contested, winner takes the thrall

Psi-targeting an *already-controlled* unit resolves as a **contest against the current controller**, not
against the victim: the attacker's psi attack strength versus the controller's (so a strong controller
holds his grip). On a win the attacker **takes the thrall** — or, if the unit is originally of the
attacker's own faction, **frees** it (it reverts to that faction as a normal unit). On a loss, the usual
failure path applies, including `backlashOnFailure` if the mod set it.

This is the piece legacy designed but never actually reached: its AI filtered controlled units out of
its target list, so it could never counter a player's MC. **DX must make the AI use it** — an enemy psi
unit that can wrest back a captured comrade is the whole point of the mechanic existing.

### 6. UI

- On-map link indicator over controller and thrall when either is selected (legacy shipped an
  `IconMindControl` sprite; DX has the Pathfinding2 marker infrastructure and the status-indicator
  system to reuse).
- A **Cancel Mind Control** action-menu entry (free), bound to a *psi* key, not the overwatch key.
- Upkeep should be visible: the controller's TU bar shortfall needs to be explicable, so a status glyph
  and a combat-log line when a link breaks.

## Decisions (Jul 2026)

1. **The upkeep model is the mod's choice, not DX's.** Rather than picking TU-vs-mana-vs-energy, the
   `upkeep:` node exposes both **flat costs** (time/energy/mana/morale/health/stun) and **recovery
   penalties** (percent of per-turn regen withheld, per resource). All default to 0. Legacy's model is
   then just one configuration (`timeRecoveryPercent: 90`), not a hard-coded rule.
2. **Whether the thrall may struggle is also the mod's choice** — `resist: { perTurn, modifier }`,
   defaulting to off (control is stable while the upkeep is paid).
3. **Counter-control is a contest, and the winner takes the thrall** (or frees it if it's originally
   theirs). The AI must be able to use it.

## Decisions, round 2 (Jul 2026)

4. **`maxThralls` defaults to 1.** Mods may raise it; upkeep stacks per thrall.
5. **A link breaks on:** upkeep the controller cannot pay, controller death, unconsciousness, panic, and
   — when `requiresWeapon` — no longer holding the amp at turn start. Taking damage does **not** jostle a
   link (no save-or-lose roll); the cost model is the tension, not extra dice.
6. **A channeling controller keeps all his abilities.** He may still panic, mind-blast, or attack
   normally — he is limited by the *resources* the upkeep is eating, not by a special-cased menu. (Legacy
   collapsed the psi-amp menu to a single "Cancel" entry; DX does not.) The Cancel entry is simply added
   alongside the others.
