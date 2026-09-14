/* EVERYTHING COUNTS - a card counting story for the Game Boy Advance
 * Copyright (C) 2026 Godless-1
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.  See LICENSE for the full text.
 */
#include "game.h"
#include "video.h"
#include "rng.h"

Run g_run;

/* ====================================================================== */
/*  chrome                                                                */
/* ====================================================================== */
const Implant IMPLANTS[IMP_COUNT] = {
{ "KANTOR OPTIC v3", "KANTOR BIOSYS",
  "Overlays correct basic strategy on the felt.",
  "You stop asking why. You just obey the green.",
  60000,  6, 6, C_CYAN },

{ "ABACUS-9 COPROC", "ZHANG-MERIDIAN",
  "Keeps the running count for you. Always.",
  "The part of you that counted goes quiet, then goes.",
  220000, 26, 16, C_AMBER },

{ "DEEPCOUNT ORACLE", "HALCYON APPLIED",
  "True count and a bet advisory, live.",
  "Halcyon hardware. Halcyon knows what it sees.",
 620000, 38, 22, C_MAG },

{ "EIDETIC SHUNT", "GREY CLINIC",
  "Exact decks remaining. No estimation.",
  "Memory becomes storage. Storage can be seized.",
  150000, 14, 11, C_GREEN },

{ "MNEMONIC WEAVE", "RIPPERDOC SPECIAL",
  "Flags index plays when the count breaks basic.",
  "Somebody else's reflexes wearing your hands.",
 330000, 22, 14, C_PURPLE },

{ "CHRONO-LOBE", "SHIRANUI NEURO",
  "Stretches a pulse check. More seconds to answer.",
  "Time dilates. Then it never quite snaps back.",
  180000, 12, 13, C_ICE },

{ "VAGUS DAMPER", "BACKSTREET GRADE",
  "Flattens your tells. The pit reads you slower.",
  "Fear was information. Now you get none.",
  110000,  9, 9, C_TEAL },

{ "SUBDERMAL VAULT", "KESTREL SALVAGE",
  "A clean credit line nobody can freeze. No wire.",
  "Honest chrome. There is almost none left.",
 280000,  0, 0, (RAMP_CHROME + 20) },
};

/* ====================================================================== */
/*  venues                                                                */
/* ====================================================================== */
const Venue VENUES[NVENUES] = {
{ "THE SLAG PIT", "SUBLEVEL 3 / FOUNDRY",
  "Two decks, bad light, honest odds. Nobody watching.",
  2, 65, 1, 0, 0, 0, 0, 3,    500,    15000,  2, 40, RAMP_GREEN,  C_GREEN,  1, 55, 111 },

{ "LUCKY NINE", "STRIP FRONTAGE",
  "Free drinks. Loud carpet. Blackjack pays 6 to 5.",
  6, 50, 1, 1, 0, 1, 0, 2,    250,    20000,  1, 55, RAMP_GOLD,   C_AMBER,  1, 217, -204 },

{ "GOLDEN LOTUS", "OLD QUARTER",
  "Old money, old rules. They cut deep and pay 3 to 2.",
  6, 75, 0, 1, 1, 0, 0, 4,   2500,   100000,  4, 32, RAMP_ALERT,  C_ORANGE, 2, 32, 24 },

{ "NEON SUTRA", "KOWLOON TERRACE",
  "Eight decks, fast shoes, faster pit. High ceiling.",
  8, 80, 1, 1, 1, 0, 0, 4,   6000,   300000,  6, 26, RAMP_MAG,    C_MAG,    3, 53, -14 },

{ "HALCYON SKYDECK", "SPIRE / FLOOR 188",
  "The best game in the city. Run by the people who own you.",
  6, 85, 0, 1, 1, 0, 1, 4,  20000,  1500000,  9, 14, RAMP_CYAN,   C_CYAN,   5, 27, 68 },

{ "THE VAULT", "SPIRE / BELOW THE WATERLINE",
  "One deck. Heads up. No cameras, because no witnesses.",
  1, 90, 0, 1, 1, 0, 0, 2, 100000,  5000000, 12, 10, RAMP_CHROME, C_PALE,   7, -7, 872 },
};

