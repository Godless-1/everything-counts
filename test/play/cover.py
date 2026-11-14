#!/usr/bin/env python3
# EVERYTHING COUNTS - a card counting story for the Game Boy Advance
# Copyright (C) 2026 Godless-1
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Builds doc/cover.png, a square cover for game launchers, out of the
game's own pixels: the title band, a dealt hand and the skyline, all cut
from harness screenshots (see gallery.py) and scaled without smoothing.

  python3 test/play/cover.py build-host/doc
"""
import os, sys
from PIL import Image, ImageDraw

src = sys.argv[1]
root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
title = Image.open(os.path.join(src, '01-title.png')).convert('RGB')
table = Image.open(os.path.join(src, '11-table2.png')).convert('RGB')

N = 512
im = Image.new('RGB', (N, N), (0, 0, 0))
up = lambda i, s: i.resize((i.width * s, i.height * s), Image.NEAREST)

# the rain-soaked skyline fills the top third
im.paste(up(title.crop((0, 0, 240, 28)), 3).crop((104, 0, 104 + N, 84)), (0, 0))
# the logo band, at twice its size, centred
band = up(title.crop((0, 28, 240, 90)), 2)
im.paste(band, ((N - band.width) // 2, 84))
# the hand on the felt: dealer above, player below
hand = up(table.crop((92, 18, 212, 110)), 3)
im.paste(hand.crop((0, 0, min(hand.width, N), hand.height)), ((N - min(hand.width, N)) // 2, 220))
# neon rules, magenta over cyan, like the title band's own
d = ImageDraw.Draw(im)
for y, c in ((216, (255, 40, 160)), (218, (120, 16, 90)), (498, (0, 200, 232)), (500, (0, 90, 110))):
    d.line([(0, y), (N, y)], fill=c, width=2)
im.save(os.path.join(root, 'doc', 'cover.png'), optimize=True)
print('doc/cover.png', im.size)
