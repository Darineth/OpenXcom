# TODO
- Visualize aim/throw cone/landing area?
- Fix offscreen/pre-battle events appearing in combat log
- Inventory grid and item rendering happens in clearly separate frames, and can sometimes get stuck with the wrong inventory layout showing?

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