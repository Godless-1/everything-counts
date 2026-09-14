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
 *  sc_menus.c - title, hub, venue select, ripperdoc, night end, endings
 * ===================================================================== */
#include "app.h"
#include "table.h"
#include "story.h"
#include "save.h"
#include "teach.h"

/* ====================================================================== */
/*  TITLE                                                                  */
/* ====================================================================== */
int sc_title(int arg)
{
    (void)arg;
    int sel = 0;
    int has_save = save_exists();
    if (!has_save) sel = 0;
    mus_play(MUS_NEON_RAIN);
    key_flush();
    app_fade_in(16);

    for (;;) {
        app_frame_begin();
        MenuItem m[5] = {
            { "NEW RUN",  0, 1, C_MAG },
            { "CONTINUE", 0, (u8)has_save, C_CYAN },
            { "DRY RUN",  0, 1, C_GREEN },
            { "DRILLS",   0, 1, C_AMBER },
            { "CODEX",    0, 1, C_ICE },
        };
        if (key_repeat(KEY_DOWN)) { do { sel = (sel + 1) % 5; } while (!m[sel].enabled); sfx(SFX_MOVE); }
        if (key_repeat(KEY_UP))   { do { sel = (sel + 4) % 5; } while (!m[sel].enabled); sfx(SFX_MOVE); }
        if (key_down(KEY_A)) {
            sfx(SFX_SELECT);
            switch (sel) {
                case 0:
                    if (!has_save || confirm_box("START A NEW RUN? THE SAVED ONE IS OVERWRITTEN.",
                                                 "NEW RUN", "CANCEL", C_MAG, BG_RAIN, RAMP_MAG)) {
                        run_reset();
                        save_write();
                        app_fade_out(16);
                        return SC_STORY;
                    }
                    key_flush();
                    break;
                case 1: save_read(); app_fade_out(16); return SC_HUB;
                case 2: app_fade_out(12); return SC_TEACH;
                case 3: app_fade_out(12); return SC_DRILL;
                case 4: app_fade_out(12); return SC_CODEX;
            }
        }

        ui_backdrop(BG_RAIN, g_t, RAMP_MAG);

        /* logo band */
        vid_rect(0, 28, 240, 48, C_BLACK);
        vid_hline(0, 28, 240, C_MAG);
        vid_hline(0, 29, 240, C_MAGD);
        vid_hline(0, 75, 240, C_CYAN);
        vid_hline(0, 74, 240, C_CYAND);
        vid_text_center_s(120, 32, "EVERYTHING", C_WHITE, 3);
        {
            int gl = (int)((g_t / 3) % 47);
            vid_text_center_s(118, 55, "COUNTS", (gl < 3) ? C_WHITE : C_MAG, 2);
            vid_text_center_s(122, 55, "COUNTS", C_CYAN, 2);
            vid_text_center_s(120, 54, "COUNTS", C_WHITE, 2);
        }
        {
            u8 lut[4] = { 0, RAMP_CYAN + 8, RAMP_CYAN + 15, C_WHITE };
            vid_blit4(10, 36, SIGIL_KESTREL_W, SIGIL_KESTREL_H, sigil_kestrel, lut);
            vid_blit4(214, 36, SIGIL_KESTREL_W, SIGIL_KESTREL_H, sigil_kestrel, lut);
        }

        /* strapline */
        vid_rect(0, 78, 240, 11, C_BLACK);
        vid_text_center(120, 80, "A CARD COUNTING STORY", C_CANDY);

        /* menu */
        vid_rect(62, 94, 116, 54, C_BLACK);
        vid_rect_dither(63, 95, 114, 52, C_BLACK, C_INDIGO);
        vid_frame_rect(62, 94, 116, 54, C_CYAND);
        ui_brackets(61, 93, 118, 56, C_CYAN, 5);

        int mx = 76;
        for (int i = 0; i < 5; i++) {
            int y = 98 + i * 10;
            u8 c = m[i].enabled ? m[i].colour : C_DIM;
            if (i == sel) {
                vid_hgrad(mx - 10, y - 1, 110, 10, RAMP_VIOLET, 10, 0);
                vid_text(mx - 8, y, "\x01", c);
                c = C_WHITE;
            }
            vid_text(mx + 4, y, m[i].label, c);
            if (!m[i].enabled) vid_text(mx + 84, y, "\x0F", C_DIM);
        }
        vid_rect(0, 150, 240, 10, C_BLACK);
        vid_text_center(120, 151, "NEW HERE? START WITH DRY RUN.", C_DIM);
        app_frame_end();
    }
}

