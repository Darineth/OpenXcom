# TODO
- Don't allow overwatch on no-ammo.
- "Turn #" display appears multiple times sometimes in the combat log?  I think this is player vs enemy turns, let's show the faction the turn belongs to.
- Switching armor does not update the inventory layout grid

- Overwatch range vs. weapon's (attacktype)Range?  What does that field do in DX anyway?
- Overwatch+dual fire?
- Fix offscreen/pre-battle events appearing in combat log
- Inventory grid and item rendering happens in clearly separate frames, and can sometimes get stuck with the wrong inventory layout showing?
- Need to re-document OXCE's features.  Extended.txt is incomplete (e.g. extendedItemReloadCost is not documented).
- Enable UFOPedia for items and weapons that don't have a configured page, so you can actually view stats for them.
- Wound indicator on hp bar

# TFTD
- Unit stats colors are wrong in inventory

# Terror Defense Future Notes

These are notes for the future Terror Defense mod.  *Ignore them for now, they are just notes for me.*

## Armor Damage Config

Based on Xus's original armor damage code.

```yaml
- armor damage config for XcomTD
    damageAlter:
      ToArmor: 0.2                 # legacy: power * 0.2  (clean-penetration wear)
      ToArmorBlocked: 0.05         # legacy: (originalPower - armor*0.5) * 0.05 + 1
      ToArmorBlockedThreshold: 0.5
      ToArmorOverPen: 1.0          # legacy: (power - armor) on a smash-through
      ToArmorOverPenThreshold: 2.0
```