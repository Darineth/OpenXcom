#!/usr/bin/env python3
"""Check OpenXcom DX inventory layouts for overlapping slots, labels and fixed UI.

Usage:  python check_layout.py <file.rul> [more.rul ...] [--layout ID] [--map]

  --map  also draw an ASCII picture of each layout (4px per character) so it can be eyeballed
         without launching the game. Slots are letters, their captions lowercase, fixed widgets '.'.

Exit code is 1 if any overlap was found, so this can gate a change.

Geometry is taken from the engine, not guessed:
  * cells are RuleInventory::SLOT_W/H = 16x16 px              (RuleInventory.h)
  * a section's label is drawn ABOVE it at y - fontHeight - spacing; FONT_SMALL
    is height 9 / spacing -1, so the label occupies the 8 rows above the slot
                                                       (Inventory::drawGridLabels)
  * the fixed widget rects below are read off InventoryState's constructor
"""
import sys
import os
import re

try:
    from font_metrics import load as load_font, text_width
except ImportError:  # invoked from elsewhere
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    from font_metrics import load as load_font, text_width

SLOT = 16
LABEL_DY = 9 - 1     # fontHeight + spacing(-1): label top sits at slotY - 8
LABEL_H = 8          # the Text widget is 9 tall, but a 9px glyph's last row is blank, so the
                     # INK occupies the 8 rows y-8..y-1. Using 9 reports a 1px sliver against
                     # the slot below every label.
CHAR_W = 8           # crude fallback only; real widths come from font_metrics (FONT_SMALL is
                     # variable-width, so len*8 over-estimates badly: "TURRET 2" is 40px, not 64).
FONT_DIR = "bin/common/Language"
SCREEN_W, SCREEN_H = 320, 200
MIN_GAP = 6          # px of blank space wanted between neighbouring sections. Not a correctness
                     # rule -- two sections can legally touch -- but abutting slots read as one
                     # block in game. Override with --gap N.

# Widgets a section may overlap without it being a defect.
#  * paperdoll - Mod auto-detects any slot overlapping it and disables that button's click
#    handlers (Mod.cpp ~L2355 / InventoryState ~L343), so the slots stay usable. NOTE the flag is
#    MOD-WIDE: one overlapping slot anywhere removes the paperdoll armour-swap shortcut for every
#    unit in the mod. The ARMOR button on the soldier info screen still works.
#  * position text - short text in a wide widget; see its entry below.
SOFT_UI = {"paperdoll", "position text"}

# InventoryState constructor: name, x, y, w, h
FIXED = [
    ("rank button",      0,   0,  26, 23),
    ("role badge",      28,   0,  23, 23),
    ("rank text",       53,   1, 184,  9),
    ("soldier name",    53,   9, 184, 17),
    ("OK button",      237,   1,  35, 22),
    ("prev button",    273,   1,  23, 22),
    ("next button",    297,   1,  23, 22),
    ("stats: weight",  245,  24,  70,  9),
    ("stats: line1",   245,  32,  70,  9),
    ("stats: TUs",     245,  40,  70,  9),
    ("stats: line2",   245,  48,  70,  9),
    ("stats: line3",   245,  56,  70,  9),
    ("stats: line4",   245,  64,  70,  9),
    ("stats: line5",   245,  72,  70,  9),
    ("stats: line6",   245,  80,  70,  9),
    ("stats: line7",   245,  88,  70,  9),
    ("unload button",  288,  64,  32, 25),
    ("paperdoll",       60,  65,  40, 70),
    # The widget is 70px wide, but it renders "Slot>12/14" -- 46px of ink. Reserving the full
    # widget needlessly walls off 24px of usable canvas, so model the realistic inked extent.
    ("position text",   65,  95,  50,  9),
    ("armor: front",   260,  96,  70,  9),
    ("armor: left",    260, 104,  70,  9),
    ("armor: right",   260, 112,  70,  9),
    ("armor: rear",    260, 120,  70,  9),
    ("armor: under",   260, 128,  70,  9),
    ("ground button",  289, 137,  32, 15),
    ("item name",      128, 140, 160,  9),
]


