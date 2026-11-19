/* EVERYTHING COUNTS - a card counting story for the Game Boy Advance
 * Copyright (C) 2026 Godless-1
 * SPDX-License-Identifier: AGPL-3.0-or-later AND CC-BY-SA-4.0
 *
 * The program code in this file is AGPL-3.0-or-later.  The creative work
 * it carries (the rules book and table prose) is also offered on its own
 * under CC-BY-SA-4.0.  See PROVENANCE.md.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.  See LICENSE for the full text.
 */
/* ========================================================================
 *  sc_table.c - the felt.  Every rule the player needs is reachable from
 *  here without prior knowledge: START opens a plain-language rules book,
 *  SELECT asks the coach, and every hand is graded afterwards.
 * ===================================================================== */
#include "app.h"
#include "table.h"
#include "cards.h"

int g_table_end;
s32 g_table_net;
int g_table_hands;

enum {
    TS_SHOE, TS_BET, TS_DEAL, TS_INSURE, TS_PEEK, TS_PLAY,
    TS_DEALER, TS_SETTLE, TS_DONE
};

typedef struct {
    const Venue *v;
    int   mode;
    Shoe  shoe;
    Counter cnt;
    Table tbl;

    s32   bet, last_bet;
    s32   bank_at_start;
    int   trust;            /* 0..100, drives the bet ceiling */
    int   heat;
    int   state;
    int   hands_this_shoe;
    int   hands_total;
    int   pending_check;
    int   coach;
    int   level;            /* practice sim: the module just finished (1..3) */
    int   showdown_timer;

    char  msg[64];
    u8    msg_col;
    int   msg_t;

    char  grade[40];
    u8    grade_col;

    int   act_sel;
    int   acts[6];
    int   nacts;

    int   tc_at_bet;
    int   last_action_ok;
    int   vi;               /* index of this room in VENUES */
    int   in_turn;          /* the player is choosing an action right now */
} Tbl;

static Tbl T;

#ifdef EC_HOST
/* test harness: set or read the live table (0 trust, 1 heat; peek 2 rc, 3 tc) */
void host_table_poke(int what, int v)
{
    if (what == 0) T.trust = v;
    else if (what == 1) T.heat = v;
}
int host_table_peek(int what)
{
    switch (what) {
    case 0: return T.trust;
    case 1: return T.heat;
    case 2: return T.cnt.running;
    default: return count_true(&T.cnt, &T.shoe);
    }
}
#endif

/* ---------------------------------------------------------------------- */
static void say(const char *s, u8 col)
{
    str_copy(T.msg, s, sizeof(T.msg));
    T.msg_col = col;
    T.msg_t = 150;
}

/* "+c1,500" / "-c500": the sign goes outside the currency mark */
static char *str_money_signed(char *p, s32 v)
{
    *p++ = (v < 0) ? '-' : '+';
    *p++ = 0x09;
    return str_money(p, v < 0 ? -v : v);
}

static s32 bet_unit(void)       { return T.v->min_bet; }
static int max_units(void)
{
    int u = 1 + T.trust / 8;
    if (u < 1) u = 1;
    s32 cap = T.v->max_bet / bet_unit();
    if (u > cap) u = (int)cap;
    if (u > 24) u = 24;
    return u;
}

static int true_count(void) { return count_true(&T.cnt, &T.shoe); }

/* what a counter should be betting at this true count */
static int advised_units(void)
{
    int tc = true_count();
    int u = tc - 1;
    if (u < 1) u = 1;
    if (u > max_units()) u = max_units();
    return u;
}

/* ====================================================================== */
/*  drawing                                                               */
/* ====================================================================== */

static void draw_hud(void)
{
    ui_titlebar(T.mode == TMODE_PRACTICE ? "GREY'S SIM" : T.v->name, 0, T.v->accent);
    char b[32];
    b[0] = 0x09; str_money(b + 1, g_run.creds);
    vid_text(236 - vid_text_len(b), 2, b, C_WHITE);
    if (T.mode == TMODE_PRACTICE)
        vid_text(230 - vid_text_len(b) - vid_text_len("PLAY MONEY"), 2, "PLAY MONEY", C_DIM);

    /* shoe penetration */
    int left = shoe_cards_left(&T.shoe);
    int dealt = T.shoe.n - left;
    int cutp = (T.shoe.cut * 52) / (T.shoe.n ? T.shoe.n : 1);
    (void)cutp;
    vid_rect(4, 13, 66, 5, C_BLACK);
    int fw = (64 * dealt) / (T.shoe.n ? T.shoe.n : 1);
    for (int i = 0; i < fw; i++) vid_vline(5 + i, 14, 3, (u8)(T.v->ramp + 8 + i / 8));
    int cx = 5 + (64 * T.shoe.cut) / T.shoe.n;
    vid_vline(cx, 12, 7, C_BLOOD);
    vid_frame_rect(4, 13, 66, 5, C_GRID);
    vid_text(72, 12, "SHOE", C_DIM);

    /* heat */
    vid_text(104, 12, "HEAT", (T.heat > 70) ? C_BLOOD : C_DIM);
    ui_bar(130, 13, 38, 5, T.heat, 100, RAMP_ALERT, (T.heat > 70) ? C_BLOOD : C_GRID);
    /* wire */
    vid_text(174, 12, "WIRE", C_DIM);
    ui_bar(200, 13, 36, 5, g_run.wire, 100, RAMP_MAG, C_GRID);
}

static void hand_label(int x, int y, const Hand *h, int hide_hole, u8 col)
{
    char b[24];
    if (hide_hole) {
        Hand t = *h;
        t.n = 1;
        int v = hand_total(&t);
        char *p = b;
        p = str_int(p, v);
        str_cat(p, " +?");
    } else {
        int tot = hand_total(h);
        char *p = b;
        if (hand_is_bj(h)) str_cat(p, "BLACKJACK");
        else {
            if (hand_is_soft(h) && !h->busted) p = str_cat(p, "SOFT ");
            p = str_int(p, tot);
            if (h->busted) str_cat(p, " BUST");
        }
    }
    vid_rect(x, y, vid_text_len(b) + 6, 10, C_BLACK);
    vid_frame_rect(x, y, vid_text_len(b) + 6, 10, col);
    vid_text(x + 3, y + 1, b, col);
}

static int tag_mode(void)
{
    /* in the sim the coach brings the counter with it; SELECT drops both */
    return ((T.mode == TMODE_PRACTICE && T.coach) || run_has(IMP_ABACUS)) ? TAG_SHOW : TAG_HIDE;
}

