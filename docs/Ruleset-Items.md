# Ruleset: `items:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`RuleItem`](../src/Mod/RuleItem.h) · **List key:** `type` · **Loader:**
[`RuleItem::load`](../src/Mod/RuleItem.cpp)

`items:` is the largest root in the ruleset. **Everything a soldier can carry, buy, shoot, throw,
wear or leave behind is an item**: firearms, ammo, melee weapons, grenades, medikits, scanners,
psi-amps, flares, corpses, armor store-items, HWP weapons, and pure economy goods (elerium, alien
alloys). A single `battleType:` decides which half of this reference actually applies to an entry.

```yaml
items:
  - type: STR_RIFLE
    battleType: 1               # BT_FIREARM
    twoHanded: true
    weight: 8
    costBuy: 3000
    costSell: 2000
    bigSprite: 1
    handSprite: 0
    fireSound: 4
    compatibleAmmo: [ STR_RIFLE_CLIP ]
    accuracySnap: 60
    tuSnap: 25                  # a fire mode exists only if its TU cost is set
    accuracyAimed: 110
    tuAimed: 80
    accuracyBurst: 50           # [DX] burst fire
    tuBurst: 45
    burstShots: 3
    listOrder: 2100
```

Entries **merge** across mods/files by `type`, support `refNode:` inheritance and `delete: true`
(see the [index](Ruleset.md#how-rulesets-load-the-60-second-version)).

## Identity & economy

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | — | Unique id; also the default display-name string key. |
| `name` | string | `type` | Display-name string key (lets two items share a name). |
| `nameAsAmmo` | string | — | Name shown instead of the weapon's when this item is loaded as ammo. |
| `ufopediaType` | string | `type` | UFOpaedia article opened for this item. |
| `refNode` | map | — | Inherit all fields from another node before applying this entry's. |
| `categories` | list | — | [Item categories](Ruleset-ItemCategories.md) for base-screen filters. |
| `requires` | list of research | — | [Research](Ruleset-Research.md) needed before the item can be used/equipped. |
| `requiresBuy` | list of research | — | Research needed before it can be purchased. |
| `requiresBuyBaseFunc` | list of base functions | — | Base facility functions required to purchase it. |
| `requiresBuyCountry` | string | — | Allied country required to purchase it. |
| `size` | float | 0.0 | Storage space consumed per unit in base stores. |
| `costBuy` | int | 0 | Purchase price (0 = not purchasable). |
| `costSell` | int | 0 | Sale price. |
| `transferTime` | int hours | 24 | Hours to transfer/deliver. |
| `monthlyBuyLimit` | int | 0 | Max units purchasable per month (0 = unlimited). |
| `monthlyBuyLimitMessage` | string | — | Message shown when the monthly buy limit is hit. |
| `monthlySalary` | int | 0 | Recurring monthly salary cost while the item is owned (hired units). |
| `monthlyMaintenance` | int | 0 | Recurring monthly maintenance cost while the item is owned. |
| `sellActionMessage` | string | — | Confirmation/warning message shown when selling the item. |
| `listOrder` | int | auto | Sort position in purchase/stores/equip lists. |
| `loadOrder` | int | `listOrder` | Priority when auto-picking ammo to load (lower = preferred). |
| `attraction` | int | 0 | How eagerly the AI picks the item up off the floor. |
| `recoveryPoints` | int | 0 | Score awarded for recovering the item after a mission. |

## Battle type, handedness & inventory

| Key | Type | Default | Meaning |
|---|---|---|---|
| `battleType` | int | 0 `BT_NONE` | 0 none, 1 firearm, 2 ammo, 3 melee, 4 grenade, 5 proximity grenade, 6 medikit, 7 scanner, 8 mind probe, 9 psi-amp, 10 flare, 11 corpse. |
| `weight` | int | 3 | Carried weight (counts against strength; also feeds the [DX] weight-based reload cost). |
| `invWidth` / `invHeight` | int | 1 / 1 | Inventory footprint in cells. |
| `twoHanded` | bool | false | Needs two hands (accuracy penalty when the other hand is full). |
| `blockBothHands` | bool | false | Cannot be used at all unless both hands are free. |
| `fixedWeapon` | bool | false | Built-in weapon that cannot be dropped; also marks an HWP weapon (an identically-named [`units:`](Ruleset-Units.md) entry makes it a vehicle). |
| `fixedWeaponShow` | bool | false | Show the fixed weapon in the unit's hand slot. |
| `vehicleFixedAmmoSlot` | int | 0 | Ammo slot of a vehicle's primary weapon (only `0` or `-1`). |
| `defaultInventorySlot` | string | — | [Inventory section](Ruleset-Invs.md) the item auto-equips into. |
| `defaultInvSlotX` / `defaultInvSlotY` | int | 0 / 0 | Position inside that default slot. |
| `supportedInventorySections` | list | all | Restricts which inventory sections may hold the item. |
| `inventoryMoveCost:` → `basePercent` | int % | 100 | Scales the TU cost of moving the item within the inventory. |
| `isConsumable` | bool | false | Medikit/primed item is consumed on use. |
| `isFireExtinguisher` | bool | false | Using the item puts out fire. |
| `isAmmoRechargeable` | bool | false | Ammo refills after the battle (and an empty clip isn't destroyed). |
| `specialUseEmptyHand` | bool | false | Special weapon is triggered from an empty hand. |
| `specialUseEmptyHandShow` | bool | false | Show its icon in the empty hand. |
| `hiddenOnMinimap` | bool | false | Item (e.g. a mine) is not drawn on the minimap. |
| `underwaterOnly` / `landOnly` | bool | false | Restrict use to underwater / land missions. |
| `psiRequired` | bool | false¹ | Wielder needs psiSkill > 0. |
| `manaRequired` | bool | false | Wielder needs mana to operate the item. |
| `stats` **[DX]** | [UnitStats](Ruleset-UnitStats.md) | all 0 | Flat stat bonuses granted while the item sits in an inventory slot. |
| `statModifiers` **[DX]** | [UnitStats](Ruleset-UnitStats.md) | all 0 | Percentage stat modifiers granted while equipped (`10` = +10%). |
| `frontArmor` / `sideArmor` / `rearArmor` / `underArmor` **[DX]** | int | 0 | Per-side armor added to the wearer while equipped (`sideArmor` covers both sides). |

¹ `psiRequired` is forced to `true` when `battleType: 9` (psi-amp) is set, and can then be
overridden explicitly.

## Fire modes & accuracy

A weapon offers a fire mode **only if that mode's TU cost is set** (`tuSnap`/`costSnap`, …). See
[Ruleset-UseCost.md](Ruleset-UseCost.md) for the full cost/flat syntax and its fallback chain.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `accuracyAimed` / `accuracySnap` / `accuracyAuto` | int % | 0 | Base accuracy of each fire mode. |
| `accuracyBurst` **[DX]** | int % | 0 | Base accuracy of the burst mode. |
| `accuracyMelee` | int % | 0 | Base accuracy of the melee/gun-bash attack. |
| `accuracyUse` / `accuracyMindControl` / `accuracyPanic` | int % | 0 / 0 / 20 | Base success chance of use / mind-control / panic actions. |
| `accuracyThrow` | int % | 100 | Base throwing accuracy. |
| `accuracyCloseQuarters` | int % | −1 → global | Melee-defence (CQB) accuracy; −1 uses the mod's global value. |
| `aimRange` / `snapRange` / `autoRange` | int tiles | 200 / 15 / 7 | Distance band inside which the mode suffers no accuracy dropoff. |
| `burstRange` **[DX]** | int tiles | 10 | Same, for burst (sits between snap and auto). |
| `maxRange` | int tiles | 200 | Absolute maximum firing distance. |
| `minRange` | int tiles | 0 | Minimum range; shots at closer targets suffer dropoff. |
| `dropoff` | int | 2 | Accuracy % lost per tile outside the mode's range band. |
| `autoShots` | int | 3 | Rounds fired per auto-shot. |
| `burstShots` **[DX]** | int | 2 | Rounds fired per burst. |
| `noLOSAccuracyPenalty` | int % | −1 → global | Accuracy multiplier when firing at a target outside line of sight. |
| `kneelBonus` | int % | −1 → global | Accuracy bonus while kneeling. |
| `oneHandedPenalty` | int % | −1 → global | Accuracy penalty when a two-handed weapon is fired one-handed. |
| `arcingShot` | bool | false | Projectile flies in an arc (lobbed) rather than straight. |
| `baseAccuracy` **[DX]** | int | 0 | Intrinsic weapon precision; `> 0` opts the weapon into the DX aim-cone model (0 = classic scatter). |
| `fireInterval` **[DX]** | int ms | 150 | Delay between consecutive rounds of a multi-shot volley (cosmetic cadence). |
| `bulletSpeed` | int | 0 | Projectile travel speed offset (0 = engine default). |
| `shotgunPellets` | int | 0 | Number of projectiles traced per shot (0 = not a shotgun). |
| `shotgunBehavior` | int | 0 | 0 = original (pellets share one trajectory check), 1 = OXCE (each pellet traced). |
| `shotgunSpread` | int | 100 | Pellet cone spread. |
| `shotgunChoke` | int | 100 | Tightens/loosens the pellet pattern (ignored under the [DX] aim-cone model). |
| `sprayWaypoints` | int | 0 | Waypoints used for a "spray" attack (auto-fire that walks along a path). |
| `waypoints` | int | 0 | Blaster-launcher waypoint count (`> 0` makes the weapon guided). |
| `accuracyMultiplier` | [stat bonus](Ruleset-StatBonus.md) | `firing` | Formula scaling accuracy by the shooter's stats. |
| `closeQuartersMultiplier` | [stat bonus](Ruleset-StatBonus.md) | closeQuarters | Formula for the CQB accuracy contest. |
| `conf<Mode>:` | map | — | Per-mode action block, see below (`confAimed`, `confSnap`, `confAuto`, `confMelee`, `confBurst` **[DX]**). |

### The `conf<Mode>:` action block

| Sub-key | Type | Default | Meaning |
|---|---|---|---|
| `name` | string | `STR_AIMED_SHOT` / `STR_SNAP_SHOT` / `STR_AUTO_SHOT` / `STR_BURST_SHOT` **[DX]** | Action-menu label for this mode. |
| `shortName` | string | — | Compact label (used by the DX action-menu/ammo readouts). |
| `shots` | int | 1 (auto 3, burst 2) | Projectiles fired by one action. |
| `spendPerShot` | int | 1 | Ammo rounds consumed per projectile. |
| `followProjectiles` | bool | true | Camera follows the projectiles of this mode. |
| `ammoSlot` | int | 0 (melee: `-1` unless `battleType: 3`) | Which ammo slot this mode draws from; `-1` = the weapon itself is the ammo. |
| `arcing` | bool | false | Force an arcing trajectory for this mode only. |
| `ammoZombieUnitChanceOverride` / `ammoSpawnUnitChanceOverride` / `ammoSpawnItemChanceOverride` | int | — | Per-mode overrides of the ammo's spawn/zombify chances. |

### **[DX]** Overwatch

Set-and-hold reaction fire over a directional cone; **opt-in** — a weapon offers it only when
`overwatchRange > 0`. Any field left unset falls back to the mod-wide `overwatchDefaults:` node
(see [DX globals](Ruleset-DX-Globals.md)).

| Key | Type | Default | Meaning |
|---|---|---|---|
| `overwatchRange` | int tiles | from `overwatchDefaults` (0) | Maximum watched distance; `0` = the weapon cannot overwatch. |
| `overwatchMinRange` | int tiles | from `overwatchDefaults` (0) | Near dead zone that is not watched. |
| `overwatchConeAngle` | int degrees | from `overwatchDefaults` (40) | Full width of the watched wedge. |
| `overwatchModifier` | int % | from `overwatchDefaults` (100) | Scales the offensive reaction score of an overwatch shot. |
| `overwatchShot` | `snap`/`burst`/`auto`/`aimed` | from `overwatchDefaults` (`snap`) | Fire mode used when the overwatch triggers. |

## Ammo & reloading

| Key | Type | Default | Meaning |
|---|---|---|---|
| `compatibleAmmo` | list of items | — | Ammo items accepted in slot 0 (a weapon with `clipSize: 0` and no ammo is a load error). |
| `ammo:` → `0`…`3` | maps | — | Per-slot ammo config; each takes `compatibleAmmo`, `tuLoad`, `tuUnload`. |
| `clipSize` | int | 0 | Rounds in this ammo item (on a weapon: built-in rounds; `-1` = unlimited). |
| `battleClipSize` **[DX]** | int | 0 | Per-round economy: ammo is stocked/bought/recovered one round at a time and packed into magazines of this size at battle start (mutually exclusive with `clipSize`, which is forced to 0). |
| `tuLoad` | int | −1 → 15 (5 with the [DX] weight-based model) | TU to load a clip into slot 0. |
| `tuUnload` | int | −1 → 8 (5 with the [DX] weight-based model) | TU to unload slot 0. |
| `ignoreAmmoPower` | bool | false | Firearm uses its **own** power/damage attributes, ignoring the loaded ammo's. |
| `isAmmoRechargeable` | bool | false | See above — clip refills after the battle. |

## Damage, power & explosions

| Key | Type | Default | Meaning |
|---|---|---|---|
| `power` | int | 0 | Base damage power. |
| `damageType` | int 0–19 | 0 | Which [damage type](Ruleset-DamageTypes.md) slot the item copies. |
| `damageAlter:` | map | — | Per-item overlay of any [`RuleDamageType`](Ruleset-DamageTypes.md#fields) field. |
| `blastRadius` | int | from the damage type | Shorthand for the damage type's `FixRadius` (explosion radius; `-1` = derive from power). |
| `damageBonus` | [stat bonus](Ruleset-StatBonus.md) | none | Formula adding power from the attacker's stats. |
| `strengthApplied` | bool | false | Legacy shorthand for `damageBonus: { strength: 1.0 }`. |
| `powerRangeReduction` | float | 0 | Power lost per tile of flight beyond `powerRangeThreshold`. |
| `powerRangeThreshold` | float | 0 | Distance (tiles) at which the power falloff begins. |
| `blastDropoff` **[DX]** | float 0–1 | 0.0 | Taper of explosion power across the blast radius (0 = flat/vanilla, 1 = full linear dropoff). |
| `powerForAnimation` | int | 0 | Power used to pick the explosion animation size (when the real power is scripted/hidden). |
| `hidePower` | bool | false | Don't print the power in the UFOpaedia. |
| `explosionSpeed` | int | 0 | Explosion animation speed offset. |
| `armor` | int | 20 | The **item's own** durability against explosions on the floor (not armor granted to the wearer). |
| `hitAnimation` / `hitMissAnimation` | sprite | 0 / −1 | Impact animation (SMOKE.PCK, or X1.PCK for explosives). |
| `vaporColor` / `vaporDensity` / `vaporProbability` | int | −1 / 0 / 15 | Vapor trail color, density and per-frame chance (land). |
| `vaporColorSurface` / `vaporDensitySurface` / `vaporProbabilitySurface` | int | −1 / 0 / 15 | Same, for surface (water) missions. |

## Melee

| Key | Type | Default | Meaning |
|---|---|---|---|
| `meleeType` | int 0–19 | 7 `DT_MELEE` | Damage type of the melee/gun-bash attack. |
| `meleeAlter:` | map | — | Per-item overlay on that melee damage type. |
| `meleePower` | int | 0 | Power of the melee attack made with a *non-melee* item (gun bash). |
| `meleeBonus` | [stat bonus](Ruleset-StatBonus.md) | none | Formula adding power to the melee attack from stats. |
| `meleeMultiplier` | [stat bonus](Ruleset-StatBonus.md) | `melee` | Formula scaling melee hit chance by stats. |
| `skillApplied` | bool | true | Legacy toggle: `false` replaces `meleeMultiplier` with a flat 100 (melee skill ignored). |
| `meleeSound` / `meleeHitSound` / `meleeMissSound` | sound(s) | — | Swing, connect and miss sounds. |
| `meleeAnimation` / `meleeMissAnimation` | sprite | 0 / −1 | Hit/miss animation (HIT.PCK). |
| `meleeAnimFrames` / `meleeMissAnimFrames` | int | −1 | Frame count (−1 = auto-detect). |

Melee cost/accuracy live in the fire-mode fields above (`accuracyMelee`, `costMelee`/`tuMelee`,
`confMelee`).

## Grenades & fuses

| Key | Type | Default | Meaning |
|---|---|---|---|
| `fuseType` | int | auto | `-3` none, `-2` instant, `-1` player-set timer, `0`–`64` fixed countdown. Defaults from `battleType` (grenade = set, proximity = instant). |
| `fuseTriggerEvents:` | map | — | When a primed item detonates: sub-keys `defaultBehavior` (true), `throwTrigger`, `throwExplode`, `proximityTrigger`, `proximityExplode` (all false). |
| `explodeInventory` | int | −1 → global | Primed explosive detonating in the inventory: 0 no, 1 yes except in hands, 2 always. |
| `primeActionName` / `unprimeActionName` | string | `STR_PRIME_GRENADE` / — | Action-menu labels (empty `primeActionName` disables priming). |
| `primeActionMessage` / `unprimeActionMessage` | string | `STR_GRENADE_IS_ACTIVATED` / `STR_GRENADE_IS_DEACTIVATED` | Confirmation messages. |
| `primeSound` / `unprimeSound` | sound(s) | — | Prime/unprime sounds. |
| `specialChance` | int % | 100 | Chance a proximity mine actually triggers (also the default for the spawn/zombify chances). |
| `hiddenOnMinimap` | bool | false | Hide the (mine) item on the minimap. |

Prime/unprime costs are `costPrime`/`tuPrime`/`flatPrime` and `costUnprime`/… — see
[Ruleset-UseCost.md](Ruleset-UseCost.md).

## Medikit

Applies to `battleType: 6`. The three charge pools are consumed independently.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `heal` | int | 0 | Number of heal charges. |
| `painKiller` | int | 0 | Number of painkiller charges. |
| `stimulant` | int | 0 | Number of stimulant charges. |
| `medikitType` | int | 0 | 0 normal (3 buttons), 1 heal-only, 2 stimulant-only, 3 painkiller-only. |
| `woundRecovery` | int | 0 | Fatal wounds healed per heal charge. |
| `healthRecovery` | int | 0 | Health restored per heal charge. |
| `stunRecovery` | int | 0 | Stun removed per stimulant charge. |
| `energyRecovery` | int | 0 | Energy restored per stimulant charge. |
| `manaRecovery` | int | 0 | Mana restored per charge. |
| `moraleRecovery` | int | 0 | Morale restored per painkiller charge. |
| `painKillerRecovery` | float | 1.0 | Extra morale restored, scaled by the target's missing health. |
| `medikitTargetSelf` | bool | false | May be used on the wielder. |
| `medikitTargetImmune` | bool | false | May target units otherwise immune to medikit use. |
| `medikitTargetMatrix` | int bitmask | 63 | Allowed targets: 1 friend on ground, 2 friend standing, 4 neutral ground, 8 neutral standing, 16 hostile ground, 32 hostile standing. |
| `medikitBackground` | string | — | Custom background image for the medikit screen. |
| `healActionName` / `stimulantActionName` / `painKillerActionName` | string | `STR_HEAL` / `STR_STIMULANT` / `STR_PAIN_KILLER` | Button labels in the medikit screen. |
| `medikitActionName` | string | `STR_USE_MEDI_KIT` | Action-menu label. |
| `isConsumable` | bool | false | The medikit is destroyed when its charges run out. |

## Psi & mind control

Applies to `battleType: 9` (psi-amp) and `8` (mind probe).

| Key | Type | Default | Meaning |
|---|---|---|---|
| `accuracyMindControl` / `accuracyPanic` / `accuracyUse` | int % | 0 / 20 / 0 | Base success chances (`accuracyUse` drives a custom `psiAttackName` action). The `BA_MINDBLAST` equivalent is **[DX]** `mindBlast: accuracy:` below. |
| `psiAttackName` | string | — | Action-menu label for a custom psi attack (its absence zeroes `costUse`). |
| `targetMatrix` | int bitmask | 7 (psi-amp: 6) | Allowed targets: 1 same faction, 2 hostile relation, 4 neutral relation. |
| `convertToCivilian` | bool | false | Mind control converts the victim to the neutral faction instead of the player's. |
| `LOSRequired` | bool | false | Psi attack requires line of sight. |
| `psiRequired` | bool | auto | Wielder needs psiSkill > 0 (auto-true for psi-amps). |
| `manaRequired` | bool | false | Wielder needs mana. |
| `manaExperience` | int | 0 | Mana experience granted per use. |
| `psiSound` / `psiMissSound` | sound(s) | — | Hit/miss sounds. |
| `psiAnimation` / `psiMissAnimation` | sprite | −1 | Hit/miss animation (HIT.PCK). |
| `psiAnimFrames` / `psiMissAnimFrames` | int | −1 | Frame counts (−1 = auto-detect). |

Psi costs are `costMindControl`/`costPanic`/`costUse`/`costClairvoyance`/`costMindBlast` **[DX]** (see
[Ruleset-UseCost.md](Ruleset-UseCost.md)); a psi-amp's `aimRange` defaults to 0 and its `dropoff` to 1,
and its accuracy formula defaults to the psi-attack one.

### **[DX]** `mindControl:` — channeled mind control

Without this node, mind control is stock: the victim is yours for the rest of your turn and reverts to
its own side at the start of its next one, free of charge. `channeled: true` makes it a link the
controller must **hold** — it persists until he can't pay the upkeep, drops the amp, dies, is knocked
out, or panics. *(feature: [DX-Features.md](../DX-Features.md); design:
[plans/Feature-ChanneledMindControl.md](../plans/Feature-ChanneledMindControl.md))*

| Key | Type | Default | Meaning |
|---|---|---|---|
| `channeled` | bool | false | Opt in. False/absent = stock one-turn mind control, unchanged. |
| `maxThralls` | int | 1 | How many units one controller may hold at once (upkeep stacks per thrall). |
| `requiresWeapon` | bool | true | The link breaks if the controller isn't holding the amp at his turn start. |
| `thrallRecoversTimeUnits` | bool | true | Whether the victim gets the full TU bar stock always grants it on capture. |
| `backlashDamageType` | int ResistType | −1 | The type the backlash power is dealt as (−1 = the amp's own). It decides **both** what the recoil does (the type's `To*` fields split the power into health/stun/morale/wounds/energy/TU) and what can resist it — point it at a type nothing resists for an unblockable backlash. |
| `upkeep:` | map | all 0 | What the controller pays each turn, per thrall — see below. |
| `backlashOnThrallDeath:` / `backlashOnFailure:` | map | 0 | `power: [min,max]` — the recoil dealt to the controller when a thrall dies / an attempt fails. A plain power, split by `backlashDamageType` like any other damage source. |
| `resist: perTurn` | bool | false | The thrall re-rolls the psi contest each turn and may break free. |
| `resist: modifier` | int | 0 | Added to the thrall's defence on that re-roll. |

`upkeep:` has **two independent, stackable kinds**, so a mod picks its own flavour:

| Key | Type | Default | Meaning |
|---|---|---|---|
| `time` / `energy` / `mana` / `morale` / `health` / `stun` | int | 0 | **Flat** cost deducted each turn, per thrall. Can't pay ⇒ the link breaks. |
| `timeRecoveryPercent` / `energyRecoveryPercent` / `manaRecoveryPercent` / `moraleRecoveryPercent` | int % | 0 | Percent of the controller's normal per-turn **regeneration** withheld while channeling (100 = no regen at all). Stacks per thrall, capped at 100. |

The legacy DX fork's entire upkeep model is one line of this: `timeRecoveryPercent: 90`.

### **[DX]** `psiAmmo:` — per-cast round cost

Makes psi actions draw rounds from the amp's loaded clip (ammo slot 0, via the stock `compatibleAmmo`
path). Every field defaults to 0 (free), so without the node psi is free exactly as in stock OXCE. Spent
on the attempt (hit or miss); a dry or unloaded amp refuses the action. Covers panic, mind control, the
`BA_USE` attack, clairvoyance and mind blast through one path.
*(design: [plans/Feature-PsiAmpAmmo.md](../plans/Feature-PsiAmpAmmo.md))*

| Key | Type | Default | Meaning |
|---|---|---|---|
| `mindControl` | int | 0 | Rounds a mind control draws from the clip. |
| `panic` | int | 0 | Rounds a panic draws. |
| `use` | int | 0 | Rounds the `BA_USE` psi-damage attack draws. |
| `clairvoyance` | int | 0 | Rounds a clairvoyant sweep draws. |
| `mindBlast` | int | 0 | Rounds a mind blast draws. |

The node refers to its actions **by key**, so a cost set for an action the amp doesn't offer would
otherwise load clean and silently never be spent. That is checked at load: a warning is logged if the
node appears on a non-psi-amp, or if it sets rounds for `clairvoyance`/`mindBlast` while that node isn't
`enabled`, or for `mindControl`/`panic`/`use` while the matching `cost*`/`tu*` is 0. Note a **mistyped
key** cannot be caught this way — it is simply not read, so the cost silently stays 0.

### **[DX]** `mindBlast:` — direct psychic damage

A `BA_MINDBLAST` attack on a target unit, resolved through the same psi contest as panic/mind control
(so it inherits their accuracy, `psiDefence`, distance falloff and script hooks). On a win it deals
damage scaled by the contest **margin**; on a miss it recoils on the caster. Costs `tuMindBlast` /
`costMindBlast` (falls back to `costUse`). *(design: [plans/Feature-MindBlast.md](../plans/Feature-MindBlast.md))*

| Key | Type | Default | Meaning |
|---|---|---|---|
| `enabled` | bool | false | Opt in. Without it the action is never offered. |
| `damageType` | int ResistType | −1 | The type the blast deals (−1 = the amp's own). Point it at a type nothing resists for an armor-ignoring blast. |
| `accuracy` | int % | 0 | **[DX]** Flat psi accuracy for the blast — the `BA_MINDBLAST` counterpart of `accuracyMindControl` / `accuracyPanic`, added to the amp's `accuracyMultiplier` psi term. Before this key existed the blast had no flat accuracy at all, so **set it**: leaving it at 0 makes a blast behave like an `accuracyMindControl: 0` amp. |
| `basePower` | int | 0 | Flat damage on any successful blast. |
| `powerPerMargin` | float | 0.0 | Added damage per point of psi-contest margin (0 = flat; a decisive win hits harder). |
| `randomRange` | int % | 0 | Damage rolls in `[(100−r)%, (100+r)%]` of the computed power (0 = exact). |
| `backlashOnFailure:` | map | 0 | `power: [min,max]` — the recoil dealt to the caster on a miss, split by `backlashDamageType`. |
| `backlashDamageType` | int ResistType | −1 | The type the backlash power is dealt as (−1 = the amp's own); its `To*` fields decide what the recoil costs the caster. |

### **[DX]** `clairvoyance:` — psychic area reveal

A `BA_CLAIRVOYANCE` action that sweeps an area around a **target tile** (no target unit). Terrain there
becomes *discovered but not visible*, so fog of war draws it remembered-and-dimmed and it stays known;
units in it are marked like motion-detector contacts (through walls, cleared at end of turn) rather than
spotted. Costs `tuClairvoyance` / `costClairvoyance` (falls back to `costUse`).
*(design: [plans/Feature-Clairvoyance.md](../plans/Feature-Clairvoyance.md))*

| Key | Type | Default | Meaning |
|---|---|---|---|
| `enabled` | bool | false | Opt in. Without it the action is never offered. |
| `radius` | int tiles | 6 | Tiles revealed around the target, at full power. |
| `levels` | int | 1 | Z levels swept above **and** below the target (0 = only its own). |
| `revealUnits` | bool | true | Also mark units in the area (motion-detector style). |
| `minPsiScore` | int | 0 | `psiStrength + psiSkill` below this can't use it at all (0 = no gate). |
| `scaleWithPsi` | int | 0 | Psi score at which the **full** radius is reached; below it the radius scales down linearly. 0 = everyone gets full radius. |

## Throwing

| Key | Type | Default | Meaning |
|---|---|---|---|
| `accuracyThrow` | int % | 100 | Base throwing accuracy. |
| `throwMultiplier` | [stat bonus](Ruleset-StatBonus.md) | `throwing` | Formula scaling throw accuracy by stats. |
| `throwRange` | int tiles | 200 | Maximum throw distance on land. |
| `underwaterThrowRange` | int tiles | 200 | Maximum throw distance underwater. |
| `throwDropoffRange` | int tiles | 99 | Distance beyond which throw accuracy starts dropping (land). |
| `underwaterThrowDropoffRange` | int tiles | 99 | Same, underwater. |
| `throwDropoff` | int % | 5 | Accuracy lost per tile past the dropoff range. |

Throw cost is `costThrow`/`tuThrow`/`flatThrow`.

## Sprites, sounds & animations

| Key | Type | Default | Meaning |
|---|---|---|---|
| `bigSprite` | sprite (BIGOBS.PCK) | −1 | Inventory image. |
| `floorSprite` | sprite (FLOOROB.PCK) | −1 | Image when lying on the ground. |
| `handSprite` | sprite (HANDOB.PCK) | 120 | Image held in a unit's hand (8 frames per item). |
| `bulletSprite` | sprite (Projectiles) | −1 | Projectile graphic (index × 35). |
| `specialIconSprite` | sprite (SPICONS.DAT) | −1 | Icon for the special-weapon button. |
| `customItemPreviewIndex` | int / list | — | CustomItemPreviews sprite(s) shown in the UFOpaedia. |
| `fireSound` | sound(s) | — | Firing sound (a list = a random pick). |
| `hitSound` / `hitMissSound` | sound(s) | — | Impact / miss sounds. |
| `explosionHitSound` | sound(s) | — | Sound of the explosion this item causes. |
| `reloadSound` | sound(s) | — | Reload sound. |
| `hitAnimation` / `hitMissAnimation` | sprite | 0 / −1 | Impact animation start frame (SMOKE.PCK, or X1.PCK for explosives). |
| `hitAnimFrames` / `hitMissAnimFrames` | int | −1 | Frame count (−1 = auto-detect). |
| `glowConeAngle` **[DX]** | int degrees | 0 | Full cone angle of the light this item casts while carried; `0` = the usual circular glow. |

`power` doubles as the light radius for `battleType: 10` (flare); **[DX]** `glowConeAngle` turns
that glow into a directional cone aimed the way the unit faces (see
[plans/Feature-LightEquipment.md](../plans/Feature-LightEquipment.md)).

## AI hints

| Key | Type | Default | Meaning |
|---|---|---|---|
| `ai:` → `useDelay` | int turns | −1 → global | First turn on which the AI may use the item (−1 = the mod's per-battle-type default). |
| `ai:` → `meleeHitCount` | int | 25 | How many melee swings the AI is willing to plan with this weapon. |
| `attraction` | int | 0 | How attractive the item is for the AI to pick up. |
| `experienceTrainingMode` | int | 0 `ETM_DEFAULT` | Which stat(s) using this item trains (see `ExperienceTrainingMode` in [RuleItem.h](../src/Mod/RuleItem.h)). |
| `targetMatrix` | int bitmask | 7 | Which factions the item may be used against (see Psi above). |

## Recovery, spawning & special

| Key | Type | Default | Meaning |
|---|---|---|---|
| `recover` | bool | true | Item is recovered from the battlefield after a mission. |
| `recoverCorpse` | bool | true | Corpse item is recovered (for `battleType: 11`). |
| `recoveryPoints` | int | 0 | Score for recovering it. |
| `recoveryDividers` | map item→int | — | Divides the recovered quantity per mission/special type. |
| `recoveryTransformations` | map item→list | — | Recover *other* item(s) instead of this one (list = quantity range). |
| `liveAlien` | bool | false | The item is a live alien (goes to a prison). |
| `prisonType` | int | 0 | Which prison type holds this live alien. |
| `ignoreInBaseDefense` | bool | false | Item is not auto-equipped during a base defense. |
| `ignoreInCraftEquip` | bool | auto | Hide from the craft-equipment screen (auto: true for non-battlescape items). |
| `specialType` | int | −1 | Special recovery type tying the item to a terrain object (e.g. UFO power source). |
| `turretType` | int | −1 | Turret sprite index for an HWP weapon. |
| `specialChance` | int % | 100 | Default chance for the spawn/zombify effects below (and for mine triggering). |
| `zombieUnit` | unit | — | Unit the victim turns into when killed by this item. |
| `zombieUnitByType` / `zombieUnitByArmorMale` / `zombieUnitByArmorFemale` | maps | — | Per-victim-type / per-armor overrides of `zombieUnit`. |
| `zombieUnitChance` | int % | `specialChance` | Chance of the zombify effect. |
| `zombieUnitFaction` | int | 1 hostile | Faction of the spawned zombie. |
| `spawnUnit` | unit | — | Unit spawned at the impact point. |
| `spawnUnitChance` | int % | `specialChance` | Chance of the unit spawn. |
| `spawnUnitFaction` | int | −1 → attacker's | Faction of the spawned unit. |
| `spawnItem` | item | — | Item spawned at the impact point. |
| `spawnItemChance` | int % | `specialChance` | Chance of the item spawn. |

## Scripting

Items expose Y-Script hooks (the `scripts:` sub-node — `recolorItemSprite`, `hitUnit`, `damageUnit`,
`createItem`, `newTurnItem`, …) and custom `tags:`. The six stat-bonus formulas above
(`damageBonus`, `meleeBonus`, `accuracyMultiplier`, `meleeMultiplier`, `throwMultiplier`,
`closeQuartersMultiplier`) may also be given as script names. See
[Ruleset-Scripting.md](Ruleset-Scripting.md).

## See also

- [Action costs](Ruleset-UseCost.md) — `costAimed`/`tuSnap`/`flatUse`/… in full
- [Damage types](Ruleset-DamageTypes.md) — `damageType:` / `damageAlter:` and the **[DX]** global root
- [Stat bonus formulas](Ruleset-StatBonus.md) — `damageBonus`, `accuracyMultiplier`, …
- [Unit stats block](Ruleset-UnitStats.md) — **[DX]** item `stats:` / `statModifiers:`
- [`armors:`](Ruleset-Armors.md) · [`invs:`](Ruleset-Invs.md) · [`itemCategories:`](Ruleset-ItemCategories.md) · [`weaponSets:`](Ruleset-WeaponSets.md)
- [DX-Features.md](../DX-Features.md) — every **[DX]** field above: burst fire, overwatch, aim cone,
  `battleClipSize`, item stats/armor, blast dropoff, light equipment
