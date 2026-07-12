# Ruleset: `transparencyLUTs:`

Back to the [ruleset index](Ruleset.md).

**Engine class:** — (raw tables on `Mod`) · **List key:** — (an ordered, unkeyed list) · **Loader:**
[`Mod::loadResourceConfigFile`](../src/Mod/Mod.cpp)

A *transparency LUT* is a **tint** — a color-plus-strength lookup table used to draw translucent
overlays over the battlescape: smoke, fire, the TFTD underwater haze, script-driven color washes.
Each LUT is one tint color, precomputed at several opacity levels; rules and Y-Scripts select a LUT by
**index** (item `damageAlter: TileDamageMethod`/smoke, tile fire, `ShaderDraw` tint calls).

```yaml
# in the mod's resource config file (metadata.yml: resourceConfig: vars.rul)
transparencyLUTs:
  - colors:
      - [  8,  8, 12, 2 ]   # LUT 0 — "white" smoke   [r, g, b, strength]
      - [ 16,  8,  0, 2 ]   # LUT 1 — orange (fire)
      - [  0, 12, 12, 2 ]   # LUT 2 — cyan
      - [  4, 16,  4, 2 ]   # LUT 3 — green
```

**This node is not read from ordinary `.rul` files.** It is only parsed from a mod's **resource config
file** — the file named by `resourceConfig:` in the mod's `metadata.yml` (TFTD's is `vars.rul`). The
list is **positional**: LUTs are numbered in the order they appear, and mods are given disjoint index
ranges, so a mod's LUT 0 does not collide with the master's.

## Structure

| Key | Type | Default | Meaning |
|---|---|---|---|
| (list item) | map | — | A group of LUTs; only the `colors:` sub-key is read, so one group is normally enough. |
| `colors` | list | — | The LUTs themselves, in index order. Each entry is either an **RGB+strength tint** (short form) or an explicit **per-opacity table** (long form). |

### Short form — `[r, g, b, strength]`

| Position | Type | Default | Meaning |
|---|---|---|---|
| 0 | int 0–255 | — | Red component of the tint. |
| 1 | int 0–255 | — | Green component of the tint. |
| 2 | int 0–255 | — | Blue component of the tint. |
| 3 | int 0–255 | 2 | **Opacity strength**: how fast the tint saturates as the overlay thickens. Higher = the color takes over sooner. |

The engine expands this into all opacity levels itself, interpolating between the underlying pixel and
the tint. At low strength the result matches the original TFTD behavior; at higher strength it drives
harder toward the pure tint color. This is the form you want.

### Long form — an explicit table

Instead of one `[r, g, b, strength]` triple-plus-strength, an entry may be a **list of per-opacity
entries**, one `[r, g, b, a]` per opacity level (the engine uses a fixed number of levels; `a` is
inverted into an alpha internally and the levels are reversed so that level 0 is the faintest). Use
this only when you need a tint curve the short form cannot express.

## Notes

- The values in the vanilla tables (`[8, 8, 12, 2]`) are **small** because they are multiplied against
  the target color rather than replacing it — they are not 0–255 RGB in the ordinary sense.
- Out-of-range values (`< 0` or `> 255`) are a **soft error**: logged and clamped, not fatal.
- Overflowing your mod's LUT budget throws `transparencyLUTs mod limit reach`.
- A LUT index used by a rule but never defined draws nothing (or the wrong tint) — check the load log.

## See also

- [`soundDefs:`](Ruleset-SoundDefs.md) — the other root parsed from the resource config file
- [`customPalettes:`](Ruleset-CustomPalettes.md) — the palettes these tints operate on
- [`items:`](Ruleset-Items.md) — the smoke/fire damage settings that pick a LUT
