#!/usr/bin/env python3
# EVERYTHING COUNTS - a card counting story for the Game Boy Advance
# Copyright (C) 2026 Godless-1
# SPDX-License-Identifier: AGPL-3.0-or-later
"""
Generates src/gen/gen_art.c -- master 256-colour palette plus the small
bitmaps that are awkward to draw procedurally (suit pips, corp sigils).
"""
import sys, os

PAL = [0] * 256

def rgb(r, g, b):
    return ((r >> 3) & 31) | (((g >> 3) & 31) << 5) | (((b >> 3) & 31) << 10)

def ramp(base, n, stops):
    """stops = [(t, (r,g,b)), ...] with t in 0..1"""
    for i in range(n):
        t = i / (n - 1.0)
        for k in range(len(stops) - 1):
            t0, c0 = stops[k]
            t1, c1 = stops[k + 1]
            if t0 <= t <= t1:
                f = 0.0 if t1 == t0 else (t - t0) / (t1 - t0)
                c = tuple(int(c0[j] + (c1[j] - c0[j]) * f + 0.5) for j in range(3))
                PAL[base + i] = rgb(*c)
                break

# ---- 0..15 : core UI ---------------------------------------------------
CORE = [
    (0x00, 0x00, 0x00),  # 0  void
    (0x05, 0x03, 0x0f),  # 1  near black violet
    (0x0b, 0x08, 0x20),  # 2  deep indigo
    (0x14, 0x10, 0x3a),  # 3  panel
    (0x1f, 0x1a, 0x55),  # 4  panel highlight
    (0x2c, 0x24, 0x78),  # 5  grid line
    (0x5b, 0x55, 0xa0),  # 6  dim text
    (0x9a, 0x94, 0xd8),  # 7  mid text
    (0xe6, 0xe3, 0xff),  # 8  bright text
    (0xff, 0xff, 0xff),  # 9  white
    (0x05, 0xd9, 0xe8),  # 10 CYAN
    (0x0a, 0x7d, 0x8a),  # 11 cyan dim
    (0xff, 0x2a, 0x6d),  # 12 MAGENTA
    (0x8f, 0x0f, 0x3c),  # 13 magenta dim
    (0xff, 0xb7, 0x03),  # 14 AMBER
    (0x39, 0xff, 0x14),  # 15 TOXIC GREEN
]
for i, c in enumerate(CORE):
    PAL[i] = rgb(*c)

# ---- gradient ramps (24 entries each) ----------------------------------
RAMP_SKY    = 16
RAMP_CYAN   = 40
RAMP_MAG    = 64
RAMP_GOLD   = 88
RAMP_GREEN  = 112
RAMP_VIOLET = 136
RAMP_ALERT  = 160
RAMP_CHROME = 184
RAMP_CARD   = 208

ramp(RAMP_SKY, 24, [(0.0, (0x04, 0x02, 0x12)), (0.45, (0x24, 0x08, 0x50)),
                    (0.75, (0x8a, 0x12, 0x6e)), (1.0, (0xff, 0x5a, 0x3c))])
ramp(RAMP_CYAN, 24, [(0.0, (0x01, 0x06, 0x0c)), (0.5, (0x03, 0x78, 0x8c)),
                     (0.85, (0x05, 0xd9, 0xe8)), (1.0, (0xd8, 0xff, 0xff))])
ramp(RAMP_MAG, 24, [(0.0, (0x0c, 0x01, 0x08)), (0.5, (0x8c, 0x0a, 0x36)),
                    (0.85, (0xff, 0x2a, 0x6d)), (1.0, (0xff, 0xd0, 0xe4))])
ramp(RAMP_GOLD, 24, [(0.0, (0x0c, 0x07, 0x00)), (0.5, (0x8a, 0x5a, 0x02)),
                     (0.85, (0xff, 0xb7, 0x03)), (1.0, (0xff, 0xf3, 0xc0))])
ramp(RAMP_GREEN, 24, [(0.0, (0x02, 0x0c, 0x02)), (0.5, (0x16, 0x8a, 0x0c)),
                      (0.85, (0x39, 0xff, 0x14)), (1.0, (0xd8, 0xff, 0xc8))])
