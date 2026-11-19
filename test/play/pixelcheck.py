#!/usr/bin/env python3
# EVERYTHING COUNTS - a card counting story for the Game Boy Advance
# Copyright (C) 2026 Godless-1
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Pixel-level text check for harness runs made with -g.

  pixelcheck.py <dir>

For every <name>.glyphs.tsv with a <name>.png beside it (names containing
"fade" or "flash" are skipped) it looks at every glyph that drew ink and
writes one line per problem to <dir>/pixel.log:

  PARTIAL      some of the glyph's pixels were painted over by something else
  OFFSCREEN    some of the glyph's pixels fell off the screen
  MISMATCH     the glyph's surviving pixels are not all one colour
  INVISIBLE    every pixel of the glyph's cell has the ink colour
  LOWCONTRAST  ink against the usual cell background has a WCAG ratio under 2.0

Exit status 1 when pixel.log is not empty.
"""
import glob, os, sys
from collections import Counter
from PIL import Image

def lum(c):
    def f(v):
        v /= 255.0
        return v / 12.92 if v <= 0.03928 else ((v + 0.055) / 1.055) ** 2.4
    return 0.2126 * f(c[0]) + 0.7152 * f(c[1]) + 0.0722 * f(c[2])

def ratio(a, b):
    la, lb = lum(a), lum(b)
    if la < lb:
        la, lb = lb, la
    return (la + 0.05) / (lb + 0.05)

def check(tsv, png, name, out):
    im = Image.open(png).convert('RGB')
    px = im.load()
    W, H = im.size
    with open(tsv) as f:
        for line in f:
            p = line.rstrip('\n').split('\t')
            if len(p) < 8:
                continue
            x, y, ch, sc, ink, lost, off = (int(v) for v in p[:7])
            s = p[7]
            pix = p[8] if len(p) > 8 else ''
            if ink <= 0:
                continue
            at = '"%s" at %d,%d' % (s, x, y)
            if lost >= 1:
                out.append('PARTIAL %s %s lost=%d/%d' % (name, at, lost, ink))
            if off >= 1:
                out.append('OFFSCREEN %s %s px=%d' % (name, at, off))
            mine = [tuple(int(v) for v in q.split(',')) for q in pix.split(';') if q]
            cols = Counter(px[a, b] for a, b in mine if 0 <= a < W and 0 <= b < H)
            if len(cols) > 1:
                out.append('MISMATCH %s %s' % (name, at))
            if not cols:
                continue
            inkc = cols.most_common(1)[0][0]
            cell = Counter()
            for yy in range(y, min(y + 8 * sc, H)):
                for xx in range(x, min(x + 6 * sc, W)):
                    if xx >= 0 and yy >= 0:
                        cell[px[xx, yy]] += 1
            others = [(n, c) for c, n in cell.items() if c != inkc]
            if not others:
                out.append('INVISIBLE %s %s' % (name, at))
                continue
            bg = max(others)[1]
            r = ratio(inkc, bg)
            if r < 2.0:
                out.append('LOWCONTRAST %s %s ratio=%.2f' % (name, at, r))

def main():
    d = sys.argv[1]
    out = []
    for tsv in sorted(glob.glob(os.path.join(d, '*.glyphs.tsv'))):
        name = os.path.basename(tsv)[:-len('.glyphs.tsv')]
        if 'fade' in name or 'flash' in name:
            continue
        png = os.path.join(d, name + '.png')
        if os.path.exists(png):
            check(tsv, png, name, out)
    with open(os.path.join(d, 'pixel.log'), 'w') as f:
        f.write(''.join(l + '\n' for l in out))
    kinds = Counter(l.split()[0] for l in out)
    print('pixelcheck %s: %d lines %s' % (d, len(out), dict(kinds)))
    return 1 if out else 0

if __name__ == '__main__':
    sys.exit(main())
