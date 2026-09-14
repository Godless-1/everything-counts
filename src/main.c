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
 *  EVERYTHING COUNTS
 *  A transhumanist cyberpunk blackjack game for the Game Boy Advance.
 * ===================================================================== */
#include "app.h"
#include "story.h"
#include "save.h"
#include "teach.h"

static void boot_screen(void)
{
    static const char *lines[] = {
        "KESTREL CITY MUNICIPAL NET",
        "NEURAL LACE .............. ABSENT",
        "CO-PROCESSOR ............. NONE",
        "TELEMETRY SIGNATURE ...... CLEAN",
        "ARITHMETIC ............... ORGANIC",
        "",
        "YOU ARE UNASSISTED.",
    };
    const int n = 7;
    int shown = 0, t = 0;
    key_flush();
    while (shown <= n) {
        app_frame_begin();
        t++;
        if ((t % 24) == 0) { shown++; if (shown <= n) sfx(SFX_MOVE); }
        if (key_down(KEY_A) || key_down(KEY_START)) break;

        vid_clear(C_BLACK);
        vid_vgrad_scan(0, 0, 240, 160, RAMP_CYAN, 2, 0, 1);
        ui_noise(0, 0, 240, 160, g_t, RAMP_CYAN + 6, 40);
        for (int i = 0; i < shown && i < n; i++)
            vid_text(16, 34 + i * 12, lines[i], i == n - 1 ? C_CYAN : C_MID);
        {
            u8 lut[4] = { 0, RAMP_CYAN + 7, RAMP_CYAN + 14, C_WHITE };
            vid_blit4(112, 12, SIGIL_KESTREL_W, SIGIL_KESTREL_H, sigil_kestrel, lut);
        }
        if (shown > n && ((g_t / 12) & 1))
            vid_text_center(120, 132, "\x13", C_CYAN);
        /* AGPLv3 s5(d): appropriate legal notices */
        vid_text_center(120, 140, "EVERYTHING COUNTS (C) 2026 GODLESS-1", C_DIM);
        vid_text_center(120, 150, "AGPLv3 - FREE SOFTWARE - NO WARRANTY", C_DIM);
        app_frame_end();
    }
    /* whatever the player did with their thumbs is our entropy */
    rng_seed(g_t * 2654435761u + (REG_TM2D | 1));
    app_fade_out(12);
}

#ifdef EC_DEBUG
/* Test-harness entry point only.  Never compiled into the shipping ROM;
 * `make debug` builds dist/everything-counts-debug.gba with it. */
static void debug_preset(void)
{
    run_reset();
    g_run.night        = EC_DEBUG_NIGHT;
    g_run.creds        = 900000;
    g_run.escrow_paid  = 1400000;
    g_run.drift        = 42;
    g_run.wire         = 31;
    g_run.rui          = 74;
    g_run.implant[IMP_OPTIC]  = 1;
    g_run.implant[IMP_ABACUS] = 1;
    g_run.hands_played = 140;
    g_run.bs_total     = 140;
    g_run.bs_correct   = 126;
    g_run.checks_passed = 22;
    g_run.checks_failed = 7;
    g_run.biggest_win  = 84000;
    g_run.best_streak  = 6;
    g_run.lessons_done = 7;
    for (int i = 0; i < EC_DEBUG_NIGHT; i++) g_run.beat_done[i] = 1;
    for (int i = 0; i < 5; i++) g_run.best_drill[i] = 70 + i * 5;
}
#endif

int main(void)
{
    vid_init();
    snd_init();
    key_flush();
    REG_TM2CNT = 0x0080;          /* free-running timer for entropy */
    run_reset();

    boot_screen();

    int scene = SC_TITLE;
    int caller = SC_TITLE;
    int arg = 0;
#ifdef EC_DEBUG
    debug_preset();
    scene = SC_HUB;
#endif

    for (;;) {
        int next;
        g_dbg_scene = scene;
        switch (scene) {
        case SC_TITLE:
            caller = SC_TITLE;
            next = sc_title(arg);
            break;

        case SC_HUB:
            caller = SC_HUB;
            save_write();
            next = sc_hub(arg);
            break;

        case SC_STORY: {
            int b = g_run.beat_done[0] ? g_run.night : 0;
            if (b > NIGHTS) b = NIGHTS;
            if (g_run.beat_done[b]) { scene = SC_HUB; continue; }
            next = sc_story(b);
            if (next == SC_TEACH) caller = SC_STORY;   /* come back here after */
            break;
        }

        case SC_VENUE:     next = sc_venue(arg); break;
        case SC_TABLE:     next = sc_table(arg); break;
        case SC_CLINIC:    next = sc_clinic(arg); break;
        case SC_NIGHT_END: next = sc_night_end(arg); break;
        case SC_ENDING:    next = sc_ending(arg); break;

        case SC_TEACH:     next = sc_teach(arg); break;
        case SC_DRILL:     next = sc_drill(arg); break;
        case SC_CODEX:     next = sc_codex(arg); break;

        default:           next = SC_TITLE; break;
        }

        if (next == SC_QUIT) next = caller;
        scene = next;
        arg = 0;
    }
}