/* ====================================================================== */
/*  HUB - the apartment, between games                                     */
/* ====================================================================== */
static void hub_status(void)
{
    char b[40], *p;
    vid_rect(0, 12, 240, 46, C_BLACK);
    vid_hline(0, 58, 240, C_GRID);

    p = str_cat(b, "NIGHT "); p = str_int(p, g_run.night);
    p = str_cat(p, " OF "); str_int(p, NIGHTS);
    vid_text(4, 15, b, C_CYAN);

    b[0] = 0x09; str_money(b + 1, g_run.creds);
    vid_text(4, 26, "ON HAND", C_DIM);
    vid_text(58, 26, b, C_WHITE);

    s32 due = run_due_tonight();
    vid_text(4, 36, "DUE TONIGHT", C_DIM);
    b[0] = 0x09; str_money(b + 1, due);
    vid_text(78, 36, b, due > g_run.creds ? C_BLOOD : C_GREEN);

    vid_text(4, 46, "ESCROW PAID", C_DIM);
    b[0] = 0x09; str_money(b + 1, g_run.escrow_paid);
    vid_text(78, 46, b, C_AMBER);
    ui_bar(150, 47, 86, 6, (int)(g_run.escrow_paid / 1000),
           ESCROW_TARGET / 1000, RAMP_GOLD, C_GRID);

    /* the three things eating you */
    vid_text(150, 15, "DRIFT", C_MAG);
    ui_bar(186, 16, 50, 6, g_run.drift, 100, RAMP_MAG, C_GRID);
    vid_text(150, 25, run_drift_name(), C_CANDY);

    vid_text(150, 35, "WIRE", C_ICE);
    ui_bar(186, 36, 50, 6, g_run.wire, 100, RAMP_CYAN, C_GRID);
}

static void hub_rui_call(void)
{
    static const char *warm[3] = {
      "You sound like you did before. That's the only thing I have to go on in here, you know. Voices. Keep sounding like you.",
      "There's no time in escrow. I've been waiting four seconds and eleven years. Don't hurry on my account. Actually - do hurry.",
      "Whatever you're putting in your head to do this faster: I would rather come out to nobody than come out to something wearing you."
    };
    static const char *cold[3] = {
      "Your voice has a latency on it now. A little pause before the feeling arrives. I keep noticing it and I keep deciding not to say anything.",
      "Who is this? ... No. No, I have you. I have you. It just took a second and it shouldn't take a second.",
      "I'd like to talk to my sister. I'm told that's who I'm speaking to."
    };
    int lvl = run_drift_level();
    const char *line = (lvl <= 1) ? warm[rng_below(3)] : cold[rng_below(3)];
    g_run.flag[FLAG_RUI_CALLED] = 1;
    u16 offs[8];
    int np = vid_wrap_pages(line, 28, 4, offs, 8);
    for (int pg = 0; pg < np; pg++) {
        const char *txt = line + offs[pg];
        int total = (pg + 1 < np) ? (int)(offs[pg + 1] - offs[pg]) : (int)ec_strlen(txt);
        int chars = 0;
        key_flush();
        for (;;) {
            app_frame_begin();
            if (chars < total) chars += 2;
            if (key_is(KEY_A)) chars = total;
            if (chars > total) chars = total;
            if (key_down(KEY_A) && chars >= total) { sfx(SFX_SELECT); break; }
            ui_backdrop(BG_VOID, g_t, RAMP_MAG);
            ui_titlebar("ESCROW LINE / WARD NINE", 0, C_CANDY);
            vid_text_center(120, 30, "CONNECTION QUALITY", C_DIM);
            ui_bar(60, 42, 120, 8, g_run.rui, 100, RAMP_MAG, C_GRID);
            {
                char b[24]; char *p = str_int(b, g_run.rui); str_cat(p, "% INTEGRITY");
                vid_text_center(120, 54, b, g_run.rui < 60 ? C_BLOOD : C_CANDY);
            }
            ui_noise(0, 60, 240, 30, g_t, C_MAG, 100 - g_run.rui);
            ui_dialogue_box(CH_RUI, txt, chars, g_t);
            if (chars >= total) ui_hint(pg + 1 < np ? "\x13 MORE" : "\x13 HANG UP");
            app_frame_end();
        }
    }
}

