# Ruleset: `customPalettes:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** [`CustomPalettes`](../src/Mod/CustomPalettes.h) · **List key:** `type` · **Loader:**
[`CustomPalettes::load`](../src/Mod/CustomPalettes.cpp) (applied in
[`Mod::loadExtraResources`](../src/Mod/Mod.cpp))

A `customPalettes:` entry **edits or creates a palette**. The engine is a palettized (8-bit) renderer:
every surface stores palette indices, and a screen's colors mean whatever its active 256-color palette
says they mean. This root lets a mod recolor an existing palette (`PAL_GEOSCAPE`, `PAL_BASESCAPE`,
`PAL_BATTLESCAPE`, `PAL_UFOPAEDIA`, `PAL_BATTLEPEDIA`, …) or define a brand-new one to point an
[`interfaces:`](Ruleset-Interfaces.md) entry at.

```yaml
customPalettes:
  - type: PAL_MYMOD_TWEAK       # id of this rule (merge key)
    target: PAL_GEOSCAPE        # which palette gets modified
    palette:                    # index: [r, g, b]  (0-255 each)
      240: [255,   0,   0]
      241: [224,   0,   0]

  - type: PAL_MYMOD_FULL
    target: PAL_MYMOD           # a NEW palette, created black then filled
    file: Resources/MyMod/mypalette.pal    # a 256-color JASC .pal file
```

Entries merge by `type`.

## Fields

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | — | Id of this palette-edit rule (the merge key). Not the palette's name — that's `target`. |
| `target` | string | — | The palette to modify. If no palette by that name exists yet, a **new all-black palette** is created under that name. |
| `file` | string path | — | A **JASC** `.pal` file (3-line header, then 256 `R G B` lines) that replaces the whole target palette. |
| `palette` | map int → `[r, g, b]` | — | Individual color overrides: palette index → RGB triple. Only the listed indices change. |

`file` and `palette` are mutually exclusive in effect: **if `file` is set, `palette` is ignored** and
the target is filled entirely from the file.

## Notes

- Palette edits are applied **after** all rulesets are loaded, in load order, so a later mod's
  `customPalettes:` wins on overlapping indices.
- After all custom palettes are applied, the engine snapshots every `PAL_*` palette as
  `BACKUP_PAL_*` — that is what the "restore original colors" paths use.
- **Index 0 is transparent** in this engine, and the top of the palette is used for the cursor and
  UI-critical colors; recoloring those has global consequences.
- Creating a new palette is only useful together with an `interfaces:` entry whose `palette:` names it
  (that is what makes a screen load it), or with a **`_CPAL` article image**: any
  [`ufopaedia:`](Ruleset-Ufopaedia.md) article whose `image_id` contains `_CPAL` takes its palette from
  the image itself rather than the standard pedia palette.
- The engine stores 8-bit-per-channel RGB here; the original `PALETTES.DAT` files are 6-bit, so
  values copied from vanilla palette dumps may need scaling.

## See also

- [`interfaces:`](Ruleset-Interfaces.md) — `palette:` per screen, and why element colors are palette indices
- [`extraSprites:`](Ruleset-ExtraSprites.md) — the images drawn with these palettes
- [`ufopaedia:`](Ruleset-Ufopaedia.md) — the `_CPAL` custom-palette article path
