---
name: ui-palette-colors
description: >-
  Pick correct OpenXcom DX UI and text/font color indices by visually reviewing the
  palette spectrum reference images. Use whenever you must choose or sanity-check a
  numeric color value (0-255) for an interfaces.rul element (color/color2/border) or a
  C++ setColor()/setSecondaryColor() call, decide which palette a screen uses, find a
  lighter/darker shade of an existing color, or explain what a given index looks like.
  The engine has no named colors — every UI color is a raw palette index whose actual
  RGB depends on the screen's active palette, so you must look at the matching palette
  image to choose well.
---

# Choosing UI / text colors from the palette reference

OpenXcom has **no named colors**. Every UI color — in `interfaces.rul` (`color`, `color2`,
`border`) and in C++ (`setColor(Uint8)`, `setSecondaryColor`, `setBorderColor`) — is a raw
**palette index 0–255**. The same index is a *different RGB* in each palette. So you cannot pick
a color from the number alone: you must look at the spectrum of the palette the screen actually
uses. This skill is how you do that.

The authoritative, offline reference is the UFOpaedia PALETTES.DAT dump vendored in the repo:
- Page: [reference/PALETTES.DAT.htm](../../../reference/PALETTES.DAT.htm)
- Spectrum images: [reference/PALETTES.DAT_files/](../../../reference/PALETTES.DAT_files/)

Each spectrum PNG is **256×75**: a horizontal strip of all 256 colors, **left→right = index 0→255**,
**one pixel wide per color**.

When selecting a text color, prefer to pick the first shade in a block that has the hue you want,
so the widget's built-in shading (e.g. `+1..+5` for `TextButton`) stays within the same hue ramp.

## Image geometry — converting X pixel ↔ color index

- The strip is **1 px per color**, so the mapping is direct: **`index = X`** (the pixel at column X
  *is* color index X).
- Colors come in **16 blocks of 16 shades** (`Palette::blockOffset(block) = block*16`). Each block
  is one hue ramp, usually **light at the low index → dark at the high index**. A block is **16 px
  wide**, so block *B* spans X `[B*16, B*16+15]`; block boundaries are the visible "seams" in the
  strip. `index = block*16 + shade` (shade 0–15).
- `BackPals.Png` / `TFTD_BackPals.Png` are **128×75** (8 palettes × 16 colors), also 1 px/color.

## Step 1 — find the screen's palette (this picks the image)

A color index is meaningless without its palette. Find the screen's palette, then open the one
matching image. The screen's `interfaces.rul` block has a top-level `palette:` key (e.g.
`PAL_GEOSCAPE`); the C++ also calls `setPalette`/`setInterface`. Engine palette ⇄ reference image:

| Engine palette (`PAL_*`) | Reference image | Used by |
|---|---|---|
| `PAL_GEOSCAPE` | [1_GeoScapePal.Png](../../../reference/PALETTES.DAT_files/1_GeoScapePal.Png) | Globe / Geoscape screens |
| `PAL_BASESCAPE` | [2_UnknownPal.Png](../../../reference/PALETTES.DAT_files/2_UnknownPal.Png) | Basescape, most menus |
| `PAL_GRAPHS` | [3_GraphPal.Png](../../../reference/PALETTES.DAT_files/3_GraphPal.Png) | Graphs screen |
| `PAL_UFOPAEDIA` / `PAL_BATTLEPEDIA` | [4_ResearchPal.Png](../../../reference/PALETTES.DAT_files/4_ResearchPal.Png) | Ufopaedia / research |
| `PAL_BATTLESCAPE` | [5_BattleScapePal.Png](../../../reference/PALETTES.DAT_files/5_BattleScapePal.Png) | Battlescape (tactical) |

TFTD (`xcom2`) uses its own palettes — `TFTD_Pal1..3.png` (Geoscape / Basescape+Research /
Graphs) and `TFTD_Tact1..4.png` (land + 3 water depths). When editing
`bin/standard/xcom2/interfaces.rul`, review the `TFTD_*` images, **not** the UFO ones — the same
index is a different color there.

## Step 2 — read the image and pick an index

1. **Read** the matching PNG with the Read tool — it renders inline so you can see the colors.
2. Find the hue/brightness you want. Read off its X position across the 256 px strip — that X
   **is** the index. To refine, identify which 16 px block it's in (block = X/16) and where in
   that block's light→dark ramp it sits.
3. The index is simply X, or equivalently `block*16 + shade`.
4. **Cross-check against existing entries.** `interfaces.rul` annotates colors inline, e.g.
   `color: 133   # minty green`, `color: 0  # brown`, `color: 138  # yellow`,
   `color: 239  # bright green`. Reusing a vetted index (and copying its comment) is safer and
   more consistent than a fresh guess. Grep the same file for the look you want.

## Step 3 — respect how widgets shade the base color

Most widgets don't use a single flat color — they render a **gradient within the base color's
16-block**:
- `TextButton::draw` paints its 3-D bevel as `_color + 1 … _color + 5` (see
  [src/Interface/TextButton.cpp](../../../src/Interface/TextButton.cpp)). `Window` borders shade
  similarly around the base.
- **Therefore pick a base near the LIGHT (low) end of a block** so `+5` stays inside the same
  hue ramp. If the base is too close to the block's high end, the bevel spills into the next
  block — a different hue — and looks wrong. Plain `Text` uses the single index as-is (no bevel),
  so for a label you may pick any shade in the ramp.
- `color2` is the secondary color (TextList alternate-row text, `Bar` second value, ComboBox
  arrow, etc.); `border` is the frame color. Pick each from the same palette image.

## Hard rules — reserved / special indices

- **Index 0 is transparent, not black.** Never use 0 for visible text/icons — it renders as a
  hole (this is why `color: 0  # brown` works for a *window* base that's redrawn, but is wrong
  for a `Text`). True black/near-black is the dark end of a neutral block, not index 0.
- **Indices 224–239 (block 14) are the window-background block.** In-game they're replaced
  per-screen by a `BACKPALS.DAT` 16-color palette (see
  [BackPals.Png](../../../reference/PALETTES.DAT_files/BackPals.Png)). Don't use them for
  widgets that need a stable color; they exist to tint bordered-window backgrounds.
- **Indices 240–255 (block 15) are reserved.** In the Tactical palette they're a hard-coded
  greyscale set; elsewhere largely unused. Avoid for UI.

## Quick recipes

- *"Make this text a darker shade of the same color"* → keep the same block, **increase** the
  index toward `block*16+15` (e.g. 133 → 138 stays in the green block). Confirm on the image that
  it's still the same hue ramp.
- *"What color is index 138 here?"* → open the screen's palette image, look at column X = 138.
- *"Pick a yellow for a Geoscape label"* → Read `1_GeoScapePal.Png`, find the yellow ramp; its
  X column is the index; cross-check with `grep yellow bin/standard/xcom1/interfaces.rul`.

## Notes

- YAML-only color edits need just a game restart, not a rebuild. C++ `setColor` changes need a
  build. See the [game-ui-screen](../game-ui-screen/SKILL.md) skill for the surrounding
  screen/widget workflow and the `interfaces.rul` element schema.
- UFO (`xcom1`) and TFTD (`xcom2`) have separate `interfaces.rul` and separate palettes — verify
  the color in **both** images if a screen exists in both games.
