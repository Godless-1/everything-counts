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
 *  teach_data.c - DRY RUN course content.
 *
 *  Assumes the player has never held a playing card.  Module 1 teaches
 *  blackjack from "what is a card worth".  Module 2 teaches basic
 *  strategy.  Module 3 teaches the count.
 * ===================================================================== */
#include "teach.h"

#define SP SUIT_SPIKE
#define PU SUIT_PULSE
#define SH SUIT_SHARD
#define WI SUIT_WIRE
#define C_(r,s) CARD(r,s)
#define R_A 0
#define R_2 1
#define R_3 2
#define R_4 3
#define R_5 4
#define R_6 5
#define R_7 6
#define R_8 7
#define R_9 8
#define R_T 9
#define R_J 10
#define R_Q 11
#define R_K 12
#define END NO_CARD

/* ---------------------------------------------------------------------- */
/*  MODULE 1 - THE GAME                                                    */
/* ---------------------------------------------------------------------- */

static const u8 d_numbers[] = { C_(R_2,SP), C_(R_5,PU), C_(R_7,SH), C_(R_9,WI), END };
static const u8 d_tens[]    = { C_(R_T,SP), C_(R_J,PU), C_(R_Q,SH), C_(R_K,WI), END };
static const u8 d_aces[]    = { C_(R_A,SP), C_(R_A,PU), END };
static const u8 d_soft17[]  = { C_(R_A,PU), C_(R_6,SH), END };
static const u8 d_hard17[]  = { C_(R_T,SP), C_(R_7,WI), END };
static const u8 d_softbust[]= { C_(R_A,PU), C_(R_6,SH), C_(R_9,SP), END };
static const u8 d_bj[]      = { C_(R_A,SP), C_(R_K,PU), END };
static const u8 d_16[]      = { C_(R_T,SH), C_(R_6,WI), END };
static const u8 d_11[]      = { C_(R_6,SP), C_(R_5,PU), END };
static const u8 d_88[]      = { C_(R_8,SP), C_(R_8,PU), END };
static const u8 d_up6[]     = { C_(R_6,WI), END };
static const u8 d_upT[]     = { C_(R_Q,SP), END };
static const u8 d_upA[]     = { C_(R_A,WI), END };
static const u8 d_up5[]     = { C_(R_5,SH), END };
static const u8 d_up9[]     = { C_(R_9,PU), END };
static const u8 d_up3[]     = { C_(R_3,SP), END };
static const u8 d_12[]      = { C_(R_T,SP), C_(R_2,PU), END };
static const u8 d_a7[]      = { C_(R_A,SP), C_(R_7,SH), END };
static const u8 d_99[]      = { C_(R_9,SP), C_(R_9,WI), END };

