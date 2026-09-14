/* EVERYTHING COUNTS - a card counting story for the Game Boy Advance
 * Copyright (C) 2026 Godless-1
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.  See LICENSE for the full text.
 */
/* ========================================================================
 *  ui.h - shared chrome: backdrops, panels, meters, portraits, dialogue
 * ===================================================================== */
#ifndef UI_H
#define UI_H
#include "gba.h"
#include "video.h"
#include "game.h"

enum {
    BG_RAIN,      /* the city seen through wet glass      */
    BG_GRID,      /* synthwave horizon - menus and sims    */
    BG_FELT,      /* the table                             */
    BG_TERMINAL,  /* dark scanned terminal - codex, shops  */
    BG_CLINIC,    /* cold surgical white-cyan              */
    BG_VOID       /* near black with drifting motes        */
};

void ui_backdrop(int style, u32 t, u8 ramp);
void ui_panel(int x, int y, int w, int h, u8 accent, const char *title);
void ui_panel_solid(int x, int y, int w, int h, u8 accent, const char *title);
void ui_brackets(int x, int y, int w, int h, u8 c, int len);
void ui_bar(int x, int y, int w, int h, int val, int max, u8 ramp, u8 frame);
void ui_pips(int x, int y, int n, int filled, u8 on, u8 off);
void ui_divider(int x, int y, int w, u8 c);
void ui_titlebar(const char *left, const char *right, u8 accent);
void ui_hint(const char *s);
void ui_hint2(const char *a, const char *b);
void ui_glitch_text(int x, int y, const char *s, u8 c, u32 t, int amount);
void ui_marquee(int x, int y, int w, const char *s, u32 t, u8 c);
void ui_noise(int x, int y, int w, int h, u32 t, u8 c, int density);

/* ---- menus ----------------------------------------------------------- */
typedef struct {
    const char *label;
    const char *desc;
    u8 enabled;
    u8 colour;
} MenuItem;

void ui_menu(int x, int y, int w, const MenuItem *items, int n, int sel,
             u8 accent, int rowh);

/* ---- characters ------------------------------------------------------ */
enum {
    CH_KESTREL, CH_RUI, CH_SABLE, CH_MARA, CH_GREY,
    CH_OZEROV, CH_VESK, CH_PIT, CH_SYSTEM, CH_COUNT
};
const char *ch_name(int id);
u8          ch_colour(int id);
void ui_portrait(int x, int y, int id, u32 t);

/* ---- dialogue -------------------------------------------------------- */
void ui_dialogue_box(int speaker, const char *text, int nchars, u32 t);
int  ui_dialogue_len(const char *text);

#endif
