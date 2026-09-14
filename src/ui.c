/* EVERYTHING COUNTS - a card counting story for the Game Boy Advance
 * Copyright (C) 2026 Godless-1
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.  See LICENSE for the full text.
 */
#include "ui.h"
#include "rng.h"
#include "input.h"
#include "audio.h"

/* deterministic hash so procedural scenery is stable frame to frame */
static u32 h32(u32 x)
{
    x ^= x >> 16; x *= 0x7FEB352Du;
    x ^= x >> 15; x *= 0x846CA68Bu;
    x ^= x >> 16;
    return x;
}

/* ====================================================================== */
/*  backdrops                                                             */
/* ====================================================================== */

static void bg_rain(u32 t, u8 ramp)
{
    (void)ramp;
    vid_vgrad_scan(0, 0, 240, 100, RAMP_SKY, 2, 18, 2);
    vid_rect_scan(0, 100, 240, 60, RAMP_SKY + 1, RAMP_SKY + 0);

    /* far skyline */
    for (int i = 0; i < 26; i++) {
        u32 r = h32(i * 2654435761u);
        int w = 6 + (r & 7);
        int x = i * 9 - 4;
        int hgt = 16 + ((r >> 4) & 31);
        vid_rect(x, 100 - hgt, w, hgt, RAMP_SKY + 5);
        vid_vline(x, 100 - hgt, hgt, RAMP_SKY + 7);
        for (int wy = 100 - hgt + 3; wy < 98; wy += 5)
            for (int wx = x + 1; wx < x + w - 1; wx += 3) {
                u32 q = h32((u32)(wx * 977 + wy * 31 + (t >> 6)));
                if ((q & 15) < 5) vid_plot(wx, wy, (q & 1) ? RAMP_GOLD + 18 : RAMP_CYAN + 16);
            }
    }
    /* near towers */
    for (int i = 0; i < 9; i++) {
        u32 r = h32(i * 40503u + 7u);
        int w = 16 + (r & 15);
        int x = i * 29 - 10;
        int hgt = 44 + ((r >> 5) & 47);
        vid_rect(x, 160 - hgt, w, hgt, RAMP_SKY + 2);
        vid_vline(x, 160 - hgt, hgt, RAMP_MAG + 8);
        vid_hline(x, 160 - hgt, w, RAMP_SKY + 6);
        for (int wy = 160 - hgt + 4; wy < 158; wy += 6)
            for (int wx = x + 2; wx < x + w - 2; wx += 4) {
                u32 q = h32((u32)(wx * 131 + wy * 7919 + (t >> 5)));
                if ((q & 31) < 8) vid_plot(wx, wy, (q & 1) ? RAMP_MAG + 17 : RAMP_CYAN + 17);
            }
    }
    /* holo advert bleeding down a tower face */
    vid_rect(184, 44, 22, 34, RAMP_MAG + 4);
    vid_frame_rect(184, 44, 22, 34, RAMP_MAG + 14);
    for (int j = 0; j < 30; j += 3)
        vid_hline(186, 47 + j, 18, (u8)(RAMP_MAG + 8 + ((j + (int)(t >> 3)) % 10)));

    /* rain */
    for (int i = 0; i < 52; i++) {
        u32 r = h32(i * 7919u);
        int x = (int)((r + t * ((r & 3) + 2)) % 240);
        int y = (int)((r / 240 + t * (3 + (r & 7))) % 172) - 10;
        vid_plot(x, y,     RAMP_CYAN + 14);
        vid_plot(x, y + 1, RAMP_CYAN + 10);
        vid_plot(x, y + 2, RAMP_CYAN + 6);
    }
}

