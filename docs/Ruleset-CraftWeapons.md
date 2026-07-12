# Ruleset: `craftWeapons:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`RuleCraftWeapon`](../src/Mod/RuleCraftWeapon.h) · **List key:** `type` ·
**Loader:** [`RuleCraftWeapon::load`](../src/Mod/RuleCraftWeapon.cpp)

A craft weapon is the thing that fills a hardpoint on a [`crafts:`](Ruleset-Crafts.md) entry. It is
really three rules glued together: the **dogfight behavior** (damage, range, accuracy, reload,
projectile), a **launcher item** and an optional **clip item** (both plain
[`items:`](Ruleset-Items.md) that live in base stores and are bought/[manufactured](Ruleset-Manufacture.md)),
and an optional **bonus craft-stats block** that upgrades the carrying craft. A "weapon" with no
damage and only `stats:` is how OXCE mods build craft *equipment modules* (extra fuel, cabin space,
shields).

```yaml
craftWeapons:
  - type: STR_STINGRAY
    sprite: 0
    sound: 5
    damage: 70
    range: 30
    accuracy: 70
    reloadCautious: 32
    reloadStandard: 24
    reloadAggressive: 16
    ammoMax: 6
    rearmRate: 1
    projectileType: 0            # 0 = Stingray missile
    projectileSpeed: 8
    launcher: STR_STINGRAY_LAUNCHER
    clip: STR_STINGRAY_MISSILE
```

Entries **merge** across mods/files by `type`, support `refNode:` inheritance and `delete: true`
(see the [index](Ruleset.md#how-rulesets-load-the-60-second-version)).

## Identity & items

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | — | Unique id; also the weapon's display-name string key. |
| `refNode` | map | — | Inherit all fields from another (anonymous) node first. |
| `ufopediaType` | string | `type` | UFOpaedia article opened for this weapon. |
| `tooltip` | string | — | String key of the tooltip shown for this weapon in the craft-equip UI. |
| `launcher` | string item | — | The [item](Ruleset-Items.md) that *is* the weapon in stores — **required**; installing/removing the weapon moves this item. |
| `clip` | string item | — | The ammo [item](Ruleset-Items.md) consumed while rearming; absent = the weapon needs no ammo item. |
| `weaponType` | int | 0 | Hardpoint type tag, matched against the craft's `weaponTypes:` slot filter ([crafts](Ruleset-Crafts.md#weapons)). |
| `underwaterOnly` | bool | false | Weapon can only be equipped/fired on water-capable craft (TFTD-style). |
| `hidePediaInfo` | bool | false | Hide the stats table in the UFOpaedia article — this is what turns an entry into a "tractor beam" (if `tractorBeamPower > 0`) or plain "equipment" article. |

## Dogfight stats

| Key | Type | Default | Meaning |
|---|---|---|---|
| `damage` | int | 0 | Damage per hit on the target craft/UFO. |
| `unifiedDamageFormula` | bool | false | Use the `clip`/`launcher` item's damage type + damage-alter rules to roll damage instead of the vanilla flat formula (requires `clip` or `launcher`). |
| `shieldDamageModifier` | int % | 100 | Effectiveness against UFO shields (100 = normal, 0 = shields immune to it). |
| `range` | int km | 0 | Maximum firing distance in the dogfight window. |
| `accuracy` | int % | 0 | Chance each shot hits. |
| `reloadCautious` | int | 0 | Seconds between shots in cautious dogfight stance. |
| `reloadStandard` | int | 0 | Seconds between shots in standard stance. |
| `reloadAggressive` | int | 0 | Seconds between shots in aggressive stance. |
| `ammoMax` | int | 0 | Rounds carried (0 = unlimited / no ammo). |
| `rearmRate` | int | 1 | Rounds restored per hour while rearming at base; must be ≥ the clip item's `clipSize` and positive whenever `ammoMax` is set. |
| `bulletSaving` | bool | false | Use statistical bullet saving — the clip item is only consumed proportionally rather than one whole clip at a time. |
| `tractorBeamPower` | int | 0 | > 0 makes this a tractor beam that slows the target UFO instead of damaging it. |

## Projectile & presentation

| Key | Type | Default | Meaning |
|---|---|---|---|
| `projectileType` | int | 2 | Blob/visual type: 0 Stingray missile, 1 Avalanche missile, 2 cannon round, 3 fusion ball, 4 laser beam, 5 plasma beam (`CWPT_*`; 0–3 are missiles, 4–5 are beams). |
| `projectileSpeed` | int | 0 | Missile travel speed — **required (> 0) for any missile-type weapon with `damage > 0`**; very low values relative to `range` log a warning. |
| `sprite` | int | −1 | Sprite index in `BASEBITS.PCK` (+48) / `INTICON.PCK` (+5); indices above 5 get the mod's sprite offset. |
| `sound` | sound id (GEO.CAT) | −1 | Firing sound in the dogfight. |

## Bonus craft stats

| Key | Type | Default | Meaning |
|---|---|---|---|
| `stats` | craft-stats map | all 0 | A [craft-stats block](Ruleset-Crafts.md#the-craft-stats-block) **added** to the carrying craft while this weapon is installed. |

Every key of the craft-stats block is legal here (`fuelMax`, `damageMax`, `speedMax`, `accel`,
`radarRange`, `radarChance`, `sightRange`, `hitBonus`, `avoidBonus`, `avoidBonus2`, `powerBonus`,
`armor`, `shieldCapacity`, `shieldRecharge`, `shieldRechargeInGeoscape`, `shieldBleedThrough`,
`soldiers`, `vehicles`, `maxItems`, `maxStorageSpace`). A module with `damage: 0` and only `stats:`
is a pure upgrade pod; note the craft's `maxUnitsLimit`/`maxHWPUnitsLimit` still cap any capacity a
module grants.

## Load-time validation

[`RuleCraftWeapon::afterLoad`](../src/Mod/RuleCraftWeapon.cpp) hard-fails the mod when:

- `launcher` is missing (every craft weapon needs a launcher item);
- `unifiedDamageFormula: true` but neither `clip` nor `launcher` resolves;
- a missile-type weapon (`projectileType` 0–3) has `damage > 0` but no positive `projectileSpeed`;
- `ammoMax` is set with `rearmRate <= 0`, or the `clip` item's `clipSize` exceeds `rearmRate`.

## See also

- [`crafts:`](Ruleset-Crafts.md) — hardpoint count, `weaponTypes:` slot filters, `fixedWeapons:`
- [`items:`](Ruleset-Items.md) — the `launcher` and `clip` store items
- [`ufos:`](Ruleset-Ufos.md) — what you shoot at (shields, `avoidBonus`)
- [`manufacture:`](Ruleset-Manufacture.md) / [`research:`](Ruleset-Research.md) — how the launcher
  and clip items become available
