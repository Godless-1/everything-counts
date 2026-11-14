#!/usr/bin/env python3
# EVERYTHING COUNTS - a card counting story for the Game Boy Advance
# Copyright (C) 2026 Godless-1
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Turns src/gba.h into a host version: same names, but every hardware
address points at a plain array owned by test/play/host.c, and the BIOS
calls become ordinary functions.  Fails loudly if gba.h stops matching."""
import re, sys

src, dst = sys.argv[1], sys.argv[2]
s = open(src).read()

def sub(pattern, repl, flags=0):
    global s
    s, n = re.subn(pattern, repl, s, count=1, flags=flags)
    if n != 1:
        sys.exit("mkshim: gba.h no longer matches /%s/" % pattern)

sub(r'#define IWRAM_CODE .*', '#define IWRAM_CODE')
sub(r'#define IWRAM_DATA .*', '#define IWRAM_DATA')
sub(r'#define REG\(addr\) .*',
    'extern u8 host_io[0x400];\n'
    '#define REG(addr)  (*(volatile u16 *)(host_io + ((addr) & 0x3FF)))')
sub(r'#define REG32\(addr\).*', '#define REG32(addr)(*(volatile u32 *)(host_io + ((addr) & 0x3FF)))')
sub(r'#define DMA \(\(volatile DmaReg \*\)0x040000B0\)',
    'extern DmaReg host_dma[4];\n#define DMA host_dma')
sub(r'#define MEM_PAL_BG .*', 'extern u16 host_pal[256];\n#define MEM_PAL_BG   host_pal')
sub(r'#define MEM_VRAM  .*', 'extern u16 host_vram[0xA000];\n#define MEM_VRAM     ((volatile u16 *)host_vram)')
sub(r'#define MEM_VRAM_PAGE1 .*', '#define MEM_VRAM_PAGE1 ((volatile u16 *)((u8 *)host_vram + 0xA000))')
sub(r'#define MEM_OAM .*', '')
sub(r'#define REG_WAVE_RAM .*', '#define REG_WAVE_RAM  ((volatile u16 *)(host_io + 0x90))')
sub(r'#define MEM_SRAM .*', 'extern u8 host_sram[0x8000];\n#define MEM_SRAM     ((volatile u8 *)host_sram)')
sub(r'/\* ---- BIOS calls.*?(?=extern volatile u32 irq_flags)',
    'void host_halt(void);\n'
    'static inline void bios_halt(void) { host_halt(); }\n'
    'static inline void bios_vblank_wait(void) { host_halt(); }\n'
    'static inline s32 bios_div(s32 num, s32 den) { return num / den; }\n\n',
    re.S)
# host.c includes libc's string.h, whose prototypes differ from the game's
sub(r'(void \*memcpy\(void \*d, const void \*s, u32 n\);\nvoid \*memset\(void \*d, int c, u32 n\);\nvoid \*memmove\(void \*d, const void \*s, u32 n\);)',
    r'#ifndef EC_HOST_HARNESS\n\1\n#endif')
if re.search(r'\*\)\s*0x0[4-7E]0', s):
    sys.exit('mkshim: a raw hardware address survived in gba.h')
open(dst, 'w').write(s)
