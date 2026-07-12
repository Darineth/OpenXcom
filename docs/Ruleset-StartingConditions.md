# Ruleset: `startingConditions:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`RuleStartingCondition`](../src/Mod/RuleStartingCondition.h) · **List key:** `type` ·
**Loader:** [`RuleStartingCondition::load`](../src/Mod/RuleStartingCondition.cpp)

A starting condition is a **gate on what X-COM may bring to a mission**: which craft may even fly
there, which soldier types may go, which armors/items/vehicles are allowed on board, and what the
craft must carry. It is attached to a mission by the
[`alienDeployments:`](Ruleset-AlienDeployments.md) `startingCondition:` key, and is enforced in two
places: on the Geoscape when you target/land at the mission
([`ConfirmLandingState`](../src/Geoscape/ConfirmLandingState.cpp)) and at map generation, where
disallowed gear is left behind and disallowed armor is swapped
([`BattlescapeGenerator`](../src/Battlescape/BattlescapeGenerator.cpp)).

```yaml
startingConditions:
  - type: STR_MARS_CONDITIONS
    allowedArmors:
      - STR_FLYING_SUIT_UC          # only powered/flying suits survive Mars
      - STR_POWER_SUIT_UC
    defaultArmor:
      STR_SOLDIER:                  # what a soldier wearing a forbidden armor is switched into
        STR_POWER_SUIT_UC: 100
    forbiddenItems: [ STR_MEDI_KIT ]
    requiredItems:
      STR_ELERIUM_115: 20
    destroyRequiredItems: true
```

Entries **merge** across mods/files by `type`, support `refNode:` inheritance and `delete: true`
(see the [index](Ruleset.md#how-rulesets-load-the-60-second-version)).

## Allow / forbid lists

Every category comes as an `allowed…` **whitelist** and a `forbidden…` **blacklist**. They are not
combined: **if the `forbidden…` list is non-empty it is used and the `allowed…` list is ignored**; if
only the `allowed…` list is set, anything not on it is rejected; if both are empty, everything is
permitted.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `allowedCraft` / `forbiddenCraft` | list of craft types | — | Which [`crafts:`](Ruleset-Crafts.md) may fly this mission at all (checked before you can even select the target). |
| `allowedSoldierTypes` / `forbiddenSoldierTypes` | list of soldier types | — | Which [`soldiers:`](Ruleset-Soldiers.md) types may board the craft. |
| `allowedArmors` / `forbiddenArmors` | list of armor types | — | Which [`armors:`](Ruleset-Armors.md) may be worn; a soldier in a rejected armor is switched per `defaultArmor` (or simply kept if there is no replacement). |
| `allowedVehicles` / `forbiddenVehicles` | list of item types | — | Which HWPs/vehicles may be deployed; rejected ones (and their ammo) stay in the base. |
| `allowedItems` / `forbiddenItems` | list of item types | — | Which [`items:`](Ruleset-Items.md) may be taken into battle. |
| `allowedItemCategories` / `forbiddenItemCategories` | list of category ids | — | Same, by [`itemCategories:`](Ruleset-ItemCategories.md). An explicitly *allowed item* wins over the category rules; an explicitly *forbidden item* is always rejected. When `shareAmmoCategories` is on, a firearm also inherits the categories of the compatible ammo carried on the craft. |

## Replacements & requirements

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | — | Unique id, referenced by a deployment's `startingCondition:`. |
| `refNode` | map | — | Inherit all fields from another (anonymous) node first. |
| `defaultArmor` | map `soldierType: {armorType: weight}` | — | Weighted replacement armor per soldier type, used when a soldier's armor is not permitted; the special pick `noChange` means "leave the armor alone". |
| `forbiddenArmorsInNextStage` | list of armor types | — | Armors that cannot continue into the **next stage** of a multi-stage mission — units wearing them are flagged and do not carry over. |
| `craftTransformations` | map `craftType: craftType` | — | Swap the craft's battlescape map: the deployed craft's map is replaced by the target craft's. Takes priority over a map script's `craftName:`. |
| `requiredItems` | map `itemType: count` | — | Items that must be **onboard the craft** before it may land (checked on the Geoscape). |
| `destroyRequiredItems` | bool | false | Consume the `requiredItems` when the mission starts (fuel-style, instead of just checking for them). |
| `requireCommanderOnboard` | bool | false | The craft must carry a soldier of Commander rank. |

## Obsolete keys

`environmentalConditions:`, `paletteTransformations:`, `armorTransformations:`,
`mapBackgroundColor:`, `inventoryShockIndicator:` and `mapShockIndicator:` were **moved out of
starting conditions** into [`enviroEffects:`](Ruleset-EnviroEffects.md). Leaving them in a starting
condition logs an error at load time and does nothing.

## See also

- [`alienDeployments:`](Ruleset-AlienDeployments.md) — attaches a starting condition to a mission
- [`enviroEffects:`](Ruleset-EnviroEffects.md) — the *in-battle* half of the old starting-condition feature set
- [`armors:`](Ruleset-Armors.md) / [`items:`](Ruleset-Items.md) / [`crafts:`](Ruleset-Crafts.md) — what the allow/forbid lists name
- [`mapScripts:`](Ruleset-MapScripts.md) — `craftName:`, which `craftTransformations:` overrides
