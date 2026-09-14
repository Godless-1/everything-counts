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
 *  sc_drill.c - the practice range.  Five drills, each isolating one
 *  skill the table will demand of you under pressure.
 * ===================================================================== */
#include "app.h"
#include "teach.h"
#include "save.h"

enum { DR_TAGS, DR_SPEED, DR_TRUE, DR_BASIC, DR_INDEX, DR_COUNT_ };

static const char *DR_NAME[DR_COUNT_] = {
    "TAG DRILL", "SPEED COUNT", "TRUE COUNT", "BASIC STRATEGY", "INDEX PLAYS"
};
static const char *DR_DESC[DR_COUNT_] = {
    "One card at a time. Call its Hi-Lo tag before the next one lands.",
    "A shoe runs past you. Hold the running count to the end of it.",
    "Running count and decks remaining. Convert, fast, in your head.",
    "A hand and an up card. The one correct play, nothing else.",
    "The count breaks the chart. Know the eighteen places where it does.",
};
static const u8 DR_COL[DR_COUNT_] = { C_GREEN, C_AMBER, C_MAG, C_CYAN, C_PURPLE };

/* ---------------------------------------------------------------------- */
static void drill_frame(const char *name, int q, int nq, int score, u8 col)
{
    ui_backdrop(BG_TERMINAL, g_t, RAMP_CYAN);
    ui_titlebar(name, 0, col);
    char b[24]; char *p = str_int(b, q);
    p = str_cat(p, "/"); str_int(p, nq);
    vid_text(200, 2, b, C_MID);
    p = str_cat(b, "SCORE "); str_int(p, score);
    vid_text(120, 2, b, C_WHITE);
}

static void drill_result(const char *name, int score, int nq, int drill)
{
    int pct = nq ? (score * 100) / nq : 0;
    if (pct > g_run.best_drill[drill]) { g_run.best_drill[drill] = (u8)pct; save_write(); }
    key_flush();
    for (;;) {
        app_frame_begin();
        if (key_down(KEY_A) || key_down(KEY_B)) { sfx(SFX_SELECT); return; }
        ui_backdrop(BG_TERMINAL, g_t, RAMP_GOLD);
        ui_titlebar(name, "RESULT", C_AMBER);
        ui_panel(26, 34, 188, 82, C_AMBER, 0);
        char b[32], *p;
        p = str_int(b, score); p = str_cat(p, " / "); str_int(p, nq);
        vid_text_center(120, 44, b, C_WHITE);
        str_int(b, pct); str_cat(b + ec_strlen(b), "%");
        vid_text_center_s(120, 56, b, pct >= 90 ? C_GREEN : (pct >= 70 ? C_AMBER : C_BLOOD), 2);
        const char *verdict =
            pct >= 95 ? "TABLE READY." :
            pct >= 80 ? "CLOSE. THE PIT PUNISHES CLOSE." :
            pct >= 50 ? "YOU ARE GUESSING SOMETIMES. IT SHOWS." :
                        "GO BACK TO DRY RUN. THIS IS NOT YET A SKILL.";
        vid_text_wrap(34, 80, 30, verdict, C_MID);
        p = str_cat(b, "BEST "); p = str_int(p, g_run.best_drill[drill]); str_cat(p, "%");
        vid_text_center(120, 106, b, C_DIM);
        ui_hint("\x13 BACK");
        app_frame_end();
    }
}