static const Page m1[] = {
{ PT_SPEAK, "DRY RUN", 
  "You lost the lace, so you lost the shortcut. Fine. I am going to teach you the game the way people learned it before anybody had chrome in their skull. Slowly, and from nothing.",
  0,0,{0},0,0,0, CH_GREY, 0 },

{ PT_TEXT, "ONE RULE FIRST",
  "Blackjack is one number against another number.\n\nYou build a total. The dealer builds a total. Whoever is closer to 21 without going over wins.\n\nGo over 21 and you lose immediately. That is called a BUST, and it is the only reason the house makes money.",
  0,0,{0},0,0,0,0,0 },

{ PT_CARDS, "NUMBER CARDS",
  "A card is worth the number printed on it. Nothing else about it matters.\n\nThe four suits - SPIKE, PULSE, SHARD, WIRE - are decoration. They change nothing.",
  d_numbers,0,{0},0,0,0,0,0 },

{ PT_CARDS, "THE PICTURE CARDS",
  "10, JACK, QUEEN and KING are all worth exactly 10.\n\nThat means sixteen cards in every 52 are worth ten. Remember that. Later it is the whole game.",
  d_tens,0,{0},0,0,0,0,0 },

{ PT_CARDS, "THE ACE IS TWO CARDS",
  "An ACE is worth 11, or 1, whichever is better for you. It changes value by itself, hand by hand.\n\nYou never have to decide. It is always counted the way that helps you.",
  d_aces,0,{0},0,0,0,0,0 },

{ PT_CARDS, "SOFT HANDS",
  "ACE plus SIX is SOFT 17: the ace is counted as 11.\n\nIt is called soft because it cannot bust. Take another card and if you go over, the ace quietly drops to 1 and you are still alive.",
  d_soft17,0,{0},0,0,0,0,0 },

{ PT_CARDS, "HARD HANDS",
  "TEN plus SEVEN is HARD 17. There is no ace to fall back on.\n\nDraw to it and any card above a four kills you. Same number, completely different hand - which is why the game keeps them apart.",
  d_hard17,0,{0},0,0,0,0,0 },

{ PT_CARDS, "WHEN SOFT GOES HARD",
  "SOFT 17 plus a NINE would be 26 with the ace at 11 - a bust. So the ace becomes 1 and the hand is 16 instead.\n\nNow it is a HARD 16. One more big card kills it.",
  d_softbust,0,{0},0,0,0,0,0 },

{ PT_QUIZ, "CHECK",
  "You are holding an ACE and a THREE. What is this hand worth?",
  0,0,{ "4", "SOFT 14", "13", "21" }, 4, 1,
  "The ace counts as 11, so 11 + 3 = 14, and it is SOFT because the ace can still fall back to 1.",
  0,0 },

{ PT_CARDS, "BLACKJACK",
  "An ACE with any ten-value card, in your first two cards, is a BLACKJACK. 21 on the deal.\n\nIt beats any other 21 and it pays extra - 3 to 2 instead of even money. Bet 100, win 150.",
  d_bj,0,{0},0,0,0,0,0 },

{ PT_TEXT, "WHAT A TABLE LOOKS LIKE",
  "You place a bet. Then:\n\n- You get two cards, both face up.\n- The dealer gets one card face up. That is the UP CARD, and it is the only thing you know about them.\n- The dealer gets one card face down. That is the HOLE CARD.\n\nYou act first. Always.",
  0,0,{0},0,0,0,0,0 },

{ PT_HAND, "YOUR TURN",
  "Here you hold 16 and the dealer shows a SIX.\n\nYour choices are HIT (take another card) or STAND (stop here). Everything else in the game is a variation on those two.",
  d_16, d_up6, {0},0,0,0,0,0 },

{ PT_QUIZ, "CHECK",
  "You hold 16. You HIT and draw a TEN. What happened?",
  0,0,{ "You have 26 and you BUST", "The ten becomes a 1", "You have 21", "Nothing, 16 is safe" }, 4, 0,
  "16 + 10 = 26. Only aces change value. You busted and you lost the bet the moment it happened - before the dealer even plays.",
  0,0 },

{ PT_TEXT, "THE DEALER IS A MACHINE",
  "The dealer makes no decisions. Ever.\n\nThey turn over the hole card and then follow one rule: draw while the total is 16 or less, stop at 17 or more.\n\nSome houses make the dealer HIT a soft 17 as well. That single rule costs you about a fifth of a percent. Always check.",
  0,0,{0},0,0,0,0,0 },

{ PT_QUIZ, "CHECK",
  "The dealer turns over 12. You are standing on 18. What will the dealer do?",
  0,0,{ "Stop, because 18 beats 12", "Draw until 17 or more", "Draw exactly one card", "Draw until they beat 18" }, 4, 1,
  "The dealer has no opinion about your hand. They draw to 16 and stand on 17 - even when standing on 17 loses to your 18.",
  0,0 },

{ PT_TEXT, "WHAT YOU GET PAID",
  "WIN     paid 1 to 1. Bet 100, get your 100 back plus 100.\nPUSH    a tie. Your bet comes back. Nothing lost.\nLOSE    the bet is gone.\nBLACKJACK  paid 3 to 2.\n\nIf a house pays 6 to 5 on blackjack, that one change costs you 1.4% - more than every other rule combined. Walk out.",
  0,0,{0},0,0,0,0,0 },

{ PT_HAND, "DOUBLE DOWN",
  "With 11 against a FIVE you are in the best spot in the game: almost any card gives you 20 or 21, and the dealer's five is the weakest card they can show.\n\nDOUBLE puts a second bet down. You get exactly ONE more card, then you must stop.",
  d_11, d_up5, {0},0,0,0,0,0 },

{ PT_HAND, "SPLITTING A PAIR",
  "Two cards of the same value can be SPLIT into two separate hands. You put up a second bet to cover the second hand.\n\n16 is the worst hand in blackjack. Two EIGHTS are 16. Split them and you are playing two hands that each start with 8 - far better.",
  d_88, d_upT, {0},0,0,0,0,0 },

{ PT_QUIZ, "CHECK",
  "Why split a pair of EIGHTS against a TEN, when both new hands will probably lose?",
  0,0,{ "Because 21 is more likely", "Because two bad hands lose less than one terrible one", "Because the dealer must then stand", "You should never split eights" }, 4, 1,
  "Hard 16 is close to hopeless. Two hands starting at 8 each lose less money on average. Basic strategy is about losing less, not only about winning.",
  0,0 },

{ PT_TEXT, "SURRENDER",
  "Some houses let you FOLD a hand before you draw: you give up half the bet and walk away from the other half.\n\nIt looks like cowardice. It is arithmetic. If a hand loses more than 75% of the time, throwing half away is cheaper than playing it.",
  0,0,{0},0,0,0,0,0 },

{ PT_HAND, "INSURANCE",
  "When the dealer shows an ACE they offer INSURANCE - a side bet, half your stake, that pays 2 to 1 if their hole card is a ten.\n\nThere are 16 ten-value cards in 52. The bet needs better than one in three to break even. It does not get there. Insurance loses money.",
  d_16, d_upA, {0},0,0,0,0,0 },

{ PT_SPEAK, "DRY RUN",
  "Refuse insurance. Every time. Until the day you can actually see that the shoe is stuffed with tens - and then take it, and take it hard. That day is what the rest of this course is for.",
  0,0,{0},0,0,0, CH_GREY, 0 },

{ PT_PRACTICE, "LIVE HANDS",
  "That is the whole game. Sit down and play some hands with the coach switched on - it will tell you the right move and grade you after every decision.\n\nNo money is at risk in the sim.",
  0,0,{0},0,0,0,0,0 },
};