static void draw_hand_full(int cx, int y, const Hand *h, int hide_hole)
{
    int n = h->n;
    if (n < 1) return;
    int sp = (n <= 4) ? 22 : (n <= 6 ? 17 : 13);
    int w = CARD_W + (n - 1) * sp;
    int x = cx - w / 2;
    for (int i = 0; i < n; i++) {
        if (hide_hole && i == 1) card_draw_back(x + i * sp, y);
        else card_draw(x + i * sp, y, h->card[i], tag_mode());
    }
}

static void draw_hand_mini(int x, int y, const Hand *h, int active, int idx)
{
    char b[20];
    u8 col = active ? C_WHITE : C_MID;
    if (active) {
        vid_rect(x - 2, y - 2, 116, 22, C_BLACK);
        ui_brackets(x - 2, y - 2, 116, 22, T.v->accent, 4);
    }
    b[0] = 'A' + idx; b[1] = 0;
    vid_text(x, y + 5, b, active ? T.v->accent : C_MID);
    /* every card is drawn: 11 px apart up to six, then squeezed into the
     * 70 px between the letter and the total (a mini card is 14 px wide) */
    int mstep = h->n <= 6 ? 11 : 56 / (h->n - 1);
    if (mstep < 4) mstep = 4;
    for (int i = 0; i < h->n; i++)
        card_draw_mini(x + 8 + i * mstep, y, h->card[i]);
    char t[16]; char *p = t;
    if (hand_is_soft(h)) p = str_cat(p, "S");
    p = str_int(p, hand_total(h));
    if (h->busted) str_cat(p, "X");
    vid_text(x + 78, y + 5, t, h->busted ? C_BLOOD : col);
    if (h->stood && !h->busted) vid_text(x + 100, y + 5, "\x10", C_GREEN);
}

static int coach_available(void);

static void draw_count_hud(void)
{
    int y = 136;
    vid_rect(0, y, 240, 13, C_BLACK);
    vid_hline(0, y, 240, C_GRID);

    int practice = (T.mode == TMODE_PRACTICE && T.coach);
    int x = 4;
    char b[24];

    if (practice || run_has(IMP_ABACUS)) {
        vid_text(x, y + 3, "RC", C_AMBER); x += 18;
        str_signed(b, T.cnt.running);
        vid_text(x, y + 3, b, C_WHITE); x += 26;
    }
    if (practice || run_has(IMP_ORACLE)) {
        vid_text(x, y + 3, "TC", C_MAG); x += 18;
        str_signed(b, true_count());
        vid_text(x, y + 3, b, C_WHITE); x += 26;
    }
    if (practice || run_has(IMP_EIDETIC)) {
        vid_text(x, y + 3, "DL", C_GREEN); x += 18;
        int q = shoe_decks_left_x4(&T.shoe);
        char *p = str_int(b, q / 4);
        *p++ = '.'; p = str_int(p, (q % 4) * 25); *p = 0;
        vid_text(x, y + 3, b, C_WHITE); x += 34;
    }
    if (x < 8) vid_text(4, y + 3, "UNAIDED - YOU COUNT IT", C_DIM);
    /* earned bet ceiling; during a decision the coach has this corner */
    if (T.in_turn && T.coach && coach_available()) return;
    vid_text(160, y + 3, "MAX", C_DIM);
    str_int(b, max_units());
    vid_text(184, y + 3, b, C_CYAN);
    vid_text(196 + (max_units() > 9 ? 6 : 0), y + 3, "UNITS", C_DIM);
}

static const char *act_label(int a)
{
    switch (a) {
        case ACT_HIT:       return "HIT";
        case ACT_STAND:     return "STAND";
        case ACT_DOUBLE:    return "DOUBLE";
        case ACT_SPLIT:     return "SPLIT";
        case ACT_SURRENDER: return "FOLD";
    }
    return "?";
}

static const char *act_help(int a)
{
    switch (a) {
        case ACT_HIT:       return "TAKE ONE MORE CARD";
        case ACT_STAND:     return "STOP. KEEP THIS TOTAL";
        case ACT_DOUBLE:    return "DOUBLE THE BET, TAKE ONE CARD ONLY";
        case ACT_SPLIT:     return "SPLIT THE PAIR INTO TWO HANDS";
        case ACT_SURRENDER: return "FOLD NOW, LOSE ONLY HALF THE BET";
    }
    return "";
}

static void draw_action_bar(void)
{
    int y = 121;
    vid_rect(0, y, 240, 14, C_BLACK);
    int x = 4;
    for (int i = 0; i < T.nacts; i++) {
        const char *l = act_label(T.acts[i]);
        int w = vid_text_len(l) + 8;
        if (i == T.act_sel) {
            vid_hgrad(x, y + 1, w, 12, RAMP_VIOLET, 10, 2);
            vid_frame_rect(x, y + 1, w, 12, T.v->accent);
            vid_text(x + 4, y + 3, l, C_WHITE);
        } else {
            vid_frame_rect(x, y + 1, w, 12, C_GRID);
            vid_text(x + 4, y + 3, l, C_MID);
        }
        x += w + 3;
    }
}

/* the free, always-available explanation of the current decision */
static void draw_situation(void)
{
    if (T.nacts <= 0) return;
    vid_rect(0, 149, 240, 11, C_BLACK);
    vid_hline(0, 149, 240, C_GRID);
    vid_text(3, 151, act_help(T.acts[T.act_sel]), C_DIM);
    vid_text(236 - vid_text_len("\x02\x01 \x13"), 151, "\x02\x01 \x13", C_MID);
}

/* the strip between the felt and the controls carries all feedback */
static void draw_message(void)
{
    vid_rect(0, 110, 240, 11, C_BLACK);
    vid_hline(0, 110, 240, C_GRID);
    int gw = T.grade[0] ? vid_text_len(T.grade) + 8 : 0;
    if (T.grade[0]) vid_text(236 - gw + 8, 112, T.grade, T.grade_col);
    if (T.msg_t > 0) {
        int avail = (232 - gw) / FONT_W;
        char cut[42];
        int i = 0;
        while (T.msg[i] && i < avail && i < (int)sizeof(cut) - 1) { cut[i] = T.msg[i]; i++; }
        cut[i] = 0;
        vid_text(4, 112, cut, T.msg_col);
    }
}

