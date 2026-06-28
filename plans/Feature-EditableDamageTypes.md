# Feature: Editable Base Damage Type Properties

**Status:** Implemented. Top-level `damageTypes:` node, applied via a global early-load pre-pass in
`Mod::loadAll` (`loadEarlyRules`) that sweeps every mod's rulesets before any `items:` load.
Resolutions to the open questions are noted inline below.

## Motivation

The behavior of each damage type (how power converts to health/armor/stun/wound/tile damage, the
random spread, radius/armor effectiveness, the various `Ignore*` flags, etc.) is baked into the
engine. A mod can only tweak these *per item* via `damageAlter:` / `meleeAlter:`. There is no way to
change the **base** behavior of, say, `DT_AP` once, globally, and have every AP weapon inherit it.

DX wants damage types themselves to be moddable:

- redefine the standard types (`DT_AP`, `DT_IN`, `DT_HE`, `DT_LASER`, `DT_PLASMA`, `DT_STUN`,
  `DT_MELEE`, `DT_ACID`, `DT_SMOKE`) globally instead of editing every weapon;
- give the ten currently-empty placeholder slots (`DT_10`..`DT_19`) real, named, configured
  behavior so mods can introduce genuinely new damage types (e.g. EMP, cold, sonic) rather than
  reusing an existing type's index and overriding `damageAlter` on every item.

## OXCE / OXCE-Plus Audit

### Current upstream behavior

The base damage types are **hard-coded in the `Mod` constructor** under the `//load base damage
types` comment in [src/Mod/Mod.cpp](../src/Mod/Mod.cpp). Each entry is a `new RuleDamageType()` with
its `ResistType` set and a handful of fields tweaked, stored into:

```cpp
std::vector<RuleDamageType*> _damageTypes;   // sized to DAMAGE_TYPES, indexed by ResistType
```

The ten placeholders are built by a loop:

```cpp
for (int itd = DT_10; itd < DAMAGE_TYPES; ++itd)
{
    dmg = new RuleDamageType();
    dmg->ResistType = static_cast<ItemDamageType>(itd);
    dmg->IgnoreOverKill = true;
    _damageTypes[dmg->ResistType] = dmg;
}
```

The enum that fixes the slot count lives in [src/Mod/RuleDamageType.h](../src/Mod/RuleDamageType.h):

```cpp
enum ItemDamageType { DT_NONE, DT_AP, DT_IN, DT_HE, DT_LASER, DT_PLASMA, DT_STUN, DT_MELEE,
                      DT_ACID, DT_SMOKE, DT_10, DT_11, DT_12, DT_13, DT_14, DT_15, DT_16, DT_17,
                      DT_18, DT_19, DAMAGE_TYPES };
```

So there are **20 fixed slots** (0–19). `DAMAGE_TYPES` is the count and is used to size `_damageTypes`,
the per-armor resistance/damage-modifier arrays, and various loops. The placeholder names `DT_10`..
`DT_19` are exactly that — empty, generic slots whose only non-default field is `IgnoreOverKill`.

`Mod::getDamageType(type)` is a trivial accessor: `return _damageTypes.at(type);`.

### Key fact: `RuleDamageType::load()` already exists — it is just never used for the base table

[src/Mod/RuleDamageType.cpp](../src/Mod/RuleDamageType.cpp) already has a full
`RuleDamageType::load(const YAML::YamlNodeReader&)` that reads every field (`RandomType`,
`ResistType`, `FixRadius`, `ArmorEffectiveness`, `RadiusEffectiveness`, all the `To*` multipliers,
all the `Random*` flags, the `Ignore*` flags, `TileDamageMethod`, `TileDamageLimit`, thresholds,
etc.). It is used **only** by per-item `damageAlter` / `meleeAlter`:

```cpp
// RuleItem::load() — src/Mod/RuleItem.cpp
_damageType = *mod->getDamageType((ItemDamageType)type.readVal<int>());  // copy base by VALUE
reader.tryRead("blastRadius", _damageType.FixRadius);
if (const auto& alter = reader["damageAlter"]) { _damageType.load(alter); }   // overlay
// ...same pattern for _meleeType / meleeAlter
```

There is **no ruleset node** that calls `load()` on the entries in `_damageTypes`. A search of the
loader (`Mod::loadFile`) and `Extended.txt` confirms the only damage-type surface mods get is the
per-item `damageAlter` / `meleeAlter`. **This feature is a genuine DX delta, not an OXCE gap-fill.**

### Critical ordering consequence

Because `RuleItem::load()` **copies the base `RuleDamageType` by value** and then overlays
`damageAlter`, any global edit to a base type only reaches an item if the base edit is applied
*before that item is loaded*. This is the central design constraint (see Open Questions).

## Proposed Approach

Add a `damageTypes:` (working name) top-level ruleset node that re-runs `RuleDamageType::load()` on
the existing `_damageTypes[ResistType]` entry, keyed by the damage type index. Nothing about the
slot count changes — we are making the existing 20 slots configurable, not adding new ones.

### Ruleset shape (proposed)

```yaml
damageTypes:
  - ResistType: 1        # DT_AP — selects which base entry to edit (required key)
    ToArmor: 0.2
    ArmorEffectiveness: 1.1
  - ResistType: 10       # DT_10 — give a placeholder real behavior
    RandomType: 8
    ToHealth: 1.0
    ToStun: 0.5
    IgnoreDirection: true
```

