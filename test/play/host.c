/* EVERYTHING COUNTS - a card counting story for the Game Boy Advance
 * Copyright (C) 2026 Godless-1
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
/* ========================================================================
 *  host.c - runs the WHOLE game on the build machine, no emulator.
 *
 *  The game's own sources are compiled unchanged against a generated copy
 *  of gba.h whose hardware addresses point at plain arrays (see run.sh).
 *  Every vid_flip() lands in host_frame(), which
 *    - feeds the pad from a script,
 *    - can write the visible page out as a PPM screenshot,
 *    - watches every glyph the renderer draws and reports text that is
 *      clipped by the screen edge, drawn on top of other text, or painted
 *      over before the frame is shown,
 *    - logs the PSG registers once per frame so the music can be rendered
 *      and checked offline.
 *
 *  Nothing in this file is compiled into the ROM.
 * ===================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define EC_HOST_HARNESS
#include "gba.h"
#include "game.h"
#include "save.h"
#include "rng.h"
#include "cards.h"
#include "table.h"

/* ---- the fake machine -------------------------------------------------- */
u8  host_io[0x400];
u16 host_pal[256];
u16 host_vram[0xA000];              /* two 240x160 pages, 0xA000 bytes each */
u8  host_sram[0x8000];
DmaReg host_dma[4];
volatile u32 irq_flags;
void (*irq_handler_c)(void);

extern u8 *vid_back;
int game_main(void);

/* ---- options ----------------------------------------------------------- */
static const char *opt_out = ".";
static const char *opt_sav;
static FILE *opt_snd;
static FILE *script;
static long  frame_no;
static int   quiet;
static int   opt_g;                       /* -g: glyph dump with every shot */
static int   opt_x;                       /* -x: box-edge events */

/* ---- text watcher ------------------------------------------------------ */
#define MAXG 4096
typedef struct {
    short x, y; u8 ch, scale;
    char text[56];
    int ink, lost;                   /* pixels drawn / later painted over */
    int clipped;
} Glyph;
static Glyph gl[MAXG];
static int   ngl;
static int   cur_g = -1;
static const char *cur_text = "";
static u16   owner[SCREEN_W * SCREEN_H];   /* glyph index + 1 */
static u8    boxmap[SCREEN_W * SCREEN_H];  /* 1 = outline pixel of a frame rect */
/* the finished frame, kept for shots taken after text_frame_end() reset it */
static Glyph sgl[MAXG];
static int   sngl;
static u16   sowner[SCREEN_W * SCREEN_H];

#define MAXEV 4000
static char *seen_ev[MAXEV];
static int   nseen;
static FILE *ev_log;
static int   ev_shots;
static int   want_event_shot;
static char  ev_shot_name[64];

static const char *printable(const char *s)
{
    static char buf[4][80];
    static int k;
    char *b = buf[k++ & 3];
    int n = 0;
    for (; *s && n < 70; s++) {
        unsigned char c = (unsigned char)*s;
        if (c == '\n') { b[n++] = '/'; }
        else if (c < 32 || c > 126) { n += sprintf(b + n, "<%02X>", c); }
        else b[n++] = (char)c;
    }
    b[n] = 0;
    return b;
}

/* reported once per kind + string, wherever on screen it first happens */
static void event(const char *kind, const char *id, const char *detail)
{
    char key[400];
    snprintf(key, sizeof key, "%s %s", kind, id);
    for (int i = 0; i < nseen; i++) if (!strcmp(seen_ev[i], key)) return;
    if (nseen >= MAXEV) return;
    seen_ev[nseen++] = strdup(key);
    if (ev_log) { fprintf(ev_log, "frame %ld  %s %s\n", frame_no, kind, detail); fflush(ev_log); }
    if (ev_shots < 400 && !want_event_shot) {
        want_event_shot = 1;
        snprintf(ev_shot_name, sizeof ev_shot_name, "ev%03d-%s", ev_shots++, kind);
    }
}

void host_text(const char *s) { cur_text = s; }

void host_glyph_begin(int x, int y, u8 ch, int scale)
{
    if (ngl >= MAXG) { cur_g = -1; return; }
    Glyph *g = &gl[ngl];
    g->x = (short)x; g->y = (short)y; g->ch = ch; g->scale = (u8)scale;
    strncpy(g->text, cur_text, sizeof g->text - 1); g->text[sizeof g->text - 1] = 0;
    g->ink = g->lost = g->clipped = 0;
    cur_g = ngl++;
}
void host_glyph_end(void) { cur_g = -1; }