/* ---------------------------------------------------------------------- */
/*  MODULE 2 - BASIC STRATEGY                                              */
/* ---------------------------------------------------------------------- */

static const Page m2[] = {
{ PT_SPEAK, "BASIC STRATEGY",
  "Every hand you can be dealt has exactly one best move. Not a feeling. A number, computed a long time ago and never overturned. Learn the chart and the house edge drops from about two percent to half of one.",
  0,0,{0},0,0,0, CH_GREY, 0 },

{ PT_TEXT, "WHY THERE IS A RIGHT ANSWER",
  "You know your two cards and the dealer's up card. That is enough to compute which choice loses the least money over thousands of hands.\n\nBASIC STRATEGY is that computation, written out. It is not a system for winning. It is the floor you stand on before you start counting.",
  0,0,{0},0,0,0,0,0 },

{ PT_TEXT, "THE DEALER'S WEAK CARDS",
  "A dealer showing 2 through 6 is in trouble: they must keep drawing, and they bust roughly 40% of the time on a five or six.\n\nA dealer showing 7 through ACE is strong.\n\nAlmost every rule in the chart comes from that one split.",
  0,0,{0},0,0,0,0,0 },

{ PT_HAND, "STIFF HANDS",
  "12 through 16 are STIFF: too low to win by standing, too high to hit safely.\n\nAgainst a weak dealer card you stand and let them bust. Against a strong one you hit and accept the risk. Here: 12 against a THREE - stand and wait.",
  d_12, d_up3, {0},0,0,0,0,0 },

{ PT_HAND, "SOFT HANDS PLAY DIFFERENTLY",
  "SOFT 18 (ace-seven) looks finished. It is not.\n\nAgainst 2-6 you double it. Against 7 or 8 you stand. Against 9, 10 or ACE you HIT an 18 - because soft 18 loses to the hands those cards make, and you cannot bust.",
  d_a7, d_up9, {0},0,0,0,0,0 },

{ PT_QUIZ, "CHECK",
  "You hold SOFT 18 against a NINE. The chart says HIT. Why is that not insane?",
  0,0,{ "The ace protects you from busting", "18 always loses", "The dealer must stand on 18", "It is insane, the chart is wrong" }, 4, 0,
  "You cannot bust a soft hand on one card. 18 is a loser against a nine, so you take a free swing at improving it.",
  0,0 },

{ PT_HAND, "PAIRS",
  "Two NINES against a NINE: split. Two nines is 18, which loses to the 19 a nine tends to make. Two hands starting at 9 do better.\n\nNever split TENS - 20 is already a winning hand. Never split FIVES - 10 is a doubling hand, not two weak fives.",
  d_99, d_up9, {0},0,0,0,0,0 },

{ PT_CHART, "THE CHART",
  "This is the whole thing. Rows are your hand, columns are the dealer's up card.\n\nMove around it. Read it. You do not need to memorise it tonight - the drills will do that to you.",
  0,0,{0},0,0,0,0,0 },

{ PT_QUIZ, "CHECK",
  "Hard 16 against a TEN, no surrender offered. What does basic strategy say?",
  0,0,{ "STAND", "HIT", "DOUBLE", "SPLIT" }, 4, 1,
  "Both options are bad. Hitting is slightly less bad. When surrender is available you fold instead - that is cheaper still.",
  0,0 },

{ PT_PRACTICE, "DRILL IT",
  "Play until the grade line says CORRECT without you thinking about it.\n\nBasic strategy has to be automatic before the count can be useful. If you are spending attention on whether to hit, you are not counting.",
  0,0,{0},0,0,0,0,0 },
};

