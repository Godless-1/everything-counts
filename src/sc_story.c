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
 *  sc_story.c - narrative beats and the choices that stick to you.
 * ===================================================================== */
#include "story.h"
#include "table.h"
#include "teach.h"

static int g_choice_made;

void story_apply(int effect)
{
    switch (effect) {
    case EFF_MARA_SPARE:
        g_run.flag[FLAG_MARA_SPARED] = 1;
        break;
    case EFF_MARA_SELL:
        g_run.flag[FLAG_MARA_SOLD] = 1;
        g_run.creds += 120000;
        run_add_drift(9);
        break;
    case EFF_HOT_TAKE:
        g_run.flag[FLAG_HOT_CHROME] = 1;
        g_run.implant[IMP_MNEMONIC] = 1;
        run_add_wire(IMPLANTS[IMP_MNEMONIC].wire);
        run_add_drift(14);
        break;
    case EFF_HOT_REFUSE:
        break;
    case EFF_LAUNDER_YES:
        g_run.flag[FLAG_LAUNDERED] = 1;
        g_run.creds -= 350000;
        if (g_run.creds < 0) g_run.creds = 0;
        run_add_drift(7);
        break;
    case EFF_LAUNDER_NO:
        g_run.flag[FLAG_REFUSED_SABLE] = 1;
        break;
    case EFF_OZEROV_TAKE:
        g_run.flag[FLAG_OZEROV_TAKEN] = 1;
        g_run.creds += 620000;
        run_add_drift(13);
        break;
    case EFF_OZEROV_SPARE:
        g_run.flag[FLAG_OZEROV_SPARED] = 1;
        break;
    case EFF_SELL_MIND:
        g_run.flag[FLAG_SOLD_MIND] = 1;
        break;
    case EFF_REFUSE_HALCYON:
        g_run.flag[FLAG_REFUSED_HALCYON] = 1;
        break;
    default:
        break;
    }
}

/* ---------------------------------------------------------------------- */
static void beat_bg(const Beat *b)
{
    ui_backdrop(b->bg, g_t, b->ramp);
    ui_titlebar(b->title, 0, C_MAG);
    char n[16]; char *p = str_cat(n, "NIGHT ");
    str_int(p, g_run.night);
    if (g_run.night >= 1 && g_run.night <= NIGHTS)
        vid_text(236 - vid_text_len(n), 2, n, C_MID);
}

static void play_lines(const Beat *b, const SLine *lines, int n)
{
    u16 offs[8];
    for (int i = 0; i < n; i++) {
        int np = vid_wrap_pages(lines[i].text, 28, 4, offs, 8);
        for (int pg = 0; pg < np; pg++) {
            const char *txt = lines[i].text + offs[pg];
            int total = (pg + 1 < np) ? (int)(offs[pg + 1] - offs[pg])
                                      : (int)ec_strlen(txt);
            int chars = 0;
            key_flush();
            for (;;) {
                app_frame_begin();
                if (chars < total) {
                    chars += 2;
                    if ((chars & 7) == 0) sfx(SFX_MOVE);
                }
                if (key_is(KEY_A) || key_is(KEY_B) || key_is(KEY_START)) chars = total;
                if (chars > total) chars = total;
                if (key_down(KEY_A) && chars >= total) { sfx(SFX_SELECT); break; }

                beat_bg(b);
                ui_dialogue_box(lines[i].speaker, txt, chars, g_t);
                if (chars >= total) ui_hint(pg + 1 < np ? "\x13 MORE" : "\x13 CONTINUE");
                app_frame_end();
            }
        }
    }
}

static int choose(const Beat *b)
{
    int sel = 0;
    key_flush();
    for (;;) {
        app_frame_begin();
        if (key_repeat(KEY_DOWN)) { sel = (sel + 1) % b->nchoices; sfx(SFX_MOVE); }
        if (key_repeat(KEY_UP))   { sel = (sel + b->nchoices - 1) % b->nchoices; sfx(SFX_MOVE); }
        if (key_down(KEY_A)) { sfx(SFX_SELECT); return sel; }

        beat_bg(b);
        int h = 24 + b->nchoices * 26;
        ui_panel(12, 152 - h, 216, h - 6, C_MAG, "DECIDE");
        vid_text(18, 158 - h, b->prompt, C_WHITE);
        for (int i = 0; i < b->nchoices; i++) {
            int y = 170 - h + i * 26;
            u8 c = (i == sel) ? C_MAG : C_GRID;
            vid_rect(18, y, 204, 22, C_BLACK);
            vid_frame_rect(18, y, 204, 22, c);
            if (i == sel) {
                ui_brackets(16, y - 2, 208, 26, C_MAG, 4);
                vid_text(22, y + 2, "\x01", C_MAG);
            }
            vid_text(32, y + 2, b->choice[i], (i == sel) ? C_WHITE : C_MID);
            if (b->note[i]) vid_text(32, y + 12, b->note[i], (i == sel) ? C_CANDY : C_DIM);
        }
        ui_hint("\x03\x04 WEIGH IT   \x13 COMMIT");
        app_frame_end();
    }
}

int sc_story(int arg)
{
    int idx = arg;
    if (idx < 0) idx = 0;
    if (idx >= NBEATS) idx = NBEATS - 1;
    const Beat *b = &BEATS[idx];

    mus_play(idx >= 6 ? MUS_ASHES : MUS_NEON_RAIN);
    app_fade_in(14);
    play_lines(b, b->line, b->nlines);

    g_choice_made = 0;
    if (b->nchoices > 0) {
        int c = choose(b);
        g_choice_made = c;
        g_run.beat_done[idx] = 1;
        story_apply(b->effect[c]);
        if (b->after[c]) play_lines(b, b->after[c], 1);

        if (b->effect[c] == EFF_TUTORIAL) {
            g_run.flag[FLAG_SAW_TUTORIAL] = 1;
            app_fade_out(14);
            return SC_TEACH;
        }
        if (b->effect[c] == EFF_SELL_MIND) {
            app_fade_out(20);
            g_run.ending = 1;
            return SC_ENDING;
        }
        if (b->effect[c] == EFF_VAULT) {
            app_fade_out(14);
            sc_table(TABLE_ARG(5, TMODE_STORY_VAULT));
            return SC_NIGHT_END;
        }
    }
    g_run.beat_done[idx] = 1;
    app_fade_out(14);
    return SC_HUB;
}
