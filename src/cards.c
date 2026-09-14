/* EVERYTHING COUNTS - a card counting story for the Game Boy Advance
 * Copyright (C) 2026 Godless-1
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.  See LICENSE for the full text.
 */
#include "cards.h"
#include "video.h"
#include "rng.h"

static const char *rank_names[13] = {
    "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"
};
static const char *suit_names[4] = { "SPIKE", "PULSE", "SHARD", "WIRE" };
static const u8 suit_col[4]  = { C_CYAN,  C_MAG,  C_AMBER, C_GREEN };
static const u8 suit_rmp[4]  = { RAMP_CYAN, RAMP_MAG, RAMP_GOLD, RAMP_GREEN };

int card_value(u8 card)
{
    int r = CARD_RANK(card);
    if (r == 0) return 11;
    if (r <= 8) return r + 1;
    return 10;
}

int card_tag(u8 card)
{
    int r = CARD_RANK(card);
    if (r == 0)  return -1;          /* ace  */
    if (r <= 5)  return  1;          /* 2-6  */
    if (r <= 8)  return  0;          /* 7-9  */
    return -1;                       /* 10 J Q K */
}

const char *card_rank_name(int rank) { return rank_names[rank & 15]; }
const char *suit_name(int suit)      { return suit_names[suit & 3]; }
u8 suit_colour(int suit)             { return suit_col[suit & 3]; }
u8 suit_ramp(int suit)               { return suit_rmp[suit & 3]; }

/* ---------------------------------------------------------------------- */
void shoe_build(Shoe *s, int decks, int pen_percent)
{
    if (decks < 1) decks = 1;
    if (decks > MAX_DECKS) decks = MAX_DECKS;
    s->decks = (u8)decks;
    s->n = (u16)(decks * 52);
    int k = 0;
    for (int d = 0; d < decks; d++)
        for (int r = 0; r < 13; r++)
            for (int su = 0; su < 4; su++)
                s->cards[k++] = CARD(r, su);
    /* Fisher-Yates */
    for (int i = s->n - 1; i > 0; i--) {
        int j = (int)rng_below((u32)(i + 1));
        u8 t = s->cards[i]; s->cards[i] = s->cards[j]; s->cards[j] = t;
    }
    s->pos = 0;
    s->cut = (u16)((s->n * pen_percent) / 100);
    if (s->cut > s->n - 4) s->cut = s->n - 4;
}

u8 shoe_draw(Shoe *s)
{
    if (s->pos >= s->n) return NO_CARD;
    return s->cards[s->pos++];
}

int shoe_exhausted(const Shoe *s)   { return s->pos >= s->cut; }
int shoe_cards_left(const Shoe *s)  { return s->n - s->pos; }

/* quarter-decks left, rounded to nearest quarter - what a counter estimates */
int shoe_decks_left_x4(const Shoe *s)
{
    int left = shoe_cards_left(s);
    int q = (left * 4 + 26) / 52;
    if (q < 1) q = 1;
    return q;
}

/* ====================================================================== */
/*  card art                                                              */
/* ====================================================================== */

void suit_pip(int x, int y, int suit, int scale, u8 hi, u8 mid, u8 lo)
{
    const u8 *src;
    switch (suit & 3) {
        case SUIT_SPIKE: src = pip_spike; break;
        case SUIT_PULSE: src = pip_pulse; break;
        case SUIT_SHARD: src = pip_shard; break;
        default:         src = pip_wire;  break;
    }
    u8 lut[4] = { 0, lo, mid, hi };
    if (scale <= 1) vid_blit4(x, y, 9, 9, src, lut);
    else            vid_blit4_scaled(x, y, 9, 9, src, lut, scale);
}

/* angular corner bevels so the cards read as etched polycarbonate */
static void bevel(int x, int y, u8 edge, u8 c)
{
    vid_plot(x,              y,              c);
    vid_plot(x + CARD_W - 1, y,              c);
    vid_plot(x,              y + CARD_H - 1, c);
    /* bottom right: a clipped corner, lit along the cut */
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4 - i; j++)
            vid_plot(x + CARD_W - 1 - j, y + CARD_H - 1 - i, c);
    for (int i = 0; i < 4; i++)
        vid_plot(x + CARD_W - 1 - i, y + CARD_H - 4 + i, edge);
}