def overlaps(a, b):
    ax, ay, aw, ah = a
    bx, by, bw, bh = b
    return ax < bx + bw and bx < ax + aw and ay < by + bh and by < ay + ah


def clearance(a, b):
    """Blank px between two non-overlapping rects along their facing edge.

    Returns None when they are not neighbours (separated on BOTH axes, i.e. diagonal), since a
    diagonal gap is never visually confusing.
    """
    ax, ay, aw, ah = a
    bx, by, bw, bh = b
    xgap = bx - (ax + aw) if bx >= ax + aw else (ax - (bx + bw) if ax >= bx + bw else None)
    ygap = by - (ay + ah) if by >= ay + ah else (ay - (by + bh) if ay >= by + bh else None)
    if xgap is None and ygap is None:
        return None          # overlapping - handled elsewhere
    if xgap is not None and ygap is not None:
        return None          # diagonal neighbours
    return xgap if xgap is not None else ygap


def intersection(a, b):
    ax, ay, aw, ah = a
    bx, by, bw, bh = b
    x0, y0 = max(ax, bx), max(ay, by)
    x1, y1 = min(ax + aw, bx + bw), min(ay + ah, by + bh)
    return (x0, y0, x1 - x0, y1 - y0)


def parse(paths):
    """Minimal reader for the invs:, inventoryLayouts: and extraStrings: roots."""
    invs, layouts, strings = {}, {}, {}
    for path in paths:
        text = open(path, encoding="utf-8", errors="replace").read()
        root, cur, in_slots = None, None, False
        for line in text.splitlines():
            if re.match(r"^[a-zA-Z]", line):
                root = line.split(":")[0].strip()
                cur, in_slots = None, False
                continue
            if root == "invs":
                m = re.match(r"\s*- id:\s*(\S+)", line)
                if m:
                    cur = {"id": m.group(1), "x": 0, "y": 0, "type": 0,
                           "slots": [], "w": 0, "h": 0}
                    invs[cur["id"]] = cur
                    in_slots = False
                    continue
                if cur is None:
                    continue
                for key, dest in (("x", "x"), ("y", "y"), ("type", "type"),
                                  ("width", "w"), ("height", "h")):
                    m = re.match(r"\s+" + key + r":\s*(-?\d+)", line)
                    if m:
                        cur[dest] = int(m.group(1))
                        in_slots = False
                if re.match(r"\s+slots:", line):
                    in_slots = True
                elif re.match(r"\s+[a-zA-Z]\w*:", line):
                    in_slots = False
                if in_slots:
                    for m in re.finditer(r"\[\s*(\d+)\s*,\s*(\d+)\s*\]", line):
                        cur["slots"].append((int(m.group(1)), int(m.group(2))))
            elif root == "inventoryLayouts":
                m = re.match(r"\s*- id:\s*(\S+)", line)
                if m:
                    cur = m.group(1)
                    layouts[cur] = []
                    continue
                if cur is None:
                    continue
                m = re.match(r"\s+invs:\s*(?:&\S+\s*)?\[(.+)\]", line)
                if m:
                    layouts[cur] += [t.strip() for t in m.group(1).split(",") if t.strip()]
                    continue
                m = re.match(r"\s+-\s+(STR_\S+)", line)
                if m:
                    layouts[cur].append(m.group(1))
            else:
                # extraStrings:, or a plain language file whose root is a locale ("en-US:").
                # Label WIDTH depends on the translated text, so this must be loaded or every
                # label falls back to its (much longer) id and reports false positives.
                m = re.match(r'\s+(STR_\S+):\s*"?([^"]*)"?\s*$', line)
                if m and m.group(2):
                    strings.setdefault(m.group(1), m.group(2))
    return invs, layouts, strings