static void draw_table(int hide_hole)
{
    ui_backdrop(BG_FELT, g_t, T.v->ramp);
    draw_hud();

    /* ---- dealer ------------------------------------------------------ */
    vid_text(3, 22, "DEALER", C_MID);
    if (T.tbl.dealer.n) {
        draw_hand_full(152, 20, &T.tbl.dealer, hide_hole);
        hand_label(3, 32, &T.tbl.dealer, hide_hole, hide_hole ? C_MID : C_WHITE);
    }

    /* ---- player ------------------------------------------------------ */
    if (T.tbl.nhands <= 1) {
        if (T.tbl.hand[0].n) {
            draw_hand_full(152, 66, &T.tbl.hand[0], 0);
            hand_label(3, 78, &T.tbl.hand[0], 0,
                       T.tbl.hand[0].busted ? C_BLOOD : C_WHITE);
        }
        if (T.tbl.hand[0].bet || T.bet) {
            char b[20];
            if (T.tbl.hand[0].doubled) vid_text(3, 91, "BET x2", C_AMBER);
            else                       vid_text(3, 91, "BET", C_DIM);
            b[0] = 0x09;
            str_money(b + 1, T.tbl.hand[0].bet ? T.tbl.hand[0].bet : T.bet);
            vid_text(3, 100, b, T.v->accent);
        }
    } else {
        for (int i = 0; i < T.tbl.nhands; i++)
            draw_hand_mini(6 + (i & 1) * 120, 64 + (i >> 1) * 24,
                           &T.tbl.hand[i], i == T.tbl.active, i);
    }
    draw_count_hud();
    draw_message();
}

/* ====================================================================== */
/*  rules book - free, always reachable with START                        */
/* ====================================================================== */
static const char *RULES_TITLE[] = {
    "THE POINT",
    "WHAT THE CARDS ARE WORTH",
    "THE ACE IS TWO NUMBERS",
    "HOW A HAND PLAYS",
    "YOUR FIVE OPTIONS",
    "HOW THE DEALER PLAYS",
    "WHAT YOU GET PAID",
    "INSURANCE (IT IS A TRAP)",
    "THIS HOUSE, THESE RULES",
};
static const char *RULES_BODY[] = {
 "Beat the dealer's hand without going over 21.\nGoing over 21 is a BUST and you lose at once, even if the dealer busts later. That is the whole house edge.",
 "Number cards are worth their number. 10, JACK, QUEEN and KING are all worth 10. An ACE is worth 11 or 1, whichever helps you.\nSuits never matter. Only the numbers.",
 "A hand holding an ace that can still count it as 11 is SOFT. SOFT 17 is A+6: it cannot bust on one card.\nIf counting the ace as 11 would bust you, it drops to 1 and the hand is HARD.",
 "You get two cards face up. The dealer gets one face up (the UP CARD) and one face down (the HOLE CARD).\nYou act first. Then the dealer turns the hole card over and plays.",
 "HIT   take another card.\nSTAND stop and keep your total.\nDOUBLE double your bet and take exactly one more card.\nSPLIT  two cards of equal value become two hands.\nFOLD   surrender and lose only half your bet.",
 "The dealer never chooses. The dealer draws to 16 and stands on 17 or more.\nSome houses make the dealer hit a SOFT 17. That rule costs you money - check the board before you sit.",
 "Win a hand: paid even money, 1 to 1.\nTie: PUSH, your bet comes back.\nBLACKJACK (an ace plus a ten-value card in your first two cards) pays 3 to 2.\nA house paying 6 to 5 on blackjack is robbing you. Walk.",
 "If the dealer shows an ace they offer INSURANCE: a side bet that pays 2 to 1 if their hole card is a ten.\nIt loses money over time. The only reason to take it is when you KNOW tens are thick in the shoe. That is what counting is for.",
 0,
};
#define NRULES 9

static void rules_book(void)
{
    int page = 0;
    key_flush();
    char dyn[400];
    for (;;) {
        app_frame_begin();
        if (key_repeat(KEY_RIGHT) && page < NRULES - 1) { page++; sfx(SFX_MOVE); }
        if (key_repeat(KEY_LEFT)  && page > 0)          { page--; sfx(SFX_MOVE); }
        if (key_down(KEY_B) || key_down(KEY_START))     { sfx(SFX_BACK); return; }

        ui_backdrop(BG_TERMINAL, g_t, T.v->ramp);
        ui_titlebar("RULES", "B: BACK", C_CYAN);
        ui_panel(8, 22, 224, 118, C_CYAN, RULES_TITLE[page]);

        const char *body = RULES_BODY[page];
        if (page == NRULES - 1) {
            char *p = dyn;
            p = str_cat(p, "DECKS IN THE SHOE: ");
            p = str_int(p, T.v->decks);
            p = str_cat(p, "\nDEALER ON SOFT 17: ");
            p = str_cat(p, T.v->h17 ? "HITS (worse for you)" : "STANDS (better for you)");
            p = str_cat(p, "\nBLACKJACK PAYS: ");
            p = str_cat(p, T.v->bj65 ? "6 to 5  - AVOID THIS TABLE" : "3 to 2");
            p = str_cat(p, "\nDOUBLE AFTER SPLIT: ");
            p = str_cat(p, T.v->das ? "YES" : "NO");
            p = str_cat(p, "\nSURRENDER: ");
            p = str_cat(p, T.v->surrender ? "YES" : "NO");
            p = str_cat(p, "\nHOUSE EDGE vs PERFECT PLAY: ");
            str_pct100(p, venue_edge_x100(T.v), 0);
            body = dyn;
        }
        vid_text_wrap(14, 32, 36, body, C_TEXT);

        for (int i = 0; i < NRULES; i++)
            vid_rect(74 + i * 10, 143, 7, 3, i == page ? C_CYAN : C_GRID);
        ui_hint2("\x15\x16 / \x02\x01 PAGE", "\x14 CLOSE");
        app_frame_end();
    }
}

/* ====================================================================== */
/*  the coach overlay                                                     */
/* ====================================================================== */
static int coach_available(void)
{
    return T.mode == TMODE_PRACTICE || run_has(IMP_OPTIC);
}

static void draw_coach(void)
{
    if (!T.coach || !coach_available()) return;
    Hand *h = &T.tbl.hand[T.tbl.active];
    if (!h->n || !T.tbl.dealer.n) return;

    int afford = g_run.creds >= h->bet;
    int cds = afford && hand_can_double(h, T.v->das);
    int cs  = afford && hand_can_split(h, T.tbl.nhands, T.v->max_hands, T.v->rsa);
    int csu = T.v->surrender && hand_can_surrender(h) && T.tbl.nhands == 1;
    int base = sim_action(h, T.tbl.dealer.card[0], T.v, T.tbl.nhands, afford);
    int dev = 0, act = base;
    if (run_has(IMP_MNEMONIC) || T.mode == TMODE_PRACTICE)
        act = dev_action(h, T.tbl.dealer.card[0], true_count(), T.v, cds, cs, csu, &dev);

    char b[32];
    char *p = str_cat(b, dev ? "INDEX \x01 " : "OPTIC \x01 ");
    str_cat(p, act_label(act));
    /* right end of the count row: clear of all five action buttons */
    int w = vid_text_len(b) + 8;
    vid_rect(238 - w, 137, w, 12, C_BLACK);
    vid_frame_rect(238 - w, 137, w, 12, dev ? C_MAG : C_CYAN);
    vid_text(242 - w, 139, b, dev ? C_MAG : C_CYAN);
}

