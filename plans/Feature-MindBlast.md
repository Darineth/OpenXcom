# Feature: Mind Blast

**Status:** ✅ Implemented (Jul 2026). Test config in `bin/standard/dx-test/dx-test.rul` (DT_STUN blast).
**Roadmap:** Phase 8 (Lighting & Psionics). Last of the psi *powers* before the shared Psi-Amp Ammo layer.
**Legacy reference:** `PsiAttackBState.cpp` in the legacy fork at `D:\Code\Projects\OpenXcomDX-Legacy`.
**Hard requirement:** **mod opt-in.** No ruleset node ⇒ the action does not exist and nothing changes.

## Motivation

Panic and mind control *coerce* a target; clairvoyance *senses*. Mind Blast is the psi power that simply
**hurts** — a direct psychic attack that deals damage through the target's will rather than its armor.
It gives a psi operative an offensive option that a strong-willed enemy can still resist, and (if the
mod wants) one that can bite back when it fails.

## Audit (Jul 2026)

### OXCE has no direct psi-damage attack

Panic, mind control and the `BA_USE` psi-amp attack exist; none of them deals damage scaled by a psi
contest. **`BA_MINDBLAST` does not exist.** The mechanic is genuinely absent.

### The existing psi machinery is the right foundation

`TileEngine::psiAttackCalculate` already resolves a psi contest and **returns the margin** — `attacker
psi accuracy + roll − (30 + victim psiDefence) − distance falloff`, positive on a hit
([TileEngine.cpp:5102](../src/Battlescape/TileEngine.cpp)). That single call already folds in the
weapon's `accuracyMindControl`-style accuracy, the armor's `psiDefence`, distance, **and** the
`tryPsiAttack*` script hooks. Mind Blast should call it and scale damage by the margin, rather than
inventing a parallel formula — that keeps it consistent with panic/MC and inherits all their tuning.

### What the legacy fork did (and what to drop)

Legacy `BA_MINDBLAST` used its **own** formula, separate from the rest of psi:

```cpp
attackStrength  = actor.psiStrength + actor.psiSkill + RNG::generate(-15, 15) - 10;
defenseStrength = victim.psiStrength + victim.psiSkill;
difference      = attackStrength - defenseStrength;
dist = (dist > 0) ? 10.0 / (dist + 10) : 1.0;       // range falloff multiplier
```

with a three-tier outcome: **decisive** (`difference > 20`) damaged only the victim; **narrow**
(`0 < difference ≤ 20`) damaged the victim *and* the caster; **failure** damaged only the caster. It
required a real line of fire plus squad-sight, and dealt `DT_PSYCHIC` — a hard-coded type configured to
**ignore armor entirely** (`ArmorEffectiveness: 0`). The "percentage of max health" psychic damage it
advertised was commented out and never shipped.

Drop: the parallel accuracy formula (use the shared psi roll), the hard-coded `DT_PSYCHIC` and
armor-ignore (let the mod pick the damage type, exactly as `mindControl.backlashDamageType` does), and
the magic constants (`20`, `/6.0`, `10/(dist+10)`) baked into C++.

## Design

### Ruleset (opt-in, per psi-amp)

```yaml
items:
  - type: STR_PSI_AMP
    tuMindBlast: 30              # or costMindBlast: { time: 30, mana: 5, ... } - the usual cost block
    mindBlast:
      enabled: true             # opt in; without this the action is never offered
      damageType: -1            # ResistType the blast deals; -1 = the amp's own damage type
      basePower: 20             # flat damage on any successful blast
      powerPerMargin: 0.5       # + this * (psi contest margin) damage - so a decisive win hits harder
      randomRange: 50           # damage rolls in [ (100-r)%, (100+r)% ] of the computed power (RNG spread)
      backlashOnFailure:        # what the CASTER suffers on a miss (stock/legacy: caster took feedback)
        damage: [0, 0]
        stun: [0, 0]
        morale: 0
      backlashDamageType: -1    # ResistType the caster backlash deals; -1 = the amp's own
```

