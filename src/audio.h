/* EVERYTHING COUNTS - a card counting story for the Game Boy Advance
 * Copyright (C) 2026 Godless-1
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.  See LICENSE for the full text.
 */
#ifndef AUDIO_H
#define AUDIO_H
#include "gba.h"

enum {
    SFX_MOVE, SFX_SELECT, SFX_BACK, SFX_DENY, SFX_DEAL, SFX_FLIP,
    SFX_CHIP, SFX_WIN, SFX_LOSE, SFX_PUSH, SFX_BJ, SFX_BUST,
    SFX_ALERT, SFX_PULSE, SFX_GOOD, SFX_BAD, SFX_CASH, SFX_GLITCH,
    SFX_IMPLANT, SFX_HEARTBEAT, SFX_COUNT
};

enum {
    MUS_NONE, MUS_NEON_RAIN, MUS_THE_SHOE, MUS_OVERCLOCK, MUS_ASHES,
    MUS_VAULT, MUS_COUNT
};

void snd_init(void);
void snd_update(void);        /* call once per frame */
void sfx(int id);
void mus_play(int song);
void mus_stop(void);
void mus_set_intensity(int level);   /* 0..2 - swaps in tenser patterns */

#endif
