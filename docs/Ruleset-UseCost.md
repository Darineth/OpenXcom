# Ruleset chunk: action costs (`RuleItemUseCost` / `RuleItemUseFlat`)

Back to the [ruleset index](Ruleset.md).

**Engine classes:** [`RuleItemUseCost` / `RuleItemUseFlat`](../src/Mod/RuleItem.h) · **Loaders:**
[`RuleItemUseCostRule::loadCost` / `RuleItemUseFlatRule::loadFlat`](../src/Mod/RuleItem.h)

Every battlescape action an item can perform (shoot, melee, throw, prime, use, mind-control…) has a
**cost block** and a matching **flat-rate block**. The cost says *how much of each resource* the
action spends; the flat flag says whether the numbers are **absolute** or a **percentage of the
unit's maximum**.

```yaml
items:
  - type: STR_RIFLE
    tuSnap: 25              # 25% of max TU (percent is the default for TU)
    costAimed:              # full block form: several resources at once
      time: 60
      energy: 10
    flatAimed:
      time: true            # ...and now "60" means 60 TU flat, not 60%
```

## The blocks

Each action has a `cost<Action>:` map and (for most) a `flat<Action>:` counterpart. There is also a
scalar shorthand `tu<Action>:` that writes only the block's `time` field.

| Cost block | Shorthand | Flat block | Action | Fallback when unset |
|---|---|---|---|---|
| `costAimed` | `tuAimed` | `flatAimed` | Aimed shot | — (0 = mode unavailable) |
| `costSnap` | `tuSnap` | `flatSnap` | Snap shot | `costAimed` / `flatAimed` |
| `costAuto` | `tuAuto` | `flatAuto` | Auto shot | `costAimed` / `flatAimed` |
| `costBurst` **[DX]** | `tuBurst` **[DX]** | `flatBurst` **[DX]** | Burst shot | `costAimed` / `flatAimed` |
| `costMelee` | `tuMelee` | `flatMelee` | Melee attack / gun bash | — (0 = unavailable) |
| `costUse` | `tuUse` | `flatUse` | Generic "use" (medikit, scanner, psi-amp, skills) | default `25` (time) |
| `costThrow` | `tuThrow` | `flatThrow` | Throw | default `25` (time) |
| `costPrime` | `tuPrime` | `flatPrime` | Prime a grenade | default `50` (time) |
| `costUnprime` | `tuUnprime` | `flatUnprime` | Unprime a grenade | default `25` (time) |
| `costMindControl` | `tuMindControl` | — | Psi mind control | `costUse` |
| `costPanic` | `tuPanic` | — | Psi panic | `costUse` |
| `costClairvoyance` **[DX]** | `tuClairvoyance` **[DX]** | — | Clairvoyant sweep | `costUse` |
| `costMindBlast` **[DX]** | `tuMindBlast` **[DX]** | — | Mind blast | `costUse` |

Notes on the fallbacks (from `RuleItem::getCost*` / `getFlat*`):

- **Setting a fire mode's TU cost is what enables that mode.** A weapon with no `tuAuto`/`costAuto`
  simply has no auto-shot in the action menu; the same is true of **[DX]** burst (`tuBurst`).
- Snap/auto/**burst** inherit *per resource* from `costAimed` — a weapon that only defines
  `costAimed: {energy: 5}` gives all three modes the same energy cost.
- The psi actions (`costMindControl`/`costPanic`/**[DX]** `costClairvoyance`/`costMindBlast`)
  inherit from `costUse`; `costUse` is forced to zero for a psi-amp that has no `psiAttackName`,
  and `costPrime` is zero when `primeActionName` is empty.
- Flat blocks for the fire modes are three-level: `flatSnap`/`flatAuto`/`flatBurst` →
  `flatAimed` → `flatUse`; `flatAimed` and `flatMelee` are two-level (→ `flatUse`).
- The legacy `flatRate: true` key is shorthand for `flatUse: { time: true }`.

## Sub-resource keys

Both the cost and the flat block are maps over the same six resources
(`RuleItemUseRuleBase`):

| Key | Type | Default | Meaning |
|---|---|---|---|
| `time` | int | 0 | Time Units spent (percent of max TU unless the matching flat flag is set). |
| `energy` | int | 0 | Stamina spent (flat by default — see below). |
| `morale` | int | 0 | Morale spent. |
| `health` | int | 0 | Health spent. |
| `stun` | int | 0 | Stun damage inflicted on the user. |
| `mana` | int | 0 | Mana spent. |

The `flat<Action>:` block takes the **same six keys** with boolean values, e.g.
`flatAimed: { time: true, energy: false }`. Passing a bare scalar (`flatAimed: true`) sets only
`time`.

**Defaults differ per resource:** `time` defaults to *percent* mode (`flat = false`) while
`energy`/`morale`/`health`/`stun`/`mana` default to *flat* mode (`flat = true`). That is the stock
X-COM convention — TU costs are percentages, everything else is an absolute number.

## Where the blocks appear

| Host | Blocks |
|---|---|
| [`items:`](Ruleset-Items.md) | all of the above |
| [`skills:`](Ruleset-Skills.md) | `costUse` / `tuUse` / `flatUse` (a skill overrides the item's use cost) |

## See also

- [`items:`](Ruleset-Items.md#fire-modes--accuracy) — the fire-mode configuration these costs belong to
- [DX-Features.md](../DX-Features.md) — burst fire ([plans/Feature-BurstFire.md](../plans/Feature-BurstFire.md))
- [Ruleset-Armors.md](Ruleset-Armors.md#movement) — the separate *move* cost pairs (`moveCost:`)