static void hub_pay(void)
{
    s32 due = run_due_tonight();
    if (due <= 0) { toast("NOTHING DUE TONIGHT", C_GREEN); app_wait(60); return; }
    s32 pay = due;
    if (pay > g_run.creds) pay = g_run.creds;
    if (pay <= 0) { sfx(SFX_DENY); return; }
    char q[80]; char *p = str_cat(q, "WIRE ");
    *p++ = 0x09; p = str_money(p, pay);
    str_cat(p, " TO HALCYON CONTINUITY?");
    if (confirm_box(q, "PAY", "NOT YET", C_AMBER, BG_TERMINAL, RAMP_GOLD)) {
        g_run.creds -= pay;
        g_run.escrow_paid += pay;
        sfx(SFX_CASH);
        toast("PAYMENT LOGGED", C_GREEN);
        app_wait(70);
        save_write();
    }
}

int sc_hub(int arg)
{
    (void)arg;
    int sel = 0;
    mus_play(MUS_NEON_RAIN);
    key_flush();
    app_fade_in(12);
    for (;;) {
        app_frame_begin();
        MenuItem m[7] = {
            { "FIND A GAME",   0, 1, C_MAG },
            { "SEE GREY",      0, 1, C_GREEN },
            { "PAY ESCROW",    0, (u8)(run_due_tonight() > 0 && g_run.creds > 0), C_AMBER },
            { "CALL RUI",      0, 1, C_CANDY },
            { "DRY RUN",       0, 1, C_CYAN },
            { "DRILLS",        0, 1, C_ICE },
            { "END THE NIGHT", 0, 1, C_WHITE },
        };
        if (key_repeat(KEY_DOWN)) { do { sel = (sel + 1) % 7; } while (!m[sel].enabled); sfx(SFX_MOVE); }
        if (key_repeat(KEY_UP))   { do { sel = (sel + 6) % 7; } while (!m[sel].enabled); sfx(SFX_MOVE); }
        if (key_down(KEY_SELECT)) { app_fade_out(10); return SC_CODEX; }
        if (key_down(KEY_A)) {
            sfx(SFX_SELECT);
            switch (sel) {
                case 0: app_fade_out(12); return SC_VENUE;
                case 1: app_fade_out(12); return SC_CLINIC;
                case 2: hub_pay(); key_flush(); break;
                case 3: hub_rui_call(); key_flush(); break;
                case 4: app_fade_out(12); return SC_TEACH;
                case 5: app_fade_out(12); return SC_DRILL;
                case 6:
                    if (confirm_box("SLEEP? THE NIGHT ENDS AND THE INSTALMENT COMES DUE.",
                                    "SLEEP", "NOT YET", C_WHITE, BG_RAIN, RAMP_CYAN)) {
                        app_fade_out(14);
                        return SC_NIGHT_END;
                    }
                    key_flush();
                    break;
            }
        }

        ui_backdrop(BG_RAIN, g_t, RAMP_MAG);
        ui_titlebar("KESTREL / SUBLET 14-C", "SELECT: CODEX", C_CYAN);
        hub_status();

        for (int i = 0; i < 7; i++) {
            int y = 64 + i * 11;
            u8 c = m[i].enabled ? m[i].colour : C_DIM;
            if (i == sel) {
                vid_hgrad(4, y - 1, 150, 11, RAMP_VIOLET, 10, 0);
                vid_text(6, y, "\x01", c);
                c = C_WHITE;
            }
            vid_text(18, y, m[i].label, c);
        }

        /* a line of context for the highlighted option */
        static const char *hint[7] = {
            "pick a room. read the board first.",
            "chrome helps. that is the problem.",
            "halcyon does not remind you twice.",
            "she can still hear the difference.",
            "learn the game, or learn it again.",
            "tags, running count, true, indices.",
            "what you have is what you have.",
        };
        vid_rect(0, 143, 240, 17, C_BLACK);
        vid_hline(0, 143, 240, C_GRID);
        vid_text(4, 147, hint[sel], C_DIM);
        app_frame_end();
    }
}

