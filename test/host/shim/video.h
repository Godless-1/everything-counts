/* EVERYTHING COUNTS - a card counting story for the Game Boy Advance
 * Copyright (C) 2026 Godless-1
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#ifndef VIDEO_H
#define VIDEO_H
#include "gba.h"
#include "gen_art.h"
#include "gen_font.h"
void vid_plot(int,int,u8);
void vid_hline(int,int,int,u8);
void vid_vline(int,int,int,u8);
void vid_rect(int,int,int,int,u8);
void vid_frame_rect(int,int,int,int,u8);
void vid_vgrad(int,int,int,int,int,int,int);
void vid_hgrad(int,int,int,int,int,int,int);
void vid_blit4(int,int,int,int,const u8*,const u8*);
void vid_blit4_scaled(int,int,int,int,const u8*,const u8*,int);
int  vid_text(int,int,const char*,u8);
int  vid_text_len(const char*);
char *str_int(char *dst, int v);
char *str_cat(char *dst, const char *s);
#endif
