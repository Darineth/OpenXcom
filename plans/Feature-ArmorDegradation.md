# Feature: Armor Degradation

**Status:** Implemented (Phase 3 follow-up).

## Motivation

DX wants armor wear to matter even when a strong hit fails to penetrate. The desired behavior is:

- weak glancing hits do nothing;
- a heavy hit that nearly breaks through can still dent the struck side's armor;
- a hit that strongly penetrates can also smash armor extra hard.

That creates sustained side-armor erosion under focused fire without making every low-power hit chip armor.

## OXCE / OXCE-Plus Audit

### Current upstream behavior

Current DX inherits the newer fork's damage-type armor hooks unchanged. In both trees, unit armor loss is
resolved in the same `BattleUnit` hit pipeline:

1. Start from the rolled unit damage for the hit.
2. Apply `ToArmorPre` from that rolled damage.
3. Subtract `currentArmor[side] * ArmorEffectiveness` from the damage.
4. Only if damage remains above zero, apply health/stun/wound/etc. and also apply `ToArmor` from the
   remaining damage.
5. Commit the accumulated armor loss back into `currentArmor[side]`.

That means the fork already supports two distinct armor-damage stages, but neither stage expresses
"only wear armor on blocked hits that were strong enough to nearly penetrate".

### `ToArmorPre` vs `ToArmor`

| Hook | When it runs | Input value | Can affect fully blocked hits? | What it is good at | What it cannot express well |
|------|--------------|-------------|--------------------------------|--------------------|-----------------------------|
| `ToArmorPre` | Before armor reduction | Rolled damage | Yes | Damage types that always scour armor regardless of penetration | A thresholded "only if this blocked hit was heavy enough" rule |
| `ToArmor` | After armor reduction, only when damage remains | Post-armor damage | No | Armor wear tied to actual penetration | Any wear on a hit that was completely stopped |

### Comparison to the newer fork

There is no material DX-vs-fork delta here yet. The local `OpenXcom-oxce-plus` mirror contains the same
`RuleDamageType` fields (`ArmorEffectiveness`, `ToArmorPre`, `ToArmor`) and the same `BattleUnit`
damage-ordering semantics as the current DX tree. For this feature, the fork provides useful building
blocks, but not the requested behavior.

### Important limitation in the current model

Fully blocked hits currently only damage armor if a modder has already opted into unconditional pre-armor
wear via `ToArmorPre`. If `ToArmorPre` is zero, a non-penetrating hit cannot wear armor at all, no matter
how close it came to punching through.

That is the exact gap this DX feature should fill.

## Shipped DX Design

### Ruleset fields

Add four optional `damageAlter` / `RuleDamageType` fields:

```yaml
damageAlter:
  ToArmorBlocked: 0.0
  ToArmorBlockedThreshold: 0.5
  ToArmorOverPen: 0.0
  ToArmorOverPenThreshold: 2.0
```

- `ToArmorBlocked`: maximum fraction of rolled damage converted to armor wear on a hit that was fully
  blocked. `0.0` keeps current behavior and is the compatibility default.
- `ToArmorBlockedThreshold`: minimum fraction of the target side's effective armor block that the rolled
  damage must reach before a blocked hit can wear armor. `0.5` matches the desired "at least 50% of the
  armor's ability to block it" rule.
- `ToArmorOverPen`: extra armor damage multiplier for strongly penetrating hits.
- `ToArmorOverPenThreshold`: minimum multiple of effective armor the rolled damage must exceed before
  `ToArmorOverPen` applies. `2.0` matches the "past armor*2" behavior.

### Mechanics

Blocked-hit wear only runs when the hit does **not** penetrate. Over-penetration wear only runs when the
hit **does** penetrate.

Definitions:

- `rawDamage` = rolled damage before armor reduction.
- `effectiveArmorBlock` = `currentArmor[side] * ArmorEffectiveness`.
- `postArmorDamage` = `rawDamage - effectiveArmorBlock`.

Flow:

1. Keep upstream `ToArmorPre` behavior exactly as-is.
2. Keep upstream armor blocking exactly as-is.
3. If `postArmorDamage > 0`, keep upstream `ToArmor` behavior exactly as-is and do **not** run the new
   blocked-hit wear step.
4. If `postArmorDamage <= 0`, evaluate blocked-hit wear:
   - If `rawDamage < effectiveArmorBlock * ToArmorBlockedThreshold`, blocked-hit armor wear is zero.
  - Otherwise, convert overflow above that threshold into armor damage.
5. If `postArmorDamage > 0`, keep upstream `ToArmor` behavior and also evaluate over-penetration wear:
  - If `rawDamage <= effectiveArmorBlock * ToArmorOverPenThreshold`, no over-penetration wear.
  - Otherwise, convert overflow above that threshold into extra armor damage.

