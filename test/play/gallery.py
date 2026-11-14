#!/usr/bin/env python3
# EVERYTHING COUNTS - a card counting story for the Game Boy Advance
# Copyright (C) 2026 Godless-1
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Rebuilds doc/shots, doc/title.png and doc/gallery.png from the harness
screenshots made by scripts/docshots-a.txt and docshots-b.txt.

  test/play/run.sh test/play/scripts/docshots-a.txt build-host/doc
  test/play/run.sh test/play/scripts/docshots-b.txt build-host/doc
  python3 test/play/gallery.py build-host/doc
"""
import glob, os, shutil, sys
from PIL import Image

src = sys.argv[1]
root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
shots = sorted(glob.glob(os.path.join(src, '[0-9][0-9]-*.png')))
assert len(shots) == 12, 'expected 12 screenshots, found %d' % len(shots)
for f in shots:
    shutil.copy(f, os.path.join(root, 'doc', 'shots', os.path.basename(f)))
shutil.copy(shots[0], os.path.join(root, 'doc', 'title.png'))

PAD, S, COLS = 8, 2, 3
W, H = 240 * S, 160 * S
rows = len(shots) // COLS
sheet = Image.new('RGB', (COLS * W + (COLS + 1) * PAD, rows * H + (rows + 1) * PAD), (0, 0, 0))
for i, f in enumerate(shots):
    im = Image.open(f).convert('RGB').resize((W, H), Image.NEAREST)
    sheet.paste(im, (PAD + (i % COLS) * (W + PAD), PAD + (i // COLS) * (H + PAD)))
sheet.save(os.path.join(root, 'doc', 'gallery.png'), optimize=True)
print('doc/gallery.png', sheet.size)
