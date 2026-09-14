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
 *  teach.c - the DRY RUN course player.
 * ===================================================================== */
#include "teach.h"
#include "table.h"

static int card_count(const u8 *p) { int n = 0; while (p && p[n] != NO_CARD) n++; return n; }

/* ---------------------------------------------------------------------- */
/*  the Hi-Lo tag table                                                    */
/* ---------------------------------------------------------------------- */
void teach_tag_table(int x, int y)
{
    static const char *grp[3] = { "2 3 4 5 6", "7 8 9", "10 J Q K A" };
    static const char *val[3] = { "+1", " 0", "-1" };
    static const char *note[3] = { "LOW - GOOD FOR YOU", "NEUTRAL", "HIGH - GOOD FOR THEM" };
    static const u8 col[3] = { C_GREEN, C_ICE, C_BLOOD };
    for (int i = 0; i < 3; i++) {
        int ry = y + i * 22;
        vid_rect(x, ry, 214, 19, C_BLACK);
        vid_frame_rect(x, ry, 214, 19, col[i]);
        vid_hgrad(x + 1, ry + 1, 30, 17, RAMP_VIOLET, 8, 0);
        vid_text_s(x + 6, ry + 2, val[i], col[i], 2);
        vid_text(x + 38, ry + 2, grp[i], C_WHITE);
        vid_text(x + 38, ry + 11, note[i], C_DIM);
    }
}

/* ---------------------------------------------------------------------- */
/*  the basic-strategy chart                                               */
/* ---------------------------------------------------------------------- */
static const char *UPS[10] = { "2","3","4","5","6","7","8","9","10","A" };

static const char *chart_row_label(int tab, int row)
{
    static char b[8];
    if (tab == 0) { str_int(b, row + 5); return b; }
    if (tab == 1) { char *p = str_cat(b, "A"); str_int(p, row + 2); return b; }
    {
        static const char *pr[10] = { "A,A","2,2","3,3","4,4","5,5","6,6","7,7","8,8","9,9","T,T" };
        return pr[row];
    }
}

/* the chart is rendered straight out of the strategy engine so it can
 * never drift from what the table actually grades you against */
