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
 *  audio.c - PSG (DMG legacy channel) synth + tiny pattern sequencer.
 *
 *  ch1  square  lead / arpeggio
 *  ch2  square  bass
 *  ch3  wave    sound effects (tonal)
 *  ch4  noise   drums + noise effects
 * ===================================================================== */
#include "audio.h"

/* 131072 / freq, for C4..B4 */
static const u16 note_div[12] = {
    501, 473, 446, 421, 398, 375, 354, 334, 316, 298, 281, 265
};

static u16 sq_reg(int note)         /* note = octave*12 + semitone */
{
    int oct = note / 12, sem = note % 12;
    int d = note_div[sem];
    if (oct < 4) d <<= (4 - oct);
    else if (oct > 4) d >>= (oct - 4);
    if (d > 2047) d = 2047;
    if (d < 1) d = 1;
    return (u16)(2048 - d);
}

static u16 wave_reg(int note)
{
    /* wave tone = 65536 / (2048 - x)  ->  x = 2048 - 65536/f */
    int oct = note / 12, sem = note % 12;
    int d = note_div[sem] >> 1;          /* 65536/f = (131072/f)/2 */
    if (oct < 4) d <<= (4 - oct);
    else if (oct > 4) d >>= (oct - 4);
    if (d > 2047) d = 2047;
    if (d < 1) d = 1;
    return (u16)(2048 - d);
}

/* ---------------------------------------------------------------------- */
/*  sound effects                                                          */
/* ---------------------------------------------------------------------- */
typedef struct {
    u8  kind;       /* 0 = wave sweep, 1 = noise */
    s16 n0, n1;     /* start / end note (wave) or shift clock (noise) */
    u8  frames;     /* duration */
    u8  vol;        /* 0..15 (wave uses 0..3 scaled) */
    u8  width;      /* noise: 0 = 15 bit, 1 = 7 bit (metallic) */
} SfxDef;

static const SfxDef sfx_def[SFX_COUNT] = {
    /*  kind  n0    n1    fr  vol  w */
    {   0,   72,   72,    3,  2,  0 },  /* MOVE     */
    {   0,   72,   84,    7,  3,  0 },  /* SELECT   */
    {   0,   72,   60,    7,  3,  0 },  /* BACK     */
    {   0,   48,   41,   11,  3,  0 },  /* DENY     */
    {   1,    5,    9,    6,  8,  1 },  /* DEAL     */
    {   1,    4,    8,    5,  9,  1 },  /* FLIP     */
    {   0,   88,   95,    5,  2,  0 },  /* CHIP     */
    {   0,   72,   91,   18,  3,  0 },  /* WIN      */
    {   0,   64,   45,   22,  3,  0 },  /* LOSE     */
    {   0,   67,   67,   12,  2,  0 },  /* PUSH     */
    {   0,   79,  103,   26,  3,  0 },  /* BJ       */
    {   1,    2,    9,   16, 10,  0 },  /* BUST     */
    {   0,   84,   72,   14,  3,  0 },  /* ALERT    */
    {   0,   96,   96,    4,  3,  0 },  /* PULSE    */
    {   0,   76,   88,   10,  3,  0 },  /* GOOD     */
    {   0,   52,   40,   16,  3,  0 },  /* BAD      */
    {   0,   84,  100,   20,  3,  0 },  /* CASH     */
    {   1,    1,   11,   10, 11,  1 },  /* GLITCH   */
    {   0,   40,   80,   24,  3,  0 },  /* IMPLANT  */
    {   1,    9,   11,   14,  7,  0 },  /* HEARTBEAT*/
};

static struct {
    const SfxDef *d;
    int t;
} sfx_st;

void sfx(int id)
{
    if (id < 0 || id >= SFX_COUNT) return;
    const SfxDef *d = &sfx_def[id];
    sfx_st.d = d;
    sfx_st.t = 0;
    if (d->kind == 0) {
        REG_SND3SEL  = 0x80;                      /* re-arm the wave channel */
        REG_SND3CNT  = (2 << 13);                 /* 100% volume */
        REG_SND3FREQ = wave_reg(d->n0) | 0x8000;
    } else {
        REG_SND4CNT  = (d->vol << 12) | (0 << 11) | (2 << 8);
        REG_SND4FREQ = (d->n0 << 4) | (d->width << 3) | 4 | 0x8000;
    }
}

