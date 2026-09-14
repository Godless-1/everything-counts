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
 *  game.h - run state, venues, chrome, and the shared save block.
 * ===================================================================== */
#ifndef GAME_H
#define GAME_H
#include "gba.h"
#include "cards.h"
#include "blackjack.h"

#define NIGHTS        7
#define ESCROW_TARGET 2000000

/* ---- chrome -------------------------------------------------------- */
enum {
    IMP_OPTIC,      /* KANTOR OPTIC    - basic-strategy overlay      */
    IMP_ABACUS,     /* ABACUS-9        - running count on the HUD    */
    IMP_ORACLE,     /* DEEPCOUNT       - true count + bet advisory   */
    IMP_EIDETIC,    /* EIDETIC SHUNT   - decks remaining, exact      */
    IMP_MNEMONIC,   /* MNEMONIC WEAVE  - index-play deviations       */
    IMP_CHRONO,     /* CHRONO-LOBE     - stretches the pulse check   */
    IMP_DAMPER,     /* VAGUS DAMPER    - bleeds pit heat             */
    IMP_SUBDERMAL,  /* SUBDERMAL VAULT - clean credit line, no drift */
    IMP_COUNT
};

typedef struct {
    const char *name;
    const char *maker;
    const char *effect;
    const char *warning;
    s32 cost;
    u8  wire;      /* telemetry signature the pit can scan for */
    u8  drift;     /* how much of you it eats                  */
    u8  colour;
} Implant;

extern const Implant IMPLANTS[IMP_COUNT];

/* ---- venues -------------------------------------------------------- */
typedef struct {
    const char *name;
    const char *district;
    const char *blurb;
    u8  decks;
    u8  pen;            /* penetration, percent of the shoe dealt   */
    u8  h17;            /* dealer hits soft 17                      */
    u8  das;            /* double after split                       */
    u8  surrender;      /* late surrender                           */
    u8  bj65;           /* blackjack pays 6:5 (a trap)              */
    u8  rsa;            /* resplit aces                             */
    u8  max_hands;
    s32 min_bet;
    s32 max_bet;
    u8  heat_rate;      /* how fast the pit notices                 */
    u8  scan_floor;     /* wire level that starts drawing scans     */
    u8  ramp;           /* palette ramp for the room                */
    u8  accent;
    u8  night_min;      /* unlocks on this night                    */
    s16 edge_x100;      /* measured house edge, hundredths of a %   */
    s16 counter_x100;   /* measured counter edge per unit, same unit*/
} Venue;

#define NVENUES 6
extern const Venue VENUES[NVENUES];

int venue_edge_x100(const Venue *v);     /* house edge, hundredths of a % */
int venue_counter_x100(const Venue *v);  /* Hi-Lo counter edge, same unit */

/* ---- story flags ---------------------------------------------------- */
enum {
    FLAG_MARA_SPARED, FLAG_MARA_SOLD, FLAG_HOT_CHROME, FLAG_LAUNDERED,
    FLAG_REFUSED_SABLE, FLAG_OZEROV_TAKEN, FLAG_OZEROV_SPARED,
    FLAG_SOLD_MIND, FLAG_REFUSED_HALCYON, FLAG_RUI_CALLED, FLAG_CLINIC_PAID,
    FLAG_BACKED_OFF, FLAG_SCANNED, FLAG_SAW_TUTORIAL, FLAG_COUNT
};

typedef struct {
    s32 creds;
    s32 escrow_paid;
    u8  night;
    u8  drift;            /* 0..100 - how much of you is gone */
    u8  wire;             /* 0..100 - telemetry signature     */
    u8  implant[IMP_COUNT];
    u8  flag[FLAG_COUNT];
    u16 hands_played;
    u16 bs_correct;
    u16 bs_total;
    u16 checks_passed;
    u16 checks_failed;
    u16 best_streak;
    u16 streak;
    s32 biggest_win;
    u8  venue_locked[NVENUES];
    u8  seen_intro;
    u8  ending;
    u8  rui;              /* Rui's engram integrity, 0..100 */
    u8  liens;            /* missed escrow instalments      */
    u8  beat_done[NIGHTS + 2];
    u8  lessons_done;     /* bitmask of finished DRY RUN modules */
    u8  best_drill[6];
} Run;

extern const s32 INSTALMENT[NIGHTS];
s32 run_due_tonight(void);

extern Run g_run;

void run_reset(void);
int  run_drift_level(void);        /* 0..4 */
const char *run_drift_name(void);
int  run_bs_accuracy(void);        /* percent */
int  run_check_accuracy(void);     /* percent */
void run_add_drift(int d);
void run_add_wire(int w);
int  run_has(int implant);

/* ---- counting ------------------------------------------------------- */
typedef struct {
    s16 running;
    u16 seen;
    u8  decks;
} Counter;

void  count_reset(Counter *c, int decks);
void  count_card(Counter *c, u8 card);
int   count_true_x10(const Counter *c, const Shoe *s);  /* true count * 10 */
int   count_true(const Counter *c, const Shoe *s);      /* rounded, floored toward zero */

/* ---- strategy ------------------------------------------------------- */
enum { ACT_HIT, ACT_STAND, ACT_DOUBLE, ACT_SPLIT, ACT_SURRENDER, ACT_NONE };

int  bs_action(const Hand *h, int dealer_up, const Venue *v, int can_double,
               int can_split, int can_surrender);
int  dev_action(const Hand *h, int dealer_up, int tc, const Venue *v,
                int can_double, int can_split, int can_surrender, int *deviated);
int  dev_insurance(int tc);
const char *action_name(int a);
const char *action_short(int a);

/* index of the dealer up-card column, 0..9 for 2..A */
int  up_index(u8 card);

#endif