def section_rect(s):
    """Pixel rect a section occupies, or None if it has no fixed footprint.

    Sizes follow Inventory::drawGrid exactly:
      * INV_SLOT draws each CELL as SLOT_W+1 x SLOT_H+1 (the +1 is the shared border), so an
        mx x my section spans 16*mx + 1 px. That trailing border pixel is real estate -- a label
        placed immediately below such a section lands on it, which reads as the caption
        overlapping the slot's bottom edge.
      * hands / utility / equip (isSingleItem) draw one box of exactly boxW*16 x boxH*16, no +1.
    """
    t = s.get("type", 0)
    if t == 1:                       # INV_HAND - always a 2x3 box, drawn exactly
        return (s["x"], s["y"], 2 * SLOT, 3 * SLOT)
    if t in (3, 4):                  # INV_UTILITY / INV_EQUIP - rule box, default 2x2, exact
        return (s["x"], s["y"], (s["w"] or 2) * SLOT, (s["h"] or 2) * SLOT)
    if t == 2:
        # INV_GROUND tiles from its origin to the screen edge (drawGrid loops x<=320, y<=200),
        # so it owns EVERYTHING below its y. Its "GROUND" caption sits in the 8 rows above that,
        # which is the part a section creeping down the left column collides with first.
        return (s["x"], s["y"], SCREEN_W - s["x"], SCREEN_H - s["y"])
    if not s["slots"]:
        return None
    mx = max(c[0] for c in s["slots"]) + 1
    my = max(c[1] for c in s["slots"]) + 1
    return (s["x"], s["y"], mx * SLOT + 1, my * SLOT + 1)


def draw_map(secs, scale=4):
    """ASCII picture of a layout: slots as letters, captions lowercase, fixed widgets '.'."""
    cols, rows = SCREEN_W // scale, SCREEN_H // scale
    grid = [[" "] * cols for _ in range(rows)]

    def put(rect, ch, over=" "):
        x, y, w, h = rect
        for py in range(max(0, y), min(SCREEN_H, y + h)):
            for px in range(max(0, x), min(SCREEN_W, x + w)):
                cy, cx = py // scale, px // scale
                if 0 <= cy < rows and 0 <= cx < cols and grid[cy][cx] in (" ", over):
                    grid[cy][cx] = ch

    for _, fx, fy, fw, fh in FIXED:
        put((fx, fy, fw, fh), ".")
    key = []
    for i, (sid, label, r, lr) in enumerate(secs):
        ch = chr(ord("A") + i) if i < 26 else "?"
        put(lr, ch.lower(), ".")
        put(r, ch, ".")
        key.append("%s = %s (%r)" % (ch, sid, label))

    out = ["    +" + "-" * cols + "+"]
    for i, row in enumerate(grid):
        out.append("%3d |%s|" % (i * scale, "".join(row)))
    out.append("    +" + "-" * cols + "+")
    out.append("    " + "  ".join(key[:4]))
    for i in range(4, len(key), 4):
        out.append("    " + "  ".join(key[i:i + 4]))
    return "\n".join(out)


