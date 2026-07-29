---
name: inventory-layout-geometry
description: Place inventory sections (`invs:` / `inventoryLayouts:`) without overlapping other slots, their labels, or the inventory screen's fixed widgets. Use when authoring or editing an inventory layout, adding a slot to one, choosing x/y for a section, or debugging text drawn over slots / slots drawn over the stats column in the inventory screen. Includes a checker script that reports collisions from a .rul file.
---

# Inventory layout geometry

The inventory screen is a fixed 320×200 canvas that is **already crowded** before a mod adds
anything. Sections are positioned by absolute pixel `x`/`y`, each one silently draws a **label in
the 8 rows above itself**, and nothing in the engine checks for collisions — a bad layout simply
renders on top of the stats panel or the soldier's name.

**Always run the checker before considering a layout done:**

```
python .claude/skills/inventory-layout-geometry/check_layout.py <layout.rul> [lang.yml ...]
python .claude/skills/inventory-layout-geometry/check_layout.py <layout.rul> --layout STR_MY_LAYOUT
```

**Pass every file the layout depends on**, not just your own:

- the **base `inventories.rul`** — layouts reference shared sections (`STR_GROUND`, `STR_BELT`, …)
  that live there. A section the checker can't resolve is silently skipped, so leaving this out
  hides real collisions. Forgetting it is how the ground slot's caption went unnoticed once already.
- the **language file(s)** — label width depends on the *translated* string. Without them every
  label falls back to its much longer id and reports false positives.

```
python check_layout.py my.rul bin/standard/xcom1/inventories.rul bin/standard/xcom1/Language/en-US.yml
```

Add **`--gap N`** to change the minimum clearance wanted between neighbouring sections
(default **6px**). Add **`--map`** to also draw an ASCII picture of the layout (4px per character) — slots as capital
letters, their captions lowercase, fixed widgets as `.`. That is the fast way to sanity-check that a
layout *reads* well before looking at it in game; the numbers tell you it is legal, the map tells you
it is sensible. (The map lets slots overwrite furniture, so use the error list for correctness.)

Exit code is 1 if any ERROR was found, so it can gate a change.

## The three things that collide

1. **Slot cells** — `x, y` plus the extent of the `slots:` grid, at `SLOT_W/H` = **16×16 px** per
   cell. A hand (`type: 1`) is always a 2×3 box; utility/equip (`type: 3`/`4`) use `width`/`height`
   (default 2×2).

   ⚠️ **A grid section is one pixel bigger than the arithmetic suggests.** `Inventory::drawGrid`
   draws every `INV_SLOT` cell as `SLOT_W + 1` × `SLOT_H + 1` — the +1 is the border shared with the
   next cell — so an `mx × my` section spans **`16·mx + 1`** px, not `16·mx`. A single-cell row at
   y34 therefore ends at **y50**, not y49. Put the next row's label at y50 and it lands exactly on
   that border, which reads in game as *"the caption overlaps the bottom pixel of the slot above"*.
   Hands, utility and equip boxes are drawn exactly (no +1).
2. **Slot labels** — `Inventory::drawGridLabels` draws each section's name at
   `y - fontHeight - spacing`. `FONT_SMALL` is height 9 / spacing −1, so the label occupies the
   **8 rows immediately above the slot**, starting at the section's `x`.

   Width is **measured, not guessed**: `font_metrics.py` reads `FontSmall.png` and replicates
   `Font::init` + `Font::getCharSize` (glyph width = ink extent in its 8×9 cell, advance = that
   + spacing). FONT_SMALL is variable-width, so `len × 8` over-estimates badly:

   | Label | real | `len × 8` |
   |---|---|---|
   | `TURRET` | 32px | 48px |
   | `TURRET 2` | **40px** | 64px |
   | `AMMO` | 22px | 32px |
   | `ENGINE` | 29px | 48px |
   | `LEFT SHOULDER` | 64px | 104px |

   A caption is routinely **wider than the slot it names** — "TURRET 2" is 40px over a 32px hand —
   so it reaches into whatever sits to its right.
3. **Clearance.** Two sections may legally touch, but abutting slots read as **one block** in game
   — the 1px grid borders merge and the player can't tell where one ends. Leave a blank gap
   (default check: **6px**; aim for 8+ where space allows — stock manages it everywhere because it
   has far fewer sections competing for the canvas).
4. **Fixed widgets** — `InventoryState`'s constructor. These never move:

