#!/usr/bin/env python3
# EVERYTHING COUNTS - a card counting story for the Game Boy Advance
# Copyright (C) 2026 Godless-1
# SPDX-License-Identifier: AGPL-3.0-or-later
"""
Random-input soak test.  Mashes buttons for a while and flags frames that
look dead (all-black, or identical across a long stretch).
"""
import os, random, sys, time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import drive
from PIL import Image

ROM = sys.argv[1] if len(sys.argv) > 1 else 'dist/everything-counts.gba'
OUT = sys.argv[2] if len(sys.argv) > 2 else 'test/soak'
STEPS = int(sys.argv[3]) if len(sys.argv) > 3 else 120
SEED = int(sys.argv[4]) if len(sys.argv) > 4 else 1

random.seed(SEED)
KEYS = ['a'] * 6 + ['up', 'down', 'left', 'right'] * 2 + ['b', 'start', 'select', 'l', 'r']

s = drive.Session(ROM, OUT)
s.start()
bad = []
prev = None
same = 0
try:
    for i in range(STEPS):
        k = random.choice(KEYS)
        s.key(k, 0.08)
        if i % 5 == 0:
            p = s.shot("step%03d" % i)
            im = Image.open(p).convert('RGB')
            px = list(im.getdata())
            dark = sum(1 for c in px if sum(c) < 24)
            if dark > len(px) * 0.985:
                bad.append((i, k, 'BLACK'))
            if prev is not None and px == prev:
                same += 1
                if same >= 3:
                    bad.append((i, k, 'FROZEN'))
            else:
                same = 0
            prev = px
            os.remove(p)
    print("soak done, %d steps" % STEPS)
    if bad:
        print("SUSPECT FRAMES:")
        for b in bad:
            print("   step %d after key %s : %s" % b)
    else:
        print("no dead frames detected")
finally:
    s.stop()
