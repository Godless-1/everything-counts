#!/usr/bin/env python3
# EVERYTHING COUNTS - a card counting story for the Game Boy Advance
# Copyright (C) 2026 Godless-1
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Patch the GBA cartridge header complement byte + pad ROM to a power of two."""
import sys, os

def main(path):
    with open(path, 'rb') as f:
        rom = bytearray(f.read())
    if len(rom) < 0xC0:
        sys.exit("ROM too small to contain a header")
    # complement check over 0xA0..0xBC
    chk = 0
    for b in rom[0xA0:0xBD]:
        chk = (chk + b) & 0xFF
    rom[0xBD] = (-(0x19 + chk)) & 0xFF
    # pad to next power-of-two >= 128KiB (mask ROMs are sized that way)
    size = max(len(rom), 128 * 1024)
    p = 1
    while p < size:
        p <<= 1
    rom.extend(b'\xff' * (p - len(rom)))
    with open(path, 'wb') as f:
        f.write(rom)
    print("gbafix: %s  complement=0x%02X  size=%dKiB" % (os.path.basename(path), rom[0xBD], p // 1024))

if __name__ == '__main__':
    main(sys.argv[1])