ramp(RAMP_VIOLET, 24, [(0.0, (0x06, 0x02, 0x14)), (0.5, (0x46, 0x1a, 0xa8)),
                       (0.85, (0x8a, 0x5c, 0xff)), (1.0, (0xe4, 0xd8, 0xff))])
ramp(RAMP_ALERT, 24, [(0.0, (0x0e, 0x00, 0x00)), (0.5, (0x9c, 0x08, 0x08)),
                      (0.8, (0xff, 0x24, 0x18)), (1.0, (0xff, 0xc8, 0x60))])
ramp(RAMP_CHROME, 24, [(0.0, (0x08, 0x09, 0x10)), (0.35, (0x3a, 0x40, 0x55)),
                       (0.7, (0x9d, 0xa8, 0xc0)), (1.0, (0xf2, 0xf6, 0xff))])
# card body: near-black slate rising to a cold holo sheen
ramp(RAMP_CARD, 24, [(0.0, (0x07, 0x07, 0x11)), (0.4, (0x11, 0x12, 0x24)),
                     (0.75, (0x1c, 0x20, 0x3a)), (1.0, (0x33, 0x3c, 0x62))])

# ---- 232..247 : named accents ------------------------------------------
ACCENT = [
    (0xff, 0x7b, 0x00),  # 232 orange
    (0x2d, 0xe2, 0xe6),  # 233 ice
    (0xa0, 0x20, 0xf0),  # 234 purple
    (0x00, 0xff, 0xc8),  # 235 teal
    (0xff, 0x00, 0x3c),  # 236 blood
    (0xd1, 0xf7, 0xff),  # 237 pale
    (0x7a, 0x04, 0xeb),  # 238 deep violet
    (0xfe, 0x53, 0xbb),  # 239 candy pink
    (0x08, 0xf7, 0xfe),  # 240 laser cyan
    (0x09, 0xfb, 0xd3),  # 241 mint
    (0xf5, 0xd3, 0x00),  # 242 sodium
    (0x3b, 0x27, 0x70),  # 243 felt shadow
    (0x59, 0x1f, 0x8a),  # 244 felt mid
    (0x0d, 0x54, 0x5c),  # 245 felt teal
    (0x24, 0x0b, 0x36),  # 246 felt deep
    (0x6b, 0x00, 0x3a),  # 247 felt crimson
]
for i, c in enumerate(ACCENT):
    PAL[232 + i] = rgb(*c)

# ---- 248..255 : runtime-cycled animation slots -------------------------
for i in range(8):
    PAL[248 + i] = rgb(0x05, 0xd9, 0xe8)

# =======================================================================
#  Bitmaps.  '#' = solid, '+' = mid tone, ':' = dark tone, '.' = clear.
# =======================================================================
BITMAPS = {}

def bmp(name, rows):
    w = len(rows[0])
    for r in rows:
        assert len(r) == w, (name, r)
    BITMAPS[name] = rows

# SPIKE (spades) -> a breach dagger: blade, crossguard, pommel
bmp('pip_spike', [
    "....#....",
    "...#+#...",
    "...#+#...",
    "...#+#...",
    "#########",
    "#..#+#..#",
    "...#+#...",
    "...###...",
    "..#####..",
])
# PULSE (hearts) -> biomonitor heart with a trailing lead
bmp('pip_pulse', [
    ".##...##.",
    "#++#.#++#",
    "#+++++++#",
    "#++#+#++#",
    ".#+++++#.",
    "..#+++#..",
    "...#+#...",
    "....#....",
    "....#....",
])
# SHARD (diamonds) -> a faceted cred shard
bmp('pip_shard', [
    "....#....",
    "...#+#...",
    "..#+++#..",
    ".#+:+:+#.",
    "#++:+:++#",
    ".#+:+:+#.",
    "..#+++#..",
    "...#+#...",
    "....#....",
])
# WIRE (clubs) -> a three-node circuit trefoil
bmp('pip_wire', [
    "...###...",
    "..#:+:#..",
    "..#+++#..",
    "##..#..##",
    "#:+:#:+:#",
    "#+++#+++#",
    ".##+#+##.",
    "...###...",
    "..#####..",
])

