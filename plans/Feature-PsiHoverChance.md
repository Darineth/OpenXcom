# Feature: Psi Success Chance on Hover

**Status:** ✅ Implemented (Jul 2026). Option C shipped; `accuracyMindBlast` blocker fixed first.
**Roadmap:** Phase 1 (Battlescape UX & Feedback) / Phase 8 (Psionics) — it is a readout over existing
psi mechanics, adding no new mechanic of its own.
**Related:** [Feature-MindBlast.md](Feature-MindBlast.md), [Feature-CombatLog.md](Feature-CombatLog.md)
(whose research-gating policy this must match).

## Motivation

**Psi is the only attack in the game with no feedback.** Every conventional attack shows an accuracy
figure in the action menu before you commit to it. A psi action shows its TU cost and nothing else — the
player picks Mind Control with no idea whether it is a coin flip or a formality, on an action that costs
a large slice of a turn and, on failure, may cost health or morale to backlash.

The information is not merely absent, it is *hidden*: an Alt-held readout on the cursor tile exists but is
undiscoverable, is a raw margin range rather than a probability, and is deliberately inaccurate (below).
Surfacing it is squarely the DX pattern of preferring visible UI to hotkey-only features.

## Audit (Jul 2026)

### The action menu shows nothing for psi

`ActionMenuState::addItem` builds the accuracy string only for a fixed list of action types
([ActionMenuState.cpp:377-397](../src/Battlescape/ActionMenuState.cpp)):

```cpp
else if (ba == BA_THROW || ba == BA_AIMEDSHOT || ba == BA_SNAPSHOT || ba == BA_AUTOSHOT
      || ba == BA_BURSTSHOT || ba == BA_LAUNCH || ba == BA_HIT)
    s1 = tr("STR_ACCURACY_SHORT").arg(Unicode::formatPercentage(acc));
```

`BA_MINDCONTROL` / `BA_PANIC` / `BA_USE` / `BA_MINDBLAST` fall through both branches, so `s1` stays empty
and only the TU cost is drawn. (Note the accuracy computed there is `getFiringAccuracy`, which is not
even the right function for psi.)

### An Alt-held indicator exists, and it deliberately lies

[Map.cpp:2072-2143](../src/Battlescape/Map.cpp), gated on `_isAltPressed` and on the weapon's Ufopaedia
article being unlocked:

```cpp
if (rule->getBattleType() == BT_PSIAMP)
{
    float attackStrength = BattleUnit::getPsiAccuracy(attack);
    float defenseStrength = 30.0f; // indicator ignores: +victim->getArmor()->getPsiDefence(victim);
    ...
    int min = attackStrength - defenseStrength - rule->getPsiAccuracyRangeReduction(dis);
    int max = min + 55;
}
```

Two things matter here. It is a **margin range, not a probability** — "12-67%" is not a hit chance and
reads as one. And it **deliberately omits the victim's `psiDefence`**, which the comment calls out
explicitly. That is best read as an intentional anti-information-leak decision upstream, not an
oversight, and it is the crux of this feature's design (see *Open decision*). It also predates DX's
counter-control rule, so it ignores that a mind control against an already-controlled unit contests the
**controller's** defence, not the victim's.

`InventoryState` explicitly declines to show psi odds at all, for a different reason
([InventoryState.cpp:2163-2166](../src/Battlescape/InventoryState.cpp)): *"doesn't show psi success
chance (distance is unknown)"*.

### There is no existing OXCE option for this

No `oxcePsi*`-style option, no `STR_*_PSI_CHANCE` string, and nothing in `Extended.txt` — the psi entries
there are all mechanics and ruleset keys, not UI. This is a genuine delta.

### The probability is exactly computable

The contest resolves through `TileEngine::psiAttackCalculate`, which delegates the roll to the
`tryPsiAttackItem` script hook. That hook's **default body**
([BattleItem.cpp:1819](../src/Savegame/BattleItem.cpp)) is a single uniform draw:

