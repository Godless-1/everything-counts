/* EVERYTHING COUNTS - a card counting story for the Game Boy Advance
 * Copyright (C) 2026 Godless-1
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
/* ========================================================================
 *  sim.c - host-side verification of the rules engine and the strategy
 *  tables.  Nothing here is compiled into the ROM.
 *
 *    1. spot-checks the basic-strategy chart against a reference
 *    2. checks hand evaluation, soft/hard, blackjack detection
 *    3. checks Hi-Lo tags and true-count arithmetic
 *    4. runs a few million hands of perfect basic strategy and reports
 *       the realised house edge, which must land near the computed one
 * ===================================================================== */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "game.h"
#include "cards.h"
#include "blackjack.h"
#include "rng.h"

/* ---- stubs for everything graphical ---------------------------------- */
const u16 g_palette[256];
const u8 pip_spike[81], pip_pulse[81], pip_shard[81], pip_wire[81];
const u8 sigil_halcyon[315], sigil_kestrel[208];
const u8 g_font6x8[128*8];
void vid_plot(int a,int b,u8 c){(void)a;(void)b;(void)c;}
void vid_hline(int a,int b,int c,u8 d){(void)a;(void)b;(void)c;(void)d;}
void vid_vline(int a,int b,int c,u8 d){(void)a;(void)b;(void)c;(void)d;}
void vid_rect(int a,int b,int c,int d,u8 e){(void)a;(void)b;(void)c;(void)d;(void)e;}
void vid_frame_rect(int a,int b,int c,int d,u8 e){(void)a;(void)b;(void)c;(void)d;(void)e;}
void vid_vgrad(int a,int b,int c,int d,int e,int f,int g){(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;}
void vid_hgrad(int a,int b,int c,int d,int e,int f,int g){(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;}
void vid_blit4(int a,int b,int c,int d,const u8*e,const u8*f){(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;}
void vid_blit4_scaled(int a,int b,int c,int d,const u8*e,const u8*f,int g){(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;}
int  vid_text(int a,int b,const char*c,u8 d){(void)a;(void)b;(void)c;(void)d;return 0;}
int  vid_text_len(const char*s){return (int)strlen(s)*6;}
char *str_int(char *d,int v){ d+=sprintf(d,"%d",v); return d; }
char *str_cat(char *d,const char *s){ while(*s)*d++=*s++; *d=0; return d; }
void *ec_memset(void *d,int c,u32 n){ return memset(d,c,n); }
void *ec_memcpy(void *d,const void *s,u32 n){ return memcpy(d,s,n); }
u32   ec_strlen(const char *s){ return (u32)strlen(s); }


static int fails = 0;
static int checks = 0;
static void CHECK(int cond, const char *what)
{
    checks++;
    if (!cond) { printf("  FAIL  %s\n", what); fails++; }
}

/* ---------------------------------------------------------------------- */
static Hand mk(int n, ...)
{
    (void)n;
    Hand h; hand_clear(&h);
    return h;
}

static Hand h2(u8 a, u8 b) { Hand h; hand_clear(&h); hand_add(&h,a); hand_add(&h,b); return h; }
static Hand h3(u8 a, u8 b, u8 c) { Hand h = h2(a,b); hand_add(&h,c); return h; }

#define A_  CARD(0,0)
#define N(r) CARD((r)-1,1)      /* 2..9 -> ranks 1..8 */
#define T_  CARD(9,2)
#define J_  CARD(10,3)
#define Q_  CARD(11,0)
#define K_  CARD(12,1)

static const char *act_ch(int a)
{
    switch(a){case ACT_HIT:return "H";case ACT_STAND:return "S";case ACT_DOUBLE:return "D";
              case ACT_SPLIT:return "P";case ACT_SURRENDER:return "R";}
    return "?";
}

/* ---------------------------------------------------------------------- */
static void test_values(void)
{
    puts("-- card values and hand evaluation");
    CHECK(card_value(A_) == 11, "ace is 11");
    CHECK(card_value(T_) == 10 && card_value(J_) == 10 &&
          card_value(Q_) == 10 && card_value(K_) == 10, "ten-value cards");
    for (int r = 2; r <= 9; r++) CHECK(card_value(N(r)) == r, "number card value");

    Hand h = h2(A_, N(6));
    CHECK(hand_total(&h) == 17, "A+6 = 17");
    CHECK(hand_is_soft(&h), "A+6 is soft");
    hand_add(&h, N(9));
    CHECK(hand_total(&h) == 16, "A+6+9 = 16 hard");
    CHECK(!hand_is_soft(&h), "A+6+9 is hard");

    Hand b = h2(A_, K_);
    CHECK(hand_total(&b) == 21 && hand_is_bj(&b), "A+K is blackjack");
    Hand nb = h3(N(7), N(7), N(7));
    CHECK(hand_total(&nb) == 21 && !hand_is_bj(&nb), "7-7-7 is 21 but not blackjack");
    Hand sp = h2(N(8), N(8));
    sp.from_split = 1;
    hand_clear(&sp); hand_add(&sp, A_); hand_add(&sp, K_); sp.from_split = 1;
    CHECK(!hand_is_bj(&sp), "21 after a split is not a blackjack");

    Hand aa = h3(A_, A_, N(9));
    CHECK(hand_total(&aa) == 21, "A+A+9 = 21");
    Hand a3 = h3(A_, A_, A_);
    CHECK(hand_total(&a3) == 13, "A+A+A = 13");
}

static void test_tags(void)
{
    puts("-- hi-lo tags");
    for (int r = 2; r <= 6; r++) CHECK(card_tag(N(r)) == 1, "2-6 is +1");
    for (int r = 7; r <= 9; r++) CHECK(card_tag(N(r)) == 0, "7-9 is 0");
    CHECK(card_tag(T_) == -1 && card_tag(J_) == -1 && card_tag(Q_) == -1 &&
          card_tag(K_) == -1 && card_tag(A_) == -1, "tens and aces are -1");

    /* a full deck must count to exactly zero */
    Shoe s; rng_seed(1); shoe_build(&s, 1, 100);
    int total = 0;
    for (int i = 0; i < s.n; i++) total += card_tag(s.cards[i]);
    CHECK(total == 0, "a balanced deck counts to zero");

    /* six decks too */
    shoe_build(&s, 6, 100);
    total = 0;
    for (int i = 0; i < s.n; i++) total += card_tag(s.cards[i]);
    CHECK(total == 0, "six decks count to zero");

    /* every rank appears 4 times per deck */
    int rc[13] = {0};
    shoe_build(&s, 1, 100);
    for (int i = 0; i < s.n; i++) rc[CARD_RANK(s.cards[i])]++;
    int ok = 1;
    for (int r = 0; r < 13; r++) if (rc[r] != 4) ok = 0;
    CHECK(ok, "one deck holds four of each rank");
}

static void test_true_count(void)
{
    puts("-- true count");
    Shoe s; shoe_build(&s, 6, 100);
    Counter c; count_reset(&c, 6);
    c.running = 9;
    s.pos = s.n - 3 * 52;               /* exactly three decks left */
    CHECK(shoe_decks_left_x4(&s) == 12, "three decks left reads as 12 quarters");
    CHECK(count_true(&c, &s) == 3, "RC +9 with 3 decks left = TC +3");
    s.pos = s.n - 6 * 52;
    CHECK(count_true(&c, &s) == 1, "RC +9 with 6 decks left = TC +1");
    c.running = -4;
    s.pos = s.n - 2 * 52;
    CHECK(count_true(&c, &s) == -2, "RC -4 with 2 decks left = TC -2");
}

/* the reference chart: 6 decks, S17, DAS, late surrender */
static const char *REF_HARD[17] = {
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
static const char *REF_SOFT[8] = {
/* A2 */ "HHHDDHHHHH",
/* A3 */ "HHHDDHHHHH",
/* A4 */ "HHDDDHHHHH",
/* A5 */ "HHDDDHHHHH",
/* A6 */ "HDDDDHHHHH",
/* A7 */ "DDDDDSSHHH",
/* A8 */ "SSSSSSSSSS",
/* A9 */ "SSSSSSSSSS",
};
static const char *REF_PAIR[10] = {
/* AA */ "PPPPPPPPPP",
/* 22 */ "PPPPPPHHHH",
/* 33 */ "PPPPPPHHHH",
/* 44 */ "HHHPPHHHHH",
/* 55 */ "DDDDDDDDHH",
/* 66 */ "PPPPPHHHHH",
/* 77 */ "PPPPPPHHHH",
/* 88 */ "PPPPPPPPPP",
/* 99 */ "PPPPPSPPSS",
/* TT */ "SSSSSSSSSS",
};

static const u8 UP[10] = { N(2),N(3),N(4),N(5),N(6),N(7),N(8),N(9),T_,A_ };

static void test_chart(void)
{
    puts("-- basic strategy chart (6 deck, S17, DAS, LS)");
    const Venue *v = &VENUES[2];
    CHECK(v->decks == 6 && !v->h17 && v->das && v->surrender, "reference venue rules");

    for (int t = 5; t <= 21; t++) {
        Hand h; hand_clear(&h);
        if (t >= 21) { hand_add(&h,T_); hand_add(&h,CARD(8,0)); hand_add(&h,N(2)); }
        else if (t <= 11) { hand_add(&h, CARD(t-2-1,0)); hand_add(&h, N(2)); }
        else { hand_add(&h, T_); hand_add(&h, CARD(t-10-1,1)); }
        if (hand_total(&h) != t) { printf("  FAIL  hard builder for %d gave %d\n", t, hand_total(&h)); fails++; }
        for (int u = 0; u < 10; u++) {
            int cds = hand_can_double(&h, v->das);
            int csu = v->surrender && hand_can_surrender(&h);
            int a = bs_action(&h, UP[u], v, cds, 0, csu);
            char want = REF_HARD[t-5][u];
            char got  = act_ch(a)[0];
            if (want != got) {
                printf("  FAIL  hard %2d vs %-2s  want %c got %c\n",
                       t, u==9?"A":(u==8?"10":(char[2]){'2'+u,0}), want, got);
                fails++;
            }
            checks++;
        }
    }
    for (int r = 0; r < 8; r++) {
        Hand h = h2(A_, CARD(r+1, 1));      /* A,2 .. A,9 */
        for (int u = 0; u < 10; u++) {
            int cds = hand_can_double(&h, v->das);
            int csu = v->surrender && hand_can_surrender(&h);
            int a = bs_action(&h, UP[u], v, cds, 0, csu);
            char want = REF_SOFT[r][u], got = act_ch(a)[0];
            if (want != got) {
                printf("  FAIL  soft A%d vs %d  want %c got %c\n", r+2, u+2, want, got);
                fails++;
            }
            checks++;
        }
    }
    for (int r = 0; r < 10; r++) {
        u8 c = (r == 0) ? A_ : (r == 9 ? T_ : CARD(r, 0));
        Hand h; hand_clear(&h); hand_add(&h, c); hand_add(&h, (u8)(c ^ 1));
        for (int u = 0; u < 10; u++) {
            int cds = hand_can_double(&h, v->das);
            int a = bs_action(&h, UP[u], v, cds, 1, 0);
            char want = REF_PAIR[r][u], got = act_ch(a)[0];
            if (want != got) {
                printf("  FAIL  pair %d vs %d  want %c got %c\n", r, u+2, want, got);
                fails++;
            }
            checks++;
        }
    }
}

static void test_deviations(void)
{
    puts("-- index plays");
    const Venue *v = &VENUES[2];
    int dev;
    Hand h16 = h2(T_, N(6));
    CHECK(dev_action(&h16, T_, -1, v, 0, 0, 0, &dev) == ACT_HIT,   "16v10 hits at TC -1");
    CHECK(dev_action(&h16, T_,  0, v, 0, 0, 0, &dev) == ACT_STAND, "16v10 stands at TC 0");
    Hand h12 = h2(T_, N(2));
    CHECK(dev_action(&h12, N(3), 1, v, 0, 0, 0, &dev) == ACT_HIT,   "12v3 hits at TC +1");
    CHECK(dev_action(&h12, N(3), 2, v, 0, 0, 0, &dev) == ACT_STAND, "12v3 stands at TC +2");
    Hand h11 = h2(N(6), N(5));
    CHECK(dev_action(&h11, A_, 0, v, 1, 0, 0, &dev) == ACT_HIT,    "11vA hits at TC 0 (S17)");
    CHECK(dev_action(&h11, A_, 1, v, 1, 0, 0, &dev) == ACT_DOUBLE, "11vA doubles at TC +1");
    Hand h13 = h2(T_, N(3));
    CHECK(dev_action(&h13, N(2), 0, v, 0, 0, 0, &dev) == ACT_STAND, "13v2 stands at TC 0");
    CHECK(dev_action(&h13, N(2), -1, v, 0, 0, 0, &dev) == ACT_HIT,  "13v2 hits at TC -1");
    CHECK(!dev_insurance(2) && dev_insurance(3), "insurance at TC +3");
}

/* ====================================================================== */
/*  monte carlo                                                           */
/* ====================================================================== */
typedef struct { long long wager; long long net; long long hands; } Res;

static void dealer_play_sim(Hand *d, Shoe *s, const Venue *v)
{
    for (;;) {
        int t = hand_total(d);
        int soft = hand_is_soft(d);
        if (t > 21) break;
        if (t > 17) break;
        if (t == 17 && !(soft && v->h17)) break;
        hand_add(d, shoe_draw(s));
    }
}

static Res simulate(const Venue *v, long long rounds, int use_count)
{
    Res r = {0,0,0};
    Shoe s;
    Counter c;
    shoe_build(&s, v->decks, v->pen);
    count_reset(&c, v->decks);

    for (long long i = 0; i < rounds; i++) {
        if (shoe_exhausted(&s)) { shoe_build(&s, v->decks, v->pen); count_reset(&c, v->decks); }
        int tc = use_count ? count_true(&c, &s) : 0;
        s32 bet = 100;

        Hand hand[MAX_HANDS];
        int nh = 1;
        hand_clear(&hand[0]);
        hand[0].bet = bet;
        Hand dealer; hand_clear(&dealer);

        u8 c1 = shoe_draw(&s); hand_add(&hand[0], c1); count_card(&c, c1);
        u8 d1 = shoe_draw(&s); hand_add(&dealer, d1);  count_card(&c, d1);
        u8 c2 = shoe_draw(&s); hand_add(&hand[0], c2); count_card(&c, c2);
        u8 d2 = shoe_draw(&s); hand_add(&dealer, d2);

        s32 net = 0;
        s32 wagered = bet;

        /* insurance, played by the index when counting */
        int insured = 0;
        if (CARD_RANK(d1) == 0 && use_count && dev_insurance(tc)) {
            insured = 1;
            wagered += bet / 2;
        }

        int dealer_bj = hand_is_bj(&dealer);
        if (insured) net += dealer_bj ? bet : -bet / 2;

        if (dealer_bj) {
            count_card(&c, d2);
            net += hand_is_bj(&hand[0]) ? 0 : -bet;
            r.wager += wagered; r.net += net; r.hands++;
            continue;
        }
        if (hand_is_bj(&hand[0])) {
            count_card(&c, d2);
            net += v->bj65 ? (bet * 6) / 5 : (bet * 3) / 2;
            r.wager += wagered; r.net += net; r.hands++;
            continue;
        }

        for (int hi = 0; hi < nh; hi++) {
            for (;;) {
                Hand *h = &hand[hi];
                if (hand_total(h) >= 21) break;
                /* split aces draw exactly one card; RSA only permits a re-split */
                if (h->from_split && CARD_RANK(h->card[0]) == 0 && h->n == 2) {
                    if (!(v->rsa && hand_can_split(h, nh, v->max_hands, 1))) break;
                }
                int cds = hand_can_double(h, v->das);
                int cs  = hand_can_split(h, nh, v->max_hands, v->rsa);
                int csu = v->surrender && hand_can_surrender(h) && nh == 1;
                int dev = 0;
                int a = use_count ? dev_action(h, d1, tc, v, cds, cs, csu, &dev)
                                  : bs_action(h, d1, v, cds, cs, csu);
                if (a == ACT_STAND) break;
                if (a == ACT_SURRENDER) { h->surrendered = 1; break; }
                if (a == ACT_DOUBLE) {
                    wagered += h->bet;
                    h->bet *= 2; h->doubled = 1;
                    u8 nc = shoe_draw(&s); hand_add(h, nc); count_card(&c, nc);
                    break;
                }
                if (a == ACT_SPLIT) {
                    Hand *nhd = &hand[nh];
                    hand_clear(nhd);
                    nhd->bet = bet; nhd->from_split = 1;
                    hand_add(nhd, h->card[1]);
                    h->n = 1; h->from_split = 1;
                    nh++;
                    wagered += bet;
                    u8 x = shoe_draw(&s); hand_add(h, x);    count_card(&c, x);
                    u8 y = shoe_draw(&s); hand_add(nhd, y);  count_card(&c, y);
                    continue;
                }
                u8 nc = shoe_draw(&s); hand_add(h, nc); count_card(&c, nc);
                if (hand_total(h) > 21) { h->busted = 1; break; }
            }
        }

        count_card(&c, d2);
        int live = 0;
        for (int hi = 0; hi < nh; hi++)
            if (!hand[hi].busted && !hand[hi].surrendered) live = 1;
        if (live) {
            /* count the dealer's draws too */
            for (;;) {
                int t = hand_total(&dealer), soft = hand_is_soft(&dealer);
                if (t > 21 || t > 17) break;
                if (t == 17 && !(soft && v->h17)) break;
                u8 nc = shoe_draw(&s); hand_add(&dealer, nc); count_card(&c, nc);
            }
        }
        int dt = hand_total(&dealer);

        for (int hi = 0; hi < nh; hi++) {
            Hand *h = &hand[hi];
            int pt = hand_total(h);
            if (h->surrendered)   net -= h->bet / 2;
            else if (h->busted)   net -= h->bet;
            else if (dt > 21)     net += h->bet;
            else if (pt > dt)     net += h->bet;
            else if (pt < dt)     net -= h->bet;
        }
        r.wager += wagered;
        r.net += net;
        r.hands++;
    }
    return r;
}

/* ---------------------------------------------------------------------- */
/*  decision EV probe: what is a specific play actually worth?             */
/* ---------------------------------------------------------------------- */
static double ev_of(const Venue *v, u8 p1, u8 p2, u8 up, int action, long long n)
{
    double sum = 0;
    for (long long i = 0; i < n; i++) {
        Shoe s; shoe_build(&s, v->decks, 100);
        /* pull the three known cards out of the shoe */
        u8 want[3] = { p1, p2, up };
        for (int k = 0; k < 3; k++)
            for (int j = 0; j < s.n; j++)
                if (s.cards[j] == want[k]) { s.cards[j] = s.cards[--s.n]; break; }

        Hand d; hand_clear(&d); hand_add(&d, up);
        /* the dealer has peeked: no blackjack under there */
        u8 hole;
        for (;;) {
            hole = s.cards[rng_below(s.n)];
            int t = card_value(up) + card_value(hole);
            if (CARD_RANK(up) == 0 || card_value(up) == 10) { if (t == 21) continue; }
            break;
        }
        for (int j = 0; j < s.n; j++) if (s.cards[j] == hole) { s.cards[j] = s.cards[--s.n]; break; }
        s.pos = 0;
        /* shuffle what is left */
        for (int j = s.n - 1; j > 0; j--) {
            int k = (int)rng_below((u32)(j + 1));
            u8 t = s.cards[j]; s.cards[j] = s.cards[k]; s.cards[k] = t;
        }
        hand_add(&d, hole);

        Hand h; hand_clear(&h); hand_add(&h, p1); hand_add(&h, p2);
        double bet = 1.0;
        if (action == ACT_SURRENDER) { sum -= 0.5; continue; }
        if (action == ACT_HIT) {
            hand_add(&h, shoe_draw(&s));
            while (hand_total(&h) <= 21) {
                int a = bs_action(&h, up, v, 0, 0, 0);
                if (a != ACT_HIT) break;
                hand_add(&h, shoe_draw(&s));
            }
        }
        if (hand_total(&h) > 21) { sum -= bet; continue; }
        dealer_play_sim(&d, &s, v);
        int dt = hand_total(&d), pt = hand_total(&h);
        if (dt > 21 || pt > dt) sum += bet;
        else if (pt < dt)       sum -= bet;
    }
    return sum / (double)n;
}

static void probe(const char *label, const Venue *v, u8 a, u8 b, u8 up)
{
    long long n = 400000;
    rng_seed(4242);  double es = ev_of(v, a, b, up, ACT_STAND, n);
    rng_seed(4242);  double eh = ev_of(v, a, b, up, ACT_HIT, n);
    double best = es > eh ? es : eh;
    printf("  %-22s stand %+.4f  hit %+.4f  surrender -0.5000  ->  %s\n",
           label, es, eh,
           (-0.5 > best) ? "SURRENDER" : (es > eh ? "STAND" : "HIT"));
}

/* ---------------------------------------------------------------------- */
static double counter_ev(const Venue *v, long long rounds, int spread)
{
    Shoe s; Counter c;
    shoe_build(&s, v->decks, v->pen);
    count_reset(&c, v->decks);
    double net = 0; double hands = 0;
    for (long long i = 0; i < rounds; i++) {
        if (shoe_exhausted(&s)) { shoe_build(&s, v->decks, v->pen); count_reset(&c, v->decks); }
        int tc = count_true(&c, &s);
        int units = tc - 1;
        if (units < 1) units = 1;
        if (units > spread) units = spread;
        s32 bet = units * 100;

        Hand hand[MAX_HANDS]; int nh = 1;
        hand_clear(&hand[0]); hand[0].bet = bet;
        Hand dealer; hand_clear(&dealer);
        u8 c1 = shoe_draw(&s); hand_add(&hand[0], c1); count_card(&c, c1);
        u8 d1 = shoe_draw(&s); hand_add(&dealer, d1);  count_card(&c, d1);
        u8 c2 = shoe_draw(&s); hand_add(&hand[0], c2); count_card(&c, c2);
        u8 d2 = shoe_draw(&s); hand_add(&dealer, d2);
        s32 r = 0;
        if (CARD_RANK(d1) == 0 && dev_insurance(tc))
            r += hand_is_bj(&dealer) ? bet : -bet / 2;
        if (hand_is_bj(&dealer)) {
            count_card(&c, d2);
            r += hand_is_bj(&hand[0]) ? 0 : -bet;
            net += r; hands++; continue;
        }
        if (hand_is_bj(&hand[0])) {
            count_card(&c, d2);
            r += v->bj65 ? (bet * 6) / 5 : (bet * 3) / 2;
            net += r; hands++; continue;
        }
        for (int hi = 0; hi < nh; hi++) {
            for (;;) {
                Hand *h = &hand[hi];
                if (hand_total(h) >= 21) break;
                if (h->from_split && CARD_RANK(h->card[0]) == 0 && h->n == 2) {
                    if (!(v->rsa && hand_can_split(h, nh, v->max_hands, 1))) break;
                }
                int cds = hand_can_double(h, v->das);
                int cs  = hand_can_split(h, nh, v->max_hands, v->rsa);
                int csu = v->surrender && hand_can_surrender(h) && nh == 1;
                int dev = 0;
                int a = dev_action(h, d1, tc, v, cds, cs, csu, &dev);
                if (a == ACT_STAND) break;
                if (a == ACT_SURRENDER) { h->surrendered = 1; break; }
                if (a == ACT_DOUBLE) {
                    h->bet *= 2; h->doubled = 1;
                    u8 nc = shoe_draw(&s); hand_add(h, nc); count_card(&c, nc);
                    break;
                }
                if (a == ACT_SPLIT) {
                    Hand *nd = &hand[nh]; hand_clear(nd);
                    nd->bet = bet; nd->from_split = 1;
                    hand_add(nd, h->card[1]);
                    h->n = 1; h->from_split = 1; nh++;
                    u8 x = shoe_draw(&s); hand_add(h, x);  count_card(&c, x);
                    u8 y = shoe_draw(&s); hand_add(nd, y); count_card(&c, y);
                    continue;
                }
                u8 nc = shoe_draw(&s); hand_add(h, nc); count_card(&c, nc);
                if (hand_total(h) > 21) { h->busted = 1; break; }
            }
        }
        count_card(&c, d2);
        int live = 0;
        for (int hi = 0; hi < nh; hi++)
            if (!hand[hi].busted && !hand[hi].surrendered) live = 1;
        if (live) {
            for (;;) {
                int t = hand_total(&dealer), soft = hand_is_soft(&dealer);
                if (t > 21 || t > 17) break;
                if (t == 17 && !(soft && v->h17)) break;
                u8 nc = shoe_draw(&s); hand_add(&dealer, nc); count_card(&c, nc);
            }
        }
        int dt = hand_total(&dealer);
        for (int hi = 0; hi < nh; hi++) {
            Hand *h = &hand[hi];
            int pt = hand_total(h);
            if (h->surrendered) r -= h->bet / 2;
            else if (h->busted) r -= h->bet;
            else if (dt > 21)   r += h->bet;
            else if (pt > dt)   r += h->bet;
            else if (pt < dt)   r -= h->bet;
        }
        net += r; hands++;
    }
    return net / hands / 100.0;      /* units won per hand */
}

int main(void)
{
    rng_seed(0xBADC0DE);
    test_values();
    test_tags();
    test_true_count();
    test_chart();
    test_deviations();

    puts("-- monte carlo (flat bets, perfect basic strategy)");
    long long rounds = 3000000;
    for (int vi = 0; vi < NVENUES; vi++) {
        const Venue *v = &VENUES[vi];
        rng_seed(0x1234567 + vi);
        Res r = simulate(v, rounds, 0);
        double edge = -100.0 * (double)r.net / (double)(r.hands * 100);
        int model = venue_edge_x100(v);
        printf("  %-18s simulated %6.3f%%   model %5.2f%%   %s\n",
               v->name, edge, model / 100.0,
               (edge - model / 100.0 < 0.30 && edge - model / 100.0 > -0.30) ? "ok" : "CHECK");
        if (!(edge - model / 100.0 < 0.35 && edge - model / 100.0 > -0.35)) fails++;
        checks++;
    }

    puts("-- decision EV probes (dealer has already peeked)");
    probe("16 vs ACE  S17", &VENUES[2], T_, CARD(5,1), A_);
    probe("16 vs ACE  H17", &VENUES[3], T_, CARD(5,1), A_);
    probe("16 vs TEN  S17", &VENUES[2], T_, CARD(5,1), T_);
    probe("15 vs TEN  S17", &VENUES[2], T_, CARD(4,1), T_);
    probe("15 vs ACE  S17", &VENUES[2], T_, CARD(4,1), A_);
    probe("17 vs ACE  H17", &VENUES[3], T_, CARD(6,1), A_);

    puts("-- counting with a bet ramp (this is the whole premise)");
    for (int vi = 0; vi < NVENUES; vi++) {
        const Venue *v = &VENUES[vi];
        rng_seed(777 + vi);
        double ev = counter_ev(v, 4000000, 12);
        printf("  %-18s counter EV %+.4f units/hand   %s\n", v->name, ev,
               ev > 0 ? "beatable" : "still a loser");
    }
    {
        rng_seed(31337);
        double ev = counter_ev(&VENUES[4], 4000000, 12);
        CHECK(ev > 0.002, "a counter spreading 1-12 beats the Skydeck game");
    }

    printf("\n%d checks, %d failures\n", checks, fails);
    return fails ? 1 : 0;
}