/* ====================================================================== */
/*  the pulse check - the counting challenge                              */
/* ====================================================================== */
enum { CHK_RUNNING, CHK_TRUE, CHK_DECKS };

static int pulse_check(int kind)
{
    int answer = 0;
    int correct;
    const char *q;
    switch (kind) {
        case CHK_TRUE:  correct = true_count();
                        q = "TRUE COUNT?"; break;
        case CHK_DECKS: correct = (shoe_decks_left_x4(&T.shoe) + 2) / 4;
                        q = "DECKS REMAINING?"; break;
        default:        correct = T.cnt.running;
                        q = "RUNNING COUNT?"; break;
    }
    int limit = run_has(IMP_CHRONO) ? 520 : 330;
    if (T.mode == TMODE_PRACTICE) limit = 600;
    int t = limit;
    sfx(SFX_ALERT);
    key_flush();

    for (;;) {
        app_frame_begin();
        t--;
        if (key_repeat(KEY_UP))    { answer++; sfx(SFX_MOVE); }
        if (key_repeat(KEY_DOWN))  { answer--; sfx(SFX_MOVE); }
        if (key_repeat(KEY_RIGHT)) { answer += 5; sfx(SFX_MOVE); }
        if (key_repeat(KEY_LEFT))  { answer -= 5; sfx(SFX_MOVE); }
        if (answer > 60) answer = 60;
        if (answer < -60) answer = -60;

        int done = key_down(KEY_A) || t <= 0;

        draw_table(0);
        /* dim the felt behind the check */
        vid_overlay();
        for (int j = 0; j < 160; j += 2) vid_hline(0, j, 240, C_BLACK);

        ui_panel(28, 38, 184, 74, C_MAG, "PULSE CHECK");
        vid_text_center(120, 46, q, C_WHITE);

        char b[16];
        str_signed(b, answer);
        vid_rect(88, 60, 64, 26, C_BLACK);
        vid_frame_rect(88, 60, 64, 26, C_MAG);
        vid_text_center_s(120, 64, b, C_WHITE, 2);
        vid_text(76, 68, "\x02", C_MAG);
        vid_text(158, 68, "\x01", C_MAG);

        ui_bar(40, 92, 160, 6, t, limit, RAMP_ALERT,
               (t < limit / 3) ? C_BLOOD : C_GRID);
        vid_text_center(120, 100, "\x03\x04 +/-1   \x02\x01 +/-5   \x13 LOCK IN", C_DIM);

        if (t < 60 && ((t / 4) & 1)) ui_noise(0, 0, 240, 160, g_t, C_MAG, 40);
        app_frame_end();

        if (done) break;
    }

    int ok = (answer == correct);
    if (ok) {
        g_run.checks_passed++;
        T.trust += 18; if (T.trust > 100) T.trust = 100;
        T.heat -= 6; if (T.heat < 0) T.heat = 0;
        sfx(SFX_GOOD);
    } else {
        g_run.checks_failed++;
        T.trust -= 26; if (T.trust < 0) T.trust = 0;
        T.heat += 5;
        sfx(SFX_BAD);
    }

    /* always show the answer and the arithmetic - this is a lesson */
    key_flush();
    for (int f = 0; f < 600; f++) {
        app_frame_begin();
        draw_table(0);
        vid_overlay();
        for (int j = 0; j < 160; j += 2) vid_hline(0, j, 240, C_BLACK);
        ui_panel(16, 26, 208, 116, ok ? C_GREEN : C_BLOOD,
                 ok ? "HELD" : "DROPPED");
        char b[64], *p;
        vid_text_center(120, 34, ok ? "YOU HELD THE COUNT" : "YOU LOST THE COUNT", ok ? C_GREEN : C_BLOOD);

        p = str_cat(b, "YOU SAID  "); p = str_signed(p, answer);
        vid_text(24, 48, b, C_MID);
        p = str_cat(b, "TRUTH     "); p = str_signed(p, correct);
        vid_text(24, 58, b, C_WHITE);

        if (kind == CHK_TRUE) {
            p = str_cat(b, "RC "); p = str_signed(p, T.cnt.running);
            p = str_cat(p, " / "); 
            int q4 = shoe_decks_left_x4(&T.shoe);
            p = str_int(p, q4 / 4); *p++ = '.'; p = str_uint_pad(p, (u32)((q4 % 4) * 25), 2, '0');
            p = str_cat(p, " DECKS = TC "); str_signed(p, correct);
            vid_text(24, 74, b, C_AMBER);
            vid_text_wrap(24, 90, 32, "TRUE COUNT = RUNNING COUNT DIVIDED BY DECKS LEFT. DROP THE FRACTION.", C_MID);
        } else if (kind == CHK_RUNNING) {
            p = str_cat(b, "CARDS SEEN THIS SHOE: ");
            str_int(p, T.cnt.seen);
            vid_text(24, 74, b, C_AMBER);
            vid_text_wrap(24, 90, 32, "LOW CARDS 2-6 ADD ONE. 7-8-9 ADD NOTHING. TENS AND ACES SUBTRACT ONE.", C_MID);
        } else {
            vid_text_wrap(24, 74, 32, "ESTIMATE BY THE DISCARD TRAY, NOT BY HOPE.", C_MID);
        }
        ui_hint("\x13 CONTINUE");
        app_frame_end();
        if (key_down(KEY_A) || key_down(KEY_B)) break;
    }
    return ok;
}

/* ====================================================================== */
/*  flow                                                                  */
/* ====================================================================== */

/* The shoe ran dry mid-round and the discards went back in.  Whatever
 * was counted is gone except the cards still face up on the felt. */
static void recount_after_recycle(void)
{
    count_reset(&T.cnt, T.v->decks);
    for (int i = 0; i < T.tbl.nhands; i++)
        for (int k = 0; k < T.tbl.hand[i].n; k++) count_card(&T.cnt, T.tbl.hand[i].card[k]);
    for (int k = 0; k < T.tbl.dealer.n; k++)
        if (k != 1 || T.tbl.hole_shown) count_card(&T.cnt, T.tbl.dealer.card[k]);
    say("SHOE DRY: RESHUFFLING", C_AMBER);
}

static u8 draw_card(void)
{
    int was = T.shoe.recycled;
    u8 c = shoe_draw(&T.shoe);
    if (T.shoe.recycled && !was) recount_after_recycle();
    return c;
}

static void deal_one(Hand *h, int faceup)
{
    u8 c = draw_card();
    hand_add(h, c);
    if (faceup) count_card(&T.cnt, c);
    sfx(SFX_DEAL);
    for (int f = 0; f < 7; f++) {
        app_frame_begin();
        draw_table(!T.tbl.hole_shown && T.tbl.dealer.n >= 2);
        app_frame_end();
    }
}

