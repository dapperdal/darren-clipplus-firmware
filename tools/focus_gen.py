#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Generate apps/focus_assets.c for the Clip+ focus shell.

Fonts are converted from the BDF files shipped in the fork's fonts/ directory
(Adobe/DEC Helvetica, permissive X11 notice retained in fonts/). Icons are
project-original and drawn here. Every bitmap is emitted in the LCD's native
format: 8-pixel-high rows, one byte per column, LSB at the top, which is what
lcd_mono_bitmap() consumes directly.

usage: focus_gen.py <rockbox-fork-dir> [preview.png]
"""
import sys
from pathlib import Path
from PIL import Image, ImageDraw

LATIN = list(range(32, 127)) + list(range(160, 256))
FONTS = [
    # name, bdf, codepoints
    ("huge", "27-Adobe-Helvetica-Bold.bdf", [ord(c) for c in " 0123456789.:-%+"]),
    ("title", "16-Adobe-Helvetica-Bold.bdf", LATIN),
    ("text", "12-Adobe-Helvetica-Bold.bdf", LATIN),
    ("small", "10-Adobe-Helvetica.bdf", LATIN),
]


def parse_bdf(path):
    glyphs = {}
    asc = desc = 0
    cur = None
    rows = None
    for line in path.read_text(errors="replace").splitlines():
        p = line.split()
        if not p:
            continue
        if p[0] == "FONT_ASCENT":
            asc = int(p[1])
        elif p[0] == "FONT_DESCENT":
            desc = int(p[1])
        elif p[0] == "STARTCHAR":
            cur = {}
        elif p[0] == "ENCODING":
            cur["cp"] = int(p[1])
        elif p[0] == "DWIDTH":
            cur["adv"] = int(p[1])
        elif p[0] == "BBX":
            cur["bbx"] = tuple(int(v) for v in p[1:5])
        elif p[0] == "BITMAP":
            rows = []
        elif p[0] == "ENDCHAR":
            cur["rows"] = rows
            glyphs[cur["cp"]] = cur
            cur = None
            rows = None
        elif rows is not None:
            rows.append(p[0])
    return asc, desc, glyphs


def glyph_cell(g, asc, height):
    """Return (width, [[0/1]*width]*height) for one glyph cell."""
    w, h, xo, yo = g["bbx"]
    left = min(xo, 0)
    width = max(g["adv"], xo + w) - left
    cell = [[0] * width for _ in range(height)]
    top = asc - yo - h
    for r, hexrow in enumerate(g["rows"]):
        bits = int(hexrow, 16)
        nbits = len(hexrow) * 4
        y = top + r
        if not 0 <= y < height:
            continue
        for c in range(w):
            if bits & (1 << (nbits - 1 - c)):
                x = xo - left + c
                if 0 <= x < width:
                    cell[y][x] = 1
    return width, cell


def pack(cell, width, height):
    out = []
    for page in range((height + 7) // 8):
        for x in range(width):
            b = 0
            for bit in range(8):
                y = page * 8 + bit
                if y < height and cell[y][x]:
                    b |= 1 << bit
            out.append(b)
    return out


def carr(name, ctype, values, per=16):
    lines = [f"static const {ctype} {name}[] = {{"]
    for i in range(0, len(values), per):
        lines.append("    " + ",".join(str(v) for v in values[i:i + per]) + ",")
    lines.append("};")
    return "\n".join(lines)


# ---------------------------------------------------------------- icons ----
def icon(size, fn):
    im = Image.new("1", size, 0)
    fn(ImageDraw.Draw(im))
    return im


def ic_music(d):
    d.ellipse((2, 15, 9, 21), fill=1)
    d.ellipse((14, 12, 21, 18), fill=1)
    d.rectangle((8, 4, 9, 18), fill=1)
    d.rectangle((20, 1, 21, 15), fill=1)
    d.polygon([(8, 4), (21, 1), (21, 6), (8, 9)], fill=1)


def ic_podcast(d):
    d.arc((1, 0, 22, 21), 205, 335, fill=1, width=2)
    d.arc((5, 4, 18, 17), 200, 340, fill=1, width=2)
    d.ellipse((9, 8, 14, 13), fill=1)
    d.polygon([(11, 13), (12, 13), (15, 23), (8, 23)], fill=1)


def ic_radio(d):
    d.line((6, 8, 17, 1), fill=1, width=2)
    d.rounded_rectangle((0, 8, 23, 22), radius=3, outline=1, width=2)
    d.ellipse((4, 11, 11, 18), outline=1, width=2)
    d.rectangle((14, 12, 20, 13), fill=1)
    d.rectangle((14, 16, 20, 17), fill=1)


def ic_record(d):
    d.rounded_rectangle((8, 0, 15, 13), radius=4, fill=1)
    d.arc((4, 3, 19, 18), 0, 180, fill=1, width=2)
    d.rectangle((11, 18, 12, 21), fill=1)
    d.rectangle((6, 21, 17, 22), fill=1)


def ic_playing(d):
    d.ellipse((0, 0, 23, 23), outline=1, width=2)
    d.polygon([(9, 6), (9, 17), (18, 11), (18, 12)], fill=1)


def ic_settings(d):
    for y, kx in ((4, 15), (11, 7), (18, 13)):
        d.rectangle((1, y, 22, y + 1), fill=1)
        d.ellipse((kx - 3, y - 3, kx + 4, y + 4), fill=0)
        d.ellipse((kx - 2, y - 2, kx + 3, y + 3), fill=1)


def ic_lock(d):
    d.arc((6, 1, 17, 14), 180, 360, fill=1, width=3)
    d.rectangle((6, 7, 8, 11), fill=1)
    d.rectangle((15, 7, 17, 11), fill=1)
    d.rounded_rectangle((3, 10, 20, 22), radius=2, fill=1)
    d.rectangle((11, 14, 12, 18), fill=0)


def ic_usb(d):
    d.rectangle((11, 3, 12, 20), fill=1)
    d.polygon([(11, 0), (12, 0), (15, 4), (8, 4)], fill=1)
    d.ellipse((9, 18, 14, 23), fill=1)
    d.line((12, 15, 17, 11), fill=1, width=2)
    d.rectangle((16, 7, 19, 10), fill=1)
    d.line((11, 17, 6, 13), fill=1, width=2)
    d.ellipse((4, 9, 8, 13), fill=1)


BIG_ICONS = [("music", ic_music), ("podcast", ic_podcast), ("radio", ic_radio),
             ("record", ic_record), ("playing", ic_playing),
             ("settings", ic_settings), ("lock", ic_lock), ("usb", ic_usb)]

SMALL_ICONS = {
    "play": ["#.......", "###.....", "#####...", "#######.", "#####...",
             "###.....", "#......."],
    "pause": ["##..##.", "##..##.", "##..##.", "##..##.", "##..##.",
              "##..##.", "##..##."],
    "stop": ["#######", "#######", "#######", "#######", "#######",
             "#######", "#######"],
    "folder": ["###......", "#########", "#.......#", "#.......#", "#.......#",
               "#.......#", "#########"],
    "note": ["...####", "...#..#", "...#..#", "...#..#", ".###.##",
             "####.##", ".##...."],
    "shuffle": ["......#.", "##...###", "..#.#.#.", "...#....", "..#.#.#.",
                "##...###", "......#."],
    "repeat": [".#####..", "#.....#.", "#....###", "......#.", "###....#",
               ".#.....#", "..#####."],
    "one": ["#", "#", "#", "#", "#"],
    "lock": [".###.", "#...#", "#...#", "#####", "##.##", "##.##", "#####"],
    "star": ["...#...", "...#...", "#######", ".#####.", ".#####.",
             "###.###", "#.....#"],
    "bolt": ["..##", ".##.", "####", ".##.", "##..", "#..."],
    "moon": [".###..", "##....", "##....", "##....", "###..#", ".####."],
    "chev_r": ["#..", ".#.", "..#", ".#.", "#.."],
    "chev_u": ["..#..", ".#.#.", "#...#"],
    "chev_d": ["#...#", ".#.#.", "..#.."],
    "mute": ["..#.....", ".##.#..#", "###..##.", "###..##.", ".##.#..#",
             "..#....."],
    "stereo": [".##..##.", "#..##..#", "#..##..#", "#..##..#", ".##..##."],
    "rec": [".###.", "#####", "#####", "#####", ".###."],
}


def ascii_icon(rows):
    w = max(len(r) for r in rows)
    im = Image.new("1", (w, len(rows)), 0)
    for y, r in enumerate(rows):
        for x, ch in enumerate(r):
            if ch == "#":
                im.putpixel((x, y), 1)
    return im


def im_cell(im):
    w, h = im.size
    return w, h, [[1 if im.getpixel((x, y)) else 0 for x in range(w)]
                  for y in range(h)]


# ----------------------------------------------------------------- main ----
def main():
    fork = Path(sys.argv[1])
    out_c = fork / "apps" / "focus_assets.c"
    out = ["/* SPDX-License-Identifier: GPL-2.0-or-later */",
           "/* GENERATED by tools/focus_gen.py - do not edit by hand.",
           " * Font bitmaps derive from the Adobe/DEC Helvetica BDF fonts in",
           " * fonts/ (notice retained there); icons are project-original. */",
           '#include "focus_ui.h"', ""]
    preview = []
    for name, bdf, cps in FONTS:
        asc, desc, glyphs = parse_bdf(fork / "fonts" / bdf)
        height = asc + desc
        index = [255] * 256
        widths, offsets, bits = [], [], []
        n = 0
        for cp in cps:
            g = glyphs.get(cp)
            if g is None:
                continue
            width, cell = glyph_cell(g, asc, height)
            if name == "huge" and chr(cp).isdigit():
                # tabular digits keep big numbers from jittering
                pad = 16 - width
                if pad > 0:
                    lp = pad // 2
                    cell = [[0] * lp + row + [0] * (pad - lp) for row in cell]
                    width = 16
            index[cp] = n
            widths.append(width)
            offsets.append(len(bits))
            bits.extend(pack(cell, width, height))
            preview.append((name, cp, width, height, cell))
            n += 1
        out.append(carr(f"{name}_index", "unsigned char", index))
        out.append(carr(f"{name}_widths", "unsigned char", widths))
        out.append(carr(f"{name}_offsets", "unsigned short", offsets))
        out.append(carr(f"{name}_bits", "unsigned char", bits))
        out.append(f"const struct focus_font focus_font_{name} = {{ {height}, "
                   f"{name}_index, {name}_widths, {name}_offsets, {name}_bits }};")
        out.append("")

    icons = []
    for name, fn in BIG_ICONS:
        icons.append((name.upper(), icon((24, 24), fn)))
    for name, rows in SMALL_ICONS.items():
        icons.append(("S_" + name.upper(), ascii_icon(rows)))
    enum = []
    table = []
    for name, im in icons:
        w, h, cell = im_cell(im)
        out.append(carr(f"icon_{name.lower()}", "unsigned char", pack(cell, w, h)))
        enum.append(f"    FOCUS_ICON_{name},")
        table.append(f"    {{ {w}, {h}, icon_{name.lower()} }},")
        preview.append(("icon", name, w, h, cell))
    out.append("const struct focus_icon focus_icons[] = {")
    out.extend(table)
    out.append("};")
    out_c.write_text("\n".join(out) + "\n")

    hdr = fork / "apps" / "focus_icons.h"
    hdr.write_text("/* SPDX-License-Identifier: GPL-2.0-or-later */\n"
                   "/* GENERATED by tools/focus_gen.py - do not edit by hand. */\n"
                   "#ifndef FOCUS_ICONS_H\n#define FOCUS_ICONS_H\nenum focus_icon_id {\n"
                   + "\n".join(enum) + "\n    FOCUS_ICON_COUNT\n};\n#endif\n")

    if len(sys.argv) > 2:
        scale = 4
        sheet = Image.new("L", (1100, 3000), 0)
        x = y = 4
        rowh = 0
        last = None
        for kind, _, w, h, cell in preview:
            if kind != last or x + w * scale > 1090:
                x = 4
                y += rowh + 8
                rowh = 0
                last = kind
            for yy in range(h):
                for xx in range(w):
                    if cell[yy][xx]:
                        for a in range(scale):
                            for b in range(scale):
                                sheet.putpixel((x + xx * scale + a, y + yy * scale + b), 255)
            x += w * scale + (8 if kind == "icon" else 0)
            rowh = max(rowh, h * scale)
        sheet.crop((0, 0, 1100, min(3000, y + rowh + 8))).save(sys.argv[2])
    print(f"wrote {out_c} ({out_c.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
