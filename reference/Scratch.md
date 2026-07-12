# TODO


# Questions
- Overwatch+dual fire?
- Overwatch range vs. weapon's (attacktype)Range?  What does that field do in DX anyway?

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

## Darkness Balance

Alien see-in-the-dark stuff?