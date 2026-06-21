# Feature: Armor Degradation

**Status:** Proposed (Phase 3 follow-up).

## Motivation

DX wants armor wear to matter even when a strong hit fails to penetrate. The desired behavior is:

- weak glancing hits do nothing;
- a heavy hit that nearly breaks through can still dent the struck side's armor;
- a hit that actually penetrates keeps using the fork's existing post-armor damage flow.

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

## Proposed DX Design

### Ruleset fields

Add two optional `damageAlter` / `RuleDamageType` fields:

```yaml
damageAlter:
  ToArmorBlocked: 0.0
  ToArmorBlockedThreshold: 0.5
```

- `ToArmorBlocked`: maximum fraction of rolled damage converted to armor wear on a hit that was fully
  blocked. `0.0` keeps current behavior and is the compatibility default.
- `ToArmorBlockedThreshold`: minimum fraction of the target side's effective armor block that the rolled
  damage must reach before a blocked hit can wear armor. `0.5` matches the desired "at least 50% of the
  armor's ability to block it" rule.

### Mechanics

This new step only runs when the hit does **not** penetrate.

Definitions:

- `rawDamage` = rolled damage before armor reduction.
- `effectiveArmorBlock` = `currentArmor[side] * ArmorEffectiveness`.
- `postArmorDamage` = `rawDamage - effectiveArmorBlock`.

Flow:

1. Keep upstream `ToArmorPre` behavior exactly as-is.
2. Keep upstream armor blocking exactly as-is.
3. If `postArmorDamage > 0`, keep upstream `ToArmor` behavior exactly as-is and do **not** run the new
   blocked-hit wear step.
4. If `postArmorDamage <= 0`, evaluate the new blocked-hit wear step:
   - If `rawDamage < effectiveArmorBlock * ToArmorBlockedThreshold`, blocked-hit armor wear is zero.
   - Otherwise, convert part of `rawDamage` into armor damage using a thresholded ramp.

Recommended ramp:

```cpp
float blockRatio = rawDamage / effectiveArmorBlock;
float progress = Clamp((blockRatio - threshold) / (1.0f - threshold), 0.0f, 1.0f);
float blockedMultiplier = ToArmorBlocked * (0.5f + 0.5f * progress);
int blockedArmorDamage = round(rawDamage * blockedMultiplier);
```

Why this shape:

- it preserves the requested 50% entry threshold;
- it lets one knob (`ToArmorBlocked`) cover the earlier balance target of roughly 10% wear at the threshold
  and 20% wear near the penetration boundary when `ToArmorBlocked = 0.2`;
- it avoids low-power chip damage;
- it leaves penetrating-hit behavior under the fork's existing `ToArmor` rules.

### Balance examples

Assume:

- side armor = `40`
- `ArmorEffectiveness = 1.0`
- `ToArmorBlockedThreshold = 0.5`
- `ToArmorBlocked = 0.2`
- `ToArmorPre = 0.0`
- `ToArmor = 0.25`

Results:

- `rawDamage = 15`: below the `20` threshold, so armor wear = `0`.
- `rawDamage = 20`: blocked, but eligible; multiplier starts at `0.1`, so armor wear = `2`.
- `rawDamage = 30`: blocked and close to penetrating; multiplier rises to `0.15`, so armor wear = `5`.
- `rawDamage = 39`: still blocked; multiplier is almost `0.2`, so armor wear = `8`.
- `rawDamage = 45`: penetrates; blocked-hit wear does not run, and normal upstream `ToArmor` applies from
  the post-armor remainder instead.

## Why This Should Be a DX Layer, Not a `ToArmorPre` Rewrite

`ToArmorPre` already has a clear upstream meaning: unconditional armor damage derived from rolled damage,
before armor mitigation. Reinterpreting it as a thresholded blocked-hit mechanic would be a behavioral break
for existing OXCE/OXCE-Plus content and would blur the distinction between the pre-armor and post-armor
stages.

The clean approach is:

- keep `ToArmorPre` for unconditional pre-armor wear;
- keep `ToArmor` for penetration-driven wear;
- add a new DX-only blocked-hit wear stage for the "nearly penetrated" case.

## Implementation Sketch

### Data model

- `src/Mod/RuleDamageType.h` / `.cpp`
  - add `float ToArmorBlocked`;
  - add `float ToArmorBlockedThreshold`;
  - load them from YAML;
  - default to `0.0f` and `0.5f` respectively.

### Hit resolution

- `src/Savegame/BattleUnit.cpp`
  - keep the existing `ToArmorPre` accumulation before armor reduction;
  - compute the blocked-hit wear candidate from `rawDamage`, the struck side's current armor, and
    `ArmorEffectiveness`;
  - only add that blocked-hit wear when `postArmorDamage <= 0`;
  - keep the existing post-penetration `ToArmor` path unchanged.

### Optional visibility / tooling follow-up

- `src/Ufopaedia/StatsForNerdsState.cpp`
  - expose the two new fields in the technical dump.
- technical language files under `bin/common/Language/Technical/`
  - add labels for the new damage-type properties.

Those are not required for the gameplay mechanic itself, but they make the rules discoverable.

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