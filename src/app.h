/* EVERYTHING COUNTS - a card counting story for the Game Boy Advance
 * Copyright (C) 2026 Godless-1
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.  See LICENSE for the full text.
 */
#ifndef APP_H
#define APP_H
#include "gba.h"
#include "video.h"
#include "input.h"
#include "audio.h"
#include "rng.h"
#include "ui.h"
#include "game.h"

enum {
    SC_BOOT, SC_TITLE, SC_INTRO, SC_HUB, SC_VENUE, SC_TABLE,
    SC_CLINIC, SC_STORY, SC_TEACH, SC_DRILL, SC_CODEX,
    SC_NIGHT_END, SC_ENDING, SC_QUIT
};

extern u32 g_t;
extern int g_dbg_scene;

void app_frame_begin(void);
void app_frame_end(void);
void app_fade_out(int frames);
void app_fade_in(int frames);
void app_wait(int frames);
void app_flash(u8 colour, int frames);

/* shared small widgets */
void draw_chips(int x, int y, s32 amount, s32 unit, u8 accent);
void draw_money(int x, int y, const char *label, s32 v, u8 lc, u8 vc);
int  confirm_box(const char *question, const char *yes, const char *no,
                 u8 accent, int bg, u8 ramp);
void toast(const char *line, u8 colour);

/* scenes */
int sc_title(int arg);
int sc_intro(int arg);
int sc_hub(int arg);
int sc_venue(int arg);
int sc_table(int arg);
int sc_clinic(int arg);
int sc_story(int arg);
int sc_teach(int arg);
int sc_drill(int arg);
int sc_codex(int arg);
int sc_night_end(int arg);
int sc_ending(int arg);

#endif