/* ---- 1: tag drill ----------------------------------------------------- */
static void drill_tags(void)
{
    const int NQ = 24;
    int score = 0;
    Shoe s; shoe_build(&s, 2, 90);
    for (int q = 0; q < NQ; q++) {
        u8 c = shoe_draw(&s);
        if (c == NO_CARD) { shoe_build(&s, 2, 90); c = shoe_draw(&s); }
        int tag = card_tag(c);
        int answer = 99, t = 0;
        int limit = 150 - q * 3;
        key_flush();
        sfx(SFX_DEAL);
        while (answer == 99) {
            app_frame_begin();
            t++;
            if (key_down(KEY_START)) return;
            if (key_down(KEY_L))    answer = -1;
            if (key_down(KEY_DOWN)) answer = 0;
            if (key_down(KEY_R))    answer = 1;
            if (t > limit) answer = 99 - 1000;      /* timed out */
            drill_frame("TAG DRILL", q + 1, NQ, score, C_GREEN);
            card_draw(105, 40, c, TAG_HIDE);
            ui_bar(40, 92, 160, 5, limit - t, limit, RAMP_ALERT, C_GRID);
            vid_text(28, 106, "\x15", C_BLOOD);  vid_text(40, 106, "-1  TENS AND ACES", C_BLOOD);
            vid_text(28, 116, "\x04", C_ICE);    vid_text(40, 116, " 0  SEVEN EIGHT NINE", C_ICE);
            vid_text(28, 126, "\x16", C_GREEN);  vid_text(40, 126, "+1  TWO THROUGH SIX", C_GREEN);
            ui_hint2("CALL THE TAG", "START: QUIT");
            app_frame_end();
        }
        int ok = (answer == tag);
        if (ok) { score++; sfx(SFX_GOOD); } else sfx(SFX_BAD);
        for (int f = 0; f < 40; f++) {
            app_frame_begin();
            drill_frame("TAG DRILL", q + 1, NQ, score, C_GREEN);
            card_draw(105, 40, c, TAG_SHOW);
            char b[24]; char *p = str_cat(b, "TAG ");
            str_signed(p, tag);
            vid_text_center(120, 96, b, C_WHITE);
            vid_text_center(120, 110, ok ? "\x10 RIGHT" : "\x11 WRONG", ok ? C_GREEN : C_BLOOD);
            app_frame_end();
            if (key_down(KEY_A)) break;
        }
    }
    drill_result("TAG DRILL", score, NQ, DR_TAGS);
}

/* ---- 2: speed count --------------------------------------------------- */
static void drill_speed(void)
{
    int rounds = 4, score = 0;
    for (int r = 0; r < rounds; r++) {
        Shoe s; shoe_build(&s, 2, 95);
        int n = 16 + r * 6;
        int rc = 0;
        int speed = 26 - r * 4;
        for (int i = 0; i < n; i++) {
            u8 c = shoe_draw(&s);
            rc += card_tag(c);
            sfx(SFX_DEAL);
            for (int f = 0; f < speed; f++) {
                app_frame_begin();
                drill_frame("SPEED COUNT", r + 1, rounds, score, C_AMBER);
                card_draw(105, 44, c, TAG_HIDE);
                char b[16]; char *p = str_int(b, i + 1);
                p = str_cat(p, " OF "); str_int(p, n);
                vid_text_center(120, 96, b, C_DIM);
                ui_hint2("HOLD THE COUNT", "\x16 FASTER   START: QUIT");
                app_frame_end();
                if (key_down(KEY_START)) return;
                if (key_is(KEY_R)) break;       /* impatient players go faster */
            }
        }
        int ans = 0;
        key_flush();
        for (;;) {
            app_frame_begin();
            if (key_repeat(KEY_UP))    { ans++; sfx(SFX_MOVE); }
            if (key_repeat(KEY_DOWN))  { ans--; sfx(SFX_MOVE); }
            if (key_repeat(KEY_RIGHT)) { ans += 5; sfx(SFX_MOVE); }
            if (key_repeat(KEY_LEFT))  { ans -= 5; sfx(SFX_MOVE); }
            if (ans > 40)  ans = 40;
            if (ans < -40) ans = -40;
            if (key_down(KEY_START)) return;
            if (key_down(KEY_A)) { sfx(SFX_SELECT); break; }

            drill_frame("SPEED COUNT", r + 1, rounds, score, C_AMBER);
            ui_panel(26, 46, 188, 62, C_AMBER, 0);
            vid_text_center(120, 54, "RUNNING COUNT?", C_WHITE);
            char b[12]; str_signed(b, ans);
            vid_rect(88, 70, 64, 24, C_BLACK);
            vid_frame_rect(88, 70, 64, 24, C_AMBER);
            vid_text_center_s(120, 74, b, C_WHITE, 2);
            vid_text(76, 78, "\x02", C_AMBER);
            vid_text(158, 78, "\x01", C_AMBER);
            ui_hint("\x03\x04 +/-1   \x02\x01 +/-5   \x13 LOCK IN");
            app_frame_end();
        }
        int ok = (ans == rc);
        if (ok) { score++; sfx(SFX_GOOD); } else sfx(SFX_BAD);
        key_flush();
        for (int f = 0; f < 400; f++) {
            app_frame_begin();
            drill_frame("SPEED COUNT", r + 1, rounds, score, C_AMBER);
            ui_panel(30, 44, 180, 62, ok ? C_GREEN : C_BLOOD, 0);
            char b[24], *p;
            p = str_cat(b, "YOU SAID  "); str_signed(p, ans);
            vid_text_center(120, 54, b, C_MID);
            p = str_cat(b, "TRUTH     "); str_signed(p, rc);
            vid_text_center(120, 68, b, C_WHITE);
            vid_text_center(120, 86, ok ? "\x10 HELD" : "\x11 DROPPED", ok ? C_GREEN : C_BLOOD);
            ui_hint("\x13 NEXT");
            app_frame_end();
            if (key_down(KEY_A)) break;
        }
    }
    drill_result("SPEED COUNT", score, rounds, DR_SPEED);
}

