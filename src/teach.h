/* EVERYTHING COUNTS - a card counting story for the Game Boy Advance
 * Copyright (C) 2026 Godless-1
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.  See LICENSE for the full text.
 */
#ifndef TEACH_H
#define TEACH_H
#include "app.h"
#include "cards.h"

enum {
    PT_TEXT,        /* heading + prose                                   */
    PT_CARDS,       /* a row of cards, each captioned with its value     */
    PT_HAND,        /* your hand against a dealer up-card                */
    PT_QUIZ,        /* multiple choice with an explanation               */
    PT_CHART,       /* the interactive basic-strategy chart              */
    PT_TAGS,        /* the Hi-Lo tag table                               */
    PT_COUNTDEMO,   /* cards revealed one at a time, count accumulating  */
    PT_SPEAK,       /* a character says something                        */
    PT_PRACTICE     /* hands the player a live table                     */
};

typedef struct {
    u8 kind;
    const char *head;
    const char *body;
    const u8 *cards;          /* NO_CARD terminated */
    const u8 *dealer;         /* NO_CARD terminated */
    const char *opt[4];
    u8 nopt;
    u8 correct;
    const char *why;
    u8 speaker;
    u8 arg;
} Page;

typedef struct {
    const char *name;
    const char *code;
    const Page *page;
    u8 npages;
    u8 module;
} Lesson;

extern const Lesson LESSONS[];
extern const int NLESSONS;

int teach_run_lesson(int index);   /* returns 1 if completed */
void teach_chart_browser(void);
void teach_tag_table(int x, int y);

#endif