/* Both numbers are measured, not guessed: test/host/sim.c plays three
 * million hands of perfect basic strategy per room for the house edge, and
 * four million hands of Hi-Lo with a 1-12 spread for the counter's edge. */
int venue_edge_x100(const Venue *v)    { return v->edge_x100; }
int venue_counter_x100(const Venue *v) { return v->counter_x100; }

/* ====================================================================== */
/*  run state                                                             */
/* ====================================================================== */
/* HALCYON does not take one payment.  They take seven. */
const s32 INSTALMENT[NIGHTS] = {
    20000, 60000, 140000, 280000, 450000, 500000, 550000
};

s32 run_due_tonight(void)
{
    int n = g_run.night - 1;
    if (n < 0) n = 0;
    if (n >= NIGHTS) n = NIGHTS - 1;
    s32 owed = 0;
    for (int i = 0; i <= n; i++) owed += INSTALMENT[i];
    s32 due = owed - g_run.escrow_paid;
    return due < 0 ? 0 : due;
}

void run_reset(void)
{
    ec_memset(&g_run, 0, sizeof(g_run));
    g_run.creds = 30000;
    g_run.night = 1;
    g_run.drift = 0;
    g_run.wire  = 0;
    g_run.rui   = 100;
}

void run_add_drift(int d)
{
    int v = g_run.drift + d;
    if (v < 0) v = 0;
    if (v > 100) v = 100;
    g_run.drift = (u8)v;
}

void run_add_wire(int w)
{
    int v = g_run.wire + w;
    if (v < 0) v = 0;
    if (v > 100) v = 100;
    g_run.wire = (u8)v;
}

int run_has(int implant) { return g_run.implant[implant]; }

int run_drift_level(void)
{
    int d = g_run.drift;
    if (d < 15) return 0;
    if (d < 35) return 1;
    if (d < 55) return 2;
    if (d < 80) return 3;
    return 4;
}

const char *run_drift_name(void)
{
    static const char *n[5] = { "INTACT", "FRAYED", "THINNING", "HOLLOW", "VACANT" };
    return n[run_drift_level()];
}

int run_bs_accuracy(void)
{
    if (!g_run.bs_total) return 100;
    return (g_run.bs_correct * 100) / g_run.bs_total;
}

int run_check_accuracy(void)
{
    int t = g_run.checks_passed + g_run.checks_failed;
    if (!t) return 100;
    return (g_run.checks_passed * 100) / t;
}

/* ====================================================================== */
/*  counting                                                              */
/* ====================================================================== */
void count_reset(Counter *c, int decks)
{
    c->running = 0;
    c->seen = 0;
    c->decks = (u8)decks;
}

void count_card(Counter *c, u8 card)
{
    if (card == NO_CARD) return;
    c->running += card_tag(card);
    c->seen++;
}

int count_true_x10(const Counter *c, const Shoe *s)
{
    int q = shoe_decks_left_x4(s);          /* quarter decks remaining */
    if (q < 1) q = 1;
    return (c->running * 40) / q;
}

int count_true(const Counter *c, const Shoe *s)
{
    int t = count_true_x10(c, s);
    return t / 10;                          /* truncates toward zero */
}

/* ====================================================================== */
/*  basic strategy                                                        */
/* ====================================================================== */
/*  Columns are dealer up-cards 2 3 4 5 6 7 8 9 10 A.
 *  H hit   S stand   D double else hit   V double else stand
 *  P split   p split only if double-after-split is allowed
 *  R surrender else hit   r surrender else stand                        */