static void bg_grid(u32 t, u8 ramp)
{
    vid_vgrad_scan(0, 0, 240, 76, RAMP_SKY, 1, 16, 2);
    vid_rect_scan(0, 76, 240, 84, C_BLACK, RAMP_VIOLET + 1);
    /* sun, sliced by its own scanlines */
    for (int j = 0; j < 26; j++) {
        int dy = j - 13;
        int rr = 676 - dy * dy;
        int hw = 0;
        while (hw * hw < rr) hw++;
        if (j > 10 && ((j + (int)(t >> 4)) % 5) == 0) continue;
        int v = 22 - j / 2;
        vid_hline(120 - hw, 46 + j, hw * 2, (u8)(RAMP_GOLD + (v < 6 ? 6 : v)));
    }
    /* horizon glow */
    vid_hline(0, 75, 240, (u8)(ramp + 21));
    vid_hline(0, 76, 240, (u8)(ramp + 16));
    vid_hline(0, 77, 240, (u8)(ramp + 9));
    /* converging lines */
    for (int i = -7; i <= 7; i++)
        vid_line(120, 76, 120 + i * 34, 160, (u8)(ramp + 11));
    /* receding rows */
    int y = 78, step;
    int phase = (int)((t / 2) % 7);
    while (y < 160) {
        int yy = y + phase * (y - 76) / 40;
        if (yy < 160) vid_hline(0, yy, 240, (u8)(ramp + 13));
        step = 2 + (y - 76) / 7;
        y += step;
    }
}

static void bg_felt(u32 t, u8 ramp)
{
    /* pool of light falling on green-black felt */
    for (int j = 0; j < 160; j++) {
        int d = j - 62; if (d < 0) d = -d;
        int v = 5 - d / 15;
        if (v < 0) v = 0;
        int base = (j & 1) ? 0 : 1;
        vid_hline(0, j, 240, (u8)(ramp + base + v));
    }
    /* vignette down the sides */
    for (int i = 0; i < 18; i++) {
        u8 c = (u8)(ramp + (i / 12));
        vid_vline(i, 0, 160, c);
        vid_vline(239 - i, 0, 160, c);
    }
    /* betting arc + insurance line */
    for (int x = 8; x < 232; x++) {
        int dx = x - 120;
        int y = 110 + (dx * dx) / 1400;
        if (y < 120) {
            vid_plot(x, y, (u8)(ramp + 14));
            vid_plot(x, y + 1, (u8)(ramp + 7));
        }
        int y2 = 64 - (dx * dx) / 2200;
        if (y2 > 18) vid_plot(x, y2, (u8)(ramp + 10));
    }
    /* drifting motes in the beam */
    for (int i = 0; i < 14; i++) {
        u32 r = h32(i * 2246822519u);
        int x = (int)((r + t / 3) % 240);
        int y = (int)((r / 7 + t / 5) % 160);
        vid_plot(x, y, (u8)(ramp + 18));
    }
}

static void bg_terminal(u32 t, u8 ramp)
{
    vid_vgrad_scan(0, 0, 240, 160, RAMP_VIOLET, 5, 1, 1);
    for (int x = 0; x < 240; x += 8) vid_vline(x, 0, 160, RAMP_VIOLET + 4);
    for (int y = 0; y < 160; y += 8) vid_hline(0, y, 240, RAMP_VIOLET + 4);
    /* sweeping read head */
    int sy = (int)((t / 2) % 210) - 30;
    for (int j = 0; j < 26; j++) {
        int yy = sy + j;
        if (yy < 0 || yy >= 160) continue;
        int v = (j < 13) ? j : 26 - j;
        vid_hline(0, yy, 240, (u8)(ramp + 2 + v));
    }
    ui_noise(0, 0, 240, 160, t, (u8)(ramp + 16), 26);
}

static void bg_clinic(u32 t, u8 ramp)
{
    vid_vgrad_scan(0, 0, 240, 160, RAMP_CYAN, 2, 9, 1);
    /* surgical lamp cone */
    for (int j = 0; j < 58; j++) {
        int hw = 22 + j;
        int v = 22 - j / 3;
        if (v < 4) v = 4;
        vid_hline(120 - hw, j, hw * 2, (u8)(RAMP_CYAN + ((j & 1) ? v - 1 : v)));
    }
    vid_rect(104, 0, 32, 7, RAMP_CHROME + 18);
    vid_rect(100, 6, 40, 3, RAMP_CHROME + 22);
    /* tiled wall seams */
    for (int i = 0; i < 7; i++) vid_vline(16 + i * 36, 58, 102, RAMP_CYAN + 4);
    vid_hline(0, 104, 240, (u8)(ramp + 14));
    vid_hline(0, 105, 240, (u8)(ramp + 7));
    (void)t;
}