static void sfx_tick(void)
{
    if (!sfx_st.d) return;
    const SfxDef *d = sfx_st.d;
    sfx_st.t++;
    if (sfx_st.t >= d->frames) {
        sfx_st.d = 0;
        REG_SND3CNT = 0;          /* silence it, but leave the channel armed */
        return;
    }
    if (d->kind == 0) {
        int n = d->n0 + ((d->n1 - d->n0) * sfx_st.t) / d->frames;
        int v = 2 - (2 * sfx_st.t) / d->frames;         /* 100 -> 50 -> 25% */
        REG_SND3CNT  = ((v < 0 ? 3 : (v == 0 ? 3 : (v == 1 ? 2 : 1))) << 13);
        REG_SND3FREQ = wave_reg(n);
    } else {
        int s = d->n0 + ((d->n1 - d->n0) * sfx_st.t) / d->frames;
        REG_SND4FREQ = (s << 4) | (d->width << 3) | 4;
    }
}

/* ---------------------------------------------------------------------- */
/*  sequencer                                                              */
/* ---------------------------------------------------------------------- */
/*  Row bytes: 0 = continue, 1 = note off, >=2 = note (octave*12+semi)+1   */
#define R_  0
#define R_X 1
#define N(o,s) ((o)*12 + (s) + 2)

/* semitones */
enum { Cn=0, Cs, Dn, Ds, En, Fn, Fs, Gn, Gs, An, As, Bn };

/* --- NEON RAIN : Am - F - C - G, rain-soaked synthwave ----------------- */
static const u8 nr_lead[64] = {
    N(5,An),R_,N(5,Cn+12),R_, N(5,En+7),R_,R_,R_,  N(5,An),R_,N(6,Cn),R_, R_,R_,R_X,R_,
    N(5,Fn),R_,N(5,An),R_,    N(6,Cn),R_,R_,R_,    N(5,Fn),R_,N(5,An),R_, R_,R_,R_X,R_,
    N(5,Cn),R_,N(5,En),R_,    N(5,Gn),R_,R_,R_,    N(5,Cn),R_,N(5,En),R_, R_,R_,R_X,R_,
    N(5,Gn),R_,N(5,Bn),R_,    N(6,Dn),R_,R_,R_,    N(5,Bn),R_,N(5,Gn),R_, R_,R_,R_X,R_,
};
static const u8 nr_bass[64] = {
    N(2,An),R_,R_,R_, N(2,An),R_,N(3,An),R_, N(2,An),R_,R_,R_, N(2,En),R_,R_,R_,
    N(2,Fn),R_,R_,R_, N(2,Fn),R_,N(3,Fn),R_, N(2,Fn),R_,R_,R_, N(2,Cn),R_,R_,R_,
    N(2,Cn),R_,R_,R_, N(2,Cn),R_,N(3,Cn),R_, N(2,Cn),R_,R_,R_, N(2,Gn),R_,R_,R_,
    N(2,Gn),R_,R_,R_, N(2,Gn),R_,N(3,Gn),R_, N(2,Dn),R_,R_,R_, N(2,Gn),R_,R_,R_,
};
static const u8 nr_drum[64] = {
    2,R_,R_,R_, 4,R_,R_,2,  R_,R_,2,R_, 4,R_,3,R_,
    2,R_,R_,R_, 4,R_,R_,2,  R_,R_,2,R_, 4,R_,3,3,
    2,R_,R_,R_, 4,R_,R_,2,  R_,R_,2,R_, 4,R_,3,R_,
    2,R_,R_,R_, 4,R_,R_,2,  R_,R_,2,R_, 4,3,3,3,
};