static const char hard_tab[17][11] = {
/*  5 */ "HHHHHHHHHH",
/*  6 */ "HHHHHHHHHH",
/*  7 */ "HHHHHHHHHH",
/*  8 */ "HHHHHHHHHH",
/*  9 */ "HDDDDHHHHH",
/* 10 */ "DDDDDDDDHH",
/* 11 */ "DDDDDDDDDH",
/* 12 */ "HHSSSHHHHH",
/* 13 */ "SSSSSHHHHH",
/* 14 */ "SSSSSHHHHH",
/* 15 */ "SSSSSHHHRH",
/* 16 */ "SSSSSHHRRR",
/* 17 */ "SSSSSSSSSS",
/* 18 */ "SSSSSSSSSS",
/* 19 */ "SSSSSSSSSS",
/* 20 */ "SSSSSSSSSS",
/* 21 */ "SSSSSSSSSS",
};

static const char soft_tab[9][11] = {
/* 13 A2 */ "HHHDDHHHHH",
/* 14 A3 */ "HHHDDHHHHH",
/* 15 A4 */ "HHDDDHHHHH",
/* 16 A5 */ "HHDDDHHHHH",
/* 17 A6 */ "HDDDDHHHHH",
/* 18 A7 */ "VVVVVSSHHH",
/* 19 A8 */ "SSSSSSSSSS",
/* 20 A9 */ "SSSSSSSSSS",
/* 21    */ "SSSSSSSSSS",
};

static const char pair_tab[10][11] = {
/* A,A  */ "PPPPPPPPPP",
/* 2,2  */ "ppPPPPHHHH",
/* 3,3  */ "ppPPPPHHHH",
/* 4,4  */ "HHHppHHHHH",
/* 5,5  */ "DDDDDDDDHH",
/* 6,6  */ "pPPPPHHHHH",
/* 7,7  */ "PPPPPPHHHH",
/* 8,8  */ "PPPPPPPPPP",
/* 9,9  */ "PPPPPSPPSS",
/* T,T  */ "SSSSSSSSSS",
};

int up_index(u8 card)
{
    int r = CARD_RANK(card);
    if (r == 0) return 9;         /* ace */
    if (r <= 8) return r - 1;     /* 2..9 */
    return 8;                     /* ten-valued */
}

static int pair_index(const Hand *h)
{
    int r = CARD_RANK(h->card[0]);
    if (r == 0) return 0;
    if (r >= 9) return 9;
    return r;                      /* rank 1 (a two) -> row 1 */
}

static int resolve(char c, int can_double, int can_split, int can_surr, int das)
{
    switch (c) {
        case 'H': return ACT_HIT;
        case 'S': return ACT_STAND;
        case 'D': return can_double ? ACT_DOUBLE : ACT_HIT;
        case 'V': return can_double ? ACT_DOUBLE : ACT_STAND;
        case 'P': return can_split  ? ACT_SPLIT  : ACT_HIT;
        case 'p': return (can_split && das) ? ACT_SPLIT : ACT_HIT;
        case 'R': return can_surr   ? ACT_SURRENDER : ACT_HIT;
        case 'r': return can_surr   ? ACT_SURRENDER : ACT_STAND;
    }
    return ACT_HIT;
}

int bs_action(const Hand *h, int dealer_up, const Venue *v, int can_double,
              int can_split, int can_surrender)
{
    int up = up_index((u8)dealer_up);
    int total = hand_total(h);

    if (h->n == 2 && can_split) {
        int pi = pair_index(h);
        int a = CARD_RANK(h->card[0]), b = CARD_RANK(h->card[1]);
        int va = (a >= 9) ? 10 : (a == 0 ? 1 : a + 1);
        int vb = (b >= 9) ? 10 : (b == 0 ? 1 : b + 1);
        if (va == vb) {
            char c = pair_tab[pi][up];
            if (c == 'P' || c == 'p' || pi == 4 || pi == 9)
                return resolve(c, can_double, can_split, can_surrender, v->das);
        }
    }

    if (hand_is_soft(h) && total >= 13) {
        int row = total - 13;
        if (row > 8) row = 8;
        char c = soft_tab[row][up];
        if (v->h17 && total == 19 && up == 4) c = 'V';     /* A8 vs 6 */
        return resolve(c, can_double, can_split, can_surrender, v->das);
    }

    if (total < 5) total = 5;
    if (total > 21) total = 21;
    char c = hard_tab[total - 5][up];
    if (v->h17) {
        if (total == 11 && up == 9) c = 'D';
        if (total == 15 && up == 9) c = 'R';
        if (total == 17 && up == 9) c = 'r';
    }
    if (!v->surrender && (c == 'R')) c = 'H';
    if (!v->surrender && (c == 'r')) c = 'S';
    return resolve(c, can_double, can_split, can_surrender, v->das);
}