| Region | Rect (x, y, w, h) |
|---|---|
| rank button / role badge | 0,0,26,23 · 28,0,23,23 |
| rank text | 53,1,184,9 |
| soldier name | 53,9,184,17 |
| OK / prev / next buttons | 237,1,35,22 · 273,1,23,22 · 297,1,23,22 |
| **stats column** (weight, TUs, 7 stat lines) | **245,24 → 245,88**, each 70×9 → occupies **x245–314, y24–96** |
| unload button | 288,64,32,25 |
| **paperdoll** *(soft)* | **60,65,40,70** |
| "SLOT n/m" text *(soft)* | 65,95,**50**,9 — the widget is 70px, but it renders `Slot>12/14` = 46px of ink. Reserving the whole widget needlessly walls off 24px of usable canvas. |
| armor readouts (front/left/right/rear/under) | 260,96 → 260,128, each 70×9 |
| ground button | 289,137,32,15 |
| item name | 128,140,160,9 |
| **ground slot** | **x0, y152 → fills to the screen edge**, and like any section it draws a **"GROUND" caption in the 8 rows above**, i.e. **y144–151**. A section reaching below y143 on the left collides with that caption long before it reaches the ground cells themselves. |

## Where a custom section can actually go

Subtract all of the above and you are left with two usable strips:

```
        0     32   60      99  128   160        192      244  260  320
      0 +--[rank]--[role]--[ name / rank text ]------[ OK ][prev][nxt]
     26 |  ....... row 1 labels .......                     | stats  |
     34 |  [ ROW 1: full width x0..244 is free ]            | column |
     50 |                                                   | x245.. |
     55 |  ... row 2 labels ...                             | ..314  |
     63 |  [hand]        PAPER      [hand]      [ engine ]  | y24..96|
        |  x0..31        DOLL       x128..159   x192..240   +--------+
    110 |  +----+        60..99     +-------+   |        |    armor  |
        |                y65..134                           260..319 |
        |          ("SLOT n/m" x65..134 y95..103)           y96..136 |
    140 |            [ item name 128..287 ]                          |
    144 |  ..GROUND caption y144..151..                               |
    152 +--------------- ground slot ---------------------------------+
```

- **Above y65** the full width `x0..244` is free — the best place for a row of small sections. The
  header ends at y25 and a label needs 8px, so **the topmost slot row is y34**.
- **Below y65** the paperdoll owns `x60..99` and the "SLOT n/m" text reaches x134, so the usable
  columns are roughly `x0..59`, `x100..127` and `x136..244` — plus the two hand positions, which
  straddle those gaps by design.
- **Never put a section at x≥245** — that is the stats column, and a slot there sits under live text.
  This is the single most common mistake, and it is invisible until you look at a real screenshot.

## Rules of thumb

- **First slot row is y34.** Anything higher puts its label into the soldier-name field.
- **Vertical pitch between stacked rows is `16·rows + 1 + 8`.** For a single-cell row that is
  **25px**, not 24 — the +1 border pixel is the part everyone forgets. A 1-cell row at y34 spans
  y34..y50, so the next row's label starts at y51 and its slots at **y59**. Row 2 at y58 puts the
  label on the border line.
- **Horizontal pitch is limited by label width, not slot width.** Two 2-cell slots 40px apart will
  bleed their labels into each other if the labels are longer than ~5 characters. Either space the
  origins ≥48px apart or accept the warning.
- **Keep labels short — ideally narrower than the slot they name.** A caption is drawn from the
  section's `x` and routinely overhangs: "TURRET 2" is 40px over a 32px mount, so it eats 8px of the
  neighbour's space *before* you even add a gap. Renaming the two vehicle mounts "GUN 1"/"GUN 2"
  (22/23px) was what finally let the weapon row close with clearance on both sides.
- **Budget a row before placing it.** Sum the slot widths, add `gap × (n − 1)`, and compare against
  the usable span. The vehicle weapon row is x120..244 = 125px for 33 + 32 + 32 = 97px of slots,
  leaving 28px — comfortably two 8px+ gaps.
- **Check whether a "constraint" is real before designing around it.** The same row was first built
  at x136 because the "SLOT n/m" *widget* is 70px wide; its text is only 46px, and reclaiming that
  difference was what let the longer `TURRET`/`TURRET 2` captions fit with clearance. Measure the
  ink, not the box.
