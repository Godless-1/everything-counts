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
 *  blackjack.h - rules engine.  Nothing here knows about pixels.
 * ===================================================================== */
#ifndef BLACKJACK_H
#define BLACKJACK_H
#include "gba.h"
#include "cards.h"

#define MAX_HANDS 4
#define HAND_CARDS 12

typedef struct {
    u8  card[HAND_CARDS];
    u8  n;
    s32 bet;
    u8  doubled;
    u8  stood;
    u8  busted;
    u8  surrendered;
    u8  from_split;
    u8  resolved;
    s8  result;        /* -1 loss, 0 push, +1 win, +2 blackjack */
    s32 payout;        /* net change to the bankroll */
} Hand;

typedef struct {
    Hand hand[MAX_HANDS];
    u8   nhands;
    u8   active;
    Hand dealer;
    u8   hole_shown;
    u8   ins_offered;
    u8   ins_taken;
    s32  ins_bet;
    s32  total_wagered;
} Table;

void hand_clear(Hand *h);
void hand_add(Hand *h, u8 card);
int  hand_total(const Hand *h);              /* best total <= 21 if possible */
int  hand_is_soft(const Hand *h);
int  hand_is_bj(const Hand *h);
int  hand_can_split(const Hand *h, int nhands, int max_hands, int rsa);
int  hand_can_double(const Hand *h, int das_ok);
int  hand_can_surrender(const Hand *h);

#endif
