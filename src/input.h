/* EVERYTHING COUNTS - a card counting story for the Game Boy Advance
 * Copyright (C) 2026 Godless-1
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.  See LICENSE for the full text.
 */
#ifndef INPUT_H
#define INPUT_H
#include "gba.h"

void key_poll(void);
u16  key_held(void);
int  key_down(u16 k);     /* pressed this frame */
int  key_up(u16 k);
int  key_is(u16 k);       /* currently held */
int  key_repeat(u16 k);   /* auto-repeat for menus */
void key_flush(void);

#endif