/* is glyph b just the shadow / glow / glitch twin of glyph a? */
static int twin(const Glyph *a, const Glyph *b)
{
    int d = 2 * (a->scale > b->scale ? a->scale : b->scale) + 1;
    int dx = a->x - b->x, dy = a->y - b->y;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    if (a->ch == b->ch && dx <= d && dy <= d) return 1;
    /* the same string redrawn a few pixels off (chromatic ghosts) */
    return dy <= d && !strcmp(a->text, b->text);
}

/* every pixel write the renderer makes, already clipped to the screen */
void host_px(int x, int y, int w, int h)
{
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            int xx = x + i, yy = y + j;
            if ((unsigned)xx >= SCREEN_W || (unsigned)yy >= SCREEN_H) continue;
            u16 *o = &owner[yy * SCREEN_W + xx];
            if (cur_g >= 0) {
                Glyph *g = &gl[cur_g];
                g->ink++;
                if (opt_x && boxmap[yy * SCREEN_W + xx]) {
                    char d[300];
                    snprintf(d, sizeof d, "\"%s\" at %d,%d", printable(g->text), g->x, g->y);
                    event("BOXEDGE", g->text, d);
                    boxmap[yy * SCREEN_W + xx] = 0;
                }
                if (*o && *o - 1 != cur_g) {
                    Glyph *p = &gl[*o - 1];
                    if (!twin(g, p)) {
                        char d[300];
                        snprintf(d, sizeof d, "\"%s\" at %d,%d over \"%s\" at %d,%d",
                                 printable(g->text), g->x, g->y,
                                 printable(p->text), p->x, p->y);
                        char id[160];
                        snprintf(id, sizeof id, "%s|%s", g->text, p->text);
                        event("OVERLAP", id, d);
                        p->lost++;
                    }                       /* a twin is meant to cover it */
                }
                *o = (u16)(cur_g + 1);
            } else {
                boxmap[yy * SCREEN_W + xx] = 0;
                if (*o) {
                    gl[*o - 1].lost++;
                    *o = 0;
                }
            }
        }
}

/* a pop-up is about to cover the picture: forget what is underneath */
void host_overlay(void) { memset(owner, 0, sizeof owner); memset(boxmap, 0, sizeof boxmap); }

/* the outline of a frame rectangle was just drawn (-x only) */
void host_box(int x, int y, int w, int h)
{
    if (!opt_x) return;
    for (int i = 0; i < w; i++)
        for (int k = 0; k < 2; k++) {
            int xx = x + i, yy = k ? y + h - 1 : y;
            if ((unsigned)xx < SCREEN_W && (unsigned)yy < SCREEN_H) boxmap[yy * SCREEN_W + xx] = 1;
        }
    for (int j = 0; j < h; j++)
        for (int k = 0; k < 2; k++) {
            int xx = k ? x + w - 1 : x, yy = y + j;
            if ((unsigned)xx < SCREEN_W && (unsigned)yy < SCREEN_H) boxmap[yy * SCREEN_W + xx] = 1;
        }
}

/* scripts can switch the random count checks off to stay deterministic */
int host_no_pulse;

/* an inked glyph pixel that fell off the screen */
void host_clip(int x, int y)
{
    (void)x; (void)y;
    if (cur_g >= 0) gl[cur_g].clipped++;
}

/* ---- autoshot: one screenshot per distinct screen ---------------------- */
static char auto_prefix[48];
static int  auto_on, auto_n;
static u32  auto_seen[8192];
static int  auto_nseen;
static u32  auto_last_sig;
static int  auto_last_n, auto_stable;
static void shot(const char *name);

static void autoshot_frame(void)
{
    /* what is on screen = the set of strings drawn (typing progress and
     * blinking one-character cursors do not count as a new screen) */
    u32 sig = 0;
    const char *prev = 0;
    for (int i = 0; i < ngl; i++) {
        const char *t = gl[i].text;
        if (prev && !strcmp(prev, t)) continue;
        prev = t;
        if (strlen(t) < 2) continue;
        u32 h = 2166136261u;
        for (const char *c = t; *c; c++) { h ^= (u8)*c; h *= 16777619u; }
        sig += h;
    }
    if (sig == auto_last_sig && ngl == auto_last_n) auto_stable++;
    else auto_stable = 0;
    auto_last_sig = sig; auto_last_n = ngl;
    if (auto_stable != 12) return;
    for (int i = 0; i < auto_nseen; i++) if (auto_seen[i] == sig) return;
    if (auto_nseen < 8192) auto_seen[auto_nseen++] = sig;
    char name[80];
    snprintf(name, sizeof name, "%s-%03d", auto_prefix, auto_n++);
    shot(name);
}

