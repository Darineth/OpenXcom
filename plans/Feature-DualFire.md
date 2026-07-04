# Feature: Dual-Fire

**Status:** **Implemented (Jul 2026)**, pending in-game testing. Fire **both hands' weapons at once** — a soldier
dual-wielding two one-handed firearms can loose a shot from each simultaneously at the same target,
each with its own ammo and aim-cone. The last core Phase 5 combat mechanic; a port of the legacy
OpenXcom+ `BA_DUALFIRE`.

## Motivation

DX already flies multiple projectiles at once (shotgun pellets, concurrent auto rounds). Dual-fire
uses that to let a pistol-in-each-hand soldier trade accuracy for volume — a distinct tactical
option that the async-projectile system finally makes clean to implement.

## Audit — what exists

- **Enabler present:** the async multi-projectile system (`Map` owns a `std::vector<Projectile*>`;
  shotgun pellets already spawn and resolve concurrently). This is what dual-fire needs.
- **No dual-fire in DX yet:** a grep finds `BA_DUALFIRE` only in `Legacy-DX-Features.md` (and hand
  rendering in `UnitSprite.cpp`). Genuine port.
- **Closest DX precedent — Burst mode:** DX added `BA_BURSTSHOT = 20` as a new fire mode with its
  own `RuleItem::_confBurst` (a `RuleItemAction`: accuracy/cost/shots/range/name), `getConfigBurst`,
  `getCostBurst`, an `ActionMenuState` entry, and a hotkey (`keyBattleActionItem6`). Dual-fire adds a
  mode the same way, but its *execution* differs (two weapons once each, vs. one weapon N times).
- **Hand access:** `BattleUnit::getRightHandWeapon()` / `getLeftHandWeapon()` already exist.
- **Legacy design:** `BA_DUALFIRE` with a nested `_dualState` sub-state in `ProjectileFlyBState`
  firing the second weapon concurrently; a per-weapon `accuracyDualFire`; `twoHandedModifier` /
  dual-wield accuracy penalty; `keyActionDualFire`.
- **Enum:** next stable value is **`BA_DUALFIRE = 21`** (after `BA_BURSTSHOT = 20`; appended to keep
  serialized values stable).

## Target design (first cut)

A **new fire mode**, offered only when the unit is **dual-wielding two eligible firearms**:

- **Eligibility (decision):** both hands hold a firearm, both loaded with usable ammo. Likely
  restricted to **one-handed** weapons (a two-handed weapon needs the other hand) — see *Open
  decisions*.
- **Behavior:** one shot from **each** hand, **concurrently**, both aimed at the same target tile.
  Each shot is fully independent — its own weapon, ammo, damage, fire sound, and **its own aim-cone**
  (so the two rounds spread separately). Reuses the async projectile collection.
- **Accuracy (decision):** each hand's shot runs the normal accuracy pipeline (aim-cone for
  `baseAccuracy > 0` weapons, native scatter otherwise) with a **dual-wield penalty** — you can't
  steady either weapon. Either a dedicated per-weapon `accuracyDualFire` (mirrors burst) or the
  weapon's snap accuracy × a flat dual-fire penalty (see *Open decisions*).
- **Cost (decision):** TU/energy = **sum of both hands'** shot cost (must afford both); or a
  dedicated per-weapon dual-fire cost.
- **UI:** an `ActionMenuState` entry (`STR_DUAL_FIRE`) shown only when eligible, bound to a new
  hotkey; shows the combined cost. The aim-cone hover readout applies per shot as usual.
- **Experience / logging:** each hand's shot awards firing experience and logs its own hit-log line
  (or a combined "dual fire" line), consistent with how auto/burst handle multi-shot.

## Execution approach (implementation detail, not a user decision)

