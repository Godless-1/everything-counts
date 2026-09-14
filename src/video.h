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
 *  video.h - Mode 4 (8bpp paletted, page-flipped) software renderer
 * ===================================================================== */
#ifndef VIDEO_H
#define VIDEO_H
#include "gba.h"
#include "gen_art.h"
#include "gen_font.h"

extern u8 *vid_back;            /* current back buffer (byte addressable) */
extern u32 vid_frame;           /* frames elapsed since boot             */

void vid_init(void);
void vid_flip(void);            /* waits for VBlank, then shows back buf */
void vid_vsync(void);
void vid_set_palette(const u16 *pal);
void vid_fade(int amount);      /* 0 = normal, 16 = black (BLDY) */
void vid_fade_white(int amount);

/* ---- primitives ------------------------------------------------------ */
void vid_clear(u8 c);
void vid_plot(int x, int y, u8 c);
void vid_hline(int x, int y, int w, u8 c);
void vid_vline(int x, int y, int h, u8 c);
void vid_rect(int x, int y, int w, int h, u8 c);
void vid_frame_rect(int x, int y, int w, int h, u8 c);
void vid_rect_dither(int x, int y, int w, int h, u8 a, u8 b);
void vid_vgrad(int x, int y, int w, int h, int ramp, int from, int to);
void vid_hgrad(int x, int y, int w, int h, int ramp, int from, int to);
void vid_scanlines(int x, int y, int w, int h, u8 c);
void vid_vgrad_scan(int x, int y, int w, int h, int ramp, int from, int to, int dark);
void vid_hgrad_scan(int x, int y, int w, int h, int ramp, int from, int to, int dark);
void vid_rect_scan(int x, int y, int w, int h, u8 a, u8 b);
void vid_line(int x0, int y0, int x1, int y1, u8 c);

/* ---- bitmaps (4 shade levels: 0 clear, 1 dark, 2 mid, 3 solid) ------- */
void vid_blit4(int x, int y, int w, int h, const u8 *src, const u8 *lut);
void vid_blit4_scaled(int x, int y, int w, int h, const u8 *src, const u8 *lut, int s);

/* ---- text ------------------------------------------------------------ */
int  vid_text(int x, int y, const char *s, u8 c);
int  vid_text_s(int x, int y, const char *s, u8 c, int scale);
int  vid_text_shadow(int x, int y, const char *s, u8 c, u8 sh);
int  vid_text_glow(int x, int y, const char *s, u8 c, u8 glow, int scale);
void vid_text_center(int cx, int y, const char *s, u8 c);
void vid_text_center_s(int cx, int y, const char *s, u8 c, int scale);
int  vid_text_wrap(int x, int y, int cols, const char *s, u8 c);
int  vid_text_wrap_n(int x, int y, int cols, const char *s, u8 c, int nchars);
int  vid_text_wrap_lines(int cols, const char *s);
int  vid_wrap_pages(const char *s, int cols, int rows, u16 *offs, int maxp);
int  vid_text_len(const char *s);

/* ---- string helpers (no libc) ---------------------------------------- */
char *str_int(char *dst, int v);
char *str_uint_pad(char *dst, u32 v, int width, char pad);
char *str_money(char *dst, s32 v);       /* 12500 -> "12,500" */
char *str_signed(char *dst, int v);      /* 3 -> "+3" */
char *str_cat(char *dst, const char *s);
void  str_copy(char *dst, const char *s, int max);

#endif