static void new_shoe(void)
{
    /* the dealer you did not sell out cuts you a deeper shoe at her room */
    int pen = T.v->pen;
    if (T.vi == VENUE_LOTUS && T.mode == TMODE_PLAY && g_run.flag[FLAG_MARA_SPARED]) pen += 10;
    shoe_build(&T.shoe, T.v->decks, pen);
    count_reset(&T.cnt, T.v->decks);
    T.hands_this_shoe = 0;

    for (int f = 0; f < 70; f++) {
        app_frame_begin();
        ui_backdrop(BG_FELT, g_t, T.v->ramp);
        draw_hud();
        if ((f % 9) == 0) sfx(SFX_FLIP);
        int n = 7;
        for (int i = 0; i < n; i++) {
            int ph = (f * 5 + i * 17) % 90;
            int x = 30 + i * 26 + (ph < 45 ? ph / 3 : (90 - ph) / 3);
            card_draw_back(x, 52 + ((i & 1) ? 4 : -4));
        }
        vid_text_center(120, 108, "SHUFFLE", C_WHITE);
        vid_text_center(120, 120, "THE COUNT GOES BACK TO ZERO", C_AMBER);
        app_frame_end();
    }
    if (T.mode == TMODE_PRACTICE && T.hands_total == 0)
        say("SELECT: COACH AND COUNTER ON/OFF", C_CYAN);
    else
        say("NEW SHOE. COUNT = 0", C_AMBER);
}

static void build_actions(void)
{
    Hand *h = &T.tbl.hand[T.tbl.active];
    T.nacts = 0;
    /* a re-splittable pair of aces may only stand or split */
    if (h->from_split && CARD_RANK(h->card[0]) == 0 && h->n == 2) {
        T.acts[T.nacts++] = ACT_STAND;
        if (hand_can_split(h, T.tbl.nhands, T.v->max_hands, 1))
            T.acts[T.nacts++] = ACT_SPLIT;
        if (T.act_sel >= T.nacts) T.act_sel = 0;
        return;
    }
    T.acts[T.nacts++] = ACT_HIT;
    T.acts[T.nacts++] = ACT_STAND;
    if (hand_can_double(h, T.v->das) && g_run.creds >= h->bet)
        T.acts[T.nacts++] = ACT_DOUBLE;
    if (hand_can_split(h, T.tbl.nhands, T.v->max_hands, T.v->rsa) && g_run.creds >= h->bet)
        T.acts[T.nacts++] = ACT_SPLIT;
    if (T.v->surrender && hand_can_surrender(h) && T.tbl.nhands == 1)
        T.acts[T.nacts++] = ACT_SURRENDER;
    if (T.act_sel >= T.nacts) T.act_sel = 0;
}

static void grade_action(int chosen)
{
    Hand *h = &T.tbl.hand[T.tbl.active];
    /* grade against what was actually on offer: no marks lost for a
     * double or split the bankroll could not cover */
    int afford = g_run.creds >= h->bet;
    int cds = afford && hand_can_double(h, T.v->das);
    int cs  = afford && hand_can_split(h, T.tbl.nhands, T.v->max_hands, T.v->rsa);
    int csu = T.v->surrender && hand_can_surrender(h) && T.tbl.nhands == 1;
    int dev = 0;
    int right = dev_action(h, T.tbl.dealer.card[0], true_count(), T.v,
                           cds, cs, csu, &dev);
    int basic = sim_action(h, T.tbl.dealer.card[0], T.v, T.tbl.nhands, afford);

    g_run.bs_total++;
    if (chosen == right || chosen == basic) {
        g_run.bs_correct++;
        str_copy(T.grade, dev && chosen == right ? "INDEX \x10" : "CORRECT \x10", sizeof(T.grade));
        T.grade_col = C_GREEN;
        T.last_action_ok = 1;
    } else {
        char *p = T.grade;
        p = str_cat(p, "BASIC: ");
        str_cat(p, act_label(basic));
        T.grade_col = C_BLOOD;
        T.last_action_ok = 0;
    }
}

static void settle(void)
{
    /* every stake left the bankroll when it was put down; this hands back
     * the surviving stakes plus winnings */
    g_run.creds += table_settle(&T.tbl, T.v->bj65);
    s32 net = T.tbl.net;

    g_table_net += net;
    if (net > g_run.biggest_win) g_run.biggest_win = net;
    if (net > 0) { g_run.streak++; if (g_run.streak > g_run.best_streak) g_run.best_streak = g_run.streak; }
    else if (net < 0) g_run.streak = 0;

    /* heat: the pit watches the spread, not the wins */
    int units = (int)(T.bet / bet_unit());
    int last_units = (int)(T.last_bet / bet_unit());
    int watch = T.v->heat_rate / 2 + 1;
    /* the Lotus pit owes you one; the Kuroda people do not, if you refused */
    if (T.vi == VENUE_LOTUS && g_run.flag[FLAG_MARA_SOLD]) watch -= 2;
    if (g_run.flag[FLAG_REFUSED_SABLE]) watch += 1;
    if (watch < 0) watch = 0;
    T.heat += watch;
    if (last_units > 0 && units >= last_units * 4) T.heat += 7;
    if (units >= 8) T.heat += 3;
    if (net > bet_unit() * 20) T.heat += 4;
    if (units <= 1) T.heat -= 2;
    if (run_has(IMP_DAMPER)) T.heat -= 3;
    if (T.heat < 0) T.heat = 0;

    /* bet-sizing feedback: this is the whole lesson */
    int adv = advised_units();
    if (T.tc_at_bet >= 2 && units <= 1) {
        say("COLD BET, HOT SHOE", C_AMBER);
    } else if (T.tc_at_bet <= 0 && units >= 4) {
        say("BIG BET, DEAD SHOE", C_BLOOD);
        T.heat += 6;
    } else if (units == adv || (units >= adv - 1 && units <= adv + 1)) {
        if (T.tc_at_bet >= 2) say("GOOD SPREAD", C_GREEN);
    }

    if (net > 0)      sfx(SFX_WIN);
    else if (net < 0) sfx(SFX_LOSE);
    else              sfx(SFX_PUSH);

    /* showdown display */
    key_flush();
    for (int f = 0; f < 600; f++) {
        app_frame_begin();
        draw_table(0);
        char b[40], *p;
        const char *head;
        u8 col;
        if (net > 0)      { head = "YOU TAKE IT"; col = C_GREEN; }
        else if (net < 0) { head = "HOUSE TAKES IT"; col = C_BLOOD; }
        else              { head = "PUSH"; col = C_AMBER; }
        /* one line in the action-bar row: the message strip above it
         * keeps showing the grade for the last decision */
        vid_rect(0, 121, 240, 15, C_BLACK);
        vid_frame_rect(2, 122, 236, 13, col);
        if (net) str_money_signed(b, net);
        else { p = b; *p++ = 0x09; *p++ = '0'; *p = 0; }
        {
            int tw = vid_text_len(head) + 12 + vid_text_len(b);
            int tx = vid_text(120 - tw / 2, 125, head, col);
            vid_text(tx + 12, 125, b, C_WHITE);
        }
        ui_hint("\x13 NEXT HAND");
        app_frame_end();
        if (key_down(KEY_A) || key_down(KEY_B) || key_down(KEY_START)) break;
    }
}