Two candidates; pick during implementation:
1. **Async-spawn (DX-native, preferred):** the fire state fires the primary (right) hand normally,
   then spawns the **off-hand** weapon's `Projectile` into the map collection with its *own*
   `BattleAction`/ammo — the same mechanism shotgun pellets use, but with the second weapon. Each
   projectile already carries its action/ammo, so impacts resolve with the correct weapon. Needs the
   off-hand's TU/ammo spent and experience awarded.
2. **Nested sub-state (legacy):** push a second `ProjectileFlyBState` for the off-hand, run
   concurrently. More faithful to legacy; reuses the full per-weapon fire pipeline verbatim.

The async-spawn path is likely cleaner given DX's existing shotgun-pellet code, but the nested
sub-state is the safer port if per-weapon impact resolution proves fiddly.

**Chosen: nested sub-state.** The primary `ProjectileFlyBState` (right hand) owns a second
`_dualState` (`_subState == true`) for the off (left) hand. The primary's `think()` drives **both**
hands' shot cadence and resolves **all** in-flight projectiles, deriving each impact's `attack` from
the **projectile's own** stored action/ammo (a no-op for existing single-weapon/shotgun cases). The
off-hand sub-state only fires its shots (its `createNewProjectile`) on cadence and never advances
projectiles, runs reaction fire, aborts the turn, or pops the queue. The action ends when **both**
sequences are exhausted and no projectiles remain. This reuses the 260-line `createNewProjectile`
per hand untouched.

## Implementation sketch (delta)

1. **`BA_DUALFIRE = 21`** in the `BattleActionType` enum (`RuleItem.h`).
2. **RuleItem config** — if using a dedicated mode config, add `_confDualFire` + accessors mirroring
   `_confBurst`; else derive from snap + a penalty constant.
3. **Action menu** — `ActionMenuState::addItem(BA_DUALFIRE, ...)`, shown only when both hands hold
   eligible loaded firearms; combined cost; new `keyBattleActionItem*` binding.
4. **Fire execution** — handle `BA_DUALFIRE` in `ProjectileFlyBState` (async-spawn the off-hand
   shot) / `BattlescapeGame`, spending both weapons' TU/ammo and awarding both experience.
5. **Accuracy** — thread the dual-wield penalty into `getFiringAccuracy` for `BA_DUALFIRE` (both
   hands occupied), so it flows into the cone and readouts.
6. **Language / hotkey / docs.**

## Resolved decisions (Jul 2026)

- **Eligibility:** **any two firearms, both loaded** (not restricted to one-handed). If both hands
  hold a firearm with usable ammo, dual-fire is offered.
- **Per-hand mode:** each hand fires the **first fire mode it has** in priority order
  **Auto → Burst → Snap → Aimed**. That mode supplies both the **accuracy** and the **shot count**,
  and **each hand fires that mode's FULL sequence** (an auto weapon dual-fires all its auto rounds).
  So dual-fire runs **two concurrent multi-shot sequences**, one per hand, both at the same target.
  (No separate dual-wield penalty constant — the rapid mode's own lower accuracy is the tradeoff;
  the standard two-handed/one-handed occupancy penalty still applies via `getFiringAccuracy`.)
