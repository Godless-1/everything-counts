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
 *  cards.h - the deck.  Re-skinned for the Kestrel City tables:
 *     SPIKE (spades)  PULSE (hearts)  SHARD (diamonds)  WIRE (clubs)
 * ===================================================================== */
#ifndef CARDS_H
#define CARDS_H
#include "gba.h"

#define NO_CARD      0xFF
#define MAX_DECKS    8
#define SHOE_MAX     (MAX_DECKS * 52)

/* rank: 0=A, 1..8 = 2..9, 9 = 10, 10=J, 11=Q, 12=K   suit: 0..3 */
#define CARD(r,s)    ((u8)(((r) << 2) | (s)))
#define CARD_RANK(c) ((c) >> 2)
#define CARD_SUIT(c) ((c) & 3)

enum { SUIT_SPIKE, SUIT_PULSE, SUIT_SHARD, SUIT_WIRE };

int  card_value(u8 card);          /* ace counts 11 here */
int  card_tag(u8 card);            /* Hi-Lo: +1 / 0 / -1 */
const char *card_rank_name(int rank);
const char *suit_name(int suit);
u8   suit_colour(int suit);
u8   suit_ramp(int suit);

typedef struct {
    u8  cards[SHOE_MAX];
    u16 n;
    u16 pos;
    u16 cut;         /* index at which the cut card appears */
    u8  decks;
} Shoe;

void shoe_build(Shoe *s, int decks, int pen_percent);
u8   shoe_draw(Shoe *s);
int  shoe_exhausted(const Shoe *s);
int  shoe_cards_left(const Shoe *s);
int  shoe_decks_left_x4(const Shoe *s);   /* quarter-decks remaining */

/* ---- rendering ------------------------------------------------------- */
#define CARD_W 30
#define CARD_H 42

enum { TAG_HIDE, TAG_SHOW };

void card_draw(int x, int y, u8 card, int tagmode);
void card_draw_back(int x, int y);
void card_draw_mini(int x, int y, u8 card);   /* 12x16 compact */
void card_draw_slot(int x, int y);            /* empty outline */
void suit_pip(int x, int y, int suit, int scale, u8 hi, u8 mid, u8 lo);

#endif
