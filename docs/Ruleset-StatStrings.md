# Ruleset: `statStrings:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`StatString`](../src/Mod/StatString.h) · **List key:** — (an ordered, unkeyed list) ·
**Loader:** [`StatString::load`](../src/Mod/StatString.cpp)

A *stat string* appends a short marker to a soldier's displayed name based on their stats — the
XcomUtil convention where a psi-strong, high-reactions rookie shows up as `Jane Doe/Pr`. Purely
cosmetic: it changes what soldier lists show, nothing else.

```yaml
statStrings:
  - string: "x"                 # weak psi: flagged first
    psiStrength: [~, 30]        # [min, max]; ~ means "unbounded"
  - string: "P"
    psiStrength: [80, ~]
  - string: "r"
    reactions: [50, 59]
  - string: "Sniper"            # multi-character: matches, prints, and STOPS
    firing: [70, ~]
    reactions: [60, ~]
```

The list is **ordered and additive**: entries are evaluated top to bottom, every entry whose
conditions all hold contributes its `string`, and the results are concatenated. Entries accumulate
across mods rather than merging (there is no key to merge on) — `statStrings:` in a later mod appends
to the list.

## Fields

| Key | Type | Default | Meaning |
|---|---|---|---|
| `string` | string | — | The literal text appended to the soldier's name when every condition below is met (raw text, **not** an `STR_*` key). |
| `psiStrength` | `[min, max]` | `[0, 255]` | Condition on the stat: the soldier's value must lie in the inclusive range. |
| `psiSkill` | `[min, max]` | `[0, 255]` | ” |
| `bravery` | `[min, max]` | `[0, 255]` | ” |
| `strength` | `[min, max]` | `[0, 255]` | ” |
| `firing` | `[min, max]` | `[0, 255]` | ” |
| `reactions` | `[min, max]` | `[0, 255]` | ” |
| `stamina` | `[min, max]` | `[0, 255]` | ” |
| `tu` | `[min, max]` | `[0, 255]` | ” |
| `health` | `[min, max]` | `[0, 255]` | ” |
| `throwing` | `[min, max]` | `[0, 255]` | ” |
| `melee` | `[min, max]` | `[0, 255]` | ” |
| `manaPool` | `[min, max]` | `[0, 255]` | ” (the soldier's mana stat). |
| `psiTraining` | `[min, max]` | `[0, 255]` | Not a stat: matches only while the soldier **is in psi training** (the engine sets the value to 1; the range is effectively ignored). |

Only the stat keys actually present in an entry become conditions; everything else is unconstrained.
Either bound may be written as `~` (null) to leave it at its default (`0` for min, `255` for max) —
so `[~, 30]` is "≤ 30" and `[80, ~]` is "≥ 80".

## Evaluation rules

From [`StatString::calcStatString`](../src/Mod/StatString.cpp):

- Conditions within one entry are **AND**ed; all must hold.
- Matching entries' strings are **concatenated in list order** — that is how the classic
  one-letter markers stack (`Pr`, `bks`, …).
- **A matching entry whose `string` is longer than one character ends the evaluation**: it is
  appended and nothing after it is considered. Use this for a single descriptive label
  ("Sniper", "Psi") that should replace, not join, the letter soup.
- **Psi hiding:** psi conditions are only evaluated once the soldier's psi stats are known — either
  the soldier has `psiSkill > 0`, or the `psiStrengthEval` option (Options → "Reveal psi stats") is
  on. Before that, psi-based entries never match, so psi-weak recruits are not silently revealed.
- A condition on a stat name the engine does not know about never matches (this is the mechanism the
  `psiTraining` pseudo-stat rides on).

## See also

- [UnitStats block](Ruleset-UnitStats.md) — the stat names themselves
- [`soldiers:`](Ruleset-Soldiers.md) — the soldier types whose names these decorate
- The bundled `XcomUtil_Statstrings` mod (`bin/standard/XcomUtil_Statstrings/`) is a complete worked example.