/* scripts can wait for a piece of text to be on screen */
static char want_text[96];
static int  want_hit;

static void text_frame_end(void)
{
    if (opt_g) {
        memcpy(sgl, gl, sizeof(Glyph) * (size_t)ngl);
        sngl = ngl;
        memcpy(sowner, owner, sizeof owner);
    }
    if (opt_x)
        for (int y = 0; y < SCREEN_H; y++)
            for (int x = 0; x < SCREEN_W; x++) {
                int o = owner[y * SCREEN_W + x];
                if (!o) continue;
                int hit = (x > 0 && boxmap[y * SCREEN_W + x - 1]) ||
                          (x < SCREEN_W - 1 && boxmap[y * SCREEN_W + x + 1]) ||
                          (y > 0 && boxmap[(y - 1) * SCREEN_W + x]) ||
                          (y < SCREEN_H - 1 && boxmap[(y + 1) * SCREEN_W + x]);
                if (hit) {
                    Glyph *g = &gl[o - 1];
                    char d[300];
                    snprintf(d, sizeof d, "\"%s\" at %d,%d", printable(g->text), g->x, g->y);
                    event("BOXTIGHT", g->text, d);
                }
            }
    if (auto_on) autoshot_frame();
    want_hit = 0;
    if (want_text[0])
        for (int i = 0; i < ngl && !want_hit; i++)
            if (strstr(gl[i].text, want_text)) want_hit = 1;
    for (int i = 0; i < ngl; i++) {
        Glyph *g = &gl[i];
        char d[300];
        if (g->clipped) {
            snprintf(d, sizeof d, "'%s' of \"%s\" at %d,%d",
                     printable((char[]){ (char)g->ch, 0 }), printable(g->text), g->x, g->y);
            event("CLIPPED", g->text, d);
        }
        if (g->ink > 0 && g->lost * 2 > g->ink) {
            snprintf(d, sizeof d, "\"%s\" at %d,%d", printable(g->text), g->x, g->y);
            event("HIDDEN", g->text, d);
        }
    }
    ngl = 0;
    memset(owner, 0, sizeof owner);
    memset(boxmap, 0, sizeof boxmap);
}

/* ---- glyph dump (-g) --------------------------------------------------- */
static int   g_head[MAXG];
static int   g_next[SCREEN_W * SCREEN_H];
static int   g_own[MAXG];
static int   sort_i[MAXG];

static int cmp_y(const void *a, const void *b)
{
    const Glyph *p = &sgl[*(const int *)a], *q = &sgl[*(const int *)b];
    return (p->y != q->y) ? p->y - q->y : p->x - q->x;
}
static int cmp_x(const void *a, const void *b)
{
    const Glyph *p = &sgl[*(const int *)a], *q = &sgl[*(const int *)b];
    return (p->x != q->x) ? p->x - q->x : p->y - q->y;
}

