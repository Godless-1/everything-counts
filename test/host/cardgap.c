/* EVERYTHING COUNTS - a card counting story for the Game Boy Advance
 * Copyright (C) 2026 Godless-1
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Draws every card (all 13 ranks, 4 suits, counting aid on and off) into a
 * plain 240x160 byte buffer with the real card_draw, and checks that the
 * rank number, which is drawn in the same colour as the outline, never sits
 * next to it: nothing in that colour may touch row 1 or column 1 of the
 * card inside the header area.  Also checks the compact card. */
#include <stdio.h>
#include <string.h>
#include "cards.h"
#include "video.h"
#include "gen_font.h"

extern const u8 g_font6x8[128*8];
static u8 fb[160][240];
static int px(int x, int y) { return x >= 0 && x < 240 && y >= 0 && y < 160; }
void vid_plot(int x, int y, u8 c) { if (px(x, y)) fb[y][x] = c; }
void vid_hline(int x, int y, int w, u8 c) { for (int i = 0; i < w; i++) vid_plot(x + i, y, c); }
void vid_vline(int x, int y, int h, u8 c) { for (int j = 0; j < h; j++) vid_plot(x, y + j, c); }
void vid_rect(int x, int y, int w, int h, u8 c) { for (int j = 0; j < h; j++) vid_hline(x, y + j, w, c); }
void vid_frame_rect(int x, int y, int w, int h, u8 c)
{ vid_hline(x, y, w, c); vid_hline(x, y + h - 1, w, c); vid_vline(x, y, h, c); vid_vline(x + w - 1, y, h, c); }
void vid_vgrad(int x, int y, int w, int h, int r, int a, int b) { (void)a; (void)b; vid_rect(x, y, w, h, (u8)r); }
void vid_hgrad(int x, int y, int w, int h, int r, int a, int b) { (void)a; (void)b; vid_rect(x, y, w, h, (u8)r); }
void vid_blit4(int x, int y, int w, int h, const u8 *s, const u8 *l) { (void)x; (void)y; (void)w; (void)h; (void)s; (void)l; }
void vid_blit4_scaled(int x, int y, int w, int h, const u8 *s, const u8 *l, int k) { (void)x; (void)y; (void)w; (void)h; (void)s; (void)l; (void)k; }
int vid_text(int x, int y, const char *s, u8 c)
{
    for (; *s; s++, x += 6) {
        const u8 *g = &g_font6x8[((u8)*s & 0x7F) * 8];
        for (int j = 0; j < 8; j++)
            for (int i = 0; i < 6; i++)
                if (g[j] & (0x80 >> i)) vid_plot(x + i, y + j, c);
    }
    return x;
}
u32 ec_strlen(const char *s) { u32 n = 0; while (s[n]) n++; return n; }
void *ec_memset(void *d, int c, u32 n) { return memset(d, c, n); }
void *ec_memcpy(void *d, const void *s, u32 n) { return memcpy(d, s, n); }
int vid_text_len(const char *s) { return (int)ec_strlen(s) * 6; }
char *str_int(char *d, int v) { (void)v; return d; }
char *str_cat(char *d, const char *s) { (void)s; return d; }

static const u8 suit_col[4] = { C_CYAN, C_MAG, C_AMBER, C_GREEN };

int main(void)
{
    int bad = 0, n = 0;
    for (int tag = 0; tag < 2; tag++)
    for (int su = 0; su < 4; su++)
    for (int r = 0; r < 13; r++) {
        memset(fb, 0, sizeof fb);
        int x = 20, y = 20;
        u8 card = (u8)((r << 2) | su);
        card_draw(x, y, card, tag ? TAG_SHOW : TAG_HIDE);
        u8 sc = suit_col[CARD_SUIT(card)];
        n++;
        /* rank area: top-left header, x+1..x+19, y+1..y+11 */
        for (int j = 1; j <= 11; j++)
            for (int i = 1; i <= 19; i++)
                if (fb[y + j][x + i] == sc && (j == 1 || i == 1)) {
                    printf("FAIL card su=%d rank=%d tag=%d: outline colour at (+%d,+%d) touches the border\n", su, r, tag, i, j);
                    bad++;
                }
        /* and the number itself must be drawn (not erased by the fix) */
        int ink = 0;
        for (int j = 2; j <= 10; j++) for (int i = 4; i <= 18; i++) if (fb[y + j][x + i] == sc) ink++;
        if (!ink) { printf("FAIL card su=%d rank=%d: no rank ink\n", su, r); bad++; }
        /* mini card */
        memset(fb, 0, sizeof fb);
        card_draw_mini(x, y, card);
        for (int j = 1; j <= 3; j++) for (int i = 1; i <= 12; i++)
            if (fb[y + j][x + i] == sc && (j == 1 || i == 1)) {
                printf("FAIL mini su=%d rank=%d: ink at (+%d,+%d) touches the border\n", su, r, i, j);
                bad++;
            }
    }
    printf("cardgap: %d cards, %d failures\n", n, bad);
    return bad != 0;
}
