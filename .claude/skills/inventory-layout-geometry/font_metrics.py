#!/usr/bin/env python3
"""Exact FONT_SMALL text widths, measured from the game's own font image.

`check_layout.py` uses this so label collisions are reported at their true size instead of a
`len(text) * 8` guess. Replicates Font::init + Font::getCharSize:

  * the font image is a grid of `width x height` cells (FONT_SMALL: 8x9), `imageW / width` per row,
    in the order given by the `chars:` string in Font.dat
  * a glyph's width is the INK extent inside its cell: (rightmost - leftmost non-zero pixel + 1)
  * the advance for a character is that width + the font's `spacing` (FONT_SMALL: -1)
  * so a string's width is the sum of its advances

Falls back to `len(text) * 8` if the font files can't be read, so the checker still runs anywhere.
"""
import os
import re
import struct
import zlib

_CACHE = {}


def _paeth(a, b, c):
    p = a + b - c
    pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
    return a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)


def _decode_png(path):
    """Minimal 8-bit indexed PNG decode -> (w, h, rows of palette indices)."""
    data = open(path, "rb").read()
    i, idat = 8, b""
    w = h = bd = ct = 0
    while i < len(data):
        ln = struct.unpack(">I", data[i:i + 4])[0]
        typ = data[i + 4:i + 8]
        if typ == b"IHDR":
            w, h, bd, ct = struct.unpack(">IIBB", data[i + 8:i + 18])[:4]
        elif typ == b"IDAT":
            idat += data[i + 8:i + 8 + ln]
        i += 12 + ln
    if bd != 8 or ct != 3:
        raise ValueError("expected 8-bit indexed PNG, got bitdepth=%d colortype=%d" % (bd, ct))
    raw = zlib.decompress(idat)
    rows, prev, pos = [], bytearray(w), 0
    for _ in range(h):
        f = raw[pos]; pos += 1
        line = bytearray(raw[pos:pos + w]); pos += w
        if f == 1:
            for x in range(1, w):
                line[x] = (line[x] + line[x - 1]) & 0xFF
        elif f == 2:
            for x in range(w):
                line[x] = (line[x] + prev[x]) & 0xFF
        elif f == 3:
            for x in range(w):
                line[x] = (line[x] + (((line[x - 1] if x else 0) + prev[x]) >> 1)) & 0xFF
        elif f == 4:
            for x in range(w):
                line[x] = (line[x] + _paeth(line[x - 1] if x else 0, prev[x],
                                            prev[x - 1] if x else 0)) & 0xFF
        rows.append(line)
        prev = line
    return w, h, rows


def _load_chars(font_dat, font_id, png_name):
    """The `chars:` folded scalar for one image of one font in Font.dat."""
    s = open(font_dat, encoding="utf-8").read()
    i = s.index("- id: %s" % font_id)
    blk = s[i:]
    j = blk.index("- file: %s" % png_name)
    k = blk.index("chars: >", j)
    out = ""
    for line in blk[k:].split("\n")[1:]:
        if not line.strip() or not line.startswith(" " * 10):
            break
        out += line.strip()
    return out


def load(common_dir, font_id="FONT_SMALL", png_name="FontSmall.png", cell=(8, 9), spacing=-1):
    """Returns {char: advance}. Cached per font id."""
    if font_id in _CACHE:
        return _CACHE[font_id]
    widths = {}
    try:
        chars = _load_chars(os.path.join(common_dir, "Font.dat"), font_id, png_name)
        w, h, rows = _decode_png(os.path.join(common_dir, png_name))
        cw, ch = cell
        per_row = w // cw
        for idx, c in enumerate(chars):
            sx, sy = (idx % per_row) * cw, (idx // per_row) * ch
            if sy + ch > h:
                break
            left = right = -1
            for x in range(sx, sx + cw):
                for y in range(sy, sy + ch):
                    if rows[y][x] != 0:
                        if left == -1:
                            left = x
                        right = x
                        break
            if left != -1:
                widths[c] = (right - left + 1) + spacing
        widths[" "] = (cw // 2) + spacing
    except Exception:
        return None
    _CACHE[font_id] = widths
    return widths


def text_width(text, widths, fallback=8):
    """Rendered width in px. `widths` may be None -> crude len*fallback upper bound."""
    if not widths:
        return len(text) * fallback
    return sum(widths.get(c, widths.get("?", fallback)) for c in text)


if __name__ == "__main__":
    import sys
    d = sys.argv[1] if len(sys.argv) > 1 else "bin/common/Language"
    wmap = load(d)
    if not wmap:
        print("could not load font metrics")
    else:
        print("%d glyphs measured" % len(wmap))
        for t in ("TURRET", "TURRET 2", "AMMO", "ENGINE", "FRONT", "LEFT", "RIGHT", "REAR",
                  "GROUND", "BACK PACK", "LEFT SHOULDER"):
            print("  %-15s %3d px   (len*8 = %3d)" % (t, text_width(t, wmap), len(t) * 8))