/* --- THE SHOE : sparse, tense, one note at a time ---------------------- */
static const u8 ts_lead[64] = {
    N(6,An),R_,R_,R_, R_,R_,R_,R_, N(6,Gs),R_,R_,R_, R_,R_,R_,R_,
    R_,R_,R_,R_,      N(6,En),R_,R_,R_, R_,R_,R_,R_, R_,R_,R_X,R_,
    N(6,Fn),R_,R_,R_, R_,R_,R_,R_, N(6,En),R_,R_,R_, R_,R_,R_,R_,
    R_,R_,N(6,Dn),R_, R_,R_,R_,R_, N(6,Cn),R_,R_,R_, R_,R_,R_X,R_,
};
static const u8 ts_bass[64] = {
    N(2,An),R_,R_,R_, R_,R_,R_,R_, N(2,An),R_,R_,R_, R_,R_,N(2,Gn),R_,
    N(2,Fn),R_,R_,R_, R_,R_,R_,R_, N(2,Fn),R_,R_,R_, R_,R_,N(2,En),R_,
    N(2,Dn),R_,R_,R_, R_,R_,R_,R_, N(2,Dn),R_,R_,R_, R_,R_,N(2,Cn),R_,
    N(2,En),R_,R_,R_, R_,R_,R_,R_, N(2,En),R_,R_,R_, R_,R_,R_X,R_,
};
static const u8 ts_drum[64] = {
    2,R_,R_,R_, R_,R_,R_,R_, 3,R_,R_,R_, R_,R_,R_,R_,
    2,R_,R_,R_, R_,R_,R_,R_, 3,R_,R_,R_, R_,R_,R_,R_,
    2,R_,R_,R_, R_,R_,R_,R_, 3,R_,R_,R_, R_,R_,R_,R_,
    2,R_,R_,R_, R_,R_,R_,R_, 3,R_,R_,R_, R_,R_,3,3,
};

/* --- OVERCLOCK : the count is hot, adrenaline ------------------------- */
static const u8 oc_lead[64] = {
    N(6,An),N(6,Cn),N(6,En),N(6,An), N(6,En),N(6,Cn),N(6,An),N(5,En),
    N(6,An),N(6,Cn),N(6,En),N(7,An), N(6,En),N(6,Cn),N(6,An),R_,
    N(6,Fn),N(6,An),N(6,Cn),N(6,Fn), N(6,Cn),N(6,An),N(6,Fn),N(5,Cn),
    N(6,Fn),N(6,An),N(6,Cn),N(7,Fn), N(6,Cn),N(6,An),N(6,Fn),R_,
    N(6,Gn),N(6,Bn),N(6,Dn),N(6,Gn), N(6,Dn),N(6,Bn),N(6,Gn),N(5,Dn),
    N(6,Gn),N(6,Bn),N(6,Dn),N(7,Gn), N(6,Dn),N(6,Bn),N(6,Gn),R_,
    N(6,En),N(6,Gs),N(6,Bn),N(6,En), N(6,Bn),N(6,Gs),N(6,En),N(5,Bn),
    N(6,En),N(6,Gs),N(6,Bn),N(7,En), N(7,En),N(6,Bn),N(6,Gs),N(6,En),
};
static const u8 oc_bass[64] = {
    N(2,An),R_,N(2,An),R_, N(3,An),R_,N(2,An),R_, N(2,An),R_,N(2,An),R_, N(2,Gn),R_,N(2,En),R_,
    N(2,Fn),R_,N(2,Fn),R_, N(3,Fn),R_,N(2,Fn),R_, N(2,Fn),R_,N(2,Fn),R_, N(2,En),R_,N(2,Cn),R_,
    N(2,Gn),R_,N(2,Gn),R_, N(3,Gn),R_,N(2,Gn),R_, N(2,Gn),R_,N(2,Gn),R_, N(2,Fn),R_,N(2,Dn),R_,
    N(2,En),R_,N(2,En),R_, N(3,En),R_,N(2,En),R_, N(2,En),R_,N(2,En),R_, N(2,Bn),R_,N(2,En),R_,
};
static const u8 oc_drum[64] = {
    2,R_,3,R_, 4,R_,3,R_, 2,R_,3,R_, 4,R_,3,3,
    2,R_,3,R_, 4,R_,3,R_, 2,R_,3,R_, 4,3,3,3,
    2,R_,3,R_, 4,R_,3,R_, 2,R_,3,R_, 4,R_,3,3,
    2,R_,3,R_, 4,R_,3,R_, 2,3,3,3,   4,3,3,3,
};