- **`basePower` + `powerPerMargin`** replace legacy's tier constants with two plain knobs: `powerPerMargin: 0`
  gives a flat blast that only cares whether you won; a low `basePower` with a higher `powerPerMargin`
  makes a psi duel's *margin* matter (a lopsided win hurts far more), which is the legacy feel without the
  magic numbers.
- **`damageType`** is the mod's call, just like the mind-control backlash. `-1` uses the amp's own type;
  point it at a free `DT_10..DT_19` slot with `ArmorEffectiveness: 0.0` (via the global `damageTypes:`
  node) for legacy's armor-ignoring psychic damage — but DX doesn't hard-code that, and a mod that wants
  armor to matter simply doesn't.
- **`backlashOnFailure`** reuses the shared psi backlash (see below), so a missed blast can hurt the
  caster — the "it can go wrong" risk legacy built in, now optional and tunable. No feedback on a
  *successful* blast (legacy's narrow-win self-damage is dropped as fiddly; a mod can approximate it by
  making blasts costly).

### Behaviour

- Targets a **unit**, joins the psi actions (`BA_MINDBLAST`, appended action type). Offered only when the
  amp has the node.
- Resolves through `psiAttackCalculate`: on a positive margin, `power = (basePower + powerPerMargin ×
  margin)`, rolled by `randomRange`, dealt via `TileEngine::hitUnit` with the chosen type. On a miss, the
  `backlashOnFailure` lands on the caster.
- **Damage goes through the standard hit path.** The first implementation called `BattleUnit::damage`
  directly; it now routes through `hitUnit`, the single entry point every other attack uses, so armor
  modifiers, the damage script hooks, the casualty path, the OXCE hit log, the combat log and — the part
  that was actually broken — **murderer attribution** all apply. Without `hitUnit`'s
  `setMurdererId`/`setMurdererWeapon`, a blast that killed (or that left the victim to bleed out or burn)
  credited nobody: no kill statistic, no promotion credit.
- **Always strikes the head**, front side, passed to `hitUnit` as an explicit side/bodypart override: a
  blast has no physical trajectory to derive a facing from, and a `relative` of `(0,0,0)` would otherwise
  be read by `damage()` as an under-the-feet hit.
- **Experience** follows the panic/mind-control convention: in `ETM_DEFAULT` the blast awards psi skill
  directly (gated on the caster being naturally psi-capable, as `psiAttack` does); with an explicit
  `experienceTrainingMode:` it defers entirely to `awardExperience` via `hitUnit`.
  This initially needed an opt-out parameter on `hitUnit`, because `awardExperience` had no `BT_PSIAMP`
  case and would have trained **firing**. That gap has since been fixed at the source — `awardExperience`
  now returns no award for a psi-amp in the default mode (see [DX-OXCE-Fixes.md](../DX-OXCE-Fixes.md)) —
  so the opt-out was removed again and the blast uses the plain `hitUnit` call.
- **Range / LOS**: limited by the amp's range like the other psi actions. (Legacy also required line of
  fire; the shared psi path already honours `LOSRequired`, so a mod gets that by setting it on the amp.)
- Cost: `tuMindBlast` / `costMindBlast`, falling back to `costUse`.

### Shared refactor

The `RuleMindControlBacklash` struct + `TileEngine::applyMindControlBacklash` are now used by two psi
features (mind-control backlash and mind-blast failure), so they are renamed to the neutral
`RulePsiBacklash` / `applyPsiBacklash`. Pure rename, no behaviour change; ~5 call sites.

## Open question

1. **Feedback on a successful blast?** Legacy damaged the caster on a *narrow* win too, modelling a
   psychic struggle that costs both sides. Dropped here as fiddly (three tiers, more constants). If we
   want it back it's a `backlashOnNarrowWin` + a `narrowMargin` threshold — but "make blasts expensive"
   covers most of the same design space without the extra surface.