static void glyph_files(const char *name)
{
    char path[512];
    for (int i = 0; i < sngl; i++) { g_head[i] = -1; g_own[i] = 0; }
    for (int p = 0; p < SCREEN_W * SCREEN_H; p++) {
        int o = sowner[p];
        if (!o || o > sngl) continue;
        g_next[p] = g_head[o - 1]; g_head[o - 1] = p; g_own[o - 1]++;
    }
    snprintf(path, sizeof path, "%s/%s.glyphs.tsv", opt_out, name);
    FILE *f = fopen(path, "w");
    if (!f) { perror(path); exit(2); }
    for (int i = 0; i < sngl; i++) {
        Glyph *g = &sgl[i];
        fprintf(f, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%s\t", g->x, g->y, g->ch, g->scale,
                g->ink, g->lost, g->clipped, printable(g->text));
        int first = 1;
        for (int p = g_head[i]; p >= 0; p = g_next[p]) {
            fprintf(f, first ? "%d,%d" : ";%d,%d", p % SCREEN_W, p / SCREEN_W);
            first = 0;
        }
        fputc('\n', f);
    }
    fclose(f);

    /* the text a person can read: visible glyphs, row by row */
    snprintf(path, sizeof path, "%s/%s.text.txt", opt_out, name);
    f = fopen(path, "w");
    if (!f) { perror(path); exit(2); }
    int n = 0;
    for (int i = 0; i < sngl; i++) if (g_own[i] > 0 && sgl[i].scale >= 1) sort_i[n++] = i;
    qsort(sort_i, (size_t)n, sizeof sort_i[0], cmp_y);
    for (int a = 0; a < n;) {
        int b = a + 1, y0 = sgl[sort_i[a]].y;
        while (b < n && sgl[sort_i[b]].y - y0 <= 3) b++;
        qsort(sort_i + a, (size_t)(b - a), sizeof sort_i[0], cmp_x);
        const Glyph *prev = 0;
        for (int k = a; k < b; k++) {
            const Glyph *g = &sgl[sort_i[k]];
            if (prev && g->ch == prev->ch && g->x - prev->x <= 3) continue;  /* shadow twin */
            if (prev && g->x - (prev->x + 6 * prev->scale) >= 6) fputc(' ', f);
            if (g->ch >= 32 && g->ch <= 126) fputc(g->ch, f);
            else fprintf(f, "<%02X>", g->ch);
            prev = g;
        }
        fputc('\n', f);
        a = b;
    }
    fclose(f);
}

/* ---- screenshots ------------------------------------------------------- */
static void shot(const char *name)
{
    char path[512];
    snprintf(path, sizeof path, "%s/%s.ppm", opt_out, name);
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(2); }
    fprintf(f, "P6\n%d %d\n255\n", SCREEN_W, SCREEN_H);
    /* host_frame() runs inside vid_vsync(), before the flip: the picture
     * just drawn (the one the glyph tables describe) is still vid_back */
    const u8 *page = vid_back;
    u16 bld = *(u16 *)(host_io + 0x50), bldy = *(u16 *)(host_io + 0x54) & 31;
    int mode = (bld >> 6) & 3;
    if (bldy > 16) bldy = 16;
    for (int i = 0; i < SCREEN_W * SCREEN_H; i++) {
        u16 c = host_pal[page[i]];
        int r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
        if (mode == 2) { r -= r * bldy / 16; g -= g * bldy / 16; b -= b * bldy / 16; }
        if (mode == 1) { r += (31 - r) * bldy / 16; g += (31 - g) * bldy / 16; b += (31 - b) * bldy / 16; }
        fputc((r << 3) | (r >> 2), f);
        fputc((g << 3) | (g >> 2), f);
        fputc((b << 3) | (b >> 2), f);
    }
    fclose(f);
    if (opt_g) glyph_files(name);
}

/* ---- the script -------------------------------------------------------- */
static int   wait_frames;
static u16   keys_held;
static int   release_in;
static int   mash_left, mash_hold, mash_gap;
static u16   mash_key;
static int   rnd_left;
static u32   rnd_state;
static int   wf_left;                  /* waitfor: frames before giving up */
static int   mu_left, mu_gap, mu_hold, mu_cool;   /* mashuntil */
static u16   mu_key;

/* cards the next shoe must start with (ranks 1..13, 1 = ace) */
static u8  stack[64];
static int nstack;

void host_shoe(Shoe *s)
{
    for (int i = 0; i < nstack && i < s->n; i++) {
        int want = stack[i] - 1;
        for (int j = i; j < s->n; j++)
            if (CARD_RANK(s->cards[j]) == want) {
                u8 t = s->cards[i]; s->cards[i] = s->cards[j]; s->cards[j] = t;
                break;
            }
    }
    nstack = 0;
}

static u16 key_by_name(const char *k)
{
    static const struct { const char *n; u16 k; } M[] = {
        {"a",KEY_A},{"b",KEY_B},{"select",KEY_SELECT},{"start",KEY_START},
        {"right",KEY_RIGHT},{"left",KEY_LEFT},{"up",KEY_UP},{"down",KEY_DOWN},
        {"r",KEY_R},{"l",KEY_L},
    };
    for (unsigned i = 0; i < sizeof M / sizeof M[0]; i++)
        if (!strcmp(M[i].n, k)) return M[i].k;
    fprintf(stderr, "script: unknown key '%s'\n", k);
    exit(2);
}