def fmt(r):
    return "x%d..%d y%d..%d (%dx%dpx)" % (r[0], r[0] + r[2] - 1, r[1], r[1] + r[3] - 1, r[2], r[3])


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    only = None
    min_gap = MIN_GAP
    if "--gap" in sys.argv:
        min_gap = int(sys.argv[sys.argv.index("--gap") + 1])
        if str(min_gap) in args:
            args.remove(str(min_gap))
    if "--layout" in sys.argv:
        only = sys.argv[sys.argv.index("--layout") + 1]
        if only in args:
            args.remove(only)
    if not args:
        print(__doc__)
        return 2

    invs, layouts, strings = parse(args)
    font = load_font(FONT_DIR)
    if not font:
        print("! font metrics unavailable (%s) - falling back to len*%d label widths"
              % (FONT_DIR, CHAR_W))
    total = 0
    for lid, ids in sorted(layouts.items()):
        if only and lid != only:
            continue
        print("\n=== %s ===" % lid)
        secs = []
        for sid in ids:
            s = invs.get(sid)
            if not s:
                continue
            r = section_rect(s)
            if not r:
                continue
            label = strings.get(sid, sid)
            secs.append((sid, label, r,
                         (s["x"], s["y"] - LABEL_DY, text_width(label, font, CHAR_W), LABEL_H)))

        found = []
        for i, (aid, alab, ar, alr) in enumerate(secs):
            for bid, blab, br, blr in secs[i + 1:]:
                if overlaps(ar, br):
                    found.append(("SLOT/SLOT  ", aid, bid, intersection(ar, br)))
                if overlaps(alr, br):
                    found.append(("LABEL/SLOT ", "%s label %r" % (aid, alab), bid, intersection(alr, br)))
                if overlaps(blr, ar):
                    found.append(("LABEL/SLOT ", "%s label %r" % (bid, blab), aid, intersection(blr, ar)))
                if overlaps(alr, blr):
                    found.append(("LABEL/LABEL", "%s %r" % (aid, alab), "%s %r" % (bid, blab), intersection(alr, blr)))
                # Legal but visually cramped: slots that abut read as one block.
                gap = clearance(ar, br)
                if gap is not None and gap < min_gap:
                    found.append(("TIGHT      ", aid, "%s (only %dpx apart)" % (bid, gap),
                                  intersection((ar[0] - 1, ar[1] - 1, ar[2] + 2, ar[3] + 2), br)))
            for name, fx, fy, fw, fh in FIXED:
                fr = (fx, fy, fw, fh)
                if overlaps(ar, fr):
                    kind = "SOFT/UI    " if name in SOFT_UI else "SLOT/UI    "
                    found.append((kind, aid, name, intersection(ar, fr)))
                if overlaps(alr, fr):
                    found.append(("LABEL/UI   ", "%s label %r" % (aid, alab), name, intersection(alr, fr)))
            if alr[1] < 0:
                found.append(("LABEL/EDGE ", aid, "above the top of the screen", alr))
            if ar[0] + ar[2] > SCREEN_W or ar[1] + ar[3] > SCREEN_H:
                found.append(("SLOT/EDGE  ", aid, "past the screen edge", ar))

        # SLOT/* collisions put two interactive regions on the same pixels -- always wrong.
        # LABEL/* collisions are cosmetic text bleed; the stock STR_STANDARD_INV has several
        # (e.g. "BACK PACK" reaching into the stats column), so they are warnings, and the
        # CHAR_W upper bound makes them slightly pessimistic. Fix errors, eyeball warnings.
        # With exact font metrics a label collision is a real, visible defect -- text printed over
        # another section's cells or over another caption -- so only LABEL/UI (bleeding across
        # static furniture, which stock does constantly) stays a warning.
        errors = [f for f in found
                  if f[0].startswith(("SLOT/SLOT", "SLOT/UI", "SLOT/EDGE",
                                      "LABEL/SLOT", "LABEL/LABEL", "LABEL/EDGE"))]
        warns = [f for f in found if f not in errors]
        if "--map" in sys.argv:
            print(draw_map(secs))
        if not found:
            print("  clean")
        for kind, a, b, r in errors:
            print("  ERROR  %s  %s  <->  %s   at %s" % (kind, a, b, fmt(r)))
        for kind, a, b, r in warns:
            print("  warn   %s  %s  <->  %s   at %s" % (kind, a, b, fmt(r)))
        print("  -- %d error(s), %d warning(s)" % (len(errors), len(warns)))
        total += len(errors)

    print("\n%d ERROR(s) total" % total)
    return 1 if total else 0


if __name__ == "__main__":
    sys.exit(main())
