#!/usr/bin/env python3
# EVERYTHING COUNTS - a card counting story for the Game Boy Advance
# Copyright (C) 2026 Godless-1
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Converts the harness's PPM screenshots to PNG.

  topng.py <dir>                      every *.ppm in <dir> -> *.png
  topng.py --sheet out.png a.png ...  labelled contact sheet, 2x scale
"""
import glob, os, sys
from PIL import Image, ImageDraw

def convert(d):
    for p in sorted(glob.glob(os.path.join(d, '*.ppm'))):
        Image.open(p).save(p[:-4] + '.png')
        os.remove(p)

def sheet(out, files, cols=3, scale=2):
    W, H, pad, lab = 240 * scale, 160 * scale, 6, 12
    rows = (len(files) + cols - 1) // cols
    im = Image.new('RGB', (cols * (W + pad) + pad, rows * (H + lab + pad) + pad), (24, 24, 32))
    dr = ImageDraw.Draw(im)
    for i, f in enumerate(files):
        x = pad + (i % cols) * (W + pad)
        y = pad + (i // cols) * (H + lab + pad)
        dr.text((x, y), os.path.basename(f)[:-4], fill=(230, 230, 230))
        im.paste(Image.open(f).convert('RGB').resize((W, H), Image.NEAREST), (x, y + lab))
    im.save(out)

if __name__ == '__main__':
    if sys.argv[1] == '--sheet':
        a = sys.argv[3:]
        cols = 3
        if a and a[0].startswith('--cols='):
            cols = int(a[0][7:]); a = a[1:]
        sheet(sys.argv[2], a, cols)
    else:
        convert(sys.argv[1])
