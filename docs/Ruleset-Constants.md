# Ruleset: `constants:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** static members of [`Mod`](../src/Mod/Mod.h) · **Node type:** singleton ·
**Loader:** [`Mod::loadConstants`](../src/Mod/Mod.cpp) (defaults in `Mod::resetGlobalStatics`)

`constants:` is the engine's grab-bag of **hardcoded numbers a mod may re-point**: which sound in
`BATTLE.CAT` is the door, where the explosion frames start in `X1.PCK`, how wide the damage roll is,
and a family of `extended*` behaviour switches that change engine rules outright.

Every value is a **global static**, not per-rule — the last mod to set it wins, and they are reset to
the stock values on every mod reload.

```yaml
constants:
  damageRange: 100
  explosiveDamageRange: 50
  fireDamageRange: [5, 10]
  trajectoryPreviewSprite: 35     # frame in Projectiles used for the DX aiming tracer
  extendedItemReloadCost: true
```

> **Legacy sequence form.** `constants:` may also be written as a **list of one-key maps** (the stock
> `xcom1` rulesets do this: `constants:` → `- damageRange: 100` → `- explosiveDamageRange: 50`). The
> loader accepts both; the map form above is preferred.

## Sound offsets

All of these are **indices into a sound set** (`BATTLE.CAT` or `GEO.CAT`), loaded through
`Mod::loadSoundOffset`, so they also accept indices added by [`extraSounds:`](Ruleset-ExtraSounds.md)
(mod-relative offsets are resolved for you).

| Key | Type | Default | Meaning |
|---|---|---|---|
| `doorSound` | sound (BATTLE.CAT) | 3 | Hinged door opening. |
| `slidingDoorSound` | sound (BATTLE.CAT) | 20 | Sliding (UFO) door opening. |
| `slidingDoorClose` | sound (BATTLE.CAT) | 21 | Sliding door closing. |
| `smallExplosion` | sound (BATTLE.CAT) | 2 | Small explosion / grenade. |
| `largeExplosion` | sound (BATTLE.CAT) | 5 | Large explosion. |
| `itemDrop` | sound (BATTLE.CAT) | 38 | Item dropped to the floor. |
| `itemThrow` | sound (BATTLE.CAT) | 39 | Item thrown. |
| `itemReload` | sound (BATTLE.CAT) | 17 | Weapon reloaded. |
| `walkOffset` | sound (BATTLE.CAT) | 22 | Base index of the footstep sound bank (per-tile footstep sounds are offsets from here). |
| `flyingSound` | sound (BATTLE.CAT) | 15 | Flying-unit movement loop. |
| `buttonPress` | sound (GEO.CAT) | 0 | UI button click. |
| `windowPopup` | list of 3 sounds (GEO.CAT) | `[1, 2, 3]` | The three window-open sounds (one is picked at random). |
| `ufoFire` | sound (GEO.CAT) | 8 | UFO fires in a dogfight. |
| `ufoHit` | sound (GEO.CAT) | 12 | UFO takes a hit. |
| `ufoCrash` | sound (GEO.CAT) | 10 | UFO crashes. |
| `ufoExplode` | sound (GEO.CAT) | 11 | UFO destroyed. |
| `interceptorHit` | sound (GEO.CAT) | 10 | Player craft takes a hit. |
| `interceptorExplode` | sound (GEO.CAT) | 13 | Player craft destroyed. |
| `selectBaseSound` | sound (BATTLE.CAT) | −1 | *(Top-level, not under `constants:` — see [Ruleset-Globals.md](Ruleset-Globals.md).)* |

## Sprite offsets

Indices into a sprite set, loaded through `Mod::loadSpriteOffset` (so
[`extraSprites:`](Ruleset-ExtraSprites.md) indices work too).

