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
 *  sc_codex.c - the reference the player can open at any time.
 * ===================================================================== */
#include "app.h"
#include "teach.h"

enum { CX_RULES, CX_GLOSS, CX_CHART, CX_COUNT, CX_INDEX, CX_ROOMS, CX_CHROME, CX_STATS, CX_N };

static const char *SECT[CX_N] = {
    "HOW TO PLAY", "GLOSSARY", "STRATEGY CHART", "THE COUNT",
    "INDEX PLAYS", "THE ROOMS", "CHROME", "YOUR NUMBERS"
};
static const u8 SCOL[CX_N] = {
    C_CYAN, C_ICE, C_GREEN, C_AMBER, C_PURPLE, C_ORANGE, C_MAG, C_WHITE
};

/* ---- plain-language rules -------------------------------------------- */
static const char *RULE_H[] = {
 "THE OBJECT", "CARD VALUES", "THE ACE", "THE DEAL",
 "YOUR OPTIONS", "THE DEALER", "PAYOUTS", "INSURANCE"
};
static const char *RULE_B[] = {
 "Beat the dealer's total without going over 21.\n\nGoing over 21 is a BUST and you lose immediately - before the dealer draws a single card. That rule, and only that rule, is where the house edge comes from.",
 "2 through 10 are worth their number.\nJACK, QUEEN and KING are worth 10.\nACE is worth 11 or 1.\n\nSuits do nothing. Sixteen of the 52 cards in a deck are worth ten.",
 "An ace counts as 11 unless that would bust you, in which case it counts as 1.\n\nA hand where the ace can still be 11 is called SOFT. A soft hand cannot bust on the next card.",
 "You bet. You get two cards face up.\nThe dealer gets one face up - the UP CARD - and one face down - the HOLE CARD.\n\nYou play your hand first. The dealer only acts once you are done.",
 "HIT      take another card\nSTAND    stop, keep your total\nDOUBLE   double the bet, take exactly one card\nSPLIT    two equal cards become two hands\nFOLD     surrender, lose half the bet",
 "The dealer follows a fixed rule and makes no choices.\n\nDraw on 16 or less. Stand on 17 or more.\n\nAt an H17 table the dealer also draws a SOFT 17. That costs you about 0.22%.",
 "Win        1 to 1\nPush (tie) bet returned\nBlackjack  3 to 2\n\nBLACKJACK is an ace with a ten-value card in the first two cards.\n\nA table paying 6 to 5 costs you 1.39%. Never sit at one.",
 "Offered only when the dealer shows an ace. Costs half your bet, pays 2 to 1 if the hole card is a ten.\n\nIt loses money at a neutral shoe. It makes money at true count +3 or better - and that is the single most valuable thing counting buys you."
};
#define NRULEP 8

static const char *GLOSS[] = {
 "SHOE",       "The box the dealer deals from. It holds several decks at once.",
 "PENETRATION","How deep into the shoe they deal before reshuffling. Deeper is better for a counter.",
 "CUT CARD",   "The marker that says stop dealing and shuffle. Everything after it is wasted information.",
 "HARD / SOFT","A soft hand holds an ace still counting as 11. It cannot bust on one card.",
 "STIFF",      "A total of 12 to 16. Too low to stand on happily, too high to hit safely.",
 "PUSH",       "A tie. Your money comes back.",
 "UP CARD",    "The dealer's face-up card. The only thing you know about their hand.",
 "HOLE CARD",  "The dealer's face-down card.",
 "UNIT",       "Your base bet. Everything is measured in units, never in credits.",
 "SPREAD",     "The ratio between your smallest and largest bet. Wide spreads win money and get you caught.",
 "RUNNING COUNT","The raw Hi-Lo total since the shuffle.",
 "TRUE COUNT", "Running count divided by decks remaining. This is the number decisions use.",
 "INDEX PLAY", "A hand where a high or low count makes basic strategy wrong.",
 "HEAT",       "How hard the pit is looking at you.",
 "BACKED OFF", "Politely told you may no longer play this game.",
 "WIRE",       "Detectable telemetry from your chrome. The pit scans for it.",
 "DRIFT",      "How much of the counting is no longer being done by you.",
};
#define NGLOSS ((int)(sizeof(GLOSS)/sizeof(char*)/2))

