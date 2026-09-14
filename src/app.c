/* EVERYTHING COUNTS - a card counting story for the Game Boy Advance
 * Copyright (C) 2026 Godless-1
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.  See LICENSE for the full text.
 */
#include "app.h"

u32 g_t;

void app_frame_begin(void)
{
    key_poll();
    snd_update();
    rng_stir();
    g_t++;
}

int g_dbg_scene = -1;
void app_frame_end(void) { vid_flip(); }

void app_wait(int frames)
{
    while (frames-- > 0) { app_frame_begin(); app_frame_end(); }
}

void app_fade_out(int frames)
{
    for (int i = 0; i <= frames; i++) {
        vid_fade((i * 16) / frames);
        app_frame_begin(); app_frame_end();
    }
}

void app_fade_in(int frames)
{
    for (int i = frames; i >= 0; i--) {
        vid_fade((i * 16) / frames);
        app_frame_begin(); app_frame_end();
    }
    vid_fade(0);
}

void app_flash(u8 colour, int frames)
{
    (void)colour;
    for (int i = frames; i > 0; i--) {
        vid_fade_white((i * 12) / frames);
        app_frame_begin(); app_frame_end();
    }
    vid_fade(0);
}

/* ---------------------------------------------------------------------- */
void draw_chips(int x, int y, s32 amount, s32 unit, u8 accent)
{
    if (unit <= 0) unit = 1;
    int n = (int)(amount / unit);
    if (n > 30) n = 30;
    if (n < 1) return;
    int per = 6;
    int cols = (n + per - 1) / per;
    x -= (cols * 16) / 2;
    for (int i = 0; i < n; i++) {
        int col = i / per, row = i % per;
        int cx = x + col * 16, cy = y + 24 - row * 4;
        u8 body = (i % 3 == 0) ? accent : ((i % 3 == 1) ? C_WHITE : C_BLACK);
        u8 edge = (body == C_BLACK) ? accent : C_BLACK;
        vid_rect(cx + 1, cy, 12, 4, body);
        vid_hline(cx, cy + 1, 14, body);
        vid_hline(cx + 2, cy, 10, edge);
        vid_hline(cx + 2, cy + 3, 10, edge);
        vid_plot(cx, cy + 1, C_BLACK);
        vid_plot(cx + 13, cy + 1, C_BLACK);
        vid_hline(cx + 4, cy + 1, 2, C_WHITE);
    }
}

void draw_money(int x, int y, const char *label, s32 v, u8 lc, u8 vc)
{
    char b[24];
    int nx = vid_text(x, y, label, lc);
    b[0] = 0x09; str_money(b + 1, v);
    vid_text(nx + 4, y, b, vc);
}

void toast(const char *line, u8 colour)
{
    int w = vid_text_len(line) + 16;
    int x = 120 - w / 2;
    vid_rect(x, 66, w, 20, C_BLACK);
    vid_frame_rect(x, 66, w, 20, colour);
    ui_brackets(x - 2, 64, w + 4, 24, colour, 4);
    vid_text_center(120, 72, line, colour);
}

int confirm_box(const char *question, const char *yes, const char *no,
                u8 accent, int bg, u8 ramp)
{
    int sel = 0;
    key_flush();
    for (;;) {
        app_frame_begin();
        if (key_repeat(KEY_LEFT) || key_repeat(KEY_RIGHT)) { sel ^= 1; sfx(SFX_MOVE); }
        if (key_down(KEY_A)) { sfx(SFX_SELECT); return sel == 0; }
        if (key_down(KEY_B)) { sfx(SFX_BACK); return 0; }

        ui_backdrop(bg, g_t, ramp);
        ui_panel(16, 34, 208, 94, accent, "CONFIRM");
        vid_text_wrap(22, 44, 33, question, C_TEXT);
        const char *opts[2] = { yes, no };
        for (int i = 0; i < 2; i++) {
            int bx = 28 + i * 94;
            u8 c = (i == sel) ? accent : C_DIM;
            vid_rect(bx, 106, 84, 14, C_BLACK);
            vid_frame_rect(bx, 106, 84, 14, c);
            vid_text_center(bx + 42, 109, opts[i], (i == sel) ? C_WHITE : C_DIM);
            if (i == sel) ui_brackets(bx - 2, 104, 88, 18, accent, 4);
        }
        ui_hint("\x13 CONFIRM   \x14 CANCEL");
        app_frame_end();
    }
}