static void dealer_play(void)
{
    T.tbl.hole_shown = 1;
    count_card(&T.cnt, T.tbl.dealer.card[1]);
    sfx(SFX_FLIP);
    for (int f = 0; f < 30; f++) { app_frame_begin(); draw_table(0); app_frame_end(); }

    /* nobody left to beat (all bust, folded, or paid as blackjack):
     * the dealer shows the hole card and draws nothing */
    if (!table_dealer_must_play(&T.tbl)) return;

    for (;;) {
        int t = hand_total(&T.tbl.dealer);
        int soft = hand_is_soft(&T.tbl.dealer);
        if (t > 21) break;
        if (t >= 18) break;
        if (t == 17 && !(soft && T.v->h17)) break;
        if (t < 17 || (t == 17 && soft && T.v->h17)) {
            deal_one(&T.tbl.dealer, 1);
            for (int f = 0; f < 18; f++) { app_frame_begin(); draw_table(0); app_frame_end(); }
        } else break;
    }
    if (hand_total(&T.tbl.dealer) > 21) {
        T.tbl.dealer.busted = 1;
        say("DEALER BUSTS", C_GREEN);
        sfx(SFX_BUST);
        for (int f = 0; f < 40; f++) { app_frame_begin(); draw_table(0); app_frame_end(); }
    }
}

static void do_split(void)
{
    Hand *h = &T.tbl.hand[T.tbl.active];
    int n = T.tbl.nhands;
    for (int i = n; i > T.tbl.active + 1; i--) T.tbl.hand[i] = T.tbl.hand[i - 1];
    Hand *nh = &T.tbl.hand[T.tbl.active + 1];
    hand_clear(nh);
    nh->bet = h->bet;
    nh->from_split = 1;
    hand_add(nh, h->card[1]);
    h->n = 1;
    h->from_split = 1;
    T.tbl.nhands++;
    g_run.creds -= h->bet;
    sfx(SFX_CHIP);
    deal_one(h, 1);
    deal_one(nh, 1);
    say("SPLIT", T.v->accent);
}

/* returns 0 to keep playing this hand, 1 when the hand is finished */
static int player_turn(void)
{
    Hand *h = &T.tbl.hand[T.tbl.active];

    /* split aces draw exactly one card each; a re-split is the one
     * exception, and only where the house allows it */
    if (h->from_split && CARD_RANK(h->card[0]) == 0 && h->n == 2) {
        if (!(T.v->rsa && hand_can_split(h, T.tbl.nhands, T.v->max_hands, 1))) {
            h->stood = 1;
            return 1;
        }
    }
    if (hand_total(h) == 21) { h->stood = 1; return 1; }

    T.act_sel = 0;        /* never inherit DOUBLE or SPLIT from the last hand */
    build_actions();
    key_flush();
    for (;;) {
        app_frame_begin();
        if (T.msg_t > 0) T.msg_t--;

        if (key_repeat(KEY_RIGHT)) { T.act_sel = (T.act_sel + 1) % T.nacts; sfx(SFX_MOVE); }
        if (key_repeat(KEY_LEFT))  { T.act_sel = (T.act_sel + T.nacts - 1) % T.nacts; sfx(SFX_MOVE); }
        if (key_down(KEY_START))   { rules_book(); key_flush(); }
        if (key_down(KEY_SELECT)) {
            if (coach_available()) {
                T.coach = !T.coach; sfx(SFX_SELECT);
                if (T.mode == TMODE_PRACTICE)
                    say(T.coach ? "COACH ON" : "COACH OFF. YOU COUNT IT.", C_CYAN);
            }
            else { say("NO OPTIC INSTALLED", C_BLOOD); sfx(SFX_DENY); }
        }
        if (key_down(KEY_A)) {
            int a = T.acts[T.act_sel];
            grade_action(a);
            sfx(SFX_SELECT);
            switch (a) {
                case ACT_HIT:
                    deal_one(h, 1);
                    if (hand_total(h) > 21) {
                        h->busted = 1; sfx(SFX_BUST); say("BUST", C_BLOOD);
                        for (int f = 0; f < 40; f++) { app_frame_begin(); draw_table(1); app_frame_end(); }
                        return 1;
                    }
                    if (hand_total(h) == 21) { h->stood = 1; return 1; }
                    build_actions();
                    break;
                case ACT_STAND:
                    h->stood = 1;
                    return 1;
                case ACT_DOUBLE:
                    g_run.creds -= h->bet;
                    h->bet *= 2;
                    h->doubled = 1;
                    sfx(SFX_CHIP);
                    deal_one(h, 1);
                    if (hand_total(h) > 21) { h->busted = 1; sfx(SFX_BUST); say("BUST", C_BLOOD); }
                    else h->stood = 1;
                    for (int f = 0; f < 30; f++) { app_frame_begin(); draw_table(1); app_frame_end(); }
                    return 1;
                case ACT_SPLIT:
                    do_split();
                    h = &T.tbl.hand[T.tbl.active];
                    if (hand_total(h) == 21) { h->stood = 1; return 1; }
                    build_actions();
                    break;
                case ACT_SURRENDER:
                    h->surrendered = 1;
                    h->stood = 1;
                    say("FOLDED - HALF BACK", C_AMBER);
                    return 1;
            }
        }

        T.in_turn = 1;
        draw_table(1);
        T.in_turn = 0;
        draw_action_bar();
        draw_coach();
        draw_situation();
        app_frame_end();
    }
}

