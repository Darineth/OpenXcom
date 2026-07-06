# Feature — Proportional Wound Recovery + Field Surgery

**Status:** ✅ Implemented (pending in-game testing). Phase 7, third and final medical item
(Bleedout & Indicators ✅ → Medikit/Stabilization Rework ✅ → **Proportional Wound Recovery +
Field Surgery**). Builds on the same `postMissionProcedures` path the medical trio touches.

## Motivation

After a battle, a wounded soldier is out of action for a number of days ("wound recovery"). In
stock OXCE that count is proportional to the **absolute** HP lost, which quietly punishes tough
soldiers: a 90-HP veteran who loses 30 HP convalesces far longer than a 30-HP rookie who loses the
same 30 HP, even though the veteran shrugged off a smaller *fraction* of their health. DX replaces
this with a **proportional** model — recovery tracks the fraction of health lost, so full-health-loss
is a roughly fixed convalescence window regardless of a soldier's toughness — and adds a research
gate, **Field Surgery**, that shortens recovery base-wide once unlocked.

## Audit — what OXCE-Plus already provides (Jul 2026)

Confirmed by source audit. The entire feature lives in **one formula**; everything else is reused.

- **Recovery is set in one place:** `BattleUnit::postMissionProcedures(mod, geoscape, battle, statsDiff)`
  ([BattleUnit.cpp:4417](../src/Savegame/BattleUnit.cpp#L4417)), called per surviving soldier from
  debriefing ([DebriefingState.cpp:1605](../src/Battlescape/DebriefingState.cpp#L1605)). The value is
  written via `s->setWoundRecovery(recovery)` ([BattleUnit.cpp:4520](../src/Savegame/BattleUnit.cpp#L4520)).
  The function **already receives both `mod` and `geoscape`**, so a research gate
  (`geoscape->isResearched(id)`) and mod-config lookups need no new plumbing.
- **Current formula** ([BattleUnit.cpp:4433-4437](../src/Savegame/BattleUnit.cpp#L4433-L4437)):
  ```cpp
  int healthLossOriginal = _stats.health - _health;                                       // max - current
  auto recovery = (int)RNG::generate((healthLossOriginal*0.5),(healthLossOriginal*1.5));  // absolute, 50%–150% spread
  ```
- **Post-formula modifiers already applied (kept intact):** `_armor->getInstantWoundRecovery()` forces
  `recovery = 0` ([:4499](../src/Savegame/BattleUnit.cpp#L4499)); a `ModScript::ReturnFromMissionUnit`
  armor script can rewrite `recovery` ([:4504-4515](../src/Savegame/BattleUnit.cpp#L4504-L4515)). Both
  run *after* our new formula, so they continue to work unchanged.
- **Health regen / sick bay — complete, no changes:** `Soldier::replenishStats` / `healWound`
  ([Soldier.cpp:1211-1283](../src/Savegame/Soldier.cpp#L1211)), `Base::getSumRecoveryPerDay`
  ([Base.cpp:2511](../src/Savegame/Base.cpp#L2511)), facility `healthRecoveryPerDay` /
  `sickBayAbsoluteBonus` / `sickBayRelativeBonus`. Recovery *display* via
  `Soldier::getWoundRecovery`/`getNeededRecoveryTime` (SoldierInfo, Soldiers, CraftSoldiers, Debriefing).
- **Medikit healing / fatal wounds — complete, no changes.**
- **Existing config surface:** the `health:` mod-info node already holds `woundThreshold` and
  `replenishAfterMission` ([Mod.cpp:3392-3396](../src/Mod/Mod.cpp#L3392-L3396)) — the natural home for
  the new keys, mirroring `_healthReplenishAfterMission` storage.
- **Absent:** any proportional/fraction-based recovery, any recovery-multiplier option, and any
  `STR_FIELD_SURGERY_UNIT` / research-gated healing (the only "surgery" in-tree is the unrelated stock
  `STR_ALIEN_SURGERY`).

## Legacy DX design (intent to reproduce)

From `Legacy-DX-Features.md` §7 (and `reference/OriginalLegacyCommitText.txt:1017`):
- **Proportional recovery:** `woundRecovery = healthLoss × RNG(20–30) / maxHealth` days — so full HP
  loss ≈ 20–30 days for any soldier, and recovery scales with the fraction of health lost.
- **Field Surgery research (`STR_FIELD_SURGERY_UNIT`):** once researched, the multiplier band drops to
  `RNG(15–25)` for **all** soldiers (research is save-wide), ~25% faster convalescence. One-time
  permanent gate.

## Scoping decisions (locked)

1. **Formula:** legacy **fraction-based** — `healthLoss × RNG(min–max) / maxHealth`, default band
   20–30. (Chosen over keeping the absolute model.)
2. **Enablement:** **opt-in**, defaults preserve stock/OXCE. A mod (or the DX ruleset) turns it on;
   stock behavior is byte-for-byte unchanged when off. Matches DX's pattern for balance-changing
   combat features (aim-cone, realistic throwing).
3. **Field Surgery:** **engine hook only, mod-configurable.** The engine reads a configurable research
   id (default `STR_FIELD_SURGERY_UNIT`, empty = disabled) and a reduced RNG band; the actual research
   topic / Ufopaedia content stays in the ruleset (not shipped by this feature).

## Proposed DX approach (deltas only)

### 1. Config — extend the `health:` mod-info node (`Mod.h` / `Mod.cpp`)

Add fields + getters on `Mod`, loaded in the existing `if (const auto& nodeHealth = loadDocInfoHelper("health"))`
block ([Mod.cpp:3392](../src/Mod/Mod.cpp#L3392)):

| Key (`health:` node)        | Field                          | Default                  | Meaning |
|-----------------------------|--------------------------------|--------------------------|---------|
| `proportionalRecovery`      | `_proportionalWoundRecovery`   | `false`                  | Master toggle. Off = stock absolute formula. |
| `recoveryDaysMin`           | `_woundRecoveryDaysMin`        | `20`                     | RNG band lower (days at 100% HP loss). |
| `recoveryDaysMax`           | `_woundRecoveryDaysMax`        | `30`                     | RNG band upper. |
| `fieldSurgeryResearch`      | `_fieldSurgeryResearch`        | `STR_FIELD_SURGERY_UNIT` | Research id gating the faster band; empty = disabled. |
| `fieldSurgeryDaysMin`       | `_fieldSurgeryDaysMin`         | `15`                     | Reduced band lower, once researched. |
| `fieldSurgeryDaysMax`       | `_fieldSurgeryDaysMax`         | `25`                     | Reduced band upper. |

Getters follow the `getReplenishHealthAfterMission()` inline style. Defaults live in the member
initializers so an absent `health:` node = stock behavior (toggle stays false → formula never engages).

### 2. Formula — replace the one line (`BattleUnit::postMissionProcedures`)

Replace [BattleUnit.cpp:4437](../src/Savegame/BattleUnit.cpp#L4437) with a branch. Everything before
(`healthLossOriginal`) and after (instant-recovery override, script hook, `setWoundRecovery`) is
untouched:

```cpp
int recovery;
if (mod->getProportionalWoundRecovery() && _stats.health > 0)
{
    int daysMin = mod->getWoundRecoveryDaysMin();
    int daysMax = mod->getWoundRecoveryDaysMax();
    const std::string& fsResearch = mod->getFieldSurgeryResearch();
    if (!fsResearch.empty() && geoscape->isResearched(fsResearch))
    {
        daysMin = mod->getFieldSurgeryDaysMin();
        daysMax = mod->getFieldSurgeryDaysMax();
    }
    // fraction of max health lost × a random full-loss convalescence window
    recovery = (int)(healthLossOriginal * (double)RNG::generate(daysMin, daysMax) / (double)_stats.health);
}
else
{
    recovery = (int)RNG::generate((healthLossOriginal * 0.5), (healthLossOriginal * 1.5)); // stock
}
```

Notes:
- `_stats.health` is max health; `healthLossOriginal = _stats.health - _health`. Dividing by
  `_stats.health` gives the fraction. Guarded by `_stats.health > 0` (falls back to stock if a unit
  somehow has 0 max HP).
- `recovery` changes from `auto` (was `int` via the cast) to an explicit `int` so both branches assign it.
- `RNG::generate(int,int)` is inclusive; a fresh roll per soldier per the legacy intent.
- The result still flows through `getInstantWoundRecovery()` and the `ReturnFromMissionUnit` script.

### 3. Localization / content

- **No new C++-facing strings** — recovery is a number rendered by existing display code.
- The `STR_FIELD_SURGERY_UNIT` **research topic + Ufopaedia article** are *out of scope* per decision 3;
  a mod/the DX ruleset can add them later. Until then the default id simply never resolves as researched,
  so the base band is always used (feature still works; Field Surgery is just dormant).

## Testing (manual, in-game)

- Toggle `proportionalRecovery: true` in a test mod's `health:` node. Wound a high-HP and a low-HP
  soldier by the same absolute amount → the high-HP soldier should recover in fewer days (fraction
  smaller). A near-death soldier of any toughness → ~20–30 days.
- Leave the toggle off → recovery matches stock (spot-check a couple of values against `RNG(0.5,1.5)×loss`).
- Grant `STR_FIELD_SURGERY_UNIT` (debug research / a stub topic) → subsequent recoveries use the 15–25
  band (visibly shorter). Verify it applies to *all* soldiers, not just newly-wounded.
- Confirm `instantWoundRecovery` armor still yields 0 days, and a `ReturnFromMissionUnit` script can
  still override the value.

## Out of scope

- Shipping the Field Surgery research topic/Ufopaedia content (engine hook only, per decision 3).
- Any change to in-base health regen, sick-bay facilities, or medikit healing (all already complete).
- Per-soldier or per-armor recovery multipliers (the `ReturnFromMissionUnit` script already covers
  bespoke per-armor cases).

## Resolved decisions

- **Fraction-based formula** (`× RNG / maxHealth`), default band 20–30.
- **Opt-in**, defaults preserve OXCE (`proportionalRecovery` defaults false).
- **Field Surgery = configurable engine hook** (`fieldSurgeryResearch` id + reduced band), content
  left to the ruleset.
- **Config lives in the `health:` mod-info node**, alongside `woundThreshold` / `replenishAfterMission`.
- **Instant-recovery + script hooks preserved** (run after the new formula, unchanged).