/* ---- 3: true count ---------------------------------------------------- */
static void drill_true(void)
{
    const int NQ = 10;
    int score = 0;
    for (int q = 0; q < NQ; q++) {
        int decks = rng_range(1, 6);
        int rc = rng_range(-12, 16);
        int tc = rc / decks;
        int v = 0;
        key_flush();
        for (;;) {
            app_frame_begin();
            if (key_repeat(KEY_UP))    { v++; sfx(SFX_MOVE); }
            if (key_repeat(KEY_DOWN))  { v--; sfx(SFX_MOVE); }
            if (key_repeat(KEY_RIGHT)) { v += 5; sfx(SFX_MOVE); }
            if (key_repeat(KEY_LEFT))  { v -= 5; sfx(SFX_MOVE); }
            if (v > 30)  v = 30;
            if (v < -30) v = -30;
            if (key_down(KEY_START)) return;
            if (key_down(KEY_A)) { sfx(SFX_SELECT); break; }

            drill_frame("TRUE COUNT", q + 1, NQ, score, C_MAG);
            char b[32], *p;
            vid_rect(24, 30, 192, 34, C_BLACK);
            vid_frame_rect(24, 30, 192, 34, C_GRID);
            p = str_cat(b, "RUNNING COUNT   "); str_signed(p, rc);
            vid_text(32, 36, b, C_AMBER);
            p = str_cat(b, "DECKS REMAINING "); str_int(p, decks);
            vid_text(32, 48, b, C_GREEN);

            vid_text_center(120, 72, "TRUE COUNT?", C_WHITE);
            str_signed(b, v);
            vid_rect(88, 84, 64, 24, C_BLACK);
            vid_frame_rect(88, 84, 64, 24, C_MAG);
            vid_text_center_s(120, 88, b, C_WHITE, 2);
            ui_hint2("\x03\x04 +/-1   \x13 LOCK IN", "START: QUIT");
            app_frame_end();
        }
        int ok = (v == tc);
        if (ok) { score++; sfx(SFX_GOOD); } else sfx(SFX_BAD);
        key_flush();
        for (int f = 0; f < 400; f++) {
            app_frame_begin();
            drill_frame("TRUE COUNT", q + 1, NQ, score, C_MAG);
            ui_panel(24, 40, 192, 70, ok ? C_GREEN : C_BLOOD, 0);
            char b[40], *p;
            p = str_int(b, rc); p = str_cat(p, " / "); p = str_int(p, decks);
            p = str_cat(p, " = "); str_int(p, tc);
            vid_text_center(120, 52, b, C_WHITE);
            vid_text_center(120, 68, ok ? "\x10 RIGHT" : "\x11 WRONG", ok ? C_GREEN : C_BLOOD);
            vid_text_wrap(32, 82, 29, "Round toward zero. Half a deck of error is survivable; a whole deck is not.", C_DIM);
            ui_hint("\x13 NEXT");
            app_frame_end();
            if (key_down(KEY_A)) break;
        }
    }
    drill_result("TRUE COUNT", score, NQ, DR_TRUE);
}