```
var int r; random.randomRange r 0 55;
add psi_attack_success attack_strength;
add psi_attack_success r;
sub psi_attack_success defense_strength;
sub psi_attack_success distance_strength_reduction;
return psi_attack_success;
```

So with `M = attackStrength − defenseStrength − distanceReduction` and `r ~ Uniform{0..55}` (56
outcomes), success is `M + r > 0`, giving a closed form:

```
P(hit) = clamp(M + 55, 0, 56) / 56       // 0% at M <= -55, 100% at M >= 1
```

*(Corrected during implementation — an earlier draft of this doc wrote `(M + 56) / 56`, which is off by
one: success is `M + r > 0`, strictly, so at `M = 0` the roll `r = 0` loses and the true chance is 55/56,
not 100%. The form above also matches the stock indicator's own framing, where the rolled total spans
`M` to `M + 55`.)*

Every input is already available at hover time, and `Map.cpp` computes `M` and `M+55` today — the
percentage is one division away. `tryPsiAttackUnit` (the armor hook) runs afterward but has **no RNG
argument**, so it can only veto or shift the result, never re-roll it.

**Caveat:** the formula lives in a *replaceable* script default. A mod can override `tryPsiAttackItem`
with an arbitrary body (different distribution, several draws, none at all), and `tryPsiAttackUnit` can
transform the outcome. A displayed percentage is therefore exact only when both hooks are unscripted —
the overwhelmingly common case, but not guaranteed.

## Design

### Where it goes

**Targeting-mode hover, not the action menu.** This is structural, not a preference: firing accuracy is
essentially target-independent and can be shown when the menu opens, but psi chance depends on the
specific victim's `psiDefence` *and* the range to them, neither of which exists at menu time. Hover is
the only place the number is well-defined. (This is also why `InventoryState` declines to show it.)

It should **supersede the Alt-held indicator for `BT_PSIAMP`**, not sit beside it — two psi numbers that
disagree is worse than one that is hidden.

### Decision: what to show against an unknown enemy — **C, shipped**

This was the one genuine design fork. Settled as **C**.

An exact percentage folds in the target's `psiDefence`, so hovering an unresearched alien would let the
player read off its psi defence by arithmetic — turning a hover into a free interrogation. That cuts
against the care taken in the combat log, which reveals exact damage only for own units and researched
hostiles and shows a vague band otherwise.

| Option | Behavior | Trade-off |
|---|---|---|
| **A. Always exact** | True odds for every target | Most usable; leaks unresearched aliens' psi stats |
| **B. Gated exact / banded** | Exact for own + researched; "Likely / Even / Unlikely" otherwise | Matches combat-log policy; band is vague at the moment of decision |
| **C. Gated exact / baseline estimate** *(chosen)* | Exact for own + researched; otherwise computed against the **baseline** defence (30, no `psiDefence`), visibly marked as an estimate | Always gives a number to act on, never reveals what you haven't researched; degrades to exactly what the current Alt indicator already assumes |

**Chosen: C.** It preserves upstream's existing anti-leak stance (the indicator already assumes
baseline defence), it gives the player a usable figure rather than a shrug, and researching a species
then *improves* your psi targeting information — a small, thematic reward that reuses the research gate
already threaded through the combat log and unit naming.

### Script fallback — not implemented

When the amp's `tryPsiAttackItem` or the target armor's `tryPsiAttackUnit` is non-default, the closed form
no longer holds and the percentage is inexact.

**No detection ships.** The default body is installed by `setDefault` at parse time, so the script
container is non-empty even when the mod supplied nothing — there is no cheap way to ask "is this the
default script", and inventing an unreliable check seemed worse than documenting the limit. A mod that
replaces the psi roll gets a confidently-wrong number; this is recorded as a known limitation in
`DX-Features.md` rather than papered over.

### Toggle