`ResistType` is the lookup key (matching the existing field name `RuleDamageType` already reads).
All other keys are exactly the fields `RuleDamageType::load()` already understands, so the loader
addition is small.

### Loader wiring

In `Mod::loadFile` (the big `iterateRules(...)` sequence in
[src/Mod/Mod.cpp](../src/Mod/Mod.cpp)), add a handler that, for each child node, reads `ResistType`,
bounds-checks it (`0 <= idx < DAMAGE_TYPES`), and calls `_damageTypes[idx]->load(child)`. This must
run **before** the `items` iteration in `loadFile` so that items in the same file pick up the edited
base. Cross-file ordering is discussed below.

### UI / placeholder naming

`DT_10`..`DT_19` already have `STR_DAMAGE_10`..`STR_DAMAGE_19` strings (used by
`StatsForNerdsState` and the damage-type text helpers). If mods are to introduce *named* new types,
consider letting `damageTypes:` carry a display string key (or just rely on the existing
`STR_DAMAGE_1x` keys that the mod can localize). Out of scope for the first cut; the first cut only
makes the *numeric behavior* editable, reusing existing strings.

## Resolutions

1. **Cross-file / cross-mod load ordering → option (b), global pre-pass.** `Mod::loadAll` runs
   `loadEarlyRules` (a generic early-load pass that currently handles `damageTypes:`) over **every
   mod's** rulesets *before* the main per-mod loop that loads `items:`. This must be global, not
   per-mod: because each mod's `loadMod` loads that mod's items, a per-mod pre-pass could not let a
   later mod retune a damage type that an earlier-loaded mod's (e.g. the master's) items inherit.
   Mods are swept in load order, files within a mod in the same sorted order `loadMod` uses, so the
   result is last-wins per field across the whole load. Neither `loadMod` nor `loadFile` processes
   `damageTypes:` (the global pre-pass owns it).
2. **Fields exposed → all of `RuleDamageType::load()`'s fields**, reused as-is.
3. **`ResistType` as key → re-locked.** After `_damageTypes[idx]->load(node)`, the loader forces
   `ResistType = idx` so an entry cannot remap itself to another slot.
4. **Bounds → soft error.** `ResistType` outside `0..DAMAGE_TYPES-1` is rejected via
   `checkForSoftError` and the entry skipped.
5. **Save compatibility → none needed.** Mod data only; balance shift documented.
6. **More than 20 types → out of scope** (unchanged).
7. **`damageAlter` precedence → confirmed.** built-in default → global `damageTypes:` → per-item
   `damageAlter`.
8. **Display names → reuse existing `STR_DAMAGE_1x` strings** for the first cut.

## Original Open Questions

1. **Cross-file load ordering.** `RuleItem::load` copies the base type by value. If mod file A loads
   an item before mod file B overrides that base type, the item keeps the old defaults. Options:
   - **(a) Accept it / document it** — overrides must be defined before the items that should inherit
     them (in load order). Simple, but a footgun.
   - **(b) Pre-pass** — collect and apply all `damageTypes:` nodes across all files *before* loading
     any `items`. Most predictable, but a structural change to `loadAll`/`loadFile` sequencing.
   - **(c) Re-derive on item finalize** — have items keep a reference to the base index and re-copy
     at `afterLoad`/cross-link time. Larger change to `RuleItem`.
   Recommendation: start with (a) for the engine change, evaluate (b) if it proves error-prone.
2. **Which fields to expose.** Likely *all* of `RuleDamageType`'s loadable fields (they are already
   in `load()`), but confirm none are unsafe to change globally (e.g. `ResistType` itself should be
   treated as the key, not a mutable field, to avoid an entry remapping itself).
3. **`ResistType` as key vs. mutation.** Guard against a node setting `ResistType` to a *different*
   index than the slot it is editing (`load()` would happily overwrite it). The loader should set/lock
   `ResistType` from the node key after `load()`, or skip reading it as a field.
4. **Bounds / validation.** Reject `ResistType` outside `0..DAMAGE_TYPES-1` with a soft error
   (consistent with the rest of the loader's `checkForSoftError` style).
5. **Save compatibility.** `_damageTypes` is mod data, not savegame state, so no save format change.
   But changing base damage behavior changes balance for existing saves — note in docs, no migration.
6. **Adding *more* than 20 types.** Explicitly out of scope. `DAMAGE_TYPES` is compiled into array
   sizes (per-armor resistances, etc.); growing it is a much larger, separate change.
7. **Interaction with `damageAlter`.** Per-item `damageAlter` still overlays on top of the (now
   editable) base — confirm precedence reads naturally: base default → global `damageTypes` edit →
   per-item `damageAlter`.

## Touched Code (anticipated)

- [src/Mod/Mod.cpp](../src/Mod/Mod.cpp) — the `//load base damage types` constructor block (defaults
  stay as the starting point) and `Mod::loadFile` (new `damageTypes:` iteration, placed before
  `items`).
- [src/Mod/RuleDamageType.cpp](../src/Mod/RuleDamageType.cpp) /
  [src/Mod/RuleDamageType.h](../src/Mod/RuleDamageType.h) — possibly a small tweak so the loader can
  set `ResistType` from the node key safely; otherwise `load()` is reused as-is.
- Documentation: `Extended.txt` (document the new `damageTypes:` node alongside `damageAlter`),
  `DX-Features.md`, and the checklist entry.

## Documentation Plan

On implementation, update `DX-Features.md` with the new `damageTypes:` node and its keys, refresh
this doc's status line, and tick the checklist item in `DX-Implementation-Checklist.md`.
