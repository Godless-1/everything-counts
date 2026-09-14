/* EVERYTHING COUNTS - a card counting story for the Game Boy Advance
 * Copyright (C) 2026 Godless-1
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.  See LICENSE for the full text.
 */
#ifndef STORY_H
#define STORY_H
#include "app.h"

enum {
    EFF_NONE,
    EFF_MARA_SPARE, EFF_MARA_SELL,
    EFF_HOT_TAKE, EFF_HOT_REFUSE,
    EFF_LAUNDER_YES, EFF_LAUNDER_NO,
    EFF_OZEROV_TAKE, EFF_OZEROV_SPARE,
    EFF_SELL_MIND, EFF_REFUSE_HALCYON,
    EFF_TUTORIAL, EFF_VAULT
};

typedef struct { u8 speaker; const char *text; } SLine;

typedef struct {
    const char *title;
    u8 bg, ramp;
    const SLine *line;
    u8 nlines;
    const char *prompt;
    const char *choice[3];
    const char *note[3];
    u8 nchoices;
    u8 effect[3];
    const SLine *after[3];      /* one closing line per branch */
} Beat;

extern const Beat BEATS[];
extern const int NBEATS;

int  story_beat_for_night(int night);
void story_apply(int effect);

#endif
