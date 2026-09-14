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
 *  video.c - Mode 4 software renderer.  Two 240x160 8bpp pages live in
 *  VRAM; we draw into the hidden one and flip on VBlank.
 * ===================================================================== */
#include "video.h"

u8 *vid_back;
u32 vid_frame;
static u32 dispcnt_base;

static volatile u32 vbl_count;

static void vbl_isr(void)
{
    vbl_count++;
}

void vid_init(void)
{
    REG_DISPCNT = DCNT_BLANK;
    dispcnt_base = DCNT_MODE4 | DCNT_BG2 | DCNT_OAM_1D;

    /* clear both pages */
    ec_memset((void *)0x06000000, 0, 0xA000 * 2);

    vid_back = (u8 *)MEM_VRAM_PAGE1;   /* page 1 hidden while page 0 shows */
    vid_frame = 0;

    vid_set_palette(g_palette);

    irq_handler_c = vbl_isr;
    REG_DISPSTAT |= DSTAT_VBL_IRQ;
    REG_IE = IRQ_VBLANK;
    REG_IF = 0xFFFF;
    REG_IME = 1;

    REG_DISPCNT = dispcnt_base;
}

void vid_set_palette(const u16 *pal)
{
    for (int i = 0; i < 256; i++) MEM_PAL_BG[i] = pal[i];
}

void vid_vsync(void)
{
    u32 t = vbl_count;
    while (vbl_count == t) bios_halt();
}

void vid_flip(void)
{
    vid_vsync();
    if (vid_back == (u8 *)MEM_VRAM_PAGE1) {
        REG_DISPCNT = dispcnt_base | DCNT_PAGE;   /* show page 1 */
        vid_back = (u8 *)MEM_VRAM;
    } else {
        REG_DISPCNT = dispcnt_base;               /* show page 0 */
        vid_back = (u8 *)MEM_VRAM_PAGE1;
    }
    vid_frame++;
}

void vid_fade(int amount)
{
    if (amount <= 0) { REG_BLDCNT = 0; return; }
    if (amount > 16) amount = 16;
    REG_BLDCNT = 0x00FF | (2 << 6);   /* all targets, fade to black */
    REG_BLDY   = amount;
}

void vid_fade_white(int amount)
{
    if (amount <= 0) { REG_BLDCNT = 0; return; }
    if (amount > 16) amount = 16;
    REG_BLDCNT = 0x00FF | (1 << 6);
    REG_BLDY   = amount;
}

/* ===================================================================== */
/*  primitives                                                            */
/* ===================================================================== */

IWRAM_CODE void vid_clear(u8 c)
{
    u32 w = c | (c << 8) | (c << 16) | (c << 24);
    static volatile u32 src;
    src = w;
    DMA[3].src = (const void *)&src;
    DMA[3].dst = vid_back;
    DMA[3].cnt = DMA_ENABLE | DMA_32 | DMA_SRC_FIX | ((SCREEN_W * SCREEN_H) / 4);
}

IWRAM_CODE void vid_plot(int x, int y, u8 c)
{
    if ((unsigned)x >= SCREEN_W || (unsigned)y >= SCREEN_H) return;
    u16 *p = (u16 *)(vid_back + ((y * SCREEN_W + x) & ~1));
    if (x & 1) *p = (*p & 0x00FF) | (c << 8);
    else       *p = (*p & 0xFF00) | c;
}

IWRAM_CODE void vid_hline(int x, int y, int w, u8 c)
{
    if ((unsigned)y >= SCREEN_H) return;
    if (x < 0) { w += x; x = 0; }
    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (w <= 0) return;

    u8 *base = vid_back + y * SCREEN_W;
    if (x & 1) {
        u16 *p = (u16 *)(base + x - 1);
        *p = (*p & 0x00FF) | (c << 8);
        x++; w--;
    }
    u16 hw = c | (c << 8);
    u16 *p = (u16 *)(base + x);
    while (w >= 2) { *p++ = hw; w -= 2; }
    if (w) *p = (*p & 0xFF00) | c;
}

IWRAM_CODE void vid_vline(int x, int y, int h, u8 c)
{
    if ((unsigned)x >= SCREEN_W) return;
    if (y < 0) { h += y; y = 0; }
    if (y + h > SCREEN_H) h = SCREEN_H - y;
    u8 *base = vid_back + y * SCREEN_W + (x & ~1);
    int hi = x & 1;
    while (h-- > 0) {
        u16 *p = (u16 *)base;
        if (hi) *p = (*p & 0x00FF) | (c << 8);
        else    *p = (*p & 0xFF00) | c;
        base += SCREEN_W;
    }
}