static void bg_void(u32 t, u8 ramp)
{
    vid_clear(C_BLACK);
    vid_vgrad_scan(0, 0, 240, 160, RAMP_VIOLET, 3, 0, 1);
    for (int i = 0; i < 46; i++) {
        u32 r = h32(i * 1664525u + 1013904223u);
        int x = (int)((r + t / 6) % 240);
        int y = (int)((r / 13 + t / 11) % 160);
        int b = (int)((r >> 20) & 7);
        vid_plot(x, y, (u8)(ramp + 5 + b));
    }
}

void ui_backdrop(int style, u32 t, u8 ramp)
{
    switch (style) {
        case BG_RAIN:     bg_rain(t, ramp); break;
        case BG_GRID:     bg_grid(t, ramp); break;
        case BG_FELT:     bg_felt(t, ramp); break;
        case BG_TERMINAL: bg_terminal(t, ramp); break;
        case BG_CLINIC:   bg_clinic(t, ramp); break;
        default:          bg_void(t, ramp); break;
    }
}

/* ====================================================================== */
/*  panels and widgets                                                    */
/* ====================================================================== */

void ui_brackets(int x, int y, int w, int h, u8 c, int len)
{
    vid_hline(x, y, len, c);                 vid_vline(x, y, len, c);
    vid_hline(x + w - len, y, len, c);       vid_vline(x + w - 1, y, len, c);
    vid_hline(x, y + h - 1, len, c);         vid_vline(x, y + h - len, len, c);
    vid_hline(x + w - len, y + h - 1, len, c);
    vid_vline(x + w - 1, y + h - len, len, c);
}

void ui_panel(int x, int y, int w, int h, u8 accent, const char *title)
{
    vid_rect(x + 1, y + 1, w - 2, h - 2, C_BLACK);
    vid_rect_dither(x + 1, y + 1, w - 2, h - 2, C_BLACK, C_INDIGO);
    vid_frame_rect(x, y, w, h, accent);
    ui_brackets(x - 1, y - 1, w + 2, h + 2, accent, 5);
    if (title) {
        int tw = vid_text_len(title);
        vid_rect(x + 5, y - 4, tw + 4, 9, C_BLACK);
        vid_text(x + 7, y - 4, title, accent);
    }
}

void ui_panel_solid(int x, int y, int w, int h, u8 accent, const char *title)
{
    vid_vgrad(x + 1, y + 1, w - 2, h - 2, RAMP_VIOLET, 4, 1);
    vid_frame_rect(x, y, w, h, accent);
    vid_hline(x + 1, y + 1, w - 2, (u8)(accent));
    ui_brackets(x - 1, y - 1, w + 2, h + 2, accent, 5);
    if (title) {
        int tw = vid_text_len(title);
        vid_rect(x + 5, y - 4, tw + 4, 9, C_BLACK);
        vid_text(x + 7, y - 4, title, accent);
    }
}

void ui_divider(int x, int y, int w, u8 c)
{
    for (int i = 0; i < w; i += 2) vid_plot(x + i, y, c);
}

void ui_bar(int x, int y, int w, int h, int val, int max, u8 ramp, u8 frame)
{
    if (max <= 0) max = 1;
    if (val < 0) val = 0;
    if (val > max) val = max;
    vid_rect(x, y, w, h, C_BLACK);
    int fw = ((w - 2) * val) / max;
    for (int i = 0; i < fw; i++) {
        int t = 6 + (16 * i) / (w - 2);
        vid_vline(x + 1 + i, y + 1, h - 2, (u8)(ramp + t));
    }
    vid_frame_rect(x, y, w, h, frame);
    /* tick marks every quarter */
    for (int q = 1; q < 4; q++) {
        int tx = x + 1 + ((w - 2) * q) / 4;
        vid_plot(tx, y, frame);
        vid_plot(tx, y + h - 1, frame);
    }
}