/* --- ASHES : the cost of it all --------------------------------------- */
static const u8 as_lead[64] = {
    N(5,Dn),R_,R_,R_, R_,R_,R_,R_, N(5,Fn),R_,R_,R_, R_,R_,R_,R_,
    N(5,An),R_,R_,R_, R_,R_,R_,R_, N(5,Gn),R_,R_,R_, R_,R_,R_X,R_,
    N(5,Cn),R_,R_,R_, R_,R_,R_,R_, N(5,En),R_,R_,R_, R_,R_,R_,R_,
    N(5,Gn),R_,R_,R_, R_,R_,R_,R_, N(5,Fn),R_,R_,R_, R_,R_,R_X,R_,
};
static const u8 as_bass[64] = {
    N(2,Dn),R_,R_,R_, R_,R_,R_,R_, R_,R_,R_,R_, R_,R_,R_,R_,
    N(2,As),R_,R_,R_, R_,R_,R_,R_, R_,R_,R_,R_, R_,R_,R_,R_,
    N(2,Fn),R_,R_,R_, R_,R_,R_,R_, R_,R_,R_,R_, R_,R_,R_,R_,
    N(2,Cn),R_,R_,R_, R_,R_,R_,R_, R_,R_,R_,R_, R_,R_,R_X,R_,
};
static const u8 as_drum[64] = { 0 };

/* --- VAULT : the last table ------------------------------------------- */
static const u8 vt_lead[64] = {
    N(6,Dn),R_,N(6,An),R_, N(6,Fn),R_,N(6,Dn),R_, N(6,Cs),R_,N(6,An),R_, R_,R_,R_X,R_,
    N(6,Dn),R_,N(6,An),R_, N(7,Dn),R_,N(6,An),R_, N(6,Fn),R_,N(6,Dn),R_, R_,R_,R_X,R_,
    N(6,As),R_,N(6,Fn),R_, N(6,Dn),R_,N(6,As),R_, N(6,An),R_,N(6,Fn),R_, R_,R_,R_X,R_,
    N(6,Gn),R_,N(6,Dn),R_, N(6,As),R_,N(6,Gn),R_, N(6,Cs),R_,N(6,En),R_, R_,R_,R_X,R_,
};
static const u8 vt_bass[64] = {
    N(2,Dn),R_,R_,N(2,Dn), R_,R_,N(2,An),R_, N(2,Dn),R_,R_,N(2,Dn), R_,R_,N(2,Cs),R_,
    N(2,Dn),R_,R_,N(2,Dn), R_,R_,N(2,An),R_, N(2,Dn),R_,R_,N(2,Dn), R_,R_,N(2,En),R_,
    N(2,As),R_,R_,N(2,As), R_,R_,N(2,Fn),R_, N(2,As),R_,R_,N(2,As), R_,R_,N(2,Dn),R_,
    N(2,Gn),R_,R_,N(2,Gn), R_,R_,N(2,Dn),R_, N(2,An),R_,R_,N(2,An), R_,R_,N(2,Cs),R_,
};
static const u8 vt_drum[64] = {
    2,R_,R_,R_, R_,R_,4,R_, 2,R_,R_,2, R_,R_,4,3,
    2,R_,R_,R_, R_,R_,4,R_, 2,R_,R_,2, R_,R_,4,3,
    2,R_,R_,R_, R_,R_,4,R_, 2,R_,R_,2, R_,R_,4,3,
    2,R_,R_,R_, R_,R_,4,R_, 2,3,3,3,   4,3,3,3,
};

typedef struct {
    const u8 *lead, *bass, *drum;
    u8 speed;          /* frames per row */
    u8 lead_duty;      /* 0..3 */
    u8 lead_vol, bass_vol;
} Song;

