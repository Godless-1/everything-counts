#!/usr/bin/env python3
# EVERYTHING COUNTS - a card counting story for the Game Boy Advance
# Copyright (C) 2026 Godless-1
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Button-glyph readability check, run by `make test`.

Reads the glyph bitmaps from tools/mkfont.py and the hint strings from src/*.c
and fails when
  - a hint uses a code that has no glyph,
  - a drawn glyph reaches the right gutter column or the bottom gutter row,
  - two button glyphs (A, B, L, R, d-pad, the four arrows) are less than 3 pixels apart (two cut-out letters differ by that much),
  - a button glyph is less than 4 pixels away from any letter or digit it could be read as,
  - the A, B, L and R glyphs do not have their letter cut out of a solid shape,
  - an arrow or d-pad is not centred between the top and bottom of the capital letters.
"""
import re, sys, glob
src = open('tools/mkfont.py').read()
G = {}
for m in re.finditer(r'^g\((0x[0-9A-Fa-f]+),((?:\s*"[.#]{6}",?){8})', src, re.M):
    G[int(m[1], 16)] = re.findall(r'"([.#]{6})"', m[2])
fails = 0
def fail(msg):
    global fails
    fails += 1
    print('FAIL glyphs:', msg)
def bits(c): return {(i, j) for j, r in enumerate(G[c]) for i, ch in enumerate(r) if ch == '#'}
def dist(a, b): return len(bits(a) ^ bits(b))

used = set()
for f in glob.glob('src/*.c'):
    for m in re.finditer(r'ui_hint2?\(([^;]*)\);', open(f).read()):
        for h in re.finditer(r'\\x([0-9a-fA-F]{2})', m[1]):
            used.add(int(h[1], 16))
for c in sorted(used):
    if c not in G: fail('hint code 0x%02X has no glyph' % c)
buttons = [0x13, 0x14, 0x15, 0x16, 0x17, 0x01, 0x02, 0x03, 0x04]
for c in buttons:
    if c not in G: fail('missing 0x%02X' % c); continue
    for (i, j) in bits(c):
        if i == 5 or j == 7: fail('0x%02X draws in the gutter at (%d,%d)' % (c, i, j))
for a in buttons:
    for b in buttons:
        if a < b and dist(a, b) < 3: fail('0x%02X and 0x%02X differ by only %d pixels' % (a, b, dist(a, b)))
    for ch in list('ABLR0O8') + ['(', ')']:
        d = dist(a, ord(ch))
        if d < 4: fail('0x%02X looks like %r (%d pixels apart)' % (a, ch, d))
LET = {  # the 3x5 letter that must be cut out of the shape, rows top to bottom
    0x13: [".#.", "#.#", "###", "#.#", "#.#"],
    0x14: ["##.", "#.#", "##.", "#.#", "##."],
    0x15: ["#..", "#..", "#..", "#..", "###"],
    0x16: ["##.", "#.#", "##.", "#.#", "#.#"],
}
for c, letter in LET.items():
    b_ = bits(c)
    for j, row in enumerate(letter):
        for i, ch in enumerate(row):
            if ((i + 1, j + 1) in b_) == (ch == '#'):
                fail('0x%02X: the letter is not cut out at (%d,%d)' % (c, i + 1, j + 1))
    for j in range(1, 6):          # the shape around the letter is closed on both sides
        if (0, j) not in b_ or (4, j) not in b_: fail('0x%02X: shape is open at row %d' % (c, j))
for c in (0x01, 0x02, 0x03, 0x04, 0x17):
    rows = sorted({j for (i, j) in bits(c)})
    mid = (rows[0] + rows[-1]) / 2.0
    if abs(mid - 3.0) > 0.5: fail('0x%02X is off-centre vertically (centre row %.1f, caps centre 3.0)' % (c, mid))
print('glyphs: %d hint codes, %d failures' % (len(used), fails))
sys.exit(1 if fails else 0)