# HALCYON sigil: an unblinking eye inside a hex. 21x15.
bmp('sigil_halcyon', [
    "....#############....",
    "...#:::::::::::::#...",
    "..#:::::#####:::::#..",
    ".#::::##+++++##::::#.",
    "#::::#+++###+++#::::#",
    "#:::#++##+++##++#:::#",
    "#:::#+#+++#+++#+#:::#",
    "#:::#+#+++#+++#+#:::#",
    "#:::#++##+++##++#:::#",
    "#::::#+++###+++#::::#",
    ".#::::##+++++##::::#.",
    "..#:::::#####:::::#..",
    "...#:::::::::::::#...",
    "....#############....",
    ".........###.........",
])

# Kestrel mark - the player's tag, a bird skull / count glyph. 16x13
bmp('sigil_kestrel', [
    "..###......###..",
    ".#+++#....#+++#.",
    "#++#++#..#++#++#",
    "#+####+##+####+#",
    ".#++++++++++++#.",
    "..#++##++##++#..",
    "...#+#++++#+#...",
    "....#++##++#....",
    ".....#+##+#.....",
    "......####......",
    ".......##.......",
    "......#..#......",
    ".....#....#.....",
])

def emit(out_dir):
    with open(os.path.join(out_dir, 'gen_art.c'), 'w') as f:
        f.write("/* generated by tools/mkart.py - do not edit */\n")
        f.write('#include "gba.h"\n\n')
        f.write("const u16 g_palette[256] = {\n")
        for i in range(0, 256, 8):
            f.write("    " + ",".join("0x%04X" % v for v in PAL[i:i+8]) + ",\n")
        f.write("};\n\n")
        for name, rows in sorted(BITMAPS.items()):
            h = len(rows)
            w = len(rows[0])
            f.write("const u8 %s[%d] = { /* %dx%d */\n" % (name, w * h, w, h))
            lut = {'.': 0, ':': 1, '+': 2, '#': 3}
            for r in rows:
                f.write("    " + ",".join(str(lut[c]) for c in r) + ",\n")
            f.write("};\n")
    with open(os.path.join(out_dir, 'gen_art.h'), 'w') as f:
        f.write("/* generated by tools/mkart.py */\n#ifndef GEN_ART_H\n#define GEN_ART_H\n")
        f.write('#include "gba.h"\n\nextern const u16 g_palette[256];\n\n')
        for name, rows in sorted(BITMAPS.items()):
            f.write("extern const u8 %s[%d];\n" % (name, len(rows) * len(rows[0])))
            f.write("#define %s_W %d\n#define %s_H %d\n" % (name.upper(), len(rows[0]),
                                                            name.upper(), len(rows)))
        f.write("""
/* palette ramp bases (24 entries each, dark -> bright) */
#define RAMP_LEN     24
#define RAMP_SKY     16
#define RAMP_CYAN    40
#define RAMP_MAG     64
#define RAMP_GOLD    88
#define RAMP_GREEN   112
#define RAMP_VIOLET  136
#define RAMP_ALERT   160
#define RAMP_CHROME  184
#define RAMP_CARD    208

/* named colour indices */
#define C_VOID    0
#define C_BLACK   1
#define C_INDIGO  2
#define C_PANEL   3
#define C_PANELHI 4
#define C_GRID    5
#define C_DIM     6
#define C_MID     7
#define C_TEXT    8
#define C_WHITE   9
#define C_CYAN    10
#define C_CYAND   11
#define C_MAG     12
#define C_MAGD    13
#define C_AMBER   14
#define C_GREEN   15
#define C_ORANGE  232
#define C_ICE     233
#define C_PURPLE  234
#define C_TEAL    235
#define C_BLOOD   236
#define C_PALE    237
#define C_DVIOLET 238
#define C_CANDY   239
#define C_LASER   240
#define C_MINT    241
#define C_SODIUM  242
#define C_FELT_SH 243
#define C_FELT_MD 244
#define C_FELT_TL 245
#define C_FELT_DP 246
#define C_FELT_CR 247
#define C_ANIM0   248
#endif
""")
    print("mkart: palette + %d bitmaps" % len(BITMAPS))

if __name__ == '__main__':
    emit(sys.argv[1] if len(sys.argv) > 1 else 'src/gen')