static const Song songs[MUS_COUNT] = {
    { 0, 0, 0, 0, 0, 0, 0 },
    { nr_lead, nr_bass, nr_drum, 9,  2,  7,  9 },   /* NEON RAIN */
    { ts_lead, ts_bass, ts_drum, 11, 1,  6,  8 },   /* THE SHOE  */
    { oc_lead, oc_bass, oc_drum, 5,  2,  8, 10 },   /* OVERCLOCK */
    { as_lead, as_bass, as_drum, 14, 0,  5,  7 },   /* ASHES     */
    { vt_lead, vt_bass, vt_drum, 7,  3,  8, 10 },   /* VAULT     */
};

static struct {
    int  song;
    int  row, tick;
    int  intensity;
    int  on;
} mus;

void snd_init(void)
{
    REG_SNDSTAT   = 0x80;                 /* master enable */
    REG_SNDDSCNT  = 0x0002;               /* PSG at 100%   */
    REG_SNDDMGCNT = 0xFF77;               /* all channels, both sides, vol 7 */
    REG_SNDBIAS   = 0xC200;

    /* wave RAM: a soft saw for effects */
    REG_SND3SEL = 0x40;                   /* select bank 1, write bank 0 */
    for (int i = 0; i < 8; i++)  REG_WAVE_RAM[i] = 0x1133 + i * 0x2222;
    REG_SND3SEL = 0x80;                   /* enable, play bank 0 */
    REG_SND3CNT = (2 << 13);
    mus.song = MUS_NONE;
}

void mus_play(int song)
{
    if (mus.song == song) return;
    mus.song = song;
    mus.row = 0; mus.tick = 0; mus.on = (song != MUS_NONE);
    if (!mus.on) {
        REG_SND1CNT = 0; REG_SND2CNT = 0;
    }
}

void mus_stop(void)
{
    mus.on = 0; mus.song = MUS_NONE;
    REG_SND1CNT = 0; REG_SND2CNT = 0; REG_SND4CNT = 0;
}

void mus_set_intensity(int level) { mus.intensity = level; }

static void mus_tick(void)
{
    if (!mus.on || mus.song <= 0 || mus.song >= MUS_COUNT) return;
    const Song *s = &songs[mus.song];
    int speed = s->speed;
    if (mus.intensity >= 2) speed = speed * 3 / 4;
    if (speed < 2) speed = 2;

    if (mus.tick++ < speed) return;
    mus.tick = 0;

    u8 l = s->lead[mus.row];
    u8 b = s->bass[mus.row];
    u8 d = s->drum ? s->drum[mus.row] : 0;

    int lv = s->lead_vol + (mus.intensity ? 2 : 0);
    if (lv > 15) lv = 15;

    if (l == R_X) { REG_SND1CNT = 0; }
    else if (l >= 2) {
        REG_SND1SWEEP = 0;
        REG_SND1CNT   = (lv << 12) | (0 << 11) | (2 << 8) | (s->lead_duty << 6);
        REG_SND1FREQ  = sq_reg(l - 2) | 0x8000;
    }

    if (b == R_X) { REG_SND2CNT = 0; }
    else if (b >= 2) {
        REG_SND2CNT  = (s->bass_vol << 12) | (0 << 11) | (3 << 8) | (2 << 6);
        REG_SND2FREQ = sq_reg(b - 2) | 0x8000;
    }

    if (d >= 2 && !sfx_st.d) {
        static const u8 dvol[5] = { 0, 0, 12, 7, 10 };
        static const u8 dshift[5] = { 0, 0, 7, 3, 5 };
        static const u8 dwidth[5] = { 0, 0, 0, 1, 0 };
        REG_SND4CNT  = (dvol[d] << 12) | (0 << 11) | (1 << 8);
        REG_SND4FREQ = (dshift[d] << 4) | (dwidth[d] << 3) | 6 | 0x8000;
    }

    mus.row = (mus.row + 1) & 63;
}

void snd_update(void)
{
    mus_tick();
    sfx_tick();
}
