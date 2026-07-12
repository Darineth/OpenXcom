# Ruleset chunk + root: damage types (`RuleDamageType` / `damageAlter:` / **[DX]** `damageTypes:`)

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`RuleDamageType`](../src/Mod/RuleDamageType.h) · **List key:** `ResistType`
(index, for the [DX] root) · **Loaders:** [`RuleDamageType::load`](../src/Mod/RuleDamageType.cpp),
[`Mod::loadEarlyRules`](../src/Mod/Mod.cpp)

A damage type is the **physics of a hit**: how raw power is rolled into damage, how much of it
reaches health / armor / morale / the tile, whether it explodes, and which column of the target's
`damageModifier:` resistance list applies. There are **20 slots** (`DT_NONE` … `DT_19`), each with
engine defaults.

Every [`items:`](Ruleset-Items.md) entry gets a **copy by value** of one of those slots (`damageType:
<index>`) and may then overlay it with a per-item `damageAlter:` map. **[DX]** adds a global
`damageTypes:` root that edits the built-in slots themselves, so a mod can retune AP or bring a
spare slot to life without touching every weapon.

```yaml
damageTypes:               # [DX] global pre-pass — edits the built-in types
  - ResistType: 1          # DT_AP
    ToArmor: 0.2
    ToArmorBlocked: 0.05   # [DX] armor degradation: wear even on a stopped hit
  - ResistType: 10         # DT_10 — a spare slot becomes "EMP"
    RandomType: 1
    ToHealth: 0.0
    ToStun: 1.0
    IgnoreDirection: true

items:
  - type: STR_EMP_GRENADE
    damageType: 10         # use the slot defined above...
    damageAlter:           # ...and tweak it for this item only
      FixRadius: 4
      ToTile: 0.0
```

**Precedence chain:** engine default → **[DX]** global `damageTypes:` edit → per-item
`damageAlter:` / `meleeAlter:`.

## The ResistType index

