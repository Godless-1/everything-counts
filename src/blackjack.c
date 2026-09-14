/* EVERYTHING COUNTS - a card counting story for the Game Boy Advance
 * Copyright (C) 2026 Godless-1
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.  See LICENSE for the full text.
 */
#include "blackjack.h"

void hand_clear(Hand *h)
{
    ec_memset(h, 0, sizeof(Hand));
}

void hand_add(Hand *h, u8 card)
{
    if (h->n < HAND_CARDS) h->card[h->n++] = card;
}

int hand_total(const Hand *h)
{
    int t = 0, aces = 0;
    for (int i = 0; i < h->n; i++) {
        int v = card_value(h->card[i]);
        if (v == 11) aces++;
        t += v;
    }
    while (t > 21 && aces) { t -= 10; aces--; }
    return t;
}

int hand_is_soft(const Hand *h)
{
    int t = 0, aces = 0;
    for (int i = 0; i < h->n; i++) {
        int v = card_value(h->card[i]);
        if (v == 11) aces++;
        t += v;
    }
    while (t > 21 && aces) { t -= 10; aces--; }
    return aces > 0 && t <= 21;
}

int hand_is_bj(const Hand *h)
{
    return h->n == 2 && !h->from_split && hand_total(h) == 21;
}

int hand_can_split(const Hand *h, int nhands, int max_hands, int rsa)
{
    if (h->n != 2 || nhands >= max_hands) return 0;
    int a = CARD_RANK(h->card[0]), b = CARD_RANK(h->card[1]);
    /* any two ten-valued cards may be split */
    int va = (a >= 9) ? 10 : (a == 0 ? 1 : a + 1);
    int vb = (b >= 9) ? 10 : (b == 0 ? 1 : b + 1);
    if (va != vb) return 0;
    if (va == 1 && h->from_split && !rsa) return 0;
    return 1;
}

int hand_can_double(const Hand *h, int das_ok)
{
    if (h->n != 2 || h->doubled) return 0;
    if (h->from_split && !das_ok) return 0;
    return 1;
}

int hand_can_surrender(const Hand *h)
{
    return h->n == 2 && !h->from_split && !h->doubled;
}