static void save_flush(void)
{
    if (!opt_sav) return;
    FILE *f = fopen(opt_sav, "wb");
    if (f) { fwrite(host_sram, 1, sizeof host_sram, f); fclose(f); }
}

int  host_table_peek(int what);
void host_table_poke(int what, int v);

static void dump(const char *label)
{
    printf("DUMP %s frame=%ld creds=%d escrow=%d night=%d drift=%d wire=%d rui=%d "
           "liens=%d hands=%d bs=%d/%d checks=%d/%d streak=%d best=%d bigwin=%d "
           "lessons=%d ending=%d tnet=%d thands=%d tend=%d save=%d flags=",
           label, frame_no, g_run.creds, g_run.escrow_paid, g_run.night, g_run.drift,
           g_run.wire, g_run.rui, g_run.liens, g_run.hands_played, g_run.bs_correct,
           g_run.bs_total, g_run.checks_passed, g_run.checks_failed, g_run.streak,
           g_run.best_streak, g_run.biggest_win, g_run.lessons_done, g_run.ending,
           g_table_net, g_table_hands, g_table_end, save_exists());
    for (int i = 0; i < FLAG_COUNT; i++) putchar('0' + g_run.flag[i]);
    printf(" implants=");
    for (int i = 0; i < IMP_COUNT; i++) putchar('0' + g_run.implant[i]);
    printf(" drills=");
    for (int i = 0; i < 5; i++) printf("%d,", g_run.best_drill[i]);
    printf(" trust=%d heat=%d rc=%d tc=%d", host_table_peek(0), host_table_peek(1),
           host_table_peek(2), host_table_peek(3));
    putchar('\n');
    fflush(stdout);
}

static void set_field(const char *f, int v)
{
    int i;
    if      (!strcmp(f, "night"))   g_run.night = (u8)v;
    else if (!strcmp(f, "creds"))   g_run.creds = v;
    else if (!strcmp(f, "escrow"))  g_run.escrow_paid = v;
    else if (!strcmp(f, "drift"))   g_run.drift = (u8)v;
    else if (!strcmp(f, "wire"))    g_run.wire = (u8)v;
    else if (!strcmp(f, "rui"))     g_run.rui = (u8)v;
    else if (!strcmp(f, "liens"))   g_run.liens = (u8)v;
    else if (!strcmp(f, "lessons")) g_run.lessons_done = (u8)v;
    else if (!strcmp(f, "started")) g_run.seen_intro = (u8)v;
    else if (!strcmp(f, "hands"))   g_run.hands_played = (u16)v;
    else if (!strcmp(f, "bs_total"))   g_run.bs_total = (u16)v;
    else if (!strcmp(f, "bs_correct")) g_run.bs_correct = (u16)v;
    else if (!strcmp(f, "checks_passed")) g_run.checks_passed = (u16)v;
    else if (!strcmp(f, "checks_failed")) g_run.checks_failed = (u16)v;
    else if (!strcmp(f, "best_streak"))   g_run.best_streak = (u16)v;
    else if (!strcmp(f, "biggest_win"))   g_run.biggest_win = v;
    else if (sscanf(f, "drill.%d", &i) == 1 && i >= 0 && i < 5)  g_run.best_drill[i] = (u8)v;
    else if (sscanf(f, "locked.%d", &i) == 1 && i >= 0 && i < 6) g_run.venue_locked[i] = (u8)v;
    else if (sscanf(f, "implant.%d", &i) == 1 && i >= 0 && i < IMP_COUNT) g_run.implant[i] = (u8)v;
    else if (sscanf(f, "flag.%d", &i) == 1 && i >= 0 && i < FLAG_COUNT)   g_run.flag[i] = (u8)v;
    else if (sscanf(f, "beat.%d", &i) == 1 && i >= 0 && i < NIGHTS + 2)   g_run.beat_done[i] = (u8)v;
    else { fprintf(stderr, "script: unknown field '%s'\n", f); exit(2); }
}

static void finish(int code)
{
    save_flush();
    if (opt_snd) fclose(opt_snd);
    if (ev_log) fclose(ev_log);
    if (!quiet) printf("END frames=%ld events=%d\n", frame_no, nseen);
    exit(code);
}