static const char *I18[] = {
 "INSURANCE       TC +3 or more   TAKE IT",
 "16 vs 10        TC  0 or more   STAND",
 "15 vs 10        TC +4 or more   STAND",
 "10,10 vs 5      TC +5 or more   SPLIT",
 "10,10 vs 6      TC +4 or more   SPLIT",
 "10 vs 10        TC +4 or more   DOUBLE",
 "12 vs 3         TC +2 or more   STAND",
 "12 vs 2         TC +3 or more   STAND",
 "11 vs A         TC +1 or more   DOUBLE",
 "9 vs 2          TC +1 or more   DOUBLE",
 "10 vs A         TC +4 or more   DOUBLE",
 "9 vs 7          TC +3 or more   DOUBLE",
 "16 vs 9         TC +5 or more   STAND",
 "13 vs 2         TC -1 or less   HIT",
 "12 vs 4         TC -1 or less   HIT",
 "12 vs 5         TC -2 or less   HIT",
 "12 vs 6         TC -1 or less   HIT",
 "13 vs 3         TC -2 or less   HIT",
};
#define NI18 ((int)(sizeof(I18)/sizeof(char*)))

/* ---------------------------------------------------------------------- */
static void codex_pages(const char **heads, const char **bodies, int n, u8 col)
{
    int p = 0;
    key_flush();
    for (;;) {
        app_frame_begin();
        if (key_repeat(KEY_RIGHT) && p < n - 1) { p++; sfx(SFX_MOVE); }
        if (key_repeat(KEY_LEFT)  && p > 0)     { p--; sfx(SFX_MOVE); }
        if (key_down(KEY_B)) { sfx(SFX_BACK); return; }
        ui_backdrop(BG_TERMINAL, g_t, RAMP_CYAN);
        ui_titlebar("CODEX", "B: BACK", col);
        ui_panel(8, 24, 224, 112, col, heads[p]);
        vid_text_wrap(14, 34, 36, bodies[p], C_TEXT);
        for (int i = 0; i < n; i++)
            vid_rect(120 - n * 5 + i * 10, 140, 7, 3, i == p ? col : C_GRID);
        ui_hint("\x02\x01 PAGE    \x14 BACK");
        app_frame_end();
    }
}

static void codex_list(const char **lines, int n, int stride, u8 col, const char *title)
{
    int top = 0, sel = 0;
    key_flush();
    for (;;) {
        app_frame_begin();
        if (key_repeat(KEY_DOWN)) { sel++; sfx(SFX_MOVE); }
        if (key_repeat(KEY_UP))   { sel--; sfx(SFX_MOVE); }
        if (key_down(KEY_B)) { sfx(SFX_BACK); return; }
        if (sel < 0) sel = n - 1;
        if (sel >= n) sel = 0;
        int rows = stride == 2 ? 9 : 10;
        if (sel < top) top = sel;
        if (sel > top + rows - 1) top = sel - rows + 1;

        ui_backdrop(BG_TERMINAL, g_t, RAMP_VIOLET);
        ui_titlebar(title, "B: BACK", col);
        for (int i = 0; i < rows && top + i < n; i++) {
            int r = top + i, y = 16 + i * 11;
            u8 c = (r == sel) ? C_WHITE : C_MID;
            if (r == sel) vid_hgrad(0, y - 1, 240, 11, RAMP_VIOLET, 9, 0);
            vid_text(6, y, lines[r * stride], (r == sel) ? col : C_MID);
            (void)c;
        }
        if (stride == 2) {
            vid_rect(0, 118, 240, 32, C_BLACK);
            vid_hline(0, 118, 240, col);
            vid_text_wrap(4, 122, 38, lines[sel * stride + 1], C_TEXT);
        }
        ui_hint("\x03\x04 SCROLL   \x14 BACK");
        app_frame_end();
    }
}

