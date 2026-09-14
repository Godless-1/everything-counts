/* EVERYTHING COUNTS - a card counting story for the Game Boy Advance
 * Copyright (C) 2026 Godless-1
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.  See LICENSE for the full text.
 */
#ifndef TABLE_H
#define TABLE_H
#include "app.h"

enum { TMODE_PLAY, TMODE_PRACTICE, TMODE_STORY_VAULT };
#define TABLE_ARG(venue, mode) ((venue) | ((mode) << 8))

/* results handed back to the night loop */
enum { TEND_LEFT, TEND_BACKED_OFF, TEND_BROKE, TEND_TARGET };
extern int g_table_end;
extern s32 g_table_net;
extern int g_table_hands;

#endif