/* runs script lines until one of them takes time */
static void script_step(void)
{
    char line[512], a[128], b[128];
    while (!wait_frames && !release_in && !mash_left && !rnd_left && !wf_left && !mu_left) {
        if (!script || !fgets(line, sizeof line, script)) finish(0);
        a[0] = b[0] = 0;
        int n = 0, m = 0;
        char cmd[64] = "";
        if (sscanf(line, "%63s", cmd) < 1 || cmd[0] == '#') continue;

        if (!strcmp(cmd, "wait")) {
            sscanf(line, "%*s %d", &wait_frames);
        } else if (!strcmp(cmd, "press")) {            /* press <key> [hold] */
            n = 4; sscanf(line, "%*s %127s %d", a, &n);
            keys_held = key_by_name(a); release_in = n; wait_frames = 0;
        } else if (!strcmp(cmd, "mash")) {             /* mash <key> <count> [gap] */
            n = 1; m = 5; sscanf(line, "%*s %127s %d %d", a, &n, &m);
            mash_key = key_by_name(a); mash_left = n; mash_hold = 0; mash_gap = m;
        } else if (!strcmp(cmd, "waitfor")) {          /* waitfor <maxframes> <text> */
            int off = 0;
            sscanf(line, "%*s %d %n", &wf_left, &off);
            strncpy(want_text, line + off, sizeof want_text - 1);
            want_text[strcspn(want_text, "\r\n")] = 0;
            want_hit = 0;
        } else if (!strcmp(cmd, "mashuntil")) {        /* mashuntil <key> <gap> <max> <text> */
            int off = 0;
            sscanf(line, "%*s %127s %d %d %n", a, &mu_gap, &mu_left, &off);
            mu_key = key_by_name(a); mu_hold = 0; mu_cool = 0;
            strncpy(want_text, line + off, sizeof want_text - 1);
            want_text[strcspn(want_text, "\r\n")] = 0;
            want_hit = 0;
        } else if (!strcmp(cmd, "random")) {           /* random <presses> <seed> */
            n = 100; m = 1; sscanf(line, "%*s %d %d", &n, &m);
            rnd_left = n; rnd_state = (u32)m * 2654435761u + 1;
        } else if (!strcmp(cmd, "shot")) {
            sscanf(line, "%*s %127s", a); shot(a);
        } else if (!strcmp(cmd, "autoshot")) {         /* autoshot <prefix> | autoshot off */
            sscanf(line, "%*s %47s", auto_prefix);
            auto_on = strcmp(auto_prefix, "off") != 0;
        } else if (!strcmp(cmd, "dump")) {
            sscanf(line, "%*s %127s", a); dump(a);
        } else if (!strcmp(cmd, "set")) {
            sscanf(line, "%*s %127s %d", a, &n); set_field(a, n);
        } else if (!strcmp(cmd, "tset")) {             /* tset trust|heat N */
            sscanf(line, "%*s %127s %d", a, &n);
            if (!strcmp(a, "trust")) host_table_poke(0, n);
            else if (!strcmp(a, "heat")) host_table_poke(1, n);
            else { fprintf(stderr, "script: unknown tset field '%s'\n", a); exit(2); }
        } else if (!strcmp(cmd, "save")) {
            save_write();
        } else if (!strcmp(cmd, "music")) {            /* music <song number> */
            void mus_play(int song);
            sscanf(line, "%*s %d", &n); mus_play(n);
        } else if (!strcmp(cmd, "nopulse")) {
            sscanf(line, "%*s %d", &host_no_pulse);
        } else if (!strcmp(cmd, "seed")) {
            sscanf(line, "%*s %d", &n); rng_seed((u32)n);
        } else if (!strcmp(cmd, "stack")) {            /* stack 1 13 8 8 ... */
            char *p = line + 5; nstack = 0;
            while (nstack < 64) {
                char *e; long v = strtol(p, &e, 10);
                if (e == p) break;
                stack[nstack++] = (u8)v; p = e;
            }
        } else if (!strcmp(cmd, "quit")) {
            finish(0);
        } else {
            fprintf(stderr, "script: unknown command '%s'\n", cmd);
            exit(2);
        }
    }
}

static u32 rnd(void)
{
    rnd_state ^= rnd_state << 13; rnd_state ^= rnd_state >> 17; rnd_state ^= rnd_state << 5;
    return rnd_state;
}