static void codex_counting(void)
{
    int page = 0;
    key_flush();
    for (;;) {
        app_frame_begin();
        if (key_repeat(KEY_RIGHT) && page < 2) { page++; sfx(SFX_MOVE); }
        if (key_repeat(KEY_LEFT)  && page > 0) { page--; sfx(SFX_MOVE); }
        if (key_down(KEY_B)) { sfx(SFX_BACK); return; }
        ui_backdrop(BG_TERMINAL, g_t, RAMP_GOLD);
        vid_rect(0, 12, 240, 132, C_BLACK);
        vid_rect_dither(0, 12, 240, 132, C_BLACK, C_INDIGO);
        ui_titlebar("THE COUNT", "B: BACK", C_AMBER);
        if (page == 0) {
            vid_text(6, 16, "HI-LO TAGS", C_AMBER);
            teach_tag_table(13, 28);
            vid_text_wrap(6, 100, 38,
              "Add the tags as cards are exposed. Start at zero after every shuffle. That total is the RUNNING COUNT.", C_TEXT);
        } else if (page == 1) {
            vid_text(6, 16, "TRUE COUNT", C_AMBER);
            vid_text(6, 28, "RUNNING COUNT / DECKS REMAINING", C_WHITE);
            vid_text_wrap(6, 42, 38,
              "+6 is huge with one deck left and nearly nothing with six. Judge the decks left from the discard tray, to the nearest half.", C_TEXT);
            vid_rect(20, 84, 200, 44, C_BLACK);
            vid_frame_rect(20, 84, 200, 44, C_AMBER);
            vid_text(26, 90, "RC +9   3 DECKS   \x01   TC +3", C_WHITE);
            vid_text(26, 102, "RC +9   6 DECKS   \x01   TC +1", C_WHITE);
            vid_text(26, 114, "RC -4   2 DECKS   \x01   TC -2", C_WHITE);
        } else {
            vid_text(6, 16, "BET RAMP", C_AMBER);
            vid_text(6, 28, "BET (TRUE COUNT - 1) UNITS, MIN ONE.", C_WHITE);
            static const char *ramp[7] = {
              "TC  -2      1 UNIT", "TC  -1      1 UNIT", "TC   0      1 UNIT",
              "TC  +1      1 UNIT", "TC  +2      1 UNIT", "TC  +3      2 UNITS",
              "TC  +5      4 UNITS" };
            for (int i = 0; i < 7; i++)
                vid_text(40, 42 + i * 11, ramp[i], i >= 5 ? C_GREEN : C_MID);
            vid_text(6, 122, "COUNTING PAYS NOTHING UNLESS THE", C_DIM);
            vid_text(6, 132, "MONEY MOVES WITH IT.", C_DIM);
        }
        for (int i = 0; i < 3; i++) vid_rect(110 + i * 10, 148, 7, 3, i == page ? C_AMBER : C_GRID);
        ui_hint("\x02\x01 PAGE    \x14 BACK");
        app_frame_end();
    }
}

static void codex_rooms(void)
{
    int sel = 0;
    key_flush();
    for (;;) {
        app_frame_begin();
        if (key_repeat(KEY_DOWN)) { sel = (sel + 1) % NVENUES; sfx(SFX_MOVE); }
        if (key_repeat(KEY_UP))   { sel = (sel + NVENUES - 1) % NVENUES; sfx(SFX_MOVE); }
        if (key_down(KEY_B)) { sfx(SFX_BACK); return; }
        const Venue *v = &VENUES[sel];
        ui_backdrop(BG_TERMINAL, g_t, v->ramp);
        ui_titlebar("THE ROOMS", "B: BACK", v->accent);
        for (int i = 0; i < NVENUES; i++) {
            int y = 16 + i * 11;
            if (i == sel) vid_hgrad(0, y - 1, 240, 11, RAMP_VIOLET, 9, 0);
            vid_text(8, y, VENUES[i].name, i == sel ? C_WHITE : VENUES[i].accent);
            char b[16]; int e = venue_edge_x100(&VENUES[i]);
            char *p = str_int(b, e / 100); *p++ = '.';
            p = str_uint_pad(p, (u32)(e % 100), 2, '0'); str_cat(p, "%");
            vid_text(200, y, b, e > 100 ? C_BLOOD : (e < 35 ? C_GREEN : C_AMBER));
        }
        vid_rect(0, 86, 240, 64, C_BLACK);
        vid_hline(0, 86, 240, v->accent);
        char b[48], *p;
        p = str_int(b, v->decks); p = str_cat(p, " DECKS   ");
        p = str_int(p, v->pen); str_cat(p, "% PENETRATION");
        vid_text(4, 90, b, C_TEXT);
        p = str_cat(b, v->h17 ? "H17  " : "S17  ");
        p = str_cat(p, v->das ? "DAS  " : "NO DAS  ");
        p = str_cat(p, v->surrender ? "SURRENDER  " : "");
        str_cat(p, v->rsa ? "RSA" : "");
        vid_text(4, 101, b, C_MID);
        vid_text(4, 112, v->bj65 ? "BLACKJACK PAYS 6:5 - A TAX" : "BLACKJACK PAYS 3:2", v->bj65 ? C_BLOOD : C_GREEN);
        vid_text_wrap(4, 124, 38, v->blurb, C_DIM);
        ui_hint("\x03\x04 ROOM    \x14 BACK");
        app_frame_end();
    }
}