`psiChanceIndicatorEnabled` (DX advanced option, Battlescape, default on). It also rides the general
crosshair-info gate (`oxceShowAccuracyOnCrosshair` / `_showInfoOnCursor`), like every other cursor readout.

## Blocker (fixed first)

`getPsiAccuracy` had no `BA_MINDBLAST` case, so any percentage shown for a mind blast would have been
computed from an accuracy value that silently ignored the amp's configuration — a faithfully-rendered
wrong number. Fixed before the readout went in; see *Known gap*, now resolved.

## Known gap — ✅ RESOLVED

`BattleUnit::getPsiAccuracy` ([BattleUnit.cpp:2652-2674](../src/Savegame/BattleUnit.cpp)) branches on
action type:

```cpp
int psiAcc = 0;
if      (actionType == BA_MINDCONTROL) psiAcc = item->getRules()->getAccuracyMind();
else if (actionType == BA_PANIC)       psiAcc = item->getRules()->getAccuracyPanic();
else if (actionType == BA_USE)         psiAcc = item->getRules()->getAccuracyUse();
psiAcc += item->getRules()->getAccuracyMultiplier(attack);
```

There is **no `BA_MINDBLAST` case**, so a mind blast gets `psiAcc = 0 + accuracyMultiplier` — the flat
per-action accuracy term is silently zero. This is a **DX bug**, not an upstream one: DX added
`BA_MINDBLAST` without extending this function, so it does not belong in `DX-OXCE-Fixes.md`.

Consequence: a mind blast is less accurate than any modder would reasonably expect, and no ruleset key
influences it. `accuracyPanic` defaults to 20 and `accuracyMindControl`/`accuracyUse` to 0
([RuleItem.cpp:183](../src/Mod/RuleItem.cpp)), so mind blast currently behaves like an
`accuracyMindControl: 0` amp regardless of configuration.

**Fixed** via option 2: a new **`accuracy`** key on the `mindBlast:` node (so the whole blast is tuned
in one place), read by a new `BA_MINDBLAST` branch in `getPsiAccuracy`.

It **defaults to 0**, matching `accuracyMindControl` and `accuracyUse` — which means the default
reproduces the old behavior exactly and a mod enabling `mindBlast:` must now set `accuracy:` to get a
competent blast. That is deliberate: silently inventing a non-zero default would retune every existing
mind-blast config on upgrade. The docs say plainly to set it, and `dx-test.rul` sets `accuracy: 20`.

---

# Implementation Notes

**Where:** `Map::drawTerrain`, as an `else if` on the existing cursor-readout chain — the branch the
aim/throw readouts already live in ([Map.cpp](../src/Battlescape/Map.cpp), after the
"end cone-model vs native accuracy readout" marker). The stock Alt-held psi block further down is now
guarded with `&& !Options::psiChanceIndicatorEnabled`, so exactly one psi number can ever appear.

**Two guards found during implementation, neither in the original design:**

1. **`BA_CLAIRVOYANCE` had to be excluded explicitly.** It shares the `CT_PSI` cursor, so a naive
   `_cursorType == CT_PSI` branch happily printed a contest percentage for it — but clairvoyance is not a
   contest at all (flat `psiScore` vs `minPsiScore`, against a tile, with no target unit). The readout is
   restricted to `BA_MINDCONTROL` / `BA_PANIC` / `BA_MINDBLAST` / `BA_USE`.
2. **Visibility gate.** The first cut drew the readout over any unit on the hovered tile, including one
   the player cannot see — the percentage appearing at all would betray that something was standing
   there, a worse leak than the `psiDefence` one this feature was careful about. Now gated on
   `unit->getVisible() || _save->getDebugMode()`, matching the hovered-unit-name overlay.

**Out-of-range** shows `0%`; note the pre-existing `_cursorType > CT_AIM` block further down also writes a
bare `"0%"` for an out-of-range psi cursor, which overwrites this branch's text in that one case. Left
as-is — same final readout, and unifying the two would touch the stock path for no visible gain.