/* ====================================================================== */
/*  VENUE SELECT - reading a board is a skill                              */
/* ====================================================================== */
int sc_venue(int arg)
{
    (void)arg;
    int sel = 0;
    key_flush();
    app_fade_in(12);
    mus_play(MUS_NEON_RAIN);
    for (;;) {
        app_frame_begin();
        if (key_repeat(KEY_DOWN)) { sel = (sel + 1) % NVENUES; sfx(SFX_MOVE); }
        if (key_repeat(KEY_UP))   { sel = (sel + NVENUES - 1) % NVENUES; sfx(SFX_MOVE); }
        if (key_down(KEY_B)) { sfx(SFX_BACK); app_fade_out(10); return SC_HUB; }
        const Venue *v = &VENUES[sel];
        int locked = g_run.night < v->night_min || sel == 5;
        if (key_down(KEY_A)) {
            if (locked) { sfx(SFX_DENY); }
            else if (g_run.creds < v->min_bet) { sfx(SFX_DENY); }
            else {
                sfx(SFX_SELECT);
                app_fade_out(12);
                sc_table(TABLE_ARG(sel, TMODE_PLAY));
                save_write();
                app_fade_in(12);
                key_flush();
            }
        }

        ui_backdrop(BG_TERMINAL, g_t, v->ramp);
        vid_rect(0, 12, 240, 70, C_BLACK);
        vid_rect_dither(0, 12, 240, 70, C_BLACK, C_INDIGO);
        ui_titlebar("THE STRIP", "B: HOME", v->accent);

        for (int i = 0; i < NVENUES; i++) {
            int y = 15 + i * 11;
            const Venue *w = &VENUES[i];
            int lk = g_run.night < w->night_min || i == 5;
            u8 c = lk ? C_DIM : w->accent;
            if (i == sel) {
                vid_hgrad(0, y - 1, 240, 11, RAMP_VIOLET, 9, 0);
                vid_text(2, y, "\x01", c);
                if (!lk) c = C_WHITE;
            }
            vid_text(12, y, w->name, c);
            if (lk) vid_text(222, y, "\x0F", C_DIM);
        }

        /* the board */
        int by = 84;
        vid_rect(0, by - 2, 240, 62, C_BLACK);
        vid_hline(0, by - 2, 240, v->accent);
        vid_text(4, by, v->district, C_DIM);
        vid_text_wrap(4, by + 10, 38, v->blurb, C_MID);

        char b[48], *p;
        int ry = by + 30;
        p = str_int(b, v->decks); str_cat(p, v->decks == 1 ? " DECK" : " DECKS");
        vid_text(4, ry, b, C_TEXT);
        p = str_int(b, v->pen); str_cat(p, "% DEALT");
        vid_text(62, ry, b, C_TEXT);
        vid_text(126, ry, v->h17 ? "H17" : "S17", v->h17 ? C_BLOOD : C_GREEN);
        vid_text(156, ry, v->das ? "DAS" : "NO DAS", v->das ? C_GREEN : C_DIM);
        vid_text(206, ry, v->surrender ? "LS" : "--", v->surrender ? C_GREEN : C_DIM);

        ry += 10;
        vid_text(4, ry, v->bj65 ? "BJ PAYS 6:5" : "BJ PAYS 3:2", v->bj65 ? C_BLOOD : C_GREEN);
        p = b; *p++ = 0x09; p = str_money(p, v->min_bet);
        p = str_cat(p, " - "); *p++ = 0x09; str_money(p, v->max_bet);
        vid_text(88, ry, b, C_MID);

        ry += 10;
        {
            int e = venue_edge_x100(v);
            int cc = venue_counter_x100(v);
            p = str_cat(b, "HOUSE ");
            if (e < 0) { *p++ = '-'; e = -e; }
            p = str_int(p, e / 100); *p++ = '.';
            p = str_uint_pad(p, (u32)(e % 100), 2, '0'); str_cat(p, "%");
            vid_text(4, ry, b, e > 100 ? C_BLOOD : (e < 35 ? C_GREEN : C_AMBER));

            p = str_cat(b, "COUNTER ");
            if (cc < 0) { *p++ = '-'; } else { *p++ = '+'; }
            int ac = cc < 0 ? -cc : cc;
            p = str_int(p, ac / 100); *p++ = '.';
            p = str_uint_pad(p, (u32)(ac % 100), 2, '0'); str_cat(p, "%");
            vid_text(100, ry, b, cc > 0 ? C_GREEN : C_BLOOD);
        }

        ry += 11;
        if (g_run.night < v->night_min) {
            p = str_cat(b, "WORD OF THIS ROOM REACHES YOU ON NIGHT ");
            str_int(p, v->night_min);
            vid_text(4, ry, b, C_DIM);
        } else if (sel == 5) {
            vid_text(4, ry, "BY INVITATION ONLY.", C_DIM);
        } else {
            if (v->bj65) vid_text(4, ry, "6:5 - THIS ROOM IS A TAX", C_BLOOD);
            vid_text(236 - vid_text_len("\x13 SIT DOWN"), ry, "\x13 SIT DOWN", C_WHITE);
        }
        app_frame_end();
    }
}

