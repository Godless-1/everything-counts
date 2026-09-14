/* EVERYTHING COUNTS - a card counting story for the Game Boy Advance
 * Copyright (C) 2026 Godless-1
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.  See LICENSE for the full text.
 */
/* SRAM lives on an 8-bit bus: byte access only, no DMA, no halfwords. */
#include "save.h"
#include "game.h"

#define SAVE_MAGIC 0x4B535452u   /* 'KSTR' */
#define SAVE_VER   3

typedef struct {
    u32 magic;
    u16 ver;
    u16 size;
    u32 sum;
    Run run;
} SaveBlock;

static u32 checksum(const u8 *p, u32 n)
{
    u32 s = 0x811C9DC5u;
    for (u32 i = 0; i < n; i++) { s ^= p[i]; s *= 16777619u; }
    return s;
}

static void sram_put(u32 off, const u8 *src, u32 n)
{
    volatile u8 *d = MEM_SRAM + off;
    for (u32 i = 0; i < n; i++) d[i] = src[i];
}

static void sram_get(u32 off, u8 *dst, u32 n)
{
    volatile u8 *s = MEM_SRAM + off;
    for (u32 i = 0; i < n; i++) dst[i] = s[i];
}

void save_write(void)
{
    SaveBlock b;
    b.magic = SAVE_MAGIC;
    b.ver = SAVE_VER;
    b.size = sizeof(Run);
    b.run = g_run;
    b.sum = checksum((const u8 *)&b.run, sizeof(Run));
    sram_put(0, (const u8 *)&b, sizeof(b));
}

int save_read(void)
{
    SaveBlock b;
    sram_get(0, (u8 *)&b, sizeof(b));
    if (b.magic != SAVE_MAGIC || b.ver != SAVE_VER || b.size != sizeof(Run)) return 0;
    if (checksum((const u8 *)&b.run, sizeof(Run)) != b.sum) return 0;
    g_run = b.run;
    return 1;
}

int save_exists(void)
{
    SaveBlock b;
    sram_get(0, (u8 *)&b, sizeof(b));
    if (b.magic != SAVE_MAGIC || b.ver != SAVE_VER || b.size != sizeof(Run)) return 0;
    return checksum((const u8 *)&b.run, sizeof(Run)) == b.sum;
}

void save_erase(void)
{
    u32 z = 0;
    sram_put(0, (const u8 *)&z, 4);
}