void ui_pips(int x, int y, int n, int filled, u8 on, u8 off)
{
    for (int i = 0; i < n; i++) {
        u8 c = (i < filled) ? on : off;
        vid_rect(x + i * 6, y, 4, 4, c);
    }
}

void ui_titlebar(const char *left, const char *right, u8 accent)
{
    vid_rect(0, 0, 240, 11, C_BLACK);
    vid_hgrad(0, 0, 90, 11, RAMP_VIOLET, 5, 0);
    vid_hline(0, 11, 240, accent);
    if (left)  vid_text(4, 2, left, accent);
    if (right) vid_text(236 - vid_text_len(right), 2, right, C_MID);
}

void ui_hint(const char *s)
{
    vid_rect(0, 150, 240, 10, C_BLACK);
    vid_hline(0, 149, 240, C_GRID);
    vid_text_center(120, 151, s, C_DIM);
}

void ui_hint2(const char *a, const char *b)
{
    vid_rect(0, 150, 240, 10, C_BLACK);
    vid_hline(0, 149, 240, C_GRID);
    if (a) vid_text(4, 151, a, C_DIM);
    if (b) vid_text(236 - vid_text_len(b), 151, b, C_DIM);
}

void ui_glitch_text(int x, int y, const char *s, u8 c, u32 t, int amount)
{
    if (amount > 0 && ((t >> 2) % 17) < (u32)amount) {
        vid_text(x + 1 + (int)(t & 1), y, s, C_MAG);
        vid_text(x - 1, y, s, C_CYAN);
    }
    vid_text(x, y, s, c);
}

void ui_marquee(int x, int y, int w, const char *s, u32 t, u8 c)
{
    int len = (int)ec_strlen(s);
    int cols = w / FONT_W;
    if (len <= cols) { vid_text(x, y, s, c); return; }
    int off = (int)((t / 8) % (u32)(len + 4));
    for (int i = 0; i < cols; i++) {
        int k = off + i;
        char ch = (k < len) ? s[k] : ' ';
        char buf[2] = { ch, 0 };
        vid_text(x + i * FONT_W, y, buf, c);
    }
}

void ui_noise(int x, int y, int w, int h, u32 t, u8 c, int density)
{
    for (int i = 0; i < density; i++) {
        u32 r = h32(i * 2654435761u + t * 2246822519u);
        vid_plot(x + (int)(r % (u32)w), y + (int)((r >> 9) % (u32)h), c);
    }
}

/* ====================================================================== */
/*  menus                                                                 */
/* ====================================================================== */

void ui_menu(int x, int y, int w, const MenuItem *items, int n, int sel,
             u8 accent, int rowh)
{
    for (int i = 0; i < n; i++) {
        int ry = y + i * rowh;
        u8 col = items[i].colour ? items[i].colour : C_TEXT;
        if (!items[i].enabled) col = C_DIM;
        if (i == sel) {
            vid_hgrad(x, ry - 1, w, rowh, RAMP_VIOLET, 9, 0);
            vid_vline(x, ry - 1, rowh, accent);
            vid_vline(x + 1, ry - 1, rowh, accent);
            vid_text(x + 5, ry, "\x01", accent);
            if (!items[i].enabled) col = C_MID;
            else col = C_WHITE;
        }
        vid_text(x + 13, ry, items[i].label, col);
        if (!items[i].enabled)
            vid_text(x + w - 12, ry, "\x0F", C_DIM);
    }
}

/* ====================================================================== */
/*  characters                                                            */
/* ====================================================================== */

typedef struct {
    const char *name;
    u8 accent;
    u8 ramp;
    u8 visor;     /* 0 none, 1 band, 2 monocle, 3 full mask, 4 cracked  */
    u8 crown;     /* hair / headgear silhouette                          */
    u8 mark;      /* cheek / neck chrome                                 */
} CharDef;

