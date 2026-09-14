/* EVERYTHING COUNTS - a card counting story for the Game Boy Advance
 * Copyright (C) 2026 Godless-1
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
/* host shim: the same integer contract as the GBA header, no hardware */
#ifndef GBA_H
#define GBA_H
#include <stdint.h>
typedef uint8_t  u8;   typedef int8_t   s8;
typedef uint16_t u16;  typedef int16_t  s16;
typedef uint32_t u32;  typedef int32_t  s32;
typedef uint64_t u64;  typedef int64_t  s64;
#define IWRAM_CODE
#define IWRAM_DATA
#define ALIGN4
#define INLINE static inline
#define SCREEN_W 240
#define SCREEN_H 160
#define REG_VCOUNT 0
#define REG_TM2D   0
void *ec_memset(void *d, int c, u32 n);
void *ec_memcpy(void *d, const void *s, u32 n);
u32   ec_strlen(const char *s);
#endif
