# Feature: Psi-Amp Ammo

**Status:** ✅ Implemented (Jul 2026). Test config in `bin/standard/dx-test/dx-test.rul` (STR_PSI_ORB clip).
**Roadmap:** Phase 8 (Lighting & Psionics). The final phase-8 item; closes the psi cluster.
**Prereqs:** `battleClipSize` (Phase 6) — in. The psi powers (panic/MC/clairvoyance/mind-blast) — in.
**Hard requirement:** **mod opt-in.** No `psiAmmo:` node ⇒ psi is free, exactly as stock OXCE.

## Motivation

Every psi power DX added is free to cast beyond its TU. Psi-Amp Ammo gives psi a *consumable* — a
psi-amp draws charges (orbs) from a loaded clip, so a psi operative can run dry, and psi becomes a
logistics decision like any other weapon rather than an infinite side-channel.

## Audit (Jul 2026)

Stock psi is free. `BattleItem::getAmmoForAction` routes through `getActionConf`
([BattleItem.cpp:847](../src/Savegame/BattleItem.cpp)), whose switch returns `nullptr` for every psi
action (`BA_MINDCONTROL`, `BA_PANIC`, `BA_USE`, and DX's `BA_CLAIRVOYANCE`/`BA_MINDBLAST`) — so a
psi-amp never consults a clip, and the shared cost block (`Time/Energy/Morale/Health/Stun/Mana`) has no
`Ammo` field. The legacy fork bolted an `Ammo` sub-key onto its psi cost blocks; DX doesn't have that
sub-key and won't add it to the shared struct (it would appear on the firing costs, where ammo is
already spent per shot). So this is a small, dedicated mechanism.

`battleClipSize` (Phase 6) already lets a psi orb be stocked one round at a time and packed into a
magazine at battle start — so the *clip* side is done; this feature just spends from it.

## Design

### Ruleset (opt-in, per psi-amp)

```yaml
items:
  - type: STR_PSI_AMP
    compatibleAmmo: [ STR_PSI_ORB ]   # OXCE: the amp loads a clip into ammo slot 0
    psiAmmo:                          # DX: per-cast round cost. Absent ⇒ psi is free (stock).
      mindControl: 4
      panic: 1
      use: 1                          # the BA_USE psi-damage attack
      clairvoyance: 2
      mindBlast: 1
      # any psi action not listed costs 0 rounds (free)

  - type: STR_PSI_ORB
    battleType: 2                     # BT_AMMO
    clipSize: 1
    battleClipSize: 12                # Phase 6: bought/stored 1 orb at a time, packed into 12-round mags
```

Each psi action's round cost is an independent key, so a mod can make mind control expensive and panic
cheap. Missing key = 0 = free, so `psiAmmo:` with only `mindControl:` set charges only for MC.

### Behaviour

- The clip is the amp's ammo in **slot 0** (`getAmmoForSlot(0)`), loaded via the stock `compatibleAmmo`
  path. A psi-amp with a `psiAmmo` cost > 0 but no clip loaded refuses the action
  (`STR_NO_AMMUNITION_LOADED`); a loaded-but-insufficient clip refuses with `STR_NO_ROUNDS_LEFT` — the
  same messages the firing path uses.
- **Spent on the attempt, hit or miss.** You fire the orb regardless of whether the target resists —
  matching real ammo and the legacy fork. The check gates the action *before* TU is spent; the spend
  happens once the action commits.
- **One chokepoint covers four of five actions.** Panic, mind control, the `BA_USE` psi attack and mind
  blast all run through `PsiAttackBState::init` — the ammo gate + spend go there, right beside the
  existing TU spend. Clairvoyance resolves in the `BattlescapeGame` handler, so it gets the same
  gate/spend inline.
- **Action menu** shows the round cost and flags an insufficient clip (greyed / warning), exactly as the
  overwatch and firing entries already do.

### Engine

- `RuleItem::getPsiAmmoCost(BattleActionType)` — reads the `psiAmmo:` node; 0 if absent/unlisted.
- `BattleItem::getPsiClip()` — the loaded clip (`getAmmoForSlot(0)`), or null.
- A shared `hasPsiAmmo` / `spendPsiAmmo` pair (on `BattleItem`, or a small `TileEngine`/`BattlescapeGame`
  helper) so the two call sites stay identical.

### Not in scope

- **Percentage-of-max-health psychic damage.** The roadmap line pairs the ammo with "percentage-based,
  armor-reducible psychic damage" — a legacy TODO that was never implemented. Mind Blast already covers
  *armor-reducible* psychic damage (its `damageType` is the mod's choice, so armor can blunt it), and a
  mod can approximate *percentage* damage with a custom damage type. A true "% of the target's max
  health" mode would be a separate small Mind-Blast option, not part of the ammo feature; deferred unless
  wanted.

## Open question

1. **Refund an orb on a wasted cast?** (e.g. clairvoyance with no valid target, or a blocked action.) The
   gate runs before commit, so a *blocked* action spends nothing; the question is only whether a
   *resolved but ineffective* cast (a resisted MC) refunds. Proposed: no — the orb was expended. Confirm.