static const CharDef CHARS[CH_COUNT] = {
    { "KESTREL",   C_CYAN,   RAMP_CYAN,   4, 1, 1 },
    { "RUI",       C_CANDY,  RAMP_MAG,    0, 2, 0 },
    { "SABLE",     C_AMBER,  RAMP_GOLD,   2, 3, 2 },
    { "MARA",      C_TEAL,   RAMP_GREEN,  0, 2, 3 },
    { "GREY",      C_GREEN,  RAMP_GREEN,  3, 0, 2 },
    { "OZEROV",    C_ORANGE, RAMP_ALERT,  1, 4, 1 },
    { "VESK",      C_PALE,   RAMP_CHROME, 3, 5, 0 },
    { "PIT BOSS",  C_BLOOD,  RAMP_ALERT,  1, 3, 0 },
    { "SYSTEM",    C_MINT,   RAMP_CYAN,   3, 0, 0 },
};

const char *ch_name(int id)  { return CHARS[id % CH_COUNT].name; }
u8          ch_colour(int id){ return CHARS[id % CH_COUNT].accent; }

#define PORT_W 44
#define PORT_H 52

void ui_portrait(int x, int y, int id, u32 t)
{
    const CharDef *c = &CHARS[id % CH_COUNT];
    u8 rp = c->ramp;

    /* frame + backlight */
    vid_rect(x, y, PORT_W, PORT_H, C_BLACK);
    vid_vgrad(x + 1, y + 1, PORT_W - 2, PORT_H - 2, rp, 1, 5);
    for (int i = 2; i < PORT_H - 2; i += 3) vid_hline(x + 1, y + i, PORT_W - 2, (u8)(rp + 2));

    int cx = x + PORT_W / 2;
    int hy = y + 12;

    /* shoulders */
    vid_rect(x + 3, y + PORT_H - 11, PORT_W - 6, 10, RAMP_CHROME + 4);
    vid_hline(x + 3, y + PORT_H - 11, PORT_W - 6, (u8)(rp + 12));
    vid_rect(cx - 6, y + PORT_H - 15, 12, 6, RAMP_CHROME + 7);

    /* head */
    vid_rect(cx - 10, hy, 20, 24, RAMP_CHROME + 8);
    vid_rect(cx - 11, hy + 4, 22, 16, RAMP_CHROME + 8);
    vid_hline(cx - 9, hy - 1, 18, RAMP_CHROME + 11);
    vid_rect(cx - 13, hy + 9, 2, 5, RAMP_CHROME + 6);
    vid_rect(cx + 11, hy + 9, 2, 5, RAMP_CHROME + 6);

    /* jaw shading */
    vid_hline(cx - 8, hy + 22, 16, RAMP_CHROME + 5);
    vid_hline(cx - 6, hy + 23, 12, RAMP_CHROME + 4);

    /* crown / hair */
    switch (c->crown) {
        case 1: vid_rect(cx - 11, hy - 3, 22, 6, (u8)(rp + 6));
                vid_rect(cx - 13, hy - 1, 3, 12, (u8)(rp + 5));
                vid_rect(cx + 10, hy - 1, 3, 12, (u8)(rp + 5)); break;
        case 2: vid_rect(cx - 12, hy - 4, 24, 8, (u8)(rp + 8));
                vid_rect(cx - 14, hy + 1, 4, 20, (u8)(rp + 6));
                vid_rect(cx + 10, hy + 1, 4, 20, (u8)(rp + 6)); break;
        case 3: vid_rect(cx - 11, hy - 2, 22, 4, RAMP_CHROME + 3);
                vid_hline(cx - 11, hy - 3, 22, (u8)(rp + 10)); break;
        case 4: vid_rect(cx - 11, hy - 2, 22, 5, RAMP_CHROME + 12);
                vid_rect(cx - 12, hy + 2, 3, 6, RAMP_CHROME + 10);
                vid_rect(cx + 9, hy + 2, 3, 6, RAMP_CHROME + 10); break;
        case 5: vid_rect(cx - 12, hy - 5, 24, 9, RAMP_CHROME + 17);
                vid_hline(cx - 12, hy + 4, 24, RAMP_CHROME + 20); break;
        default: break;
    }

    /* eyes / visor */
    int ey = hy + 10;
    switch (c->visor) {
        case 1:
            vid_rect(cx - 10, ey - 1, 20, 5, C_BLACK);
            vid_hline(cx - 9, ey + 1, 18, c->accent);
            break;
        case 2:
            vid_rect(cx - 8, ey, 3, 3, C_WHITE);
            vid_rect(cx + 3, ey - 2, 8, 7, C_BLACK);
            vid_frame_rect(cx + 3, ey - 2, 8, 7, c->accent);
            vid_rect(cx + 5, ey, 3, 3, c->accent);
            break;
        case 3:
            vid_rect(cx - 11, ey - 3, 22, 9, C_BLACK);
            vid_frame_rect(cx - 11, ey - 3, 22, 9, (u8)(rp + 14));
            for (int i = 0; i < 4; i++)
                vid_vline(cx - 7 + i * 5, ey - 1, 5, (u8)(rp + 10 + ((t / 8 + i) % 8)));
            break;
        case 4:
            vid_rect(cx - 8, ey, 3, 3, C_WHITE);
            vid_rect(cx + 4, ey, 3, 3, c->accent);
            vid_line(cx + 5, ey - 5, cx + 3, ey + 8, (u8)(rp + 13));
            vid_plot(cx + 4, ey + 4, c->accent);
            break;
        default:
            vid_rect(cx - 8, ey, 3, 3, C_WHITE);
            vid_rect(cx + 4, ey, 3, 3, C_WHITE);
            vid_plot(cx - 7, ey + 1, C_BLACK);
            vid_plot(cx + 5, ey + 1, C_BLACK);
            break;
    }

    /* cheek / neck chrome */
    switch (c->mark) {
        case 1: vid_rect(cx + 6, ey + 6, 4, 1, c->accent);
                vid_rect(cx + 6, ey + 8, 4, 1, c->accent);
                vid_plot(cx + 10, ey + 7, c->accent); break;
        case 2: vid_rect(cx - 12, ey + 7, 3, 3, (u8)(rp + 16));
                vid_line(cx - 10, ey + 8, cx - 4, ey + 12, (u8)(rp + 12)); break;
        case 3: for (int i = 0; i < 3; i++) vid_plot(cx - 9 + i * 2, ey + 9, (u8)(rp + 18)); break;
        default: break;
    }

    /* mouth */
    vid_hline(cx - 3, hy + 19, 6, RAMP_CHROME + 3);

    /* frame */
    vid_frame_rect(x, y, PORT_W, PORT_H, c->accent);
    ui_brackets(x, y, PORT_W, PORT_H, C_WHITE, 3);
    /* live-feed tick */
    if ((t / 24) & 1) vid_rect(x + PORT_W - 5, y + 2, 2, 2, C_BLOOD);
}

