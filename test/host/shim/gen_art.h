/* EVERYTHING COUNTS - a card counting story for the Game Boy Advance
 * Copyright (C) 2026 Godless-1
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#ifndef GEN_ART_H
#define GEN_ART_H
#include "gba.h"
extern const u16 g_palette[256];
extern const u8 pip_spike[81], pip_pulse[81], pip_shard[81], pip_wire[81];
extern const u8 sigil_halcyon[315], sigil_kestrel[208];
#define SIGIL_HALCYON_W 21
#define SIGIL_HALCYON_H 15
#define SIGIL_KESTREL_W 16
#define SIGIL_KESTREL_H 13
#define RAMP_LEN 24
#define RAMP_SKY 16
#define RAMP_CYAN 40
#define RAMP_MAG 64
#define RAMP_GOLD 88
#define RAMP_GREEN 112
#define RAMP_VIOLET 136
#define RAMP_ALERT 160
#define RAMP_CHROME 184
#define RAMP_CARD 208
#define C_VOID 0
#define C_BLACK 1
#define C_INDIGO 2
#define C_PANEL 3
#define C_PANELHI 4
#define C_GRID 5
#define C_DIM 6
#define C_MID 7
#define C_TEXT 8
#define C_WHITE 9
#define C_CYAN 10
#define C_CYAND 11
#define C_MAG 12
#define C_MAGD 13
#define C_AMBER 14
#define C_GREEN 15
#define C_ORANGE 232
#define C_ICE 233
#define C_PURPLE 234
#define C_TEAL 235
#define C_BLOOD 236
#define C_PALE 237
#define C_DVIOLET 238
#define C_CANDY 239
#define C_LASER 240
#define C_MINT 241
#define C_SODIUM 242
#define C_FELT_SH 243
#define C_FELT_MD 244
#define C_FELT_TL 245
#define C_FELT_DP 246
#define C_FELT_CR 247
#define C_ANIM0 248
#endif