| Key | Type | Default | Meaning |
|---|---|---|---|
| `explosionOffset` | sprite (X1.PCK) | 0 | First frame of the explosion animation set. |
| `smokeOffset` | sprite (SMOKE.PCK) | 8 | First frame of the smoke/fire animation set. |
| `underwaterSmokeOffset` | sprite (SMOKE.PCK) | 0 | Same, for underwater (TFTD) missions. |
| `trajectoryPreviewSprite` **[DX]** | sprite (Projectiles) | 35 | Frame used as the tracer dot in the DX [live trajectory preview](../DX-Features.md#live-trajectory-preview). |

## Music

| Key | Type | Default | Meaning |
|---|---|---|---|
| `goodDebriefingMusic` | music id | `GMMARS` | Track played on a successful debriefing. |
| `badDebriefingMusic` | music id | `GMMARS` | Track played on a failed debriefing. |

## Cursor colors

The mouse-cursor palette index used on each screen family (the cursor has no palette of its own —
it borrows the active screen's).

| Key | Type | Default | Meaning |
|---|---|---|---|
| `geoscapeCursor` | int (palette index) | 252 | Cursor color on the Geoscape. |
| `basescapeCursor` | int | 252 | Cursor color on the Basescape. |
| `battlescapeCursor` | int | 144 | Cursor color on the Battlescape (and battlescape-palette pedia pages). |
| `ufopaediaCursor` | int | 252 | Cursor color in the UFOpaedia. |
| `graphsCursor` | int | 252 | Cursor color on the Graphs screen. |

## Damage randomization

These set the **spread of the damage roll** for the three random types (see
[`RuleDamageType::RandomType`](Ruleset-DamageTypes.md) and
[`RuleDamageType.cpp`](../src/Mod/RuleDamageType.cpp)).

| Key | Type | Default | Meaning |
|---|---|---|---|
| `damageRange` | int % | 100 | `DRT_STANDARD` spread: damage rolls uniformly in `power ± damageRange%` (100 ⇒ the classic 0–200% of power). |
| `explosiveDamageRange` | int % | 50 | `DRT_EXPLOSION` spread: `power ± 50%` by default. |
| `fireDamageRange` | list of 2 ints | `[5, 10]` | `DRT_FIRE` damage is a flat roll between these two absolute values (power is ignored). |

## UFOpaedia rendering

| Key | Type | Default | Meaning |
|---|---|---|---|
| `extendedPediaFacilityParams` | list of 4 ints | `[2, 2, 0, 0]` | Facility-article render box: max width, max height (in base-grid squares), X offset, Y offset. |

## Behaviour switches (`extended*`)

These change engine **rules**, not assets. They exist because the behaviour they enable is not
save-compatible with the stock one — turn them on deliberately, mod-wide.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `extendedItemReloadCost` | bool | false | Reloading costs the **ammo item's** TU cost instead of a flat 15% of the weapon's TUs. |
| `extendedInventorySlotSorting` | bool | false | Sort inventory slots by their ruleset order rather than by legacy hardcoded order. |
| `extendedRunningCost` | bool | false | Apply the armor's `runPercent` move cost to the *energy* term as well (stock only scales time). |
| `extendedMovementCostRounding` | int 0–2 | 0 | Rounding of the per-tile move cost: 0 = truncate, 1 = round to nearest, 2 = round up. |
| `extendedHwpLoadOrder` | bool | false | Load HWPs/tanks into the craft before soldiers (changes deployment order at mission start). |
| `extendedSpotOnHitForSniping` | int | 0 | Whether being hit reveals the shooter to the target's side (for sniper/spotter AI): 0 = off. |
| `extendedBerserkWithAimed` | int | 0 | Allow berserking units to use aimed shots (0 = snap/auto only). |
| `extendedMeleeReactions` | int 0–2 | 0 | Melee reaction fire: 0 = off, 1 = basic, 2 = full (melee units may also react to adjacent movement). |
| `extendedTerrainMelee` | int | 0 | Allow melee attacks against terrain/objects. |
| `extendedUnderwaterThrowFactor` | int % | 0 | Scales throwing range underwater (0 = no change; e.g. 50 = half range at depth). |
| `extendedExperienceAwardSystem` | bool | false | Use the extended (scriptable, per-hit) experience award pipeline. |
| `extendedForceSpawn` | bool | false | Force-spawn units that would otherwise fail placement, next to a friend. |
| `extendedSmokeOffset` | int 0–2 | 0 | Which smoke-frame indexing scheme [`Map`](../src/Battlescape/Map.cpp) uses when drawing smoke/fire. |
| `extendedCurrencySymbol` | string | `$` | The currency symbol shown throughout the UI. |

## See also

- [Ruleset-Globals.md](Ruleset-Globals.md) — the other singleton tuning nodes
- [Ruleset-DamageTypes.md](Ruleset-DamageTypes.md) — what `damageRange` & friends actually feed
- [Extended.txt](../Extended.txt) — the OXCE changelog entry for each `extended*` switch