/* ====================================================================== */
/*  dialogue                                                              */
/* ====================================================================== */

int ui_dialogue_len(const char *text) { return (int)ec_strlen(text); }

void ui_dialogue_box(int speaker, const char *text, int nchars, u32 t)
{
    const CharDef *c = &CHARS[speaker % CH_COUNT];
    int bx = 54, by = 96, bw = 182, bh = 50;

    ui_portrait(6, 94, speaker, t);

    vid_rect(bx, by, bw, bh, C_BLACK);
    vid_rect_dither(bx + 1, by + 1, bw - 2, bh - 2, C_BLACK, C_INDIGO);
    vid_frame_rect(bx, by, bw, bh, c->accent);
    ui_brackets(bx - 1, by - 1, bw + 2, bh + 2, c->accent, 5);

    int nw = vid_text_len(c->name);
    vid_rect(bx + 5, by - 5, nw + 6, 10, C_BLACK);
    vid_frame_rect(bx + 5, by - 5, nw + 6, 10, c->accent);
    vid_text(bx + 8, by - 4, c->name, c->accent);

    vid_text_wrap_n(bx + 5, by + 7, 28, text, C_TEXT, nchars);

    if (nchars >= (int)ec_strlen(text) && ((t / 16) & 1))
        vid_text(bx + bw - 11, by + bh - 11, "\x04", c->accent);
}
