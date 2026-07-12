# Ruleset: `extraStrings:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`ExtraStrings`](../src/Mod/ExtraStrings.h) · **List key:** `type` (a **language id**) ·
**Loader:** [`ExtraStrings::load`](../src/Mod/ExtraStrings.cpp)

`extraStrings:` is the mod-side translation table. Every player-facing string in the engine is a
`STR_*` key resolved through the active language; `extraStrings:` adds new keys and overrides existing
ones, per language.

```yaml
extraStrings:
  - type: en-US
    strings:
      STR_STEALTH_SUIT: "Stealth Suit"
      STR_STEALTH_SUIT_UC: "STEALTH SUIT"
      STR_HEAVY_CANNON: "Heavy Sniper"          # override a vanilla string
      STR_STEALTH_SUIT_UFOPEDIA: "A prototype suit that bends light around the wearer..."
      STR_ALIENS_KILLED:                        # plural forms
        zero: "No aliens killed"
        one: "{0} alien killed"
        other: "{0} aliens killed"
```

## Fields

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | — | The **language id** these strings belong to (`en-US`, `en-GB`, `de`, `fr`, `ru`, … — the same ids the language files use). |
| `strings` | map | — | The table itself: `STR_KEY: "text"`, or `STR_KEY:` followed by a map of plurality forms. |

Entries **merge** by language: a second `extraStrings:` entry for the same `type` adds to (and
overwrites keys in) the first, and mods layer in load order — the last mod to define a key wins.

## Plurality

If a key's value is a **map** rather than a string, each sub-key is appended to the key with an
underscore. So

```yaml
      STR_ALIENS_KILLED:
        one: "{0} alien killed"
        other: "{0} aliens killed"
```

stores `STR_ALIENS_KILLED_one` and `STR_ALIENS_KILLED_other`, which is what the engine's plural
lookup (`Language::getString(key, count)`) asks for. Which sub-keys exist is language-dependent
(English uses `one`/`other`; Slavic languages add `few`/`many`, etc.).

## Notes

- `{0}`, `{1}`, … are the argument placeholders filled by `tr(...).arg(...)` in C++.
- `{ALT}` / `{NEWLINE}` / `{SMALLLINE}` and the other markup tokens the base language files use are
  available here too.
- A `STR_*` key referenced from C++ but never defined renders as the **raw token** on screen — that is
  the usual symptom of a missing `extraStrings` entry.
- **DX convention:** strings for DX's own engine features live in `bin/common/Language/DX/*.yml`, not
  in `extraStrings:`. Use `extraStrings:` for *mod content*.

## See also

- [`ufopaedia:`](Ruleset-Ufopaedia.md) — `title:`/`text:` keys are defined here
- [`statStrings:`](Ruleset-StatStrings.md) — the soldier-name suffixes (raw text, **not** `STR_*` keys)
