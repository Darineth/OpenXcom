# TODO

* Focus camera on dying units over projectiles
* I really want the automatic UFOPedia thing done.  This will help make it way easier to get access to item info ingame.

# Documentation
- Write a full ruleset document.  Explain fields, structures, options. **DONE, but might need a review pass?**

# Questions
- Overwatch+dual fire?
- Overwatch range vs. weapon's (attacktype)Range?  What does that field do in DX anyway?
- Multiple reaction fires in the log during a multi-shot attack maybe?
- Should psi backlash potentially be able to take the target's stats into account?

# Needs review
- Psi chance displays/calculations - especially Mind Blast

# TFTD


# Terror Defense Future Notes

These are notes for the future Terror Defense mod.  *Ignore them for now, they are just notes for me.*

## TODO

- Write a TD catalogue document, explaining all the items, units, and mechanics specific to the original Terror Defense mod.  This will be the starting point for future work building the real mod.
- Add a surface load option to convert TFTD surfaces into XCOM1 palettes.  We don't want to convert/modify the original game assets!  Palette mapping config?

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