/* ---- the Illustrious 18 ---------------------------------------------- */
typedef struct {
    s8 pair_tens;   /* 1 = applies only to a pair of tens */
    s8 total;
    s8 up;          /* column index */
    s8 at;          /* threshold */
    s8 ge;          /* 1 = tc >= at, 0 = tc <= at */
    s8 act;
} Dev;

static const Dev DEVS[] = {
    { 0, 16, 8,  0, 1, ACT_STAND  },   /* 16 v 10 : stand at 0 or better */
    { 0, 15, 8,  4, 1, ACT_STAND  },
    { 1, 20, 3,  5, 1, ACT_SPLIT  },   /* 10,10 v 5 */
    { 1, 20, 4,  4, 1, ACT_SPLIT  },   /* 10,10 v 6 */
    { 0, 10, 8,  4, 1, ACT_DOUBLE },
    { 0, 12, 1,  2, 1, ACT_STAND  },
    { 0, 12, 0,  3, 1, ACT_STAND  },
    { 0, 11, 9,  1, 1, ACT_DOUBLE },
    { 0,  9, 0,  1, 1, ACT_DOUBLE },
    { 0, 10, 9,  4, 1, ACT_DOUBLE },
    { 0,  9, 5,  3, 1, ACT_DOUBLE },
    { 0, 16, 7,  5, 1, ACT_STAND  },
    { 0, 13, 0, -1, 0, ACT_HIT    },
    { 0, 12, 2, -1, 0, ACT_HIT    },
    { 0, 12, 3, -2, 0, ACT_HIT    },
    { 0, 12, 4, -1, 0, ACT_HIT    },
    { 0, 13, 1, -2, 0, ACT_HIT    },
};
#define NDEVS ((int)(sizeof(DEVS)/sizeof(DEVS[0])))

int dev_insurance(int tc) { return tc >= 3; }

int dev_action(const Hand *h, int dealer_up, int tc, const Venue *v,
               int can_double, int can_split, int can_surrender, int *deviated)
{
    int base = bs_action(h, dealer_up, v, can_double, can_split, can_surrender);
    if (deviated) *deviated = 0;
    if (hand_is_soft(h)) return base;

    int up = up_index((u8)dealer_up);
    int total = hand_total(h);
    int is_tens = (h->n == 2 &&
                   CARD_RANK(h->card[0]) >= 9 && CARD_RANK(h->card[1]) >= 9);

    for (int i = 0; i < NDEVS; i++) {
        const Dev *d = &DEVS[i];
        if (d->up != up || d->total != total) continue;
        if (d->pair_tens && !(is_tens && can_split)) continue;
        if (!d->pair_tens && is_tens && d->total == 20) continue;
        int fires = d->ge ? (tc >= d->at) : (tc <= d->at);
        if (!fires) continue;
        int act = d->act;
        if (act == ACT_DOUBLE && !can_double) act = ACT_HIT;
        if (act == ACT_SPLIT && !can_split)   continue;
        if (act != base && deviated) *deviated = 1;
        return act;
    }
    return base;
}

const char *action_name(int a)
{
    switch (a) {
        case ACT_HIT:       return "HIT";
        case ACT_STAND:     return "STAND";
        case ACT_DOUBLE:    return "DOUBLE";
        case ACT_SPLIT:     return "SPLIT";
        case ACT_SURRENDER: return "FOLD";
    }
    return "-";
}

const char *action_short(int a)
{
    switch (a) {
        case ACT_HIT:       return "HIT";
        case ACT_STAND:     return "STD";
        case ACT_DOUBLE:    return "DBL";
        case ACT_SPLIT:     return "SPL";
        case ACT_SURRENDER: return "FLD";
    }
    return "-";
}
