# TODO
- Motion detector blips are not saved
- Rename existing role icons to be more physical
- Create new role icons for heavy weapons and stealth
- Can we add key stealth metrics to the armor info screens
- Document more modded field details from OXCE (such as camouflageAtDay, which is not really documented)
- Write a full ruleset document.  Explain fields, structures, options.

# Documentation

Two possible engine bugs surfaced and are noted in the docs, worth a separate look if you use those features: the adhoc tag-matching loop in GeoscapeState.cpp has an unconditional break, so only the first entry of adhocMissionScriptTags is ever compared; and RuleMissionScript/RuleArcScript never initialize _counterMin/_counterMax (only RuleEventScript does).

# Questions
- Overwatch+dual fire?
- Overwatch range vs. weapon's (attacktype)Range?  What does that field do in DX anyway?

# TFTD


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