/* ---- once per displayed frame ------------------------------------------ */
void host_frame(void)
{
    frame_no++;
    text_frame_end();
    if (want_event_shot) { shot(ev_shot_name); want_event_shot = 0; }

    if (opt_snd) {
        /* 0x60..0x9F: every PSG register and the wave RAM */
        fwrite(host_io + 0x60, 1, 0x40, opt_snd);
        /* the trigger bits are write-only strobes on the real machine */
        static const int trig[4] = { 0x64, 0x6C, 0x74, 0x7C };
        for (int i = 0; i < 4; i++) *(u16 *)(host_io + trig[i]) &= 0x7FFF;
    }

    if (wait_frames > 0) wait_frames--;
    if (release_in > 0 && --release_in == 0) { keys_held = 0; wait_frames = 3; }
    if (mash_left > 0) {
        if (mash_hold > 0) { if (--mash_hold == 0) { keys_held = 0; mash_left--; wait_frames = mash_gap; } }
        else if (!wait_frames) { keys_held = mash_key; mash_hold = 3; }
    }
    if (wf_left > 0) {
        if (want_hit) { wf_left = 0; want_text[0] = 0; wait_frames = 2; }
        else if (--wf_left == 0) {
            fprintf(stderr, "script: gave up waiting for \"%s\" at frame %ld\n", want_text, frame_no);
            shot("waitfor-timeout");
            finish(3);
        }
    }
    if (mu_left > 0) {
        if (want_hit && !mu_hold) { mu_left = 0; want_text[0] = 0; keys_held = 0; wait_frames = 2; }
        else if (mu_hold > 0) { if (--mu_hold == 0) { keys_held = 0; mu_cool = mu_gap; } }
        else if (mu_cool > 0) mu_cool--;
        else if (--mu_left == 0) {
            fprintf(stderr, "script: mashed without ever seeing \"%s\" (frame %ld)\n", want_text, frame_no);
            shot("mashuntil-timeout");
            finish(3);
        } else { keys_held = mu_key; mu_hold = 3; }
    }
    if (rnd_left > 0 && !wait_frames && !release_in) {
        static const u16 K[] = { KEY_A, KEY_A, KEY_A, KEY_A, KEY_A, KEY_A, KEY_DOWN, KEY_DOWN,
            KEY_UP, KEY_LEFT, KEY_RIGHT, KEY_RIGHT, KEY_B, KEY_START, KEY_SELECT, KEY_L, KEY_R };
        keys_held = K[rnd() % (sizeof K / sizeof K[0])];
        release_in = 2 + (int)(rnd() % 4);
        rnd_left--;
    }
    script_step();

    *(u16 *)(host_io + 0x130) = (u16)(~keys_held & 0x3FF);
    if (irq_handler_c) irq_handler_c();
}

/* called by the shimmed bios_halt() inside vid_vsync() */
void host_halt(void) { host_frame(); }

int main(int argc, char **argv)
{
    const char *spath = 0;
    for (int i = 1; i < argc; i++) {
        if      (!strcmp(argv[i], "-o") && i + 1 < argc) opt_out = argv[++i];
        else if (!strcmp(argv[i], "-s") && i + 1 < argc) opt_sav = argv[++i];
        else if (!strcmp(argv[i], "-a") && i + 1 < argc) opt_snd = fopen(argv[++i], "wb");
        else if (!strcmp(argv[i], "-q")) quiet = 1;
        else if (!strcmp(argv[i], "-g")) opt_g = 1;
        else if (!strcmp(argv[i], "-x")) opt_x = 1;
        else spath = argv[i];
    }
    if (!spath) {
        fprintf(stderr, "usage: play [-o outdir] [-s file.sav] [-a psg.log] [-q] [-g] [-x] script\n");
        return 2;
    }
    script = strcmp(spath, "-") ? fopen(spath, "r") : stdin;
    if (!script) { perror(spath); return 2; }

    memset(host_sram, 0xFF, sizeof host_sram);
    if (opt_sav) {
        FILE *f = fopen(opt_sav, "rb");
        if (f) { if (fread(host_sram, 1, sizeof host_sram, f)) {} fclose(f); }
    }
    char ep[512];
    snprintf(ep, sizeof ep, "%s/events.log", opt_out);
    ev_log = fopen(ep, "w");

    *(u16 *)(host_io + 0x130) = 0x3FF;
    game_main();
    return 0;
}