/* ====================================================================== */
/*  THE RIPPERDOC                                                          */
/* ====================================================================== */
int sc_clinic(int arg)
{
    (void)arg;
    int sel = 0;
    key_flush();
    app_fade_in(12);
    mus_play(MUS_ASHES);
    for (;;) {
        app_frame_begin();
        if (key_repeat(KEY_DOWN)) { sel = (sel + 1) % IMP_COUNT; sfx(SFX_MOVE); }
        if (key_repeat(KEY_UP))   { sel = (sel + IMP_COUNT - 1) % IMP_COUNT; sfx(SFX_MOVE); }
        if (key_down(KEY_B)) { sfx(SFX_BACK); app_fade_out(10); return SC_HUB; }
        const Implant *im = &IMPLANTS[sel];
        if (key_down(KEY_A)) {
            if (g_run.implant[sel]) { sfx(SFX_DENY); }
            else if (g_run.creds < im->cost) { sfx(SFX_DENY); toast("NOT ENOUGH ON HAND", C_BLOOD); app_wait(50); }
            else {
                char q[120]; char *p = str_cat(q, "INSTALL ");
                p = str_cat(p, im->name); p = str_cat(p, "? ");
                p = str_cat(p, im->warning);
                if (confirm_box(q, "CUT ME OPEN", "NOT TONIGHT", im->colour, BG_CLINIC, RAMP_GREEN)) {
                    g_run.creds -= im->cost;
                    g_run.implant[sel] = 1;
                    run_add_wire(im->wire);
                    run_add_drift(im->drift);
                    sfx(SFX_IMPLANT);
                    app_flash(C_WHITE, 24);
                    save_write();
                }
                key_flush();
            }
        }

        ui_backdrop(BG_CLINIC, g_t, RAMP_GREEN);
        vid_rect(0, 12, 240, 82, C_BLACK);
        vid_rect_dither(0, 12, 240, 82, C_BLACK, C_INDIGO);
        vid_hline(0, 93, 240, C_GREEN);
        ui_titlebar("GREY / SUBLEVEL 2", "B: LEAVE", C_GREEN);
        {
            char b[24]; b[0] = 0x09; str_money(b + 1, g_run.creds);
            vid_text(236 - vid_text_len(b), 2, b, C_WHITE);
        }

        for (int i = 0; i < IMP_COUNT; i++) {
            int y = 14 + i * 10;
            const Implant *w = &IMPLANTS[i];
            u8 c = g_run.implant[i] ? C_DIM : w->colour;
            if (i == sel) {
                vid_hgrad(0, y - 1, 240, 10, RAMP_VIOLET, 9, 0);
                vid_text(2, y, "\x01", c);
                if (!g_run.implant[i]) c = C_WHITE;
            }
            vid_text(10, y, w->name, c);
            if (g_run.implant[i]) vid_text(210, y, "IN", C_GREEN);
            else {
                char b[16]; b[0] = 0x09; str_money(b + 1, w->cost);
                vid_text(236 - vid_text_len(b), y, b,
                         g_run.creds >= w->cost ? C_AMBER : C_BLOOD);
            }
        }

        const Implant *v = &IMPLANTS[sel];
        int by = 96;
        vid_rect(0, by - 2, 240, 52, C_BLACK);
        vid_hline(0, by - 2, 240, v->colour);
        vid_text(4, by, v->maker, C_DIM);
        vid_text_wrap(4, by + 10, 38, v->effect, C_TEXT);
        vid_text_wrap(4, by + 30, 38, v->warning, C_CANDY);

        vid_rect(0, 148, 240, 12, C_BLACK);
        char b[40], *p;
        p = str_cat(b, "WIRE +"); p = str_int(p, v->wire);
        vid_text(4, 150, b, C_ICE);
        p = str_cat(b, "DRIFT +"); p = str_int(p, v->drift);
        vid_text(70, 150, b, C_MAG);
        if (v->drift == 0) vid_text(150, 150, "CLEAN CHROME", C_GREEN);
        app_frame_end();
    }
}