static void codex_chrome(void)
{
    int sel = 0;
    key_flush();
    for (;;) {
        app_frame_begin();
        if (key_repeat(KEY_DOWN)) { sel = (sel + 1) % IMP_COUNT; sfx(SFX_MOVE); }
        if (key_repeat(KEY_UP))   { sel = (sel + IMP_COUNT - 1) % IMP_COUNT; sfx(SFX_MOVE); }
        if (key_down(KEY_B)) { sfx(SFX_BACK); return; }
        const Implant *im = &IMPLANTS[sel];
        ui_backdrop(BG_TERMINAL, g_t, RAMP_MAG);
        ui_titlebar("CHROME", "B: BACK", C_MAG);
        for (int i = 0; i < IMP_COUNT; i++) {
            int y = 16 + i * 10;
            if (i == sel) vid_hgrad(0, y - 1, 240, 10, RAMP_VIOLET, 9, 0);
            vid_text(8, y, IMPLANTS[i].name, i == sel ? C_WHITE : IMPLANTS[i].colour);
            if (g_run.implant[i]) vid_text(218, y, "IN", C_GREEN);
        }
        vid_rect(0, 100, 240, 50, C_BLACK);
        vid_hline(0, 100, 240, im->colour);
        vid_text_wrap(4, 104, 38, im->effect, C_TEXT);
        vid_text_wrap(4, 124, 38, im->warning, C_CANDY);
        ui_hint("\x03\x04 ITEM    \x14 BACK");
        app_frame_end();
    }
}

static void codex_stats(void)
{
    key_flush();
    for (;;) {
        app_frame_begin();
        if (key_down(KEY_B) || key_down(KEY_A)) { sfx(SFX_BACK); return; }
        ui_backdrop(BG_TERMINAL, g_t, RAMP_CHROME);
        ui_titlebar("YOUR NUMBERS", "B: BACK", C_WHITE);
        char b[48], *p;
        int y = 20;
        p = str_cat(b, "HANDS PLAYED       "); str_int(p, g_run.hands_played);
        vid_text(8, y, b, C_TEXT); y += 12;
        p = str_cat(b, "BASIC STRATEGY     "); p = str_int(p, run_bs_accuracy()); str_cat(p, "%");
        vid_text(8, y, b, C_CYAN); y += 12;
        p = str_cat(b, "COUNT CHECKS       "); p = str_int(p, run_check_accuracy()); str_cat(p, "%");
        vid_text(8, y, b, C_GREEN); y += 12;
        p = str_cat(b, "BEST WIN STREAK    "); str_int(p, g_run.best_streak);
        vid_text(8, y, b, C_AMBER); y += 12;
        p = str_cat(b, "BIGGEST POT        "); *p++ = 0x09; str_money(p, g_run.biggest_win);
        vid_text(8, y, b, C_WHITE); y += 16;
        vid_text(8, y, "RANGE BESTS", C_DIM); y += 12;
        static const char *dn[5] = { "TAGS", "SPEED COUNT", "TRUE COUNT", "BASIC", "INDEX" };
        for (int i = 0; i < 5; i++) {
            p = str_cat(b, dn[i]);
            vid_text(16, y, b, C_MID);
            p = str_int(b, g_run.best_drill[i]); str_cat(p, "%");
            vid_text(160, y, b, g_run.best_drill[i] >= 90 ? C_GREEN : C_MID);
            y += 11;
        }
        ui_hint("\x14 BACK");
        app_frame_end();
    }
}