void vid_rect(int x, int y, int w, int h, u8 c)
{
    if (y < 0) { h += y; y = 0; }
    if (y + h > SCREEN_H) h = SCREEN_H - y;
    while (h-- > 0) vid_hline(x, y++, w, c);
}

void vid_frame_rect(int x, int y, int w, int h, u8 c)
{
    vid_hline(x, y, w, c);
    vid_hline(x, y + h - 1, w, c);
    vid_vline(x, y, h, c);
    vid_vline(x + w - 1, y, h, c);
}

void vid_rect_dither(int x, int y, int w, int h, u8 a, u8 b)
{
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            vid_plot(x + i, y + j, ((i + j) & 1) ? b : a);
}

void vid_vgrad(int x, int y, int w, int h, int ramp, int from, int to)
{
    if (h <= 0) return;
    for (int j = 0; j < h; j++) {
        int t = (h == 1) ? from : from + ((to - from) * j) / (h - 1);
        vid_hline(x, y + j, w, (u8)(ramp + t));
    }
}

/* Gradients that carry their own CRT banding: every other scanline steps
 * back down the ramp instead of being blacked out, so the picture keeps
 * its colour and still reads as a scanned display. */
void vid_vgrad_scan(int x, int y, int w, int h, int ramp, int from, int to, int dark)
{
    if (h <= 0) return;
    for (int j = 0; j < h; j++) {
        int t = (h == 1) ? from : from + ((to - from) * j) / (h - 1);
        if (j & 1) { t -= dark; if (t < 0) t = 0; }
        vid_hline(x, y + j, w, (u8)(ramp + t));
    }
}

void vid_hgrad_scan(int x, int y, int w, int h, int ramp, int from, int to, int dark)
{
    if (w <= 0) return;
    for (int i = 0; i < w; i++) {
        int t = (w == 1) ? from : from + ((to - from) * i) / (w - 1);
        for (int j = 0; j < h; j++) {
            int tt = t - ((j & 1) ? dark : 0);
            if (tt < 0) tt = 0;
            vid_plot(x + i, y + j, (u8)(ramp + tt));
        }
    }
}

void vid_rect_scan(int x, int y, int w, int h, u8 a, u8 b)
{
    for (int j = 0; j < h; j++) vid_hline(x, y + j, w, (j & 1) ? b : a);
}

void vid_hgrad(int x, int y, int w, int h, int ramp, int from, int to)
{
    if (w <= 0) return;
    for (int i = 0; i < w; i++) {
        int t = (w == 1) ? from : from + ((to - from) * i) / (w - 1);
        vid_vline(x + i, y, h, (u8)(ramp + t));
    }
}

void vid_scanlines(int x, int y, int w, int h, u8 c)
{
    for (int j = 0; j < h; j += 2) vid_hline(x, y + j, w, c);
}

void vid_line(int x0, int y0, int x1, int y1, u8 c)
{
    int dx = x1 - x0, dy = y1 - y0;
    int ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
    int sx = dx < 0 ? -1 : 1, sy = dy < 0 ? -1 : 1;
    if (ax > ay) {
        int e = ax >> 1;
        for (int i = 0; i <= ax; i++) {
            vid_plot(x0, y0, c);
            e -= ay; if (e < 0) { y0 += sy; e += ax; }
            x0 += sx;
        }
    } else {
        int e = ay >> 1;
        for (int i = 0; i <= ay; i++) {
            vid_plot(x0, y0, c);
            e -= ax; if (e < 0) { x0 += sx; e += ay; }
            y0 += sy;
        }
    }
}

/* ===================================================================== */
/*  bitmaps                                                               */
/* ===================================================================== */

void vid_blit4(int x, int y, int w, int h, const u8 *src, const u8 *lut)
{
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            u8 v = src[j * w + i];
            if (v) vid_plot(x + i, y + j, lut[v]);
        }
}

void vid_blit4_scaled(int x, int y, int w, int h, const u8 *src, const u8 *lut, int s)
{
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            u8 v = src[j * w + i];
            if (!v) continue;
            u8 c = lut[v];
            for (int b = 0; b < s; b++)
                vid_hline(x + i * s, y + j * s + b, s, c);
        }
}