static int offer_insurance(void)
{
    int sel = 1;   /* default: decline */
    key_flush();
    for (;;) {
        app_frame_begin();
        if (key_repeat(KEY_LEFT) || key_repeat(KEY_RIGHT)) { sel ^= 1; sfx(SFX_MOVE); }
        if (key_down(KEY_START)) { rules_book(); key_flush(); }
        if (key_down(KEY_A)) { sfx(SFX_SELECT); break; }

        draw_table(1);
        vid_overlay();
        vid_rect(0, 108, 240, 42, C_BLACK);
        vid_hline(0, 108, 240, C_AMBER);
        vid_text(4, 111, "INSURANCE? THE DEALER SHOWS AN ACE.", C_AMBER);
        vid_text(4, 122, "COSTS HALF YOUR BET. PAYS 2 TO 1.", C_MID);
        const char *o[2] = { "TAKE", "NO" };
        for (int i = 0; i < 2; i++) {
            int bx = 148 + i * 46;
            u8 c = (i == sel) ? C_AMBER : C_GRID;
            vid_rect(bx, 132, 42, 13, C_BLACK);
            vid_frame_rect(bx, 132, 42, 13, c);
            vid_text_center(bx + 21, 135, o[i], (i == sel) ? C_WHITE : C_DIM);
        }
        if (run_has(IMP_ORACLE) || (T.mode == TMODE_PRACTICE && T.coach)) {
            char b[40]; char *p = str_cat(b, "TC ");
            p = str_signed(p, true_count());
            str_cat(p, dev_insurance(true_count()) ? " - TAKE IT" : " - DECLINE");
            vid_text(4, 135, b, dev_insurance(true_count()) ? C_GREEN : C_BLOOD);
        } else {
            vid_text(4, 135, "GOOD ONLY AT TC +3", C_MID);
        }
        ui_hint2("\x02\x01 CHOOSE  \x13 OK", "START RULES");
        app_frame_end();
    }
    return sel == 0;
}

/* ====================================================================== */
/* a boxed paragraph over the dimmed felt; A to go on */
static void epilogue(const char *title, const char *text, const char *hint, u8 col)
{
    key_flush();
    for (;;) {
        app_frame_begin();
        draw_table(0);
        vid_overlay();
        for (int j = 0; j < 160; j += 2) vid_hline(0, j, 240, C_BLACK);
        ui_panel(10, 22, 220, 124, col, title);
        vid_text_wrap(16, 33, 35, text, C_TEXT);
        ui_hint(hint);
        app_frame_end();
        if (key_down(KEY_A)) { sfx(SFX_SELECT); break; }
    }
}

/* Chrome talks.  Every room has a wire level it tolerates; above it, any
 * hand may be the one where a wand passes behind your chair. */
static void scan_check(void)
{
    if (T.mode != TMODE_PLAY) return;
    if (g_run.flag[FLAG_LAUNDERED]) return;        /* the Kuroda hand is over you */
    int over = (int)g_run.wire - (int)T.v->scan_floor;
    if (over <= 0) return;
    if (!rng_chance(3 + over / 3)) return;

    g_run.flag[FLAG_SCANNED] = 1;
    T.heat += 10 + over / 3;
    sfx(SFX_GLITCH);
    epilogue("SCANNED",
        "A floor runner drifts past with a wand held low against his leg. "
        "It chirps once at the back of your skull.\n\n"
        "He does not look at you. He looks at the pit, and the pit looks at you.\n\n"
        "Your chrome is louder than this room allows.",
        "\x13 KEEP PLAYING", C_MAG);
    say("WIRE OVER ROOM LIMIT", C_MAG);
}