int sc_codex(int arg)
{
    (void)arg;
    int sel = 0;
    key_flush();
    app_fade_in(10);
    for (;;) {
        app_frame_begin();
        if (key_repeat(KEY_DOWN)) { sel = (sel + 1) % CX_N; sfx(SFX_MOVE); }
        if (key_repeat(KEY_UP))   { sel = (sel + CX_N - 1) % CX_N; sfx(SFX_MOVE); }
        if (key_down(KEY_B)) { sfx(SFX_BACK); app_fade_out(10); return SC_QUIT; }
        if (key_down(KEY_A)) {
            sfx(SFX_SELECT);
            switch (sel) {
                case CX_RULES:  codex_pages(RULE_H, RULE_B, NRULEP, C_CYAN); break;
                case CX_GLOSS:  codex_list(GLOSS, NGLOSS, 2, C_ICE, "GLOSSARY"); break;
                case CX_CHART:  teach_chart_browser(); break;
                case CX_COUNT:  codex_counting(); break;
                case CX_INDEX:  codex_list(I18, NI18, 1, C_PURPLE, "THE ILLUSTRIOUS 18"); break;
                case CX_ROOMS:  codex_rooms(); break;
                case CX_CHROME: codex_chrome(); break;
                case CX_STATS:  codex_stats(); break;
            }
            key_flush();
        }
        ui_backdrop(BG_TERMINAL, g_t, RAMP_CYAN);
        ui_titlebar("CODEX", "B: BACK", C_CYAN);
        for (int i = 0; i < CX_N; i++) {
            int y = 24 + i * 13;
            u8 c = SCOL[i];
            if (i == sel) {
                vid_hgrad(0, y - 2, 240, 13, RAMP_VIOLET, 10, 0);
                vid_text(6, y, "\x01", c);
                c = C_WHITE;
            }
            vid_text(20, y, SECT[i], c);
        }
        vid_rect(0, 136, 240, 24, C_BLACK);
        vid_hline(0, 136, 240, C_GRID);
        vid_text(4, 138, "ALL OF IT IS FREE, AND ALWAYS OPEN.", C_DIM);
        vid_text(4, 147, "THE HOUSE HAS NO RULE AGAINST KNOWING.", C_DIM);
        vid_text(4, 155, "AGPLv3 \x12 (C) 2026 GODLESS-1 \x12 NO WARRANTY", C_GRID);
        app_frame_end();
    }
}

/* ---------------------------------------------------------------------- */
int sc_teach(int arg)
{
    (void)arg;
    int sel = 0;
    key_flush();
    app_fade_in(10);
    mus_play(MUS_NEON_RAIN);
    for (;;) {
        app_frame_begin();
        if (key_repeat(KEY_DOWN)) { sel = (sel + 1) % (NLESSONS + 1); sfx(SFX_MOVE); }
        if (key_repeat(KEY_UP))   { sel = (sel + NLESSONS) % (NLESSONS + 1); sfx(SFX_MOVE); }
        if (key_down(KEY_B)) { sfx(SFX_BACK); app_fade_out(10); return SC_QUIT; }
        if (key_down(KEY_A)) {
            sfx(SFX_SELECT);
            if (sel < NLESSONS) {
                if (teach_run_lesson(sel)) g_run.lessons_done |= (1 << sel);
            } else {
                teach_chart_browser();
            }
            key_flush();
            mus_play(MUS_NEON_RAIN);
        }

        ui_backdrop(BG_CLINIC, g_t, RAMP_GREEN);
        ui_titlebar("DRY RUN", "B: BACK", C_GREEN);
        vid_text_center(120, 16, "GREY'S TRAINING CONSTRUCT", C_DIM);
        for (int i = 0; i < NLESSONS; i++) {
            int y = 34 + i * 20;
            u8 c = C_GREEN;
            int done = (g_run.lessons_done >> i) & 1;
            if (i == sel) {
                vid_hgrad(4, y - 2, 232, 19, RAMP_VIOLET, 10, 0);
                vid_text(8, y + 4, "\x01", c);
                c = C_WHITE;
            }
            vid_text(22, y, LESSONS[i].code, C_DIM);
            vid_text(58, y, LESSONS[i].name, c);
            char b[16]; char *p = str_int(b, LESSONS[i].npages); str_cat(p, " PAGES");
            vid_text(58, y + 9, b, C_DIM);
            if (done) vid_text(216, y + 4, "\x10", C_GREEN);
        }
        {
            int y = 34 + NLESSONS * 20;
            u8 c = C_CYAN;
            if (sel == NLESSONS) {
                vid_hgrad(4, y - 2, 232, 19, RAMP_VIOLET, 10, 0);
                vid_text(8, y + 4, "\x01", c);
                c = C_WHITE;
            }
            vid_text(22, y, "REF", C_DIM);
            vid_text(58, y, "THE STRATEGY CHART", c);
            vid_text(58, y + 9, "browse every decision", C_DIM);
        }
        vid_text(4, 138, "NEVER PLAYED CARDS? START AT MOD 1.", C_DIM);
        vid_text(4, 148, "NOTHING HERE ASSUMES YOU HAVE.", C_DIM);
        app_frame_end();
    }
}
