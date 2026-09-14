/* EVERYTHING COUNTS - a card counting story for the Game Boy Advance
 * Copyright (C) 2026 Godless-1
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.  See LICENSE for the full text.
 */
#include "rng.h"

static u32 s0 = 0x9E3779B9, s1 = 0x243F6A88, s2 = 0xB7E15162, s3 = 0xDEADBEEF;

void rng_seed(u32 s)
{
    s0 = s ^ 0x9E3779B9;
    s1 = (s << 13) ^ 0x243F6A88;
    s2 = (s >> 7)  ^ 0xB7E15162;
    s3 = (s << 5)  ^ 0x1BADB002;
    for (int i = 0; i < 16; i++) rng_u32();
}

/* xoshiro128** */
u32 rng_u32(void)
{
    u32 r = s1 * 5;
    r = ((r << 7) | (r >> 25)) * 9;
    u32 t = s1 << 9;
    s2 ^= s0; s3 ^= s1; s1 ^= s2; s0 ^= s3; s2 ^= t;
    s3 = (s3 << 11) | (s3 >> 21);
    return r;
}

void rng_stir(void)
{
    s0 ^= REG_VCOUNT * 0x2545F491u;
    s2 += REG_TM2D | 1;
    rng_u32();
}

u32 rng_below(u32 n)
{
    if (n == 0) return 0;
    return (u32)(((u64)rng_u32() * n) >> 32);
}

int rng_range(int lo, int hi) { return lo + (int)rng_below((u32)(hi - lo + 1)); }
int rng_chance(int percent)   { return (int)rng_below(100) < percent; }