`ResistType` is both the **slot id** for the [DX] root and the column index into an armor's
`damageModifier:` list (see [`armors:`](Ruleset-Armors.md#protection--damage-model)).

| Index | Name | Built-in role |
|---|---|---|
| 0 | `DT_NONE` | No damage (rolls 0). |
| 1 | `DT_AP` | Armor piercing. |
| 2 | `DT_IN` | Incendiary (fire blast calc, ignores armor & direction). |
| 3 | `DT_HE` | High explosive (explosion roll, damages items). |
| 4 | `DT_LASER` | Laser. |
| 5 | `DT_PLASMA` | Plasma. |
| 6 | `DT_STUN` | Stun (all power to stun, no health/armor/tile damage). |
| 7 | `DT_MELEE` | Melee (no self-destruct trigger). |
| 8 | `DT_ACID` | Acid. |
| 9 | `DT_SMOKE` | Smoke (stun only, ignores armor & direction, creates smoke). |
| 10–19 | `DT_10` … `DT_19` | Spare slots — behave like AP until a mod defines them. |

The number of slots is **fixed at 20**; new types cannot be added, only the spares defined.

## `RandomType` — how power becomes damage

| Value | Name | Roll |
|---|---|---|
| 0 | `DRT_DEFAULT` | Pick by `ResistType`: none→`DRT_NONE`, IN→`DRT_FIRE`, HE→`DRT_EXPLOSION`, smoke→`DRT_NONE`, else `DRT_STANDARD`. |
| 1 | `DRT_UFO` | 0–200% of power. |
| 2 | `DRT_TFTD` | 50–150% of power. |
| 3 | `DRT_FLAT` | Exactly 100% of power. |
| 4 | `DRT_FIRE` | The mod's `FIRE_DAMAGE_RANGE` (fire dice). |
| 5 | `DRT_NONE` | Always 0. |
| 6 | `DRT_UFO_WITH_TWO_DICE` | Two rolls of 0–power, summed. |
| 7 | `DRT_EASY` | 50–200% of power. |
| 8 | `DRT_STANDARD` | The mod's `DAMAGE_RANGE` constant (default 0–200%). |
| 9 | `DRT_EXPLOSION` | The mod's `EXPLOSIVE_DAMAGE_RANGE` constant (default 50–150%). |

## Fields

All of these are valid inside a per-item `damageAlter:` / `meleeAlter:` map **and** inside a
**[DX]** `damageTypes:` entry. Defaults below are the `RuleDamageType()` constructor defaults; the
built-in types override some of them (see the next section).

### Core

| Key | Type | Default | Meaning |
|---|---|---|---|
| `ResistType` | int 0–19 | 0 | Which resistance column of the target's `damageModifier:` applies — and, in the [DX] root, which slot the entry edits. |
| `RandomType` | int 0–9 | 8 `DRT_STANDARD` | The power→damage roll (table above). |
| `FixRadius` | int | 0 | Explosion radius in tiles: `0` = single-target hit, `-1` = compute it from power × `RadiusEffectiveness`, `>0` = that fixed radius. |
| `ArmorEffectiveness` | float | 1.0 | Fraction of the target's armor value that actually subtracts from damage (0 = armor ignored). |
| `RadiusEffectiveness` | float | 0.0 | Tiles of blast radius per point of power, when `FixRadius: -1`. |
| `RadiusReduction` | float | 10.0 | Power lost per tile of distance from the blast centre. |
| `FireBlastCalc` | bool | false | Use the fire blast calculation (radius +1, incendiary spread). |
| `FireThreshold` | int | 1000 | Damage above which the hit sets fire on the tile. |
| `SmokeThreshold` | int | 1000 | Damage above which the hit creates smoke. |
| `IgnoreDirection` | bool | false | Hit uses the same armor value regardless of which side was struck. |
| `IgnoreSelfDestruct` | bool | false | Killing a unit with this type does not trigger its `selfDestructItem`. |
| `IgnorePainImmunity` | bool | false | Can stun even `painImmune` (big) units. |
| `IgnoreNormalMoraleLose` | bool | false | Health damage does not cause the usual morale loss. |
| `IgnoreOverKill` | bool | false | Health cannot be driven below 0 (no gibbing/corpse destruction). |

### Conversion multipliers (`To*`)

Each `To<X>` converts the rolled damage into an effect; the matching `Random<X>` flag makes that
conversion roll `0…damage` first instead of using the damage flat.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `ToHealth` | float | 1.0 | Damage → health loss. |
| `ToMana` | float | 0.0 | Damage → mana loss. |
| `ToStun` | float | 0.25 | Damage → stun level. |
| `ToMorale` | float | 0.0 | Damage → morale loss. |
| `ToEnergy` | float | 0.0 | Damage → stamina loss. |
| `ToTime` | float | 0.0 | Damage → TU loss. |
| `ToWound` | float | 1.0 | Damage → fatal wounds (with `RandomWound`, it is a chance; without, a count). |
| `ToItem` | float | 0.0 | Damage → damage dealt to items lying on the tile. |
| `ToTile` | float | 0.5 | Damage → terrain damage (see `TileDamageMethod`). |
| `ToArmor` | float | 0.1 | **Penetrating** damage → permanent armor loss on the struck side. |
| `ToArmorPre` | float | 0.0 | Armor loss applied *before* armor reduction (unconditional wear). |
| `ToArmorBlocked` **[DX]** | float | 0.0 | Armor loss on a hit that was **fully stopped**, per point of damage past the threshold (0 = off). |
| `ToArmorBlockedThreshold` **[DX]** | float | 0.5 | Fraction (0–1) of the struck side's effective armor the damage must reach before blocked-hit wear starts. |
| `ToArmorOverPen` **[DX]** | float | 0.0 | Extra armor loss on a hit that **smashed clean through**, per point of damage past the threshold (0 = off). |
| `ToArmorOverPenThreshold` **[DX]** | float | 2.0 | Multiple of the struck side's effective armor the damage must exceed before over-penetration wear starts. |

### Random flags

| Key | Type | Default | Meaning |
|---|---|---|---|
| `RandomHealth` | bool | false | Roll `0…damage` before applying `ToHealth`. |
| `RandomMana` | bool | false | Same for `ToMana`. |
| `RandomArmor` | bool | false | Same for `ToArmor`. |
| `RandomArmorPre` | bool | false | Same for `ToArmorPre`. |
| `RandomWound` | bool | **true** | Wounds become a chance roll (1–3 wounds) instead of a linear count. |
| `RandomItem` | bool | false | Same for `ToItem`. |
| `RandomTile` | bool | false | Same for `ToTile`. |
| `RandomStun` | bool | **true** | Roll `0…damage` before applying `ToStun`. |
| `RandomEnergy` | bool | false | Same for `ToEnergy`. |
| `RandomTime` | bool | false | Same for `ToTime`. |
| `RandomMorale` | bool | false | Same for `ToMorale`. |

### Tile damage

| Key | Type | Default | Meaning |
|---|---|---|---|
| `TileDamageMethod` | int 1/2 | 1 | `1` = tile damage is rolled from *power* (50–150% × `ToTile`); `2` = it is derived from the *unit damage roll* (`damage × RandomTile × ToTile`). |
| `TileDamageLimit` | int | −1 | Hard cap on final tile damage (−1 = uncapped). |

## Built-in type defaults

The engine constructs each slot before any ruleset loads
([`Mod::Mod`](../src/Mod/Mod.cpp)). Everything not listed keeps the constructor defaults above.

| Type | Notable engine settings |
|---|---|
| `DT_NONE` (0) | `RandomType: 5` (never damages). |
| `DT_AP` (1) | `IgnoreOverKill: true`. |
| `DT_IN` (2) | `RandomType: 4`, `FixRadius: -1`, `FireBlastCalc`, `IgnoreDirection`, `IgnoreSelfDestruct`, `ArmorEffectiveness: 0`, `RadiusEffectiveness: 0.03`, `FireThreshold: 0`, `ToArmor/ToWound/ToItem/ToTile/ToStun: 0`, `TileDamageMethod: 2`. |
| `DT_HE` (3) | `RandomType: 9`, `FixRadius: -1`, `IgnoreSelfDestruct`, `RadiusEffectiveness: 0.05`, `ToItem: 1.0`, `TileDamageMethod: 2`. |
| `DT_LASER` (4), `DT_PLASMA` (5), `DT_ACID` (8) | `IgnoreOverKill: true` (otherwise plain AP behavior). |
| `DT_STUN` (6) | `FixRadius: -1`, `IgnorePainImmunity`, `IgnoreSelfDestruct`, `RadiusEffectiveness: 0.05`, `ToHealth/ToArmor/ToWound/ToItem/ToTile: 0`, `ToStun: 1.0`, `RandomStun: false`, `TileDamageMethod: 2`. |
| `DT_MELEE` (7) | `IgnoreOverKill`, `IgnoreSelfDestruct`. |
| `DT_SMOKE` (9) | `RandomType: 5`, `FixRadius: -1`, `IgnoreDirection`, `ArmorEffectiveness: 0`, `RadiusEffectiveness: 0.05`, `SmokeThreshold: 0`, `ToHealth/ToArmor/ToWound/ToItem/ToTile: 0`, `ToStun: 1.0`, `TileDamageMethod: 2`. |
| `DT_10` … `DT_19` | Constructor defaults + `IgnoreOverKill: true`. |

## Using it per item

- `damageType: <index>` — copy that slot as the item's damage type.
- `damageAlter: { … }` — overlay any of the fields above on this item's copy.
- `blastRadius: <int>` — shorthand that writes `FixRadius` on the item's damage type.
- `meleeType: <index>` / `meleeAlter: { … }` — the same pair for a firearm's melee (gun-bash)
  attack; it defaults to a copy of `DT_MELEE`.

See [`items:`](Ruleset-Items.md#damage-power--explosions).

## **[DX]** The global `damageTypes:` root

```yaml
damageTypes:
  - ResistType: 3          # required: which built-in slot to edit
    RadiusReduction: 8.0
```

- Entries are a **list**, keyed by `ResistType` (0–19). An out-of-range or missing `ResistType` is a
  soft error (logged, entry skipped).
- `ResistType` is a **lookup key, not a mutable field** — the loader re-locks it, so an entry cannot
  remap a slot to another index.
- The node is swept in an **early pre-pass across every mod's rulesets**, before any `items:` load,
  because `RuleItem` copies the damage type by value. It therefore does not matter which file or
  which mod declares it; later mods win, field by field.
- It is mod data, not save state — retuning changes balance for existing saves but does not break
  them.

Design docs: [plans/Feature-EditableDamageTypes.md](../plans/Feature-EditableDamageTypes.md) ·
[plans/Feature-ArmorDegradation.md](../plans/Feature-ArmorDegradation.md).

## See also

- [`items:`](Ruleset-Items.md) — the host of `damageType:`/`damageAlter:`
- [`armors:`](Ruleset-Armors.md#protection--damage-model) — `damageModifier:` (the per-type resistance list indexed by `ResistType`)
- [Ruleset-Constants.md](Ruleset-Constants.md) — `DAMAGE_RANGE`, `EXPLOSIVE_DAMAGE_RANGE`, `FIRE_DAMAGE_RANGE`
