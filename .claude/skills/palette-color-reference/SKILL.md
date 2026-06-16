---
name: palette-color-reference
description: >-
  Offline reference data for the X-COM PALETTES.DAT colors — the actual per-block
  breakdown of every UFO and TFTD palette (which index ranges are which hue, and the
  reserved/special blocks). Use this to LOOK UP what a palette index range contains,
  find the index where a given hue's ramp starts, or understand the palette file
  format (256 colors, 6-bit RGB, transparency index 0, the 16-color blocks, BACKPALS
  window backgrounds, the tactical greyscale). Each palette is one hue per 16-index
  block; text uses the FIRST color of a block, so start recoloring from there. For the
  workflow of CHOOSING a color for an interfaces.rul element or setColor() call, see
  the ui-palette-colors skill.
---

# PALETTES.DAT color reference

This is the table-ized form of the UFOpaedia [PALETTES.DAT](https://www.ufopaedia.org/index.php?title=PALETTES.DAT)
page. It is **reference data** — the block-by-block contents of every palette — so you can
look up an index range without re-reading the source HTML or eyeballing the strip images. To
*pick* a color for a screen (and respect widget bevel shading), use the
[ui-palette-colors](../ui-palette-colors/SKILL.md) skill; this skill is the lookup table behind it.

The matching spectrum images live next to this file in [images/](images/) (each is a 256×75
strip, **1 px per color, left→right = index 0→255**). RGB hexes in the tables below are the
8-bit values shown in those strips.

## File format facts

- A palette is **256 colors**, each stored as 3 bytes RGB → **768 bytes**. Each palette is
  followed by a 6-byte buffer of unknown meaning. UFO's `PALETTES.DAT` holds **5** palettes
  (3,870 bytes); TFTD's holds **3** (2,322 bytes) and rewrites the rest on the fly / from the EXE.
- Stored RGB is **6-bit** (max intensity `0x3F` = 63), the VGA Mode 13h standard. The CE/OXCE
  engines multiply each channel by 4 to map it into 8-bit (0–255) display space — that is why the
  table hexes top out at `0xFF`/`0xFC`-ish steps.
- **Index 0 is transparency, not black** — pixels at index 0 are not drawn. Never use 0 for
  visible text/icons. True black is the dark end of a neutral block.
- Colors are organized into **16 blocks of 16 shades** (`block B = indices [B*16, B*16+15]`).
  Within a "ramp" block the color is one hue going **light (low index) → dark (high index)**.
  Some blocks are not single ramps — they pack two 8-shade half-ramps or a set of fixed accent
  colors; those are flagged below.
- **Indices 224–239 (block 14) are the window-background block.** In game they are replaced
  per-screen by a 16-color [BACKPALS.DAT](#backpalsdat-window-background-palettes) palette to tint
  bordered-window backgrounds; the values baked into the main palette (listed below) are only the
  defaults where no background image overrides them.
- **In the Tactical palette, indices 240–255 (block 15) are replaced** by a hard-coded
  16-step **greyscale** (see [Tactical greyscale](#tactical-greyscale-replaces-240255-in-battle)),
  not loaded from any palette file. In non-tactical palettes block 15 is mostly reserved/unused
  (the last four entries `FFFFA5 DEEB8C C6D77B A5C36B` are a recurring pale yellow-green marker set).

## Reading the block tables

Each row is one 16-index block. **Start color** is the color at the block's first index — for
ramp blocks this is the lightest shade and the one plain `Text` defaults to, so it is where you
start any recolor. **Contents** describes the hue/ramp (or "fixed"/"split" for non-ramp blocks).

## Palette → image map

| # | UFO palette | Engine `PAL_*` | Image | TFTD equivalent |
|---|---|---|---|---|
| 1 | GeoScape | `PAL_GEOSCAPE` | [1_GeoScapePal.Png](images/1_GeoScapePal.Png) | [TFTD_Pal1.png](images/TFTD_Pal1.png) |
| 2 | BaseScape | `PAL_BASESCAPE` | [2_UnknownPal.Png](images/2_UnknownPal.Png) | [TFTD_Pal2.png](images/TFTD_Pal2.png) (Base+Research) |
| 3 | Graphs | `PAL_GRAPHS` | [3_GraphPal.Png](images/3_GraphPal.Png) | [TFTD_Pal3.png](images/TFTD_Pal3.png) |
| 4 | Research / Ufopaedia | `PAL_UFOPAEDIA` / `PAL_BATTLEPEDIA` | [4_ResearchPal.Png](images/4_ResearchPal.Png) | (uses TFTD_Pal2) |
| 5 | Tactical / Battlescape | `PAL_BATTLESCAPE` | [5_BattleScapePal.Png](images/5_BattleScapePal.Png) | [TFTD_Tact1..4](#tftd-tactical-palettes-depth-variants) |

The UFOpaedia uses palettes 1, 2, 4, or 5 depending on the entry being displayed.

---

# UFO palettes

## 1. GeoScape — `PAL_GEOSCAPE` · [image](images/1_GeoScapePal.Png)

| Block | Indices | Start color | Contents |
|---|---|---|---|
| 0 | 0–15 | `000000` (transparent) | **Fixed accents** — 0 transparent; pink `FFA6D6`, mauve, orange `FF8200`, white `FFFFFF`, light grey, green `00FF00`, cyan `00FFFF`, yellow `FFFF00`, red `FF0000`, black. Not a ramp. |
| 1 | 16–31 | `008229` green | Green ramp → very dark green |
| 2 | 32–47 | `739E29` olive-green | Olive-green ramp → dark green |
| 3 | 48–63 | `ADA600` gold | Gold/olive ramp → dark olive |
| 4 | 64–79 | `94A631` khaki | Khaki / yellow-green ramp → dark |
| 5 | 80–95 | `7BF7DE` pale teal | **Split**: teal→indigo (80–89), then lime `DEFF00`→dark green (90–95) |
| 6 | 96–111 | `CEBECE` lavender-grey | Lavender-grey ramp (96–105); then odd entries (teal `184973`, lime, greens) |
| 7 | 112–127 | `63A200` green | Green ramp → dark green |
| 8 | 128–143 | `FF0000` red | **Split**: pure red ×6, teal `63CFBD`→dark (134–138), olive `ADBE52`→dark (139–143) |
| 9 | 144–159 | `B59642` tan | Tan/brown ramp → dark brown |
| 10 | 160–175 | `9CB2B5` blue-grey | Blue-grey ramp → dark |
| 11 | 176–191 | `733084` purple | Purple/violet ramp → black |
| 12 | 192–207 | `002473` navy | Navy blue (nearly flat) |
| 13 | 208–223 | `001852` dark navy | Dark navy ramp |
| 14 | 224–239 | `6B7D42` olive | **Window-bg block** (BACKPALS-replaced); default olive-grey ramp |
| 15 | 240–255 | `42B26B` green | Reserved/special: greens & blues; ends `FFFFA5 DEEB8C C6D77B A5C36B` |

## 2. BaseScape — `PAL_BASESCAPE` · [image](images/2_UnknownPal.Png)

| Block | Indices | Start color | Contents |
|---|---|---|---|
| 0 | 0–15 | `000000` (transparent) | Greyscale ramp `FFFFFF`→black (0 is transparent) |
| 1 | 16–31 | `FFD300` amber | Amber/gold → dark red-brown |
| 2 | 32–47 | `FF797B` salmon | Salmon/red → dark red |
| 3 | 48–63 | `D6E763` yellow-green | Yellow-green → very dark green |
| 4 | 64–79 | `EFEFFF` lavender-white | Lavender / blue-grey ramp |
| 5 | 80–95 | `FFFBFF` white | Warm grey / taupe ramp |
| 6 | 96–111 | `F7C74A` gold | Gold / orange-brown ramp |
| 7 | 112–127 | `CEBAAD` tan | Tan → plum ramp |
| 8 | 128–143 | `ADD3F7` sky blue | Sky-blue ramp → dark navy |
| 9 | 144–159 | `FFFF7B` pale yellow | Yellow → brown ramp |
| 10 | 160–175 | `BDA25A` tan | Tan / brown ramp |
| 11 | 176–191 | `FFDFD6` pale pink | Pink/rose → magenta ramp |
| 12 | 192–207 | `63415A` muted purple | Muted purple ramp |
| 13 | 208–223 | `FFDFD6` pink | **Mixed special**: pinks, greys, khaki `DEDB84`, blues `84B2DE` |
| 14 | 224–239 | `522C84` purple | **Window-bg block** (BACKPALS-replaced); default purple→dark-blue ramp |
| 15 | 240–255 | `000000` | Reserved/special: pinks, purples `B596EF`; ends `FFFFA5 DEEB8C C6D77B A5C36B` |

## 3. Graphs — `PAL_GRAPHS` · [image](images/3_GraphPal.Png)

Graph blocks are mostly **two 8-shade half-ramps** (a bright variant 0–7 and a darker/alt variant
8–15) so each graph line gets a distinct hue pair.

| Block | Indices | Start color | Contents |
|---|---|---|---|
| 0 | 0–15 | `000000` (transparent) | Greyscale ramp `FFFFFF`→`181818` |
| 1 | 16–31 | `FFFF00` yellow | Yellow ramp (16–23) / red ramp (24–31) |
| 2 | 32–47 | `00FF00` green | Green ramp (32–39) / blue ramp (40–47) |
| 3 | 48–63 | `00FFFF` cyan | Cyan ramp (48–55) / pink-magenta ramp (56–63) |
| 4 | 64–79 | `FF7100` orange | Orange ramp (64–71) / tan ramp (72–79) |
| 5 | 80–95 | `BD79FF` lavender | Purple ramp (80–87) / slate-blue ramp (88–95) |
| 6 | 96–111 | `FFFFFF` white | Grey ramp (96–103) / green ramp `18DF8C` (104–111) |
| 7 | 112–127 | `E78263` salmon | Salmon ramp (112–119) / lavender-grey ramp (120–127) |
| 8 | 128–143 | `C6A642` gold | Gold ramp (128–135) / pink ramp `FF9694` (136–143) |
| 9 | 144–159 | `D62084` magenta | Magenta ramp (144–151) / grey ramp (152–159) |
| 10 | 160–175 | `004500` dark green | Dark-green ramp (160–164), then flat grey `636163` filler (165–175) |
| 11 | 176–191 | `8CF7E7` pale teal | Teal ramp (176–183) / blue ramp `004DA5` (184–191) |
| 12 | 192–207 | `399EBD` blue | Blue ramp (192–199) / green ramp `317129` (200–207) |
| 13 | 208–223 | `004DC6` blue | Blue ramp (nearly flat) |
| 14 | 224–239 | `084D08` dark green | Dark-green ramp |
| 15 | 240–255 | `001442` navy | Reserved/special: blues, grey filler; ends `FFFFA5 DEEB8C C6D77B A5C36B` |

## 4. Research / Ufopaedia — `PAL_UFOPAEDIA` / `PAL_BATTLEPEDIA` · [image](images/4_ResearchPal.Png)

| Block | Indices | Start color | Contents |
|---|---|---|---|
| 0 | 0–15 | `000000` (transparent) | Cream `FFFFE7` → peach/orange `FF9A4A` ramp |
| 1 | 16–31 | `FFEB00` yellow | Yellow → orange-red ramp |
| 2 | 32–47 | `FFFFEF` off-white | Off-white → grey-green ramp |
| 3 | 48–63 | `84968C` grey-green | Grey / slate ramp → dark |
| 4 | 64–79 | `EFFF7B` pale lime | Lime / green ramp → dark green |
| 5 | 80–95 | `EFEFFF` lavender-white | Lavender / periwinkle ramp |
| 6 | 96–111 | `6B5D8C` muted purple | Purple ramp → very dark |
| 7 | 112–127 | `FFD3FF` pale pink | Pink → magenta-purple ramp |
| 8 | 128–143 | `CEEFFF` pale blue | Light-blue ramp → medium blue |
| 9 | 144–159 | `39659C` medium blue | Blue ramp → dark navy (continues block 8 darker) |
| 10 | 160–175 | `FFDFD6` pale pink | Pink / rose ramp → dark rose |
| 11 | 176–191 | `FFE7BD` cream | Cream / tan ramp |
| 12 | 192–207 | `D6B66B` tan | Tan / brown ramp |
| 13 | 208–223 | `A53800` rust-red | **Split**: rust-red→dark red (208–215), pale green `CEFBC6`→green (216–223) |
| 14 | 224–239 | `F7DFCE` cream | Cream → brown ramp (**window-bg block**) |
| 15 | 240–255 | `9C96BD` muted purple | Reserved/special: purples, teals, `FF00FF` magenta markers; ends `FFFFA5 DEEB8C C6D77B A5C36B` |

## 5. Tactical / Battlescape — `PAL_BATTLESCAPE` · [image](images/5_BattleScapePal.Png)

| Block | Indices | Start color | Contents |
|---|---|---|---|
| 0 | 0–15 | `000000` (transparent) | Greyscale ramp `FFFFFF`→black |
| 1 | 16–31 | `FFD300` amber | Amber / orange ramp → dark brown-red |
| 2 | 32–47 | `FF797B` salmon | Salmon / red ramp |
| 3 | 48–63 | `A5E384` light green | Green ramp → dark green |
| 4 | 64–79 | `D6E763` yellow-green | Yellow-green ramp → dark |
| 5 | 80–95 | `FFFBFF` white | Warm grey / brown ramp |
| 6 | 96–111 | `F7C74A` gold | Gold / brown ramp |
| 7 | 112–127 | `8CB2CE` blue-grey | Blue-grey ramp → dark navy |
| 8 | 128–143 | `ADD3F7` sky blue | Sky-blue ramp → dark navy |
| 9 | 144–159 | `FFFF7B` pale yellow | Yellow / brown ramp |
| 10 | 160–175 | `BDA25A` tan | Tan / brown ramp |
| 11 | 176–191 | `FFDFD6` pale pink | Pink / rose ramp |
| 12 | 192–207 | `DEC7FF` lavender | Lavender / purple ramp |
| 13 | 208–223 | `42C7FF` bright sky | Bright blue ramp → dark blue |
| 14 | 224–239 | `EFEFFF` lavender-white | Lavender / blue-grey ramp (**window-bg block**) |
| 15 | 240–255 | (greyscale) | **Replaced in battle by the [tactical greyscale](#tactical-greyscale-replaces-240255-in-battle)** `8C9694`→`000000` |

---

# TFTD palettes

TFTD uses its own palettes — the **same index is a different color** than in UFO. Edit
`bin/standard/xcom2/interfaces.rul` against these, not the UFO tables.

## 1. GeoScape World · [image](images/TFTD_Pal1.png)

| Block | Indices | Start color | Contents |
|---|---|---|---|
| 0 | 0–15 | `000000` (transparent) | **Fixed accents**: white, greys, blue-greys, navy `101463`, reds |
| 1 | 16–31 | `9CAE00` olive-yellow | Olive ramp → dark olive |
| 2 | 32–47 | `7B6D08` olive-brown | Olive-brown ramp → dark |
| 3 | 48–63 | `9CC3D6` light blue | Light-blue → dark-navy ramp |
| 4 | 64–79 | `63B2CE` cyan-blue | Cyan-blue ramp → dark navy |
| 5 | 80–95 | `5AC763` green | **Split**: green (80–86), blues `5AB2DE` (87–95) |
| 6 | 96–111 | `3179B5` blue | Blue ramp → dark navy |
| 7 | 112–127 | `000000` (transparent) | **Fixed accents** (pink, orange, white, greens, cyans, yellows, reds) — UFO-pal1-block-0 style |
| 8 | 128–143 | `CED7AD` pale green-grey | Mixed terrain accents (greens, oranges, teals) |
| 9 | 144–159 | `FFFF7B` pale yellow | Yellow ramp + orange/purple accents |
| 10 | 160–175 | `081C39` dark blue | Dark-blue ramp (rises to `0875AD` then back) |
| 11 | 176–191 | `B55D00` orange-brown | Orange-brown ramp → black |
| 12 | 192–207 | `1055CE` blue | Blue ramp → very dark |
| 13 | 208–223 | `104D8C` blue | Blue ramp → black |
| 14 | 224–239 | `ADFF00` lime | Lime / green ramp |
| 15 | 240–255 | `ADDBE7` pale blue | Reserved/special: blues, greens; ends `FFFFA5 DEEB8C C6D77B A5C36B` |

## 2. BaseScape / Research · [image](images/TFTD_Pal2.png)

| Block | Indices | Start color | Contents |
|---|---|---|---|
| 0 | 0–15 | `000000` (transparent) | Mixed: pink-grey ramp → purple `39144A`, reds, `FF00FF` |
| 1 | 16–31 | `FFFFC6` cream | Cream → olive-green ramp |
| 2 | 32–47 | `395539` dark green | **Split**: dark green (32–40), cream `FFF3BD`→ (41–47) |
| 3 | 48–63 | `E7EFB5` pale green | Pale green → dark teal ramp |
| 4 | 64–79 | `FFF3BD` cream | Cream → red ramp |
| 5 | 80–95 | `FFFF94` pale yellow | **Split**: yellow-green (80–85), pale cyan `E7FFFF` (86–95) |
| 6 | 96–111 | `5AA2A5` teal | Teal ramp → dark teal |
| 7 | 112–127 | `A5CFFF` light blue | Light-blue ramp → dark navy |
| 8 | 128–143 | `CEFFFF` pale cyan | Cyan → blue ramp |
| 9 | 144–159 | `FF86FF` pink | Pink/magenta → purple ramp |
| 10 | 160–175 | `FFF7C6` cream | Cream/gold → brown ramp |
| 11 | 176–191 | `F7BAA5` peach | Peach/orange → brown ramp |
| 12 | 192–207 | `F7FFFF` white | Teal-grey ramp |
| 13 | 208–223 | `EFB694` tan | Tan / brown ramp |
| 14 | 224–239 | `CE7994` rose | Rose → slate ramp (**window-bg block**) |
| 15 | 240–255 | `F7FBFF` white | Periwinkle/blue ramp → navy (reserved) |

## 3. Graphs · [image](images/TFTD_Pal3.png)

Structurally the same split-ramp layout as the **UFO Graphs palette** (blocks 0–13 are
identical); only the tail differs:

| Block | Indices | Start color | Contents |
|---|---|---|---|
| 0–13 | 0–223 | — | **Same as [UFO Graphs](#3-graphs--pal_graphs--image)** |
| 14 | 224–239 | `C6AADE` lavender | Lavender → dark-purple ramp (differs from UFO graphs' dark-green) |
| 15 | 240–255 | `001442` navy | Blues + greys; ends `FFFFA5 DEEB8C C6D77B A5C36B` |

## TFTD Tactical palettes (depth variants)

TFTD has **four** tactical palettes (stored in `D0…D3.LBM`) — land + three water depths — all
sharing the **same block layout**; deeper variants tint the ramps progressively bluer and
desaturate them (the deep-water tint converges on `00288C` / `082463`). Block 15 stays a clean
cyan ramp at every depth. Images:
[Tact1 land](images/TFTD_Tact1.png) · [Tact2 shallow](images/TFTD_Tact2.png) ·
[Tact3 medium](images/TFTD_Tact3.png) · [Tact4 deep](images/TFTD_Tact4.png).

Block layout (values shown for **Tact1 / Land & Inventory**):

| Block | Indices | Start color | Contents |
|---|---|---|---|
| 0 | 0–15 | `000000` (transparent) | **Fixed accents**: white, blue-greys, navy, reds |
| 1 | 16–31 | `FFFB08` yellow | Yellow → green ramp (deeper: → blue) |
| 2 | 32–47 | `FFFFC6` cream | Cream / khaki ramp |
| 3 | 48–63 | `FFFFFF` white | Greyscale ramp → `181818` |
| 4 | 64–79 | `9C7931` brown | Brown ramp → black |
| 5 | 80–95 | `BDFF42` lime | Lime / green ramp |
| 6 | 96–111 | `31FFB5` aqua-green | Aqua / teal ramp |
| 7 | 112–127 | `210000` near-black | Very dark / shadow ramp → black |
| 8 | 128–143 | `FFDB00` gold | Gold → green ramp |
| 9 | 144–159 | `FFFF7B` pale yellow | Yellow ramp + accents (shared with geoscape block 9) |
| 10 | 160–175 | `FFAE08` orange | Orange / amber ramp |
| 11 | 176–191 | `F75900` orange-red | Orange-red ramp → dark brown |
| 12 | 192–207 | `EFF7FF` white-blue | Blue-grey ramp |
| 13 | 208–223 | `EFA27B` peach | Tan / skin-tone ramp → dark brown |
| 14 | 224–239 | `F7A2BD` pink | Pink / rose ramp |
| 15 | 240–255 | `00FFFF` cyan | Cyan ramp → dark teal |

---

# BACKPALS.DAT — window-background palettes

`BACKPALS.DAT` holds **8 palettes of 16 colors** (128 colors, 384 bytes). In game each replaces
indices **224–239** (block 14) of the active main palette to tint bordered-window backgrounds,
load-screen backgrounds, and the terror-site announcement. Each is its own light→dark ramp; the
**first color is the lightest** (text/border start). Images:
[BackPals.Png](images/BackPals.Png) · [TFTD_BackPals.Png](images/TFTD_BackPals.Png).

## UFO BACKPALS

| Palette | Start color | Ramp |
|---|---|---|
| 0 | `6B7D42` | Olive / khaki → dark |
| 1 | `84797B` | Mauve-grey → dark brown |
| 2 | `DE6500` | Orange → dark red-purple |
| 3 | `BD8A7B` | Tan-pink → dark |
| 4 | `295D73` | Teal-blue → dark green |
| 5 | `C60400` | Red / crimson → dark purple |
| 6 | `CE8663` | Tan-orange → dark brown |
| 7 | `18598C` | Blue → dark indigo |

## TFTD BACKPALS

| Palette | Start color | Ramp |
|---|---|---|
| 0 | `ADFF00` | Lime → dark green |
| 1 | `00A69C` | Teal → dark |
| 2 | `D6867B` | Salmon → dark red |
| 3 | `E7E352` | Yellow / gold → dark brown |
| 4 | `5AB67B` | Green → dark green |
| 5 | `C6A652` | Gold → blue (warm→cool) |
| 6 | `3996FF` | Bright blue → dark |
| 7 | `C68E00` | Amber → dark red-brown |

---

# Tactical greyscale (replaces 240–255 in battle)

In the Battlescape, block 15 of the tactical palette is overwritten by this hard-coded 16-step
greyscale (it is not in any palette file). Light → dark, index 240 → 255. Image:
[5_BattleScapePal2.Png](images/5_BattleScapePal2.Png).

| Index | 240 | 241 | 242 | 243 | 244 | 245 | 246 | 247 | 248 | 249 | 250 | 251 | 252 | 253 | 254 | 255 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| RGB | `8C9694` | `848A8C` | `737D84` | `6B757B` | `5A696B` | `525D63` | `4A515A` | `394552` | `313842` | `293039` | `212431` | `181C21` | `101418` | `080C10` | `000408` | `000000` |

---

# See also

- [ui-palette-colors](../ui-palette-colors/SKILL.md) — how to *pick* a color for an
  `interfaces.rul` element or a C++ `setColor()` call (palette selection, pixel↔index math,
  widget bevel shading, reserved-index rules). That skill is the workflow; this one is the data.
- Source HTML this was generated from: [reference/PALETTES.DAT.htm](../../../reference/PALETTES.DAT.htm).