/* ====================================================================== */
/*  NIGHT END                                                              */
/* ====================================================================== */
int sc_night_end(int arg)
{
    (void)arg;
    s32 due = run_due_tonight();
    s32 paid = 0;
    int lien = 0;
    if (due > 0) {
        paid = due;
        if (paid > g_run.creds) paid = g_run.creds;
        g_run.creds -= paid;
        g_run.escrow_paid += paid;
        if (paid < due) {
            lien = 1;
            g_run.liens++;
            int dmg = 14 + g_run.liens * 3;
            g_run.rui = (u8)(g_run.rui > dmg ? g_run.rui - dmg : 0);
        }
    }
    mus_play(lien ? MUS_ASHES : MUS_NEON_RAIN);
    save_write();
    key_flush();
    app_fade_in(14);

    for (;;) {
        app_frame_begin();
        if (key_down(KEY_A)) {
            sfx(SFX_SELECT);
            app_fade_out(16);
            if (g_run.rui == 0) { g_run.ending = 2; return SC_ENDING; }
            if (g_run.night >= NIGHTS) { g_run.ending = 0; return SC_ENDING; }
            g_run.night++;
            save_write();
            return SC_STORY;
        }
        ui_backdrop(BG_RAIN, g_t, lien ? RAMP_ALERT : RAMP_CYAN);
        char b[48], *p;
        p = str_cat(b, "NIGHT "); p = str_int(p, g_run.night); str_cat(p, " CLOSES");
        ui_titlebar(b, 0, lien ? C_BLOOD : C_CYAN);

        ui_panel(10, 20, 220, 116, lien ? C_BLOOD : C_CYAN, "LEDGER");

        int y = 30;
        p = str_cat(b, "INSTALMENT DUE   "); *p++ = 0x09; str_money(p, due);
        vid_text(16, y, b, C_MID); y += 11;
        p = str_cat(b, "WIRED            "); *p++ = 0x09; str_money(p, paid);
        vid_text(16, y, b, paid >= due ? C_GREEN : C_BLOOD); y += 11;
        p = str_cat(b, "ESCROW TOTAL     "); *p++ = 0x09; str_money(p, g_run.escrow_paid);
        vid_text(16, y, b, C_AMBER); y += 11;
        p = str_cat(b, "ON HAND          "); *p++ = 0x09; str_money(p, g_run.creds);
        vid_text(16, y, b, C_WHITE); y += 14;

        ui_divider(16, y - 4, 208, C_GRID);
        p = str_cat(b, "HANDS PLAYED     "); str_int(p, g_run.hands_played);
        vid_text(16, y, b, C_MID); y += 11;
        p = str_cat(b, "BASIC STRATEGY   "); p = str_int(p, run_bs_accuracy()); str_cat(p, "%");
        vid_text(16, y, b, run_bs_accuracy() >= 90 ? C_GREEN : C_AMBER); y += 11;
        p = str_cat(b, "PULSE CHECKS     "); p = str_int(p, run_check_accuracy()); str_cat(p, "%");
        vid_text(16, y, b, run_check_accuracy() >= 70 ? C_GREEN : C_AMBER); y += 11;
        p = str_cat(b, "DRIFT            "); p = str_int(p, g_run.drift);
        p = str_cat(p, "  "); str_cat(p, run_drift_name());
        vid_text(16, y, b, C_MAG);

        if (lien) {
            vid_rect(0, 138, 240, 12, C_BLACK);
            vid_text_center(120, 140, "LIEN APPLIED. THEY TOOK A PIECE OF HER.", C_BLOOD);
            ui_noise(0, 0, 240, 160, g_t, C_BLOOD, 30);
        }
        ui_hint("\x13 SLEEP");
        app_frame_end();
    }
}