- **Vertical gaps are consumed by the lower row's caption** (which is 8px tall and lives in exactly
  that space); horizontal gaps are genuine blank pixels. So 8px vertical and 8px horizontal look
  quite different — the vertical one is full of text.
- **The bottom of the screen is closer than it looks.** The ground slot starts at y152 but its
  caption starts at **y144**, so a left-column section must end by y143. There is no room down there
  for a section taller than one cell, which is why tall sections (an ammo rack that has to hold a
  1×3 rocket clip, say) belong on the weapon row rather than tucked underneath it.

## Severity

- **warn — `TIGHT`.** Sections closer than the minimum gap. Legal, but they visually merge.
- **ERROR — anything touching a slot, and any caption collision.** `SLOT/SLOT`, `SLOT/UI`,
  `SLOT/EDGE`, `LABEL/SLOT`, `LABEL/LABEL`, `LABEL/EDGE`. Because label widths are exact, a reported
  caption collision is a real visible defect — text printed over another section's cells or over
  another caption. Fix all of these.
- **warn — `SOFT/UI`.** Overlapping a *soft* widget. Two qualify:
  - **paperdoll** — `Mod` auto-detects any slot overlapping it and disables that button's click
    handlers, so the slots stay usable. ⚠️ The flag is **mod-wide**: one overlapping slot anywhere
    removes the paperdoll armour-swap shortcut for *every* unit in the mod (the ARMOR button on the
    soldier info screen still works). Fine for a vehicle-focused mod, rude in a general one.
  - **"SLOT n/m" text** — modelled at its real inked width, so overlap here means real overlap.
- **warn — `LABEL/UI`.** A caption bleeding across static furniture. Stock does this
  constantly, and it is usually invisible because the furniture there is empty background.

Useful baseline: **stock `STR_STANDARD_INV` reports 0 errors.** (It appears to clash with the
"SLOT n/m" text only if you reserve that widget's full 70px rather than its 46px of ink — a good
reminder that an over-reserved widget invents constraints that do not exist.) **A new layout should
reach 0 errors**; the three vehicle layouts in `dx-test` do, at an 8px minimum gap.

## Keep weapons where hands are

Players build muscle memory from the soldier screen, so a chassis' weapon mounts should sit where
hands do: **x0, y63** (right hand) and **x128, y63** (left hand). Two hands per layout is the
engine maximum, which is exactly what a twin-turret vehicle needs.

Note that the stock left hand at x128 clips the "SLOT n/m" text (x65..134) by 7px — a long-standing
upstream quirk every soldier screen already shows. **Putting the second mount at x136 instead keeps
it visually on the weapon row while clearing that text entirely**, which is what the vehicle layouts
do. x0 for the first mount is unambiguous; only the second one has to dodge.

## Worked example

`bin/standard/dx-test/dx-test-vehicles.rul` has three vehicle layouts built against these rules:

- three armor facings across the top row at **y34**, the fourth at **y59** on the left,
- the **engine bay top-right** at x196, y34 (3×3, x196..244 — one pixel clear of the stats column),
  because it is the widest block and the only place a 49px-wide section fits without fighting the
  weapon row,
- **armor hardpoints arranged where they physically sit** — FRONT above (x64), LEFT/RIGHT flanking
  (y59), REAR and UNDER below (y91/y116). REAR and UNDER stack on the left rather than centring
  under FRONT, because the paperdoll owns x60..99 from y65 down to y134: the centre column is only
  available *above* the paperdoll, which is why FRONT gets it.
- **one weapon row at y91** shared by every chassis: ammo rack at x120, then the mounts at x164 and
  x204 — 11px and 8px of blank between them.

All three report **0 errors, 0 warnings**.

Two lessons worth keeping:

- **When a row won't close, move the widest block to another band** rather than shaving pixels off
  everything. Turret 2 + rack + engine could not all fit right of x136 (109px available, ~114px
  needed), so the engine went to the top row and everything else fell into place.
- **Arrange related sections spatially where the furniture allows.** Armor facings read far better
  as front/left/right/rear/under placed the way they sit on the unit than as an arbitrary row. The
  paperdoll usually forces at least one compromise — decide which facing earns the centre column
  (the one above it, since below y65 that column is gone) and stack the rest deliberately.
- **Keep a family of layouts on the same rows.** Putting one chassis' rack 7px lower than another's
  to dodge a collision is a smell — it looks like a mistake in game even when it measures clean. If
  a section will not sit on the shared row, move it somewhere deliberate instead of nudging it.