/* ---- 4 and 5: play drills -------------------------------------------- */
static void drill_play(int use_index)
{
    const int NQ = 14;
    int score = 0;
    const Venue *v = &VENUES[2];
    for (int q = 0; q < NQ; q++) {
        Hand h; hand_clear(&h);
        Shoe s; shoe_build(&s, 6, 80);
        u8 up;
        int tc = use_index ? rng_range(-4, 6) : 0;
        /* build a hand worth asking about */
        do {
            hand_clear(&h);
            hand_add(&h, shoe_draw(&s));
            hand_add(&h, shoe_draw(&s));
        } while (hand_total(&h) == 21);
        up = shoe_draw(&s);

        int cds = hand_can_double(&h, v->das);
        int cs  = hand_can_split(&h, 1, v->max_hands, v->rsa);
        int csu = v->surrender && hand_can_surrender(&h);
        int dev = 0;
        int right = use_index ? dev_action(&h, up, tc, v, cds, cs, csu, &dev)
                              : bs_action(&h, up, v, cds, cs, csu);

        int acts[5]; int na = 0;
        acts[na++] = ACT_HIT; acts[na++] = ACT_STAND;
        if (cds) acts[na++] = ACT_DOUBLE;
        if (cs)  acts[na++] = ACT_SPLIT;
        if (csu) acts[na++] = ACT_SURRENDER;
        int sel = 0;
        key_flush();
        for (;;) {
            app_frame_begin();
            if (key_repeat(KEY_RIGHT)) { sel = (sel + 1) % na; sfx(SFX_MOVE); }
            if (key_repeat(KEY_LEFT))  { sel = (sel + na - 1) % na; sfx(SFX_MOVE); }
            if (key_down(KEY_START)) return;
            if (key_down(KEY_A)) { sfx(SFX_SELECT); break; }

            drill_frame(use_index ? "INDEX PLAYS" : "BASIC STRATEGY", q + 1, NQ, score,
                        use_index ? C_PURPLE : C_CYAN);
            vid_text(4, 18, "DEALER", C_DIM);
            card_draw(100, 16, up, TAG_HIDE);
            card_draw_back(134, 16);
            vid_text(4, 62, "YOU", C_DIM);
            card_draw(100, 60, h.card[0], TAG_HIDE);
            card_draw(134, 60, h.card[1], TAG_HIDE);
            {
                char b[20]; char *p = b;
                if (hand_is_soft(&h)) p = str_cat(p, "SOFT ");
                str_int(p, hand_total(&h));
                vid_text(4, 74, b, C_WHITE);
            }
            if (use_index) {
                char b[16]; char *p = str_cat(b, "TC "); str_signed(p, tc);
                vid_rect(4, 88, 54, 12, C_BLACK);
                vid_frame_rect(4, 88, 54, 12, C_PURPLE);
                vid_text(8, 90, b, C_PURPLE);
            }
            int x = 4;
            for (int i = 0; i < na; i++) {
                const char *l = action_name(acts[i]);
                int w = vid_text_len(l) + 8;
                u8 c = (i == sel) ? (use_index ? C_PURPLE : C_CYAN) : C_GRID;
                vid_rect(x, 106, w, 13, C_BLACK);
                vid_frame_rect(x, 106, w, 13, c);
                vid_text(x + 4, 108, l, (i == sel) ? C_WHITE : C_MID);
                x += w + 3;
            }
            ui_hint2("\x02\x01 CHOOSE   \x13 PLAY IT", "START: QUIT");
            app_frame_end();
        }
        int ok = (acts[sel] == right);
        if (ok) { score++; sfx(SFX_GOOD); } else sfx(SFX_BAD);
        key_flush();
        for (int f = 0; f < 400; f++) {
            app_frame_begin();
            drill_frame(use_index ? "INDEX PLAYS" : "BASIC STRATEGY", q + 1, NQ, score,
                        use_index ? C_PURPLE : C_CYAN);
            ui_panel(16, 30, 208, 84, ok ? C_GREEN : C_BLOOD, 0);
            vid_text_center(120, 38, ok ? "\x10 CORRECT" : "\x11 WRONG", ok ? C_GREEN : C_BLOOD);
            char b[40], *p;
            p = str_cat(b, "YOU PLAYED  "); str_cat(p, action_name(acts[sel]));
            vid_text(24, 54, b, C_MID);
            p = str_cat(b, "CORRECT     "); str_cat(p, action_name(right));
            vid_text(24, 66, b, C_GREEN);
            if (use_index && dev)
                vid_text_wrap(24, 82, 31, "The count moved this one off the chart. That is an index play.", C_PURPLE);
            else
                vid_text_wrap(24, 82, 31, "Straight basic strategy. No count involved.", C_DIM);
            ui_hint("\x13 NEXT");
            app_frame_end();
            if (key_down(KEY_A)) break;
        }
    }
    drill_result(use_index ? "INDEX PLAYS" : "BASIC STRATEGY", score, NQ,
                 use_index ? DR_INDEX : DR_BASIC);
}