- **Cost:** `TU = min(96, round( max(handTU_left, handTU_right) * 1.1 ))`, where each `handTU` is the
  TU of that hand's chosen mode. The **higher** of the two (they fire simultaneously) × **1.1**,
  **capped at 96** (both values carried over from the legacy fork; the 96 cap's original rationale is
  unknown — likely to keep it affordable within one turn's TU). Energy handled analogously.
- **Opt-in:** **always available when eligible** (no option/flag) — it's a player action, not a
  per-weapon behavior change.

## Execution — nested dual sequence (chosen)

Because BattleStates run one-at-a-time on the queue, two separate fire states would fire
**sequentially**, not concurrently. To fire both hands' full sequences at once, port the legacy
nested approach: one `ProjectileFlyBState` (for `BA_DUALFIRE`) drives **two** shot sequences — the
primary (right) hand and an off-hand sub-sequence (`_dualState` sub-state or an internal second
weapon/ammo/counter). Each `think()` advances both hands' cadence; the action ends when **both**
sequences are done and all projectiles have resolved. The off-hand sub-state must **not** run the
turn-end / popState / unit-abort logic (guarded by a `_subState` flag) — only the primary does.

## Implemented — where the pieces landed

- **`BA_DUALFIRE = 21`** (`RuleItem.h`); `RuleItem::getDualFireMode()` (first of auto/burst/snap/aimed).
- **`BattleUnit::canDualFire()` / `getDualFireCost()`** — eligibility + `min(96, round(max×1.1))` cost.
- **Cost plumbing:** `BattleActionCost::updateTU` routes `BA_DUALFIRE` to `getDualFireCost`, so the
  menu, affordability, and spend all agree. The primary keeps `_action`'s cost = the dual cost and
  spends it once; the sub-state spends nothing.
- **Action menu** (`ActionMenuState`): eligibility-gated entry, own compact row (no single accuracy),
  `keyBattleActionItem7`; the menu array was grown 6→8 (a full firearm + dual-fire is 7 rows).
- **Dispatch** (`BattlescapeGame`): `BA_DUALFIRE` added to the attack-action list.
- **Per-projectile resolution** (`ProjectileFlyBState::think`): each impact's `attack` now comes from
  the projectile's own `getAction()`/`getAmmo()` (no-op for single-weapon/shotgun; required so the
  off-hand round applies the off-hand weapon's damage). Explosion-radius check uses `attack.damage_item`.
- **Nested execution** (`ProjectileFlyBState`): `setupDualFire()` converts the primary to the right
  hand's mode and builds a manually-configured `_dualState` for the left (never run through `init()`,
  so it can't pop/abort/spend). Both first shots fire in `init()`; `think()` drives both via the new
  `advanceFiring()` and finishes only when **both** sequences are exhausted and no projectiles remain.
  `createNewProjectile`'s no-LOF failure paths are `_subState`-guarded so the off-hand can't
  abort/pop the primary (it just skips its shot). Each hand awards its own experience / hit-log /
  ammo; accuracy is each hand's chosen mode (two-handed penalty still applies via `getFiringAccuracy`).
- **Stagger:** the primary fires its first shot in `init()`; the off-hand's cooldown is primed to
  `DUAL_FIRE_STAGGER` (6 think-cycles, ~100 ms) so it starts firing a beat later via `advanceFiring()`
  rather than in lockstep — the state stays alive through the stagger because the off-hand's
  `advanceFiring()` reports "still firing" while its first shot is pending.

## Known caveats / minor

- **Right hand is "primary"** (arbitrary); both fire the same target regardless of which weapon opened
  the menu.
- **Targeting display.** `BA_DUALFIRE` has no single weapon/mode, so `getAmmoForAction`/
  `getFiringAccuracy` don't resolve it. Two display paths in `Map.cpp` handle it:
  - **Crosshair readout — both hands.** Shows `R:<right>% L:<left>% @<dist>m`, each hand's own
    percentage (aim-cone hit-chance for a cone weapon, folded accuracy for a vanilla one), since the
    two weapons/modes can differ. Both values are cached (`_cacheHitChance` = right,
    `_cacheHitChance2` = left); color-graded by the better hand. (Averaging/combining was rejected —
    two honest numbers beat one number that hides the per-hand asymmetry.)
  - **Trajectory preview — one line.** Draws a single tracer line for one hand via
    `BattleUnit::getDualFireDisplayWeapon()` (prefers a cone-model hand). Both hands aim at the same
    tile, so the one line is representative; a second line wasn't worth the clutter.
- **CQB** and the explosion "last shot" cleanup flag are primary-hand-based (minor).
- The off-hand plays its own fire sound and `aim()`; two fire-log lines per dual-fire (one per hand).

## Future / deferred

- **Different targets per hand** — first cut fires both at the same target.
- **Dual-fire + reaction/overwatch** — first cut is a deliberate player action only.