/* ===================================================================== */
/*  text                                                                  */
/* ===================================================================== */

IWRAM_CODE static void glyph(int x, int y, u8 ch, u8 c)
{
    const u8 *g = &g_font6x8[(ch & 0x7F) * 8];
    for (int j = 0; j < 8; j++) {
        u8 row = g[j];
        if (!row) continue;
        int yy = y + j;
        if ((unsigned)yy >= SCREEN_H) continue;
        for (int i = 0; i < 6; i++)
            if (row & (0x80 >> i)) vid_plot(x + i, yy, c);
    }
}

static void glyph_s(int x, int y, u8 ch, u8 c, int s)
{
    if (s == 1) { glyph(x, y, ch, c); return; }
    const u8 *g = &g_font6x8[(ch & 0x7F) * 8];
    for (int j = 0; j < 8; j++) {
        u8 row = g[j];
        if (!row) continue;
        for (int i = 0; i < 6; i++)
            if (row & (0x80 >> i))
                for (int b = 0; b < s; b++)
                    vid_hline(x + i * s, y + j * s + b, s, c);
    }
}

int vid_text(int x, int y, const char *s, u8 c)
{
    while (*s) { glyph(x, y, (u8)*s++, c); x += FONT_W; }
    return x;
}

int vid_text_s(int x, int y, const char *s, u8 c, int scale)
{
    while (*s) { glyph_s(x, y, (u8)*s++, c, scale); x += FONT_W * scale; }
    return x;
}

int vid_text_shadow(int x, int y, const char *s, u8 c, u8 sh)
{
    vid_text(x + 1, y + 1, s, sh);
    return vid_text(x, y, s, c);
}

/* chromatic-aberration headline: magenta ghost left, cyan ghost right */
int vid_text_glow(int x, int y, const char *s, u8 c, u8 glow, int scale)
{
    vid_text_s(x - scale, y, s, glow, scale);
    vid_text_s(x + scale, y, s, glow, scale);
    vid_text_s(x, y - 1, s, glow, scale);
    return vid_text_s(x, y, s, c, scale);
}

int vid_text_len(const char *s) { return (int)ec_strlen(s) * FONT_W; }

void vid_text_center(int cx, int y, const char *s, u8 c)
{
    vid_text(cx - vid_text_len(s) / 2, y, s, c);
}

void vid_text_center_s(int cx, int y, const char *s, u8 c, int scale)
{
    vid_text_s(cx - (int)ec_strlen(s) * FONT_W * scale / 2, y, s, c, scale);
}

/* Word wrap. '\n' forces a break, '|' is a soft paragraph break.
 * Returns the number of lines drawn. */
int vid_text_wrap_n(int x, int y, int cols, const char *s, u8 c, int nchars)
{
    int line = 0, col = 0, shown = 0;
    const char *p = s;
    while (*p) {
        if (nchars >= 0 && shown >= nchars) break;
        if (*p == '\n') { p++; line++; col = 0; shown++; continue; }
        /* measure the next word */
        const char *w = p;
        int wl = 0;
        while (w[wl] && w[wl] != ' ' && w[wl] != '\n') wl++;
        if (col + wl > cols && col > 0) { line++; col = 0; }
        for (int i = 0; i < wl; i++) {
            if (nchars >= 0 && shown >= nchars) return line + 1;
            glyph(x + col * FONT_W, y + line * (FONT_H + 2), (u8)w[i], c);
            col++; shown++;
            if (col >= cols) { line++; col = 0; }
        }
        p = w + wl;
        if (*p == ' ') {
            p++;
            if (col > 0 && col < cols) { col++; }
            shown++;
        }
    }
    return line + 1;
}

int vid_text_wrap(int x, int y, int cols, const char *s, u8 c)
{
    return vid_text_wrap_n(x, y, cols, s, c, -1);
}

/* Records the character offset at which each page of `rows` wrapped lines
 * begins, mirroring vid_text_wrap_n's greedy layout exactly.  Returns the
 * page count (always at least 1). */