/* ---------------------------------------------------------------------- */
int sc_drill(int arg)
{
    (void)arg;
    int sel = 0;
    key_flush();
    app_fade_in(12);
    mus_play(MUS_OVERCLOCK);
    for (;;) {
        app_frame_begin();
        if (key_repeat(KEY_DOWN)) { sel = (sel + 1) % DR_COUNT_; sfx(SFX_MOVE); }
        if (key_repeat(KEY_UP))   { sel = (sel + DR_COUNT_ - 1) % DR_COUNT_; sfx(SFX_MOVE); }
        if (key_down(KEY_B)) { sfx(SFX_BACK); app_fade_out(10); return SC_QUIT; }
        if (key_down(KEY_A)) {
            sfx(SFX_SELECT);
            switch (sel) {
                case DR_TAGS:  drill_tags(); break;
                case DR_SPEED: drill_speed(); break;
                case DR_TRUE:  drill_true(); break;
                case DR_BASIC: drill_play(0); break;
                case DR_INDEX: drill_play(1); break;
            }
            key_flush();
            mus_play(MUS_OVERCLOCK);
        }

        ui_backdrop(BG_GRID, g_t, RAMP_CYAN);
        ui_titlebar("THE RANGE", "B: BACK", C_ICE);
        for (int i = 0; i < DR_COUNT_; i++) {
            int y = 22 + i * 14;
            u8 c = DR_COL[i];
            if (i == sel) {
                vid_hgrad(0, y - 2, 240, 14, RAMP_VIOLET, 10, 0);
                vid_text(4, y, "\x01", c);
                c = C_WHITE;
            }
            vid_text(16, y, DR_NAME[i], c);
            if (g_run.best_drill[i]) {
                char b[12]; char *p = str_int(b, g_run.best_drill[i]); str_cat(p, "%");
                vid_text(210, y, b, C_DIM);
            }
        }
        vid_rect(0, 96, 240, 54, C_BLACK);
        vid_hline(0, 96, 240, DR_COL[sel]);
        vid_text(4, 100, DR_NAME[sel], DR_COL[sel]);
        vid_text_wrap(4, 112, 38, DR_DESC[sel], C_TEXT);
        ui_hint("\x13 START");
        app_frame_end();
    }
}