/* ====================================================================== */
/*  ENDINGS                                                                */
/* ====================================================================== */
typedef struct { const char *title; const char *body; u8 col; u8 ramp; } Ending;

static const Ending ENDINGS[] = {
{ "FLESH AND BLOOD",
  "You paid it in full with a head nobody touched.\n\nThey grow her a body in eleven days. She comes out of the tank furious about the haircut and she knows your voice on the first syllable.\n\nYou never play again. You keep the count anyway - it is yours, it was always the only thing that was.",
  C_GREEN, RAMP_GREEN },
{ "ASCENSION",
  "You paid it in full, and there is less of you than there was.\n\nShe comes out of the tank. She looks at you for four seconds too long before she says your name, and she never once mentions the pause.\n\nYou tell yourself you would do it again. The part of you that would have argued was sold on night three.",
  C_MAG, RAMP_MAG },
{ "AN INSTANCE OF YOU",
  "Rui walks out in the morning, exactly as promised.\n\nSomewhere under the Spire a description of you sits at nine hundred tables at once and never gets tired and never gets bored and never once wonders why it is doing this.\n\nIt is not you. That distinction is legally settled. She visits it on Sundays.",
  C_PALE, RAMP_CHROME },
{ "RESOLVED",
  "The balance did not clear.\n\nThey do not make a ceremony of it. An adjudicator reads a number out, and the parts of her that were worth something go into a composite mind that manages freight scheduling on the orbital lift.\n\nSomewhere in that system, occasionally, a shipment is routed the long way past the window with the good view of the city.",
  C_BLOOD, RAMP_ALERT },
{ "HOLLOWED",
  "You cleared the balance. You can no longer remember why it mattered.\n\nThe chrome counts. The chrome bets. The chrome smiles at the woman who comes out of the tank and calls her by her name because that is what the file says to do.\n\nShe stays for a year. Then she leaves, and you file it correctly.",
  C_PURPLE, RAMP_VIOLET },
};