int vid_wrap_pages(const char *s, int cols, int rows, u16 *offs, int maxp)
{
    int line = 0, col = 0, np = 0;
    const char *p = s;
    if (maxp > 0) offs[np++] = 0;

    while (*p) {
        if (*p == '\n') {
            p++; line++; col = 0;
            if (line >= rows) {
                line = 0;
                while (*p == ' ') p++;
                if (*p && np < maxp) offs[np++] = (u16)(p - s);
            }
            continue;
        }
        const char *w = p;
        int wl = 0;
        while (w[wl] && w[wl] != ' ' && w[wl] != '\n') wl++;

        if (col + wl > cols && col > 0) {
            line++; col = 0;
            if (line >= rows) {            /* this word starts the next page */
                line = 0;
                if (np < maxp) offs[np++] = (u16)(w - s);
            }
        }
        col += wl;
        p = w + wl;
        if (col >= cols) {
            line++; col = 0;
            if (line >= rows) {
                line = 0;
                const char *q = p;
                while (*q == ' ') q++;
                if (*q && np < maxp) offs[np++] = (u16)(q - s);
            }
        }
        if (*p == ' ') {
            p++;
            if (col > 0 && col < cols) col++;
        }
    }
    return np ? np : 1;
}

int vid_text_wrap_lines(int cols, const char *s)
{
    int line = 0, col = 0;
    const char *p = s;
    while (*p) {
        if (*p == '\n') { p++; line++; col = 0; continue; }
        const char *w = p; int wl = 0;
        while (w[wl] && w[wl] != ' ' && w[wl] != '\n') wl++;
        if (col + wl > cols && col > 0) { line++; col = 0; }
        col += wl;
        if (col >= cols) { line++; col = 0; }
        p = w + wl;
        if (*p == ' ') { p++; if (col > 0 && col < cols) col++; }
    }
    return line + 1;
}

/* ===================================================================== */
/*  freestanding string helpers                                           */
/* ===================================================================== */

void *ec_memset(void *d, int c, u32 n)
{
    u8 *p = (u8 *)d;
    while (n--) *p++ = (u8)c;
    return d;
}

void *ec_memcpy(void *d, const void *s, u32 n)
{
    u8 *p = (u8 *)d; const u8 *q = (const u8 *)s;
    while (n--) *p++ = *q++;
    return d;
}

u32 ec_strlen(const char *s)
{
    u32 n = 0;
    while (s[n]) n++;
    return n;
}

char *str_int(char *dst, int v)
{
    char tmp[12];
    int n = 0, neg = 0;
    if (v < 0) { neg = 1; v = -v; }
    do { tmp[n++] = '0' + (v % 10); v /= 10; } while (v);
    if (neg) *dst++ = '-';
    while (n) *dst++ = tmp[--n];
    *dst = 0;
    return dst;
}

char *str_signed(char *dst, int v)
{
    if (v >= 0) *dst++ = '+';
    return str_int(dst, v);
}

char *str_uint_pad(char *dst, u32 v, int width, char pad)
{
    char tmp[12];
    int n = 0;
    do { tmp[n++] = '0' + (v % 10); v /= 10; } while (v);
    for (int i = n; i < width; i++) *dst++ = pad;
    while (n) *dst++ = tmp[--n];
    *dst = 0;
    return dst;
}

char *str_money(char *dst, s32 v)
{
    char tmp[12];
    int n = 0;
    if (v < 0) { *dst++ = '-'; v = -v; }
    do { tmp[n++] = '0' + (v % 10); v /= 10; } while (v);
    for (int i = n - 1; i >= 0; i--) {
        *dst++ = tmp[i];
        if (i && (i % 3) == 0) *dst++ = ',';
    }
    *dst = 0;
    return dst;
}

char *str_cat(char *dst, const char *s)
{
    while (*s) *dst++ = *s++;
    *dst = 0;
    return dst;
}

void str_copy(char *dst, const char *s, int max)
{
    int i = 0;
    while (s[i] && i < max - 1) { dst[i] = s[i]; i++; }
    dst[i] = 0;
}

/* GCC synthesises calls to these for struct copies; we are freestanding. */
void *memcpy(void *d, const void *s, u32 n) { return ec_memcpy(d, s, n); }
void *memset(void *d, int c, u32 n)         { return ec_memset(d, c, n); }
void *memmove(void *d, const void *s, u32 n)
{
    u8 *p = (u8 *)d; const u8 *q = (const u8 *)s;
    if (p == q || !n) return d;
    if (p < q) { while (n--) *p++ = *q++; }
    else { p += n; q += n; while (n--) *--p = *--q; }
    return d;
}