Implemented blocked-hit wear:

```cpp
float thresholdDamage = effectiveArmorBlock * ToArmorBlockedThreshold;
float overflowDamage = rawDamage - thresholdDamage;
int blockedArmorDamage = round(overflowDamage * ToArmorBlocked) + 1;
```

Implemented over-penetration wear:

```cpp
float thresholdDamage = effectiveArmorBlock * ToArmorOverPenThreshold;
float overflowDamage = rawDamage - thresholdDamage;
int overPenArmorDamage = round(overflowDamage * ToArmorOverPen);
```

Why this shape:

- it preserves the requested 50% blocked-hit entry threshold and guarantees at least 1 armor damage once
  blocked wear is eligible;
- it adds the requested "smashed through" behavior for strong penetration past `armor*2`;
- it avoids low-power chip damage;
- it keeps existing `ToArmorPre` and `ToArmor` semantics intact.

### Balance examples

Assume:

- side armor = `40`
- `ArmorEffectiveness = 1.0`
- `ToArmorBlockedThreshold = 0.5`
- `ToArmorBlocked = 0.2`
- `ToArmorOverPenThreshold = 2.0`
- `ToArmorOverPen = 0.2`
- `ToArmorPre = 0.0`
- `ToArmor = 0.25`

Results:

- `rawDamage = 15`: below the `20` blocked threshold, so blocked wear = `0`.
- `rawDamage = 21`: blocked and eligible; overflow is `1`, so blocked wear = `1` (guaranteed).
- `rawDamage = 39`: blocked and near penetration; overflow is `19`, so blocked wear = `5`.
- `rawDamage = 45`: penetrates; normal `ToArmor` applies from post-armor damage, over-pen still `0` because
  `45 <= 80`.
- `rawDamage = 100`: strong penetration; normal `ToArmor` applies plus over-pen overflow (`100 - 80 = 20`)
  adds extra armor wear.

## Why This Should Be a DX Layer, Not a `ToArmorPre` Rewrite

`ToArmorPre` already has a clear upstream meaning: unconditional armor damage derived from rolled damage,
before armor mitigation. Reinterpreting it as a thresholded blocked-hit mechanic would be a behavioral break
for existing OXCE/OXCE-Plus content and would blur the distinction between the pre-armor and post-armor
stages.

The clean approach is:

- keep `ToArmorPre` for unconditional pre-armor wear;
- keep `ToArmor` for penetration-driven wear;
- add a new DX-only blocked-hit wear stage for the "nearly penetrated" case.

## Implementation

### Data model

- `src/Mod/RuleDamageType.h` / `.cpp`
  - added `float ToArmorBlocked`;
  - added `float ToArmorBlockedThreshold`;
  - added `float ToArmorOverPen`;
  - added `float ToArmorOverPenThreshold`;
  - both load from YAML;
  - defaults are `0.0f` / `0.5f` for blocked wear and `0.0f` / `2.0f` for over-penetration wear.

### Hit resolution

- `src/Savegame/BattleUnit.cpp`
  - keeps the existing `ToArmorPre` accumulation before armor reduction;
  - computes blocked-hit wear from `rawDamage`, the struck side's effective armor block, and the
    blocked-hit ruleset fields;
  - only applies that blocked-hit wear when the hit was fully stopped by armor;
  - adds over-penetration armor wear on penetrating hits based on raw damage past `armor*threshold`;
  - keeps existing post-penetration `ToArmor` behavior.

### Visibility / tooling

- `src/Ufopaedia/StatsForNerdsState.cpp`
  - now exposes the two new fields in the technical dump.
- technical language files under `bin/common/Language/Technical/`
  - now include labels for the new damage-type properties.

`Extended.txt` is also updated so the new `damageAlter` properties are documented alongside the
existing `ToArmorPre` / `ToArmor` fields.

## Save / Mod Compatibility

- Save compatibility is straightforward: the feature still mutates the already-serialized `currentArmor[]`
  array on `BattleUnit`.
- Mod compatibility is preserved by defaulting `ToArmorBlocked` to `0.0`.
- Existing content using `ToArmorPre` or `ToArmor` keeps its current behavior.

## Recommendation

Implement armor degradation as a new blocked-hit wear stage on `RuleDamageType`, not as an `Armor` rule
field and not as a reinterpretation of `ToArmorPre`.

Reasoning:

- the requested behavior is about how a damage type interacts with armor, not about a passive armor suit
  property;
- the newer fork's existing armor hooks already live on `RuleDamageType`;
- the proposal composes cleanly with upstream `ToArmorPre` and `ToArmor` instead of replacing them.