/* EVERYTHING COUNTS - a card counting story for the Game Boy Advance
 * Copyright (C) 2026 Godless-1
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.  See LICENSE for the full text.
 */
#include "input.h"

static u16 cur, prev;
static u16 rpt_key;
static int rpt_timer;

void key_poll(void)
{
    prev = cur;
    cur = (~REG_KEYINPUT) & KEY_ANY;

    u16 dirs = cur & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_L | KEY_R);
    if (dirs != rpt_key) { rpt_key = dirs; rpt_timer = 0; }
    else if (dirs) rpt_timer++;
}

u16  key_held(void)     { return cur; }
int  key_down(u16 k)    { return (cur & k) && !(prev & k); }
int  key_up(u16 k)      { return !(cur & k) && (prev & k); }
int  key_is(u16 k)      { return (cur & k) != 0; }
void key_flush(void)    { prev = cur = (~REG_KEYINPUT) & KEY_ANY; rpt_timer = 0; }

int key_repeat(u16 k)
{
    if (key_down(k)) return 1;
    if (!(cur & k)) return 0;
    if (rpt_timer > 22 && ((rpt_timer - 22) % 5) == 0) return 1;
    return 0;
}