void card_draw(int x, int y, u8 card, int tagmode)
{
    if (card == NO_CARD) { card_draw_slot(x, y); return; }
    int r  = CARD_RANK(card);
    int su = CARD_SUIT(card);
    u8  sc = suit_col[su];
    u8  rp = suit_rmp[su];

    /* body: cold slate gradient, darker toward the foot */
    vid_vgrad(x + 1, y + 1, CARD_W - 2, CARD_H - 2, RAMP_CARD, 11, 2);

    /* header band behind the rank */
    vid_hgrad(x + 4, y + 1, CARD_W - 5, 11, rp, 5, 0);
    vid_hline(x + 4, y + 12, CARD_W - 5, rp + 10);

    /* etched circuit traces */
    vid_vline(x + CARD_W - 5, y + 15, 8, RAMP_CARD + 14);
    vid_plot(x + CARD_W - 5, y + 14, rp + 15);
    vid_plot(x + CARD_W - 5, y + 23, rp + 15);

    /* holographic edge strip */
    vid_vgrad(x + 1, y + 1, 2, CARD_H - 2, rp, 18, 4);
    vid_vline(x + 3, y + 1, CARD_H - 2, RAMP_CARD + 14);

    /* border */
    vid_frame_rect(x, y, CARD_W, CARD_H, sc);

    /* centre emblem */
    if (r == 0) {
        /* the Ace is the house's own eye */
        u8 lut[4] = { 0, rp + 7, rp + 14, rp + 22 };
        vid_blit4(x + 5, y + 14, SIGIL_HALCYON_W, SIGIL_HALCYON_H, sigil_halcyon, lut);
    } else if (r >= 10) {
        /* face cards: chromed servo brackets clamped around the pip */
        suit_pip(x + 11, y + 17, su, 1, sc, rp + 15, rp + 8);
        vid_hline(x +  8, y + 15, 15, RAMP_CHROME + 16);
        vid_hline(x +  8, y + 28, 15, RAMP_CHROME + 16);
        vid_vline(x +  8, y + 15, 4, RAMP_CHROME + 16);
        vid_vline(x + 22, y + 15, 4, RAMP_CHROME + 16);
        vid_vline(x +  8, y + 25, 4, RAMP_CHROME + 16);
        vid_vline(x + 22, y + 25, 4, RAMP_CHROME + 16);
        vid_plot(x +  8, y + 21, sc);
        vid_plot(x + 22, y + 21, sc);
    } else {
        suit_pip(x + 7, y + 13, su, 2, sc, rp + 15, rp + 7);
    }

    /* rank: top-left, echoed bottom-left clear of the cut corner */
    const char *rn = rank_names[r];
    vid_text(x + 5, y + 2, rn, C_WHITE);
    vid_text(x + 4, y + 1, rn, sc);
    vid_text(x + 5, y + CARD_H - 10, rn, sc);
    /* suit tick, top right */
    vid_rect(x + CARD_W - 8, y + 4, 3, 3, sc);
    vid_plot(x + CARD_W - 9, y + 5, rp + 12);
    vid_plot(x + CARD_W - 5, y + 5, rp + 12);

    /* hi-lo tag badge - only lit when something is doing the counting */
    if (tagmode == TAG_SHOW) {
        int t = card_tag(card);
        u8 tc = (t > 0) ? C_GREEN : (t < 0 ? C_BLOOD : C_ICE);
        const char *tg = (t > 0) ? "+" : (t < 0 ? "-" : "0");
        vid_rect(x + 18, y + CARD_H - 12, 8, 9, C_BLACK);
        vid_frame_rect(x + 18, y + CARD_H - 12, 8, 9, tc);
        vid_text(x + 19, y + CARD_H - 11, tg, tc);
    }

    bevel(x, y, sc, C_BLACK);
}

void card_draw_back(int x, int y)
{
    vid_vgrad(x + 1, y + 1, CARD_W - 2, CARD_H - 2, RAMP_VIOLET, 3, 9);
    /* circuit lattice */
    for (int i = 4; i < CARD_W - 3; i += 4) vid_vline(x + i, y + 3, CARD_H - 6, RAMP_VIOLET + 13);
    for (int j = 4; j < CARD_H - 3; j += 5) vid_hline(x + 3, y + j, CARD_W - 6, RAMP_VIOLET + 11);
    for (int i = 4; i < CARD_W - 3; i += 8)
        for (int j = 4; j < CARD_H - 3; j += 10) vid_plot(x + i, y + j, RAMP_VIOLET + 19);
    /* house sigil */
    u8 lut[4] = { 0, RAMP_MAG + 5, RAMP_MAG + 12, RAMP_MAG + 20 };
    vid_blit4(x + 5, y + 13, SIGIL_HALCYON_W, SIGIL_HALCYON_H, sigil_halcyon, lut);
    vid_frame_rect(x, y, CARD_W, CARD_H, C_DVIOLET);
    vid_hline(x + 1, y + 1, CARD_W - 2, RAMP_VIOLET + 16);
    bevel(x, y, C_PURPLE, C_BLACK);
}

void card_draw_slot(int x, int y)
{
    for (int j = 0; j < CARD_H; j += 3) {
        vid_plot(x, y + j, C_GRID);
        vid_plot(x + CARD_W - 1, y + j, C_GRID);
    }
    for (int i = 0; i < CARD_W; i += 3) {
        vid_plot(x + i, y, C_GRID);
        vid_plot(x + i, y + CARD_H - 1, C_GRID);
    }
}

/* 14x18 compact card used for split hands and history strips */
void card_draw_mini(int x, int y, u8 card)
{
    if (card == NO_CARD) return;
    int r = CARD_RANK(card), su = CARD_SUIT(card);
    u8 sc = suit_col[su], rp = suit_rmp[su];
    vid_rect(x + 1, y + 1, 12, 16, RAMP_CARD + 4);
    vid_vline(x + 1, y + 1, 16, rp + 12);
    vid_frame_rect(x, y, 14, 18, sc);
    const char *rn = rank_names[r];
    if (r == 9) vid_text(x + 2, y + 2, "1", sc), vid_text(x + 6, y + 2, "0", sc);
    else        vid_text(x + 4, y + 2, rn, sc);
    suit_pip(x + 3, y + 9, su, 1, sc, rp + 12, rp + 6);
    vid_plot(x, y, C_BLACK);
    vid_plot(x + 13, y, C_BLACK);
    vid_plot(x, y + 17, C_BLACK);
    vid_plot(x + 13, y + 17, C_BLACK);
}