int sc_table(int arg)
{
    int vi = arg & 0xFF;
    T.mode = (arg >> 8) & 0x0F;
    T.level = (arg >> 12) & 0x0F;
    T.v = &VENUES[vi];
    T.vi = vi;

    /* Grey's construct runs on play money: nothing that happens in it
     * touches the real bankroll, the stats or the save */
    Run real = g_run;
    if (T.mode == TMODE_PRACTICE) g_run.creds = 250000;   /* a unit of the Lotus is 2,500: 100 units */
    int song = T.mode == TMODE_PRACTICE    ? MUS_NEON_RAIN
             : T.mode == TMODE_STORY_VAULT ? MUS_VAULT : MUS_THE_SHOE;

    T.trust = 50;
    T.heat = 0;
    T.bet = T.v->min_bet;
    T.last_bet = T.v->min_bet;
    T.hands_total = 0;
    T.coach = (T.mode == TMODE_PRACTICE);
    T.grade[0] = 0;
    g_table_net = 0;
    g_table_hands = 0;
    g_table_end = TEND_LEFT;

    mus_play(song);
    app_fade_in(10);
    key_flush();
    new_shoe();

    for (;;) {
        /* ---- shoe boundary ------------------------------------------ */
        if (shoe_exhausted(&T.shoe)) {
            /* a recycled shoe has no honest count left to be asked about */
            if (!T.shoe.recycled)
                pulse_check(T.hands_total > 6 ? CHK_TRUE : CHK_RUNNING);
            new_shoe();
        }

        if (g_run.creds < T.v->min_bet) { g_table_end = TEND_BROKE; break; }
        /* at the Vault they want to watch you count; nobody is thrown out */
        if (T.mode == TMODE_STORY_VAULT) T.heat = 0;
        if (T.heat >= 100) { g_table_end = TEND_BACKED_OFF; break; }

        /* ---- betting ------------------------------------------------ */
        {
            int leaving = 0;
            key_flush();
            int units = (int)(T.bet / bet_unit());
            if (units < 1) units = 1;
            for (;;) {
                app_frame_begin();
                if (T.msg_t > 0) T.msg_t--;
                int mu = max_units();
                if (key_repeat(KEY_UP))    { units++; sfx(SFX_CHIP); }
                if (key_repeat(KEY_DOWN))  { units--; sfx(SFX_CHIP); }
                if (key_repeat(KEY_RIGHT)) { units += 5; sfx(SFX_CHIP); }
                if (key_repeat(KEY_LEFT))  { units -= 5; sfx(SFX_CHIP); }
                if (units > mu) { units = mu; }
                if (units < 1) units = 1;
                while (units > 1 && bet_unit() * units > g_run.creds) units--;
                T.bet = bet_unit() * units;

                if (key_down(KEY_START)) { rules_book(); key_flush(); }
                if (key_down(KEY_B)) {
                    if (confirm_box("LEAVE THE TABLE AND CASH OUT?", "CASH OUT", "STAY",
                                    T.v->accent, BG_FELT, T.v->ramp)) { leaving = 1; break; }
                    key_flush();
                }
                if (key_down(KEY_A)) { sfx(SFX_SELECT); break; }

                ui_backdrop(BG_FELT, g_t, T.v->ramp);
                draw_hud();
                vid_text_center(120, 26, "PLACE YOUR BET", C_WHITE);

                char b[32];
                b[0] = 0x09; str_money(b + 1, T.bet);
                /* the box grows with the stake (2x text) so seven digits fit */
                int bw = vid_text_len(b) * 2 + 12;
                if (bw < 116) bw = 116;
                int bx = 120 - bw / 2;
                vid_rect(bx, 40, bw, 30, C_BLACK);
                vid_frame_rect(bx, 40, bw, 30, T.v->accent);
                ui_brackets(bx - 1, 39, bw + 2, 32, T.v->accent, 5);
                vid_text_center_s(120, 46, b, C_WHITE, 2);

                char *p = str_int(b, units);
                str_cat(p, units == 1 ? " UNIT" : " UNITS");
                vid_text_center(120, 75, b, C_MID);

                /* the stack sits beside the readout, clear of every label */
                /* the stake itself, on its own spot beside the readout */
                vid_rect(4, 40, 46, 30, C_BLACK);
                vid_frame_rect(4, 40, 46, 30, C_GRID);
                draw_chips(27, 68, T.bet, bet_unit(), T.v->accent);

                /* the advisory is the lesson: bet with the count */
                if (run_has(IMP_ORACLE) || (T.mode == TMODE_PRACTICE && T.coach)) {
                    char a[40]; char *q = str_cat(a, "ORACLE SAYS ");
                    q = str_int(q, advised_units());
                    str_cat(q, advised_units() == 1 ? " UNIT" : " UNITS");
                    vid_text_center(120, 92, a, C_MAG);
                } else if (T.hands_total > 0) {
                    vid_text_center(120, 92, "SMALL WHEN COLD. BIG WHEN RICH.", C_MID);
                }

                draw_count_hud();
                draw_message();
                vid_rect(0, 121, 240, 15, C_BLACK);
                vid_text_center(120, 125, "HOLD THE COUNT AND THE CEILING RISES", C_DIM);
                ui_hint2("\x03\x04 BET   \x13 DEAL", "\x14 LEAVE   START RULES");
                app_frame_end();
            }
            if (leaving) break;
            T.tc_at_bet = true_count();
        }

        /* ---- deal ---------------------------------------------------- */
        ec_memset(&T.tbl, 0, sizeof(T.tbl));
        T.tbl.nhands = 1;
        T.tbl.active = 0;
        T.tbl.hand[0].bet = T.bet;
        g_run.creds -= T.bet;
        T.grade[0] = 0;
        T.msg_t = 0;                  /* last hand's result is not this hand's */
        shoe_round_begin(&T.shoe);

        deal_one(&T.tbl.hand[0], 1);
        deal_one(&T.tbl.dealer, 1);
        deal_one(&T.tbl.hand[0], 1);
        {   /* hole card stays dark and uncounted */
            u8 c = draw_card();
            hand_add(&T.tbl.dealer, c);
            sfx(SFX_DEAL);
            for (int f = 0; f < 10; f++) { app_frame_begin(); draw_table(1); app_frame_end(); }
        }

        /* ---- insurance ---------------------------------------------- */
        if (CARD_RANK(T.tbl.dealer.card[0]) == 0) {
            if (g_run.creds < T.bet / 2) {
                say("NO CREDIT LEFT FOR INSURANCE", C_DIM);
            } else if (offer_insurance()) {
                T.tbl.ins_taken = 1;
                T.tbl.ins_bet = T.bet / 2;
                g_run.creds -= T.tbl.ins_bet;
            }
        }

        /* ---- dealer peek -------------------------------------------- */
        int up = CARD_RANK(T.tbl.dealer.card[0]);
        if (up == 0 || card_value(T.tbl.dealer.card[0]) == 10) {
            if (hand_is_bj(&T.tbl.dealer)) {
                T.tbl.hole_shown = 1;
                count_card(&T.cnt, T.tbl.dealer.card[1]);
                say("DEALER BLACKJACK", C_BLOOD);
                sfx(SFX_ALERT);
                for (int f = 0; f < 50; f++) { app_frame_begin(); draw_table(0); app_frame_end(); }
                settle();
                goto hand_done;
            }
        }
        if (hand_is_bj(&T.tbl.hand[0])) {
            say("BLACKJACK", C_AMBER);
            sfx(SFX_BJ);
            T.tbl.hand[0].stood = 1;
            for (int f = 0; f < 60; f++) { app_frame_begin(); draw_table(1); app_frame_end(); }
            dealer_play();
            settle();
            goto hand_done;
        }

        /* ---- player ------------------------------------------------- */
        T.tbl.active = 0;
        while (T.tbl.active < T.tbl.nhands) {
            while (!player_turn()) { }
            T.tbl.active++;
        }
        T.tbl.active = 0;

        dealer_play();
        settle();

hand_done:
        T.hands_total++;
        T.hands_this_shoe++;
        T.last_bet = T.bet;
        g_table_hands++;
        g_run.hands_played++;

        scan_check();

        /* random pulse check - the pit is not the only thing watching */
        {
            int chance = 6 + g_run.night * 2;
            if (T.mode == TMODE_PRACTICE) chance = 14;
#ifdef EC_HOST
            { extern int host_no_pulse;
              if (host_no_pulse == 1) chance = 0;
              if (host_no_pulse == 2) chance = 100; }
#endif
#ifdef EC_DEBUG
            chance = 100;
#endif
            if (T.hands_this_shoe >= 3 && !T.shoe.recycled && rng_chance(chance))
                pulse_check(rng_chance(45) ? CHK_TRUE : CHK_RUNNING);
        }

        /* a rich shoe changes the music; a watchful pit sharpens it */
        mus_play(true_count() >= 3 ? MUS_OVERCLOCK : song);
        mus_set_intensity(T.heat > 60 ? 1 : 0);
    }

    mus_set_intensity(0);
    if (T.mode == TMODE_PRACTICE) {
        if (g_table_end == TEND_BROKE) {
            sfx(SFX_BAD);
            epilogue("PLAY MONEY GONE",
                "The sim has no more play money. Nothing real was lost.\n\n"
                "Grey clears the table. Back to the course.",
                "\x13 LEAVE", C_AMBER);
        }
        g_run = real;
        return SC_HUB;
    }

    /* out-of-the-chair epilogue */
    if (g_table_end == TEND_BACKED_OFF) {
        g_run.flag[FLAG_BACKED_OFF] = 1;
        g_run.venue_locked[T.vi] = 1;          /* until tomorrow night */
        sfx(SFX_ALERT);
        epilogue("BACKED OFF",
            "A hand on your shoulder. No scene, no police. Your chips are counted "
            "for you and the pit never breaks eye contact.\n\n"
            "\"You're welcome at any of our other games. Not this one. Not tonight.\"",
            "\x13 WALK", C_BLOOD);
    } else if (g_table_end == TEND_BROKE) {
        sfx(SFX_BAD);
        epilogue("TAPPED OUT",
            T.mode == TMODE_STORY_VAULT
            ? "The felt in front of you is empty. Vesk squares the deck with one finger and does not deal.\n\n\"That is the session, Ms. Naem.\""
            : "What is left would not cover the minimum here. The dealer waits one polite second, then deals around you as if the chair were empty.",
            "\x13 GET UP", C_AMBER);
    }
    return SC_HUB;
}