/* ---------------------------------------------------------------------- */
/*  MODULE 3 - THE COUNT                                                   */
/* ---------------------------------------------------------------------- */

static const u8 d_low[]  = { C_(R_2,SP), C_(R_3,PU), C_(R_4,SH), C_(R_5,WI), C_(R_6,SP), END };
static const u8 d_mid[]  = { C_(R_7,SP), C_(R_8,PU), C_(R_9,SH), END };
static const u8 d_high[] = { C_(R_T,SP), C_(R_J,PU), C_(R_Q,SH), C_(R_K,WI), C_(R_A,SP), END };
static const u8 d_run1[] = { C_(R_5,SP), C_(R_K,PU), C_(R_3,SH), C_(R_8,WI),
                             C_(R_2,SP), C_(R_A,PU), C_(R_6,SH), C_(R_T,WI), END };

static const Page m3[] = {
{ PT_SPEAK, "THE COUNT",
  "Here is the secret, and it is not a secret. Cards do not come back. Every card dealt is one less card in the shoe. If you know what is left, you know something the house does not think you know.",
  0,0,{0},0,0,0, CH_GREY, 0 },

{ PT_TEXT, "WHY IT WORKS AT ALL",
  "A shoe rich in TENS and ACES is good for you. Three reasons:\n\n- You get more blackjacks, and blackjack pays 3 to 2 while the dealer's only pays even.\n- Your doubles land more tens.\n- The dealer, forced to draw on 12-16, busts more often.\n\nA shoe rich in small cards is good for the house.",
  0,0,{0},0,0,0,0,0 },

{ PT_CARDS, "LOW CARDS: PLUS ONE",
  "2, 3, 4, 5, 6.\n\nWhen one of these is dealt, the shoe just got better for you - a small card is gone. Add ONE to your count.",
  d_low,0,{0},0,0,0,0,0 },

{ PT_CARDS, "MIDDLE CARDS: ZERO",
  "7, 8, 9.\n\nThey barely move the needle either way. Count NOTHING. Look at them and move on.",
  d_mid,0,{0},0,0,0,0,0 },

{ PT_CARDS, "HIGH CARDS: MINUS ONE",
  "10, JACK, QUEEN, KING, ACE.\n\nA big card just left the shoe, which is bad for you. Subtract ONE.",
  d_high,0,{0},0,0,0,0,0 },

{ PT_TAGS, "THE WHOLE SYSTEM",
  "That is Hi-Lo. Three values, thirteen cards, no memory required.\n\nYou are not remembering cards. You are holding one small number and nudging it up or down as the felt fills.",
  0,0,{0},0,0,0,0,0 },

{ PT_COUNTDEMO, "RUNNING COUNT",
  "Watch. Each card moves the number. Start at zero every time a fresh shoe is shuffled.",
  d_run1,0,{0},0,0,0,0,0 },

{ PT_QUIZ, "CHECK",
  "The cards dealt were 5, K, 3, 8, 2, A, 6, 10. What is the running count?",
  0,0,{ "+4", "+1", "0", "-2" }, 4, 1,
  "5(+1) K(-1) 3(+1) 8(0) 2(+1) A(-1) 6(+1) 10(-1) = +1. Cancel pairs as they land and it gets easy fast.",
  0,0 },

{ PT_TEXT, "THE TRUE COUNT",
  "A running count of +6 means very different things with one deck left versus six decks left.\n\nTRUE COUNT = RUNNING COUNT divided by DECKS REMAINING.\n\n+6 with 2 decks left is a true count of +3. +6 with 6 decks left is only +1. Estimate the decks left from the discard tray. Nearest half deck is close enough.",
  0,0,{0},0,0,0,0,0 },

{ PT_QUIZ, "CHECK",
  "Running count +8, about 4 decks still in the shoe. True count?",
  0,0,{ "+8", "+4", "+2", "+1" }, 4, 2,
  "8 divided by 4 is 2. The true count is what every decision actually uses. The running count on its own tells you very little.",
  0,0 },

{ PT_TEXT, "BETTING THE COUNT",
  "Counting earns nothing unless your money moves with it.\n\nA workable ramp: bet (TRUE COUNT minus 1) units, minimum one unit.\n\nTC 0 or less: 1 unit. TC +2: 1 unit. TC +3: 2 units. TC +5: 4 units.\n\nSmall when the shoe is dead. Large when it is rich. That difference is your entire wage.",
  0,0,{0},0,0,0,0,0 },

{ PT_TEXT, "THE PART THAT GETS YOU CAUGHT",
  "Pit bosses do not catch counters by counting. They catch them by watching bet sizes.\n\nA player who bets one unit for ten hands and then suddenly slides out eight units is advertising. Move in steps. Keep some bets wrong on purpose. Lose a little cover money.\n\nOn this floor that is the HEAT bar. Fill it and you are done for the night.",
  0,0,{0},0,0,0,0,0 },

{ PT_TEXT, "INDEX PLAYS",
  "At extreme counts, basic strategy stops being correct.\n\nThe big one: take INSURANCE at true count +3 or higher. That single deviation is worth more than most of the others put together.\n\nThe next: STAND on 16 against a TEN at true count 0 or better.\n\nThe game will flag these as INDEX when you have the chrome for it.",
  0,0,{0},0,0,0,0,0 },

{ PT_QUIZ, "CHECK",
  "The dealer shows an ACE. True count is +4. Take insurance?",
  0,0,{ "No, insurance is always bad", "Yes", "Only with a blackjack", "Only if you are losing" }, 4, 1,
  "Insurance is a bet on tens. At true count +3 and above the shoe holds enough tens to make it profitable. This is the single most valuable deviation in the game.",
  0,0 },

{ PT_SPEAK, "THE COUNT",
  "One number. In your head. Never on a screen, never on chrome the pit can scan. The second a machine does it for you, it is not yours and it is not deniable.",
  0,0,{0},0,0,0, CH_GREY, 0 },

{ PT_PRACTICE, "PUT IT TOGETHER",
  "Live hands, coach on, counter on. Play until the pulse checks stop frightening you.\n\nThen switch the counter off and do it again.",
  0,0,{0},0,0,0,0,0 },
};

const Lesson LESSONS[] = {
    { "THE GAME",       "MOD 1", m1, (u8)(sizeof(m1)/sizeof(Page)), 1 },
    { "BASIC STRATEGY", "MOD 2", m2, (u8)(sizeof(m2)/sizeof(Page)), 2 },
    { "THE COUNT",      "MOD 3", m3, (u8)(sizeof(m3)/sizeof(Page)), 3 },
};
const int NLESSONS = (int)(sizeof(LESSONS) / sizeof(Lesson));
