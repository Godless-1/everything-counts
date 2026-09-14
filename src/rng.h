/* EVERYTHING COUNTS - a card counting story for the Game Boy Advance
 * Copyright (C) 2026 Godless-1
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.  See LICENSE for the full text.
 */
#ifndef RNG_H
#define RNG_H
#include "gba.h"

void rng_seed(u32 s);
void rng_stir(void);          /* fold in timer entropy - call while waiting on input */
u32  rng_u32(void);
u32  rng_below(u32 n);        /* 0 .. n-1 */
int  rng_range(int lo, int hi);
int  rng_chance(int percent);

#endif