static void chart_build_hand(int tab, int row, Hand *h)
{
    hand_clear(h);
    if (tab == 0) {
        int total = row + 5;
        if (total >= 21) {                 /* 21 has to be three cards */
            hand_add(h, CARD(9, 0));
            hand_add(h, CARD(8, 1));
            hand_add(h, CARD(1, 2));
        } else if (total <= 11) {
            hand_add(h, CARD(total - 2 - 1, 0));   /* e.g. 9 -> a seven */
            hand_add(h, CARD(1, 1));               /* plus a two       */
        } else {
            hand_add(h, CARD(9, 0));               /* a ten            */
            hand_add(h, CARD(total - 10 - 1, 1));
        }
    } else if (tab == 1) {
        hand_add(h, CARD(0, 0));
        hand_add(h, CARD(row + 1, 1));       /* A,2 .. A,9 */
    } else {
        static const u8 pr[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        hand_add(h, CARD(pr[row], 0));
        hand_add(h, CARD(pr[row], 1));
    }
}

static const u8 UPCARD[10] = { CARD(1,0),CARD(2,0),CARD(3,0),CARD(4,0),CARD(5,0),
                               CARD(6,0),CARD(7,0),CARD(8,0),CARD(9,0),CARD(0,0) };

void teach_chart_browser(void)
{
    int tab = 0, row = 0, up = 0, top = 0;
    static const int nrows[3] = { 17, 8, 10 };
    static const char *tabname[3] = { "HARD", "SOFT", "PAIRS" };
    const Venue *v = &VENUES[2];
    key_flush();
    for (;;) {
        app_frame_begin();
        if (key_repeat(KEY_DOWN))  { row++; sfx(SFX_MOVE); }
        if (key_repeat(KEY_UP))    { row--; sfx(SFX_MOVE); }
        if (key_repeat(KEY_RIGHT)) { up++;  sfx(SFX_MOVE); }
        if (key_repeat(KEY_LEFT))  { up--;  sfx(SFX_MOVE); }
        if (key_down(KEY_R)) { tab = (tab + 1) % 3; row = 0; top = 0; sfx(SFX_SELECT); }
        if (key_down(KEY_L)) { tab = (tab + 2) % 3; row = 0; top = 0; sfx(SFX_SELECT); }
        if (key_down(KEY_B) || key_down(KEY_START)) { sfx(SFX_BACK); return; }
        if (row < 0) row = nrows[tab] - 1;
        if (row >= nrows[tab]) row = 0;
        if (up < 0) up = 9;
        if (up > 9) up = 0;
        if (row < top) top = row;
        if (row > top + 6) top = row - 6;

        ui_backdrop(BG_TERMINAL, g_t, RAMP_CYAN);
        ui_titlebar("BASIC STRATEGY", tabname[tab], C_CYAN);

        /* column headers */
        vid_text(6, 16, "VS", C_DIM);
        for (int i = 0; i < 10; i++) {
            int cx = 34 + i * 20;
            vid_text(cx + (ec_strlen(UPS[i]) == 1 ? 4 : 1), 16, UPS[i],
                     (i == up) ? C_WHITE : C_MID);
            if (i == up) vid_rect(cx, 25, 18, 1, C_CYAN);
        }

        for (int r = 0; r < 7 && top + r < nrows[tab]; r++) {
            int rr = top + r;
            int ry = 28 + r * 13;
            u8 lab = (rr == row) ? C_WHITE : C_MID;
            if (rr == row) vid_hgrad(0, ry - 1, 240, 13, RAMP_VIOLET, 7, 0);
            vid_text(4, ry + 1, chart_row_label(tab, rr), lab);
            Hand h; chart_build_hand(tab, rr, &h);
            for (int i = 0; i < 10; i++) {
                int cds = hand_can_double(&h, v->das);
                int cs  = (tab == 2);
                int csu = v->surrender && hand_can_surrender(&h) && tab != 2;
                int a = bs_action(&h, UPCARD[i], v, cds, cs, csu);
                const char *g = (a == ACT_HIT) ? "H" : (a == ACT_STAND) ? "S" :
                                (a == ACT_DOUBLE) ? "D" : (a == ACT_SPLIT) ? "P" : "F";
                u8 c = (a == ACT_HIT) ? C_ICE : (a == ACT_STAND) ? C_BLOOD :
                       (a == ACT_DOUBLE) ? C_GREEN : (a == ACT_SPLIT) ? C_AMBER : C_PURPLE;
                int cx = 34 + i * 20;
                if (rr == row && i == up) {
                    vid_rect(cx, ry, 18, 11, c);
                    vid_text(cx + 6, ry + 1, g, C_BLACK);
                } else {
                    vid_text(cx + 6, ry + 1, g, c);
                }
            }
        }

        /* legend + plain english for the selected cell */
        vid_rect(0, 122, 240, 28, C_BLACK);
        vid_hline(0, 122, 240, C_GRID);
        vid_text(4, 125, "H HIT", C_ICE);
        vid_text(46, 125, "S STAND", C_BLOOD);
        vid_text(100, 125, "D DOUBLE", C_GREEN);
        vid_text(160, 125, "P SPLIT", C_AMBER);
        vid_text(208, 125, "F FOLD", C_PURPLE);

        {
            Hand h; chart_build_hand(tab, row, &h);
            int cds = hand_can_double(&h, v->das);
            int cs  = (tab == 2);
            int csu = v->surrender && hand_can_surrender(&h) && tab != 2;
            int a = bs_action(&h, UPCARD[up], v, cds, cs, csu);
            char b[64]; char *p = b;
            p = str_cat(p, tab == 2 ? "PAIR " : (tab == 1 ? "SOFT " : "HARD "));
            p = str_cat(p, chart_row_label(tab, row));
            p = str_cat(p, " vs ");
            p = str_cat(p, UPS[up]);
            p = str_cat(p, "  \x01  ");
            str_cat(p, action_name(a));
            vid_text(4, 137, b, C_WHITE);
        }
        ui_hint2("\x17 MOVE   \x15\x16 TAB", "\x14 BACK");
        app_frame_end();
    }
}

/* ---------------------------------------------------------------------- */
/*  page renderers                                                         */
/* ---------------------------------------------------------------------- */

static void page_header(const Lesson *L, int idx, const char *head)
{
    char b[24]; char *p = str_int(b, idx + 1);
    p = str_cat(p, "/"); str_int(p, L->npages);
    ui_titlebar("DRY RUN", b, C_CYAN);
    if (head) {
        vid_rect(0, 12, 240, 13, C_BLACK);
        vid_hline(0, 24, 240, C_CYAND);
        vid_text(6, 14, head, C_CYAN);
        vid_text(236 - vid_text_len(L->code), 14, L->code, C_DIM);
    }
}

static void draw_card_row(const u8 *cards, int y, int show_value)
{
    int n = card_count(cards);
    if (!n) return;
    int sp = (n <= 4) ? 36 : 30;
    int x = 120 - (n * sp - (sp - CARD_W)) / 2;
    for (int i = 0; i < n; i++) {
        card_draw(x + i * sp, y, cards[i], TAG_HIDE);
        if (show_value) {
            char b[12];
            int r = CARD_RANK(cards[i]);
            if (r == 0) str_copy(b, "1/11", sizeof(b));
            else str_int(b, card_value(cards[i]));
            int w = vid_text_len(b);
            vid_text(x + i * sp + CARD_W / 2 - w / 2, y + CARD_H + 3, b, C_AMBER);
        }
    }
}

static int page_text(const Lesson *L, int idx, const Page *p)
{
    key_flush();
    int chars = 0;
    int total = (int)ec_strlen(p->body);
    for (;;) {
        app_frame_begin();
        chars += 3;
        if (key_is(KEY_A) || key_is(KEY_B)) chars = total;
        if (chars > total) chars = total;
        if (key_down(KEY_A) && chars >= total) { sfx(SFX_SELECT); return 1; }
        if (key_down(KEY_B)) { sfx(SFX_BACK); return -1; }
        if (key_down(KEY_SELECT)) { sfx(SFX_BACK); return -2; }

        ui_backdrop(BG_TERMINAL, g_t, RAMP_CYAN);
        page_header(L, idx, p->head);

        int ny = 30;
        if (p->kind == PT_CARDS) {
            vid_text_wrap_n(6, 30, 38, p->body, C_TEXT, chars);
            draw_card_row(p->cards, 92, 1);
        } else if (p->kind == PT_HAND) {
            vid_text(6, 28, "DEALER SHOWS", C_DIM);
            if (p->dealer) {
                int n = card_count(p->dealer);
                for (int i = 0; i < n; i++) card_draw(92 + i * 34, 26, p->dealer[i], TAG_HIDE);
                card_draw_back(92 + n * 34, 26);
            }
            vid_text(6, 74, "YOU HOLD", C_DIM);
            if (p->cards) {
                int n = card_count(p->cards);
                for (int i = 0; i < n; i++) card_draw(92 + i * 34, 72, p->cards[i], TAG_HIDE);
                Hand h; hand_clear(&h);
                for (int i = 0; i < n; i++) hand_add(&h, p->cards[i]);
                char b[20]; char *q = b;
                if (hand_is_soft(&h)) q = str_cat(q, "SOFT ");
                str_int(q, hand_total(&h));
                vid_text(6, 86, b, C_WHITE);
            }
            vid_rect(0, 116, 240, 34, C_BLACK);
            vid_hline(0, 116, 240, C_GRID);
            vid_text_wrap_n(4, 119, 38, p->body, C_TEXT, chars);
        } else {
            vid_text_wrap_n(6, ny, 38, p->body, C_TEXT, chars);
        }
        ui_hint2(chars >= total ? "\x13 NEXT   \x14 BACK" : "\x13 SKIP", "SELECT: LEAVE");
        app_frame_end();
    }
}

static int page_speak(const Lesson *L, int idx, const Page *p)
{
    u16 offs[8];
    int np = vid_wrap_pages(p->body, 28, 4, offs, 8);
    for (int pg = 0; pg < np; pg++) {
        const char *txt = p->body + offs[pg];
        int total = (pg + 1 < np) ? (int)(offs[pg + 1] - offs[pg]) : (int)ec_strlen(txt);
        int chars = 0;
        key_flush();
        for (;;) {
            app_frame_begin();
            chars += 2;
            if (key_is(KEY_A)) chars = total;
            if (chars > total) chars = total;
            if (key_down(KEY_A) && chars >= total) { sfx(SFX_SELECT); break; }
            if (key_down(KEY_B)) { sfx(SFX_BACK); return -1; }
            if (key_down(KEY_SELECT)) { sfx(SFX_BACK); return -2; }
        if (key_down(KEY_SELECT)) { sfx(SFX_BACK); return -2; }

            ui_backdrop(BG_CLINIC, g_t, RAMP_GREEN);
            page_header(L, idx, p->head);
            vid_text_center_s(120, 42, L->name, C_WHITE, 2);
            vid_text_center(120, 66, "TRAINING CONSTRUCT ACTIVE", C_GREEN);
            ui_dialogue_box(p->speaker, txt, chars, g_t);
            if (chars >= total) ui_hint(pg + 1 < np ? "\x13 MORE" : "\x13 NEXT");
            app_frame_end();
        }
    }
    return 1;
}

static int page_quiz(const Lesson *L, int idx, const Page *p)
{
    int sel = 0, answered = -1;
    key_flush();
    for (;;) {
        app_frame_begin();
        if (answered < 0) {
            if (key_repeat(KEY_DOWN)) { sel = (sel + 1) % p->nopt; sfx(SFX_MOVE); }
            if (key_repeat(KEY_UP))   { sel = (sel + p->nopt - 1) % p->nopt; sfx(SFX_MOVE); }
            if (key_down(KEY_A)) {
                answered = sel;
                sfx(answered == p->correct ? SFX_GOOD : SFX_BAD);
            }
            if (key_down(KEY_B)) { sfx(SFX_BACK); return -1; }
            if (key_down(KEY_SELECT)) { sfx(SFX_BACK); return -2; }
        if (key_down(KEY_SELECT)) { sfx(SFX_BACK); return -2; }
        } else {
            if (key_down(KEY_A)) { sfx(SFX_SELECT); return 1; }
            if (key_down(KEY_B)) { answered = -1; sfx(SFX_BACK); }
        }

        ui_backdrop(BG_TERMINAL, g_t, RAMP_MAG);
        page_header(L, idx, p->head);

        if (answered < 0) {
            vid_text_wrap(6, 30, 38, p->body, C_TEXT);
            int oy = 30 + vid_text_wrap_lines(38, p->body) * 10 + 6;
            if (oy > 74) oy = 74;
            for (int i = 0; i < p->nopt; i++) {
                int by = oy + i * 15;
                u8 c = (i == sel) ? C_CYAN : C_GRID;
                u8 tc = (i == sel) ? C_WHITE : C_MID;
                vid_rect(10, by, 220, 13, C_BLACK);
                vid_frame_rect(10, by, 220, 13, c);
                char lab[2] = { (char)('A' + i), 0 };
                vid_text(15, by + 2, lab, c);
                vid_text(30, by + 2, p->opt[i], tc);
                if (i == sel) vid_text(218, by + 2, "\x01", C_CYAN);
            }
            ui_hint("\x03\x04 CHOOSE   \x13 ANSWER");
        } else {
            int ok = (answered == p->correct);
            u8 col = ok ? C_GREEN : C_BLOOD;
            vid_rect(0, 28, 240, 18, C_BLACK);
            vid_hline(0, 28, 240, col);
            vid_hline(0, 45, 240, col);
            vid_text_center(120, 33, ok ? "\x10  RIGHT" : "\x11  NOT QUITE", col);

            if (!ok) {
                vid_text(6, 52, "YOU SAID", C_DIM);
                vid_text(60, 52, p->opt[answered], C_BLOOD);
            }
            vid_text(6, !ok ? 64 : 56, "ANSWER", C_DIM);
            vid_text(60, !ok ? 64 : 56, p->opt[p->correct], C_GREEN);

            int wy = !ok ? 80 : 72;
            vid_hline(6, wy - 6, 228, C_GRID);
            vid_text_wrap(6, wy, 38, p->why, C_TEXT);
            ui_hint2("\x14 TRY AGAIN", "\x13 CONTINUE");
        }
        app_frame_end();
    }
}

static int page_tags(const Lesson *L, int idx, const Page *p)
{
    key_flush();
    for (;;) {
        app_frame_begin();
        if (key_down(KEY_A)) { sfx(SFX_SELECT); return 1; }
        if (key_down(KEY_B)) { sfx(SFX_BACK); return -1; }
        if (key_down(KEY_SELECT)) { sfx(SFX_BACK); return -2; }
        ui_backdrop(BG_TERMINAL, g_t, RAMP_GREEN);
        page_header(L, idx, p->head);
        teach_tag_table(13, 30);
        vid_text_wrap(6, 100, 38, p->body, C_TEXT);
        ui_hint("\x13 NEXT    \x14 BACK");
        app_frame_end();
    }
}

static int page_countdemo(const Lesson *L, int idx, const Page *p)
{
    int n = card_count(p->cards);
    int shown = 0, timer = 0, rc = 0;
    key_flush();
    for (;;) {
        app_frame_begin();
        timer++;
        if (timer > 42 && shown < n) {
            rc += card_tag(p->cards[shown]);
            shown++; timer = 0; sfx(SFX_DEAL);
        }
        if (key_down(KEY_A)) {
            if (shown < n) { while (shown < n) { rc += card_tag(p->cards[shown]); shown++; } }
            else { sfx(SFX_SELECT); return 1; }
        }
        if (key_down(KEY_B)) { sfx(SFX_BACK); return -1; }
        if (key_down(KEY_SELECT)) { sfx(SFX_BACK); return -2; }

        ui_backdrop(BG_TERMINAL, g_t, RAMP_GOLD);
        page_header(L, idx, p->head);
        vid_text_wrap(6, 28, 38, p->body, C_TEXT);

        for (int i = 0; i < n; i++) {
            int x = 8 + (i % 4) * 58, y = 52 + (i / 4) * 30;
            if (i < shown) {
                card_draw_mini(x, y, p->cards[i]);
                int tg = card_tag(p->cards[i]);
                char b[6];
                str_signed(b, tg);
                vid_text(x + 18, y + 5, b, tg > 0 ? C_GREEN : (tg < 0 ? C_BLOOD : C_ICE));
            } else {
                vid_rect(x, y, 14, 18, C_BLACK);
                vid_frame_rect(x, y, 14, 18, C_GRID);
            }
        }

        vid_rect(150, 52, 84, 48, C_BLACK);
        vid_frame_rect(150, 52, 84, 48, C_AMBER);
        ui_brackets(149, 51, 86, 50, C_AMBER, 5);
        vid_text_center(192, 56, "RUNNING", C_AMBER);
        vid_text_center(192, 66, "COUNT", C_AMBER);
        char b[8]; str_signed(b, rc);
        vid_text_center_s(192, 78, b, C_WHITE, 2);

        ui_hint(shown < n ? "\x13 SKIP AHEAD" : "\x13 NEXT");
        app_frame_end();
    }
}

static int page_chart(const Lesson *L, int idx, const Page *p)
{
    key_flush();
    for (;;) {
        app_frame_begin();
        if (key_down(KEY_A)) { teach_chart_browser(); key_flush(); }
        if (key_down(KEY_R)) { sfx(SFX_SELECT); return 1; }
        if (key_down(KEY_B)) { sfx(SFX_BACK); return -1; }
        if (key_down(KEY_SELECT)) { sfx(SFX_BACK); return -2; }
        ui_backdrop(BG_TERMINAL, g_t, RAMP_CYAN);
        page_header(L, idx, p->head);
        vid_text_wrap(6, 30, 38, p->body, C_TEXT);
        vid_rect(40, 86, 160, 22, C_BLACK);
        vid_frame_rect(40, 86, 160, 22, C_CYAN);
        ui_brackets(39, 85, 162, 24, C_CYAN, 5);
        vid_text_center(120, 92, "\x13  OPEN THE CHART", C_WHITE);
        ui_hint2("\x13 OPEN CHART", "\x16 CONTINUE");
        app_frame_end();
    }
}

static int page_practice(const Lesson *L, int idx, const Page *p)
{
    key_flush();
    for (;;) {
        app_frame_begin();
        if (key_down(KEY_A)) {
            sfx(SFX_SELECT);
            app_fade_out(14);
            sc_table(TABLE_ARG(0, TMODE_PRACTICE));
            app_fade_in(14);
            key_flush();
            return 1;
        }
        if (key_down(KEY_R)) { sfx(SFX_SELECT); return 1; }
        if (key_down(KEY_B)) { sfx(SFX_BACK); return -1; }
        if (key_down(KEY_SELECT)) { sfx(SFX_BACK); return -2; }
        ui_backdrop(BG_FELT, g_t, RAMP_GREEN);
        page_header(L, idx, p->head);
        vid_text_wrap(6, 30, 38, p->body, C_TEXT);
        vid_rect(30, 92, 180, 24, C_BLACK);
        vid_frame_rect(30, 92, 180, 24, C_GREEN);
        ui_brackets(29, 91, 182, 26, C_GREEN, 5);
        vid_text_center(120, 99, "\x13  SIT DOWN AT THE SIM", C_WHITE);
        ui_hint2("\x13 PLAY", "\x16 SKIP");
        app_frame_end();
    }
}

/* ---------------------------------------------------------------------- */
int teach_run_lesson(int index)
{
    const Lesson *L = &LESSONS[index];
    int i = 0;
    mus_play(MUS_NEON_RAIN);
    while (i < L->npages) {
        const Page *p = &L->page[i];
        int r;
        switch (p->kind) {
            case PT_QUIZ:      r = page_quiz(L, i, p); break;
            case PT_TAGS:      r = page_tags(L, i, p); break;
            case PT_COUNTDEMO: r = page_countdemo(L, i, p); break;
            case PT_CHART:     r = page_chart(L, i, p); break;
            case PT_PRACTICE:  r = page_practice(L, i, p); break;
            case PT_SPEAK:     r = page_speak(L, i, p); break;
            default:           r = page_text(L, i, p); break;
        }
        if (r == -2) return 0;              /* SELECT walks out of the sim */
        if (r > 0) i++;
        else if (i > 0) i--;
        else return 0;
    }
    return 1;
}