int sc_ending(int arg)
{
    (void)arg;
    int e;
    if (g_run.flag[FLAG_SOLD_MIND])      e = 2;
    else if (g_run.rui == 0)             e = 3;
    else if (g_run.escrow_paid >= ESCROW_TARGET) {
        if (g_run.drift >= 75)           e = 4;
        else if (g_run.drift >= 35)      e = 1;
        else                             e = 0;
    } else                               e = 3;

    const Ending *E = &ENDINGS[e];
    mus_play(e == 0 ? MUS_NEON_RAIN : MUS_ASHES);
    key_flush();
    app_fade_in(24);
    int chars = 0;
    int total = (int)ec_strlen(E->body);
    for (;;) {
        app_frame_begin();
        if (chars < total) chars++;
        if (key_is(KEY_A)) chars = total;
        if (chars > total) chars = total;
        if (key_down(KEY_A) && chars >= total) { sfx(SFX_SELECT); break; }

        ui_backdrop(BG_VOID, g_t, E->ramp);
        vid_hline(0, 26, 240, E->col);
        vid_text_center_s(120, 10, E->title, E->col, 2);
        vid_text_wrap_n(10, 36, 37, E->body, C_TEXT, chars);
        if (chars >= total) ui_hint("\x13");
        app_frame_end();
    }

    /* the run report */
    key_flush();
    for (;;) {
        app_frame_begin();
        if (key_down(KEY_A)) { sfx(SFX_SELECT); app_fade_out(20); save_erase(); return SC_TITLE; }
        ui_backdrop(BG_TERMINAL, g_t, E->ramp);
        ui_titlebar("RUN REPORT", 0, E->col);
        char b[48], *p;
        int y = 22;
        ui_panel(10, 16, 220, 118, E->col, 0);
        p = str_cat(b, "ESCROW PAID      "); *p++ = 0x09; str_money(p, g_run.escrow_paid);
        vid_text(16, y, b, C_AMBER); y += 11;
        p = str_cat(b, "HANDS PLAYED     "); str_int(p, g_run.hands_played);
        vid_text(16, y, b, C_TEXT); y += 11;
        p = str_cat(b, "BASIC STRATEGY   "); p = str_int(p, run_bs_accuracy()); str_cat(p, "%");
        vid_text(16, y, b, C_CYAN); y += 11;
        p = str_cat(b, "COUNT ACCURACY   "); p = str_int(p, run_check_accuracy()); str_cat(p, "%");
        vid_text(16, y, b, C_GREEN); y += 11;
        p = str_cat(b, "CHECKS HELD      "); p = str_int(p, g_run.checks_passed);
        p = str_cat(p, " / "); str_int(p, g_run.checks_passed + g_run.checks_failed);
        vid_text(16, y, b, C_MID); y += 11;
        p = str_cat(b, "DRIFT            "); p = str_int(p, g_run.drift);
        p = str_cat(p, "  "); str_cat(p, run_drift_name());
        vid_text(16, y, b, C_MAG); y += 11;
        p = str_cat(b, "WIRE             "); str_int(p, g_run.wire);
        vid_text(16, y, b, C_ICE); y += 11;
        p = str_cat(b, "BIGGEST POT      "); *p++ = 0x09; str_money(p, g_run.biggest_win);
        vid_text(16, y, b, C_WHITE); y += 14;
        vid_text_wrap(16, y, 34,
            "The count was never the hard part. Deciding what to spend to keep it was.", C_DIM);
        ui_hint("\x13 TITLE");
        app_frame_end();
    }
}
