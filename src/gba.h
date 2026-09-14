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
 *  gba.h - bare-metal Game Boy Advance hardware definitions
 * ===================================================================== */
#ifndef GBA_H
#define GBA_H

typedef unsigned char      u8;
typedef signed   char      s8;
typedef unsigned short     u16;
typedef signed   short     s16;
typedef unsigned int       u32;
typedef signed   int       s32;
typedef unsigned long long u64;
typedef signed long long   s64;

#define IWRAM_CODE __attribute__((section(".iwram"), long_call))
#define IWRAM_DATA __attribute__((section(".iwram_bss")))
#define ALIGN4     __attribute__((aligned(4)))
#define INLINE     static inline __attribute__((always_inline))

#define REG(addr)  (*(volatile u16 *)(addr))
#define REG32(addr)(*(volatile u32 *)(addr))

/* ---- display ---------------------------------------------------------- */
#define REG_DISPCNT   REG32(0x04000000)
#define REG_DISPSTAT  REG(0x04000004)
#define REG_VCOUNT    REG(0x04000006)
#define REG_BG0CNT    REG(0x04000008)
#define REG_BG2CNT    REG(0x0400000C)
#define REG_BG2PA     REG(0x04000020)
#define REG_BG2PB     REG(0x04000022)
#define REG_BG2PC     REG(0x04000024)
#define REG_BG2PD     REG(0x04000026)
#define REG_BG2X      REG32(0x04000028)
#define REG_BG2Y      REG32(0x0400002C)
#define REG_BLDCNT    REG(0x04000050)
#define REG_BLDALPHA  REG(0x04000052)
#define REG_BLDY      REG(0x04000054)
#define REG_MOSAIC    REG(0x0400004C)

#define DCNT_MODE3    0x0003
#define DCNT_MODE4    0x0004
#define DCNT_MODE5    0x0005
#define DCNT_PAGE     0x0010
#define DCNT_OAM_1D   0x0040
#define DCNT_BLANK    0x0080
#define DCNT_BG0      0x0100
#define DCNT_BG2      0x0400
#define DCNT_OBJ      0x1000

#define DSTAT_VBL_IRQ 0x0008

/* ---- interrupts ------------------------------------------------------- */
#define REG_IE        REG(0x04000200)
#define REG_IF        REG(0x04000202)
#define REG_IME       REG(0x04000208)
#define IRQ_VBLANK    0x0001
#define IRQ_HBLANK    0x0002
#define IRQ_TIMER0    0x0008
#define IRQ_TIMER1    0x0010

/* ---- keys ------------------------------------------------------------- */
#define REG_KEYINPUT  REG(0x04000130)
#define KEY_A       0x0001
#define KEY_B       0x0002
#define KEY_SELECT  0x0004
#define KEY_START   0x0008
#define KEY_RIGHT   0x0010
#define KEY_LEFT    0x0020
#define KEY_UP      0x0040
#define KEY_DOWN    0x0080
#define KEY_R       0x0100
#define KEY_L       0x0200
#define KEY_ANY     0x03FF

/* ---- timers ----------------------------------------------------------- */
#define REG_TM0D      REG(0x04000100)
#define REG_TM0CNT    REG(0x04000102)
#define REG_TM1D      REG(0x04000104)
#define REG_TM1CNT    REG(0x04000106)
#define REG_TM2D      REG(0x04000108)
#define REG_TM2CNT    REG(0x0400010A)

/* ---- DMA -------------------------------------------------------------- */
typedef struct { const void *src; void *dst; u32 cnt; } DmaReg;
#define DMA ((volatile DmaReg *)0x040000B0)
#define DMA_ENABLE   0x80000000
#define DMA_32       0x04000000
#define DMA_16       0x00000000
#define DMA_SRC_FIX  0x01000000
#define DMA_NOW      0x00000000

/* ---- memory ----------------------------------------------------------- */
#define MEM_PAL_BG   ((volatile u16 *)0x05000000)
#define MEM_VRAM     ((volatile u16 *)0x06000000)
#define MEM_VRAM_PAGE1 ((volatile u16 *)0x0600A000)
#define MEM_OAM      ((volatile u16 *)0x07000000)
#define MEM_SRAM     ((volatile u8  *)0x0E000000)

#define SCREEN_W 240
#define SCREEN_H 160

/* ---- sound ------------------------------------------------------------ */
#define REG_SND1SWEEP REG(0x04000060)
#define REG_SND1CNT   REG(0x04000062)
#define REG_SND1FREQ  REG(0x04000064)
#define REG_SND2CNT   REG(0x04000068)
#define REG_SND2FREQ  REG(0x0400006C)
#define REG_SND3SEL   REG(0x04000070)
#define REG_SND3CNT   REG(0x04000072)
#define REG_SND3FREQ  REG(0x04000074)
#define REG_SND4CNT   REG(0x04000078)
#define REG_SND4FREQ  REG(0x0400007C)
#define REG_SNDDMGCNT REG(0x04000080)
#define REG_SNDDSCNT  REG(0x04000082)
#define REG_SNDSTAT   REG(0x04000084)
#define REG_SNDBIAS   REG(0x04000088)
#define REG_WAVE_RAM  ((volatile u16 *)0x04000090)

/* ---- BIOS calls ------------------------------------------------------- */
INLINE void bios_halt(void)      { asm volatile("swi 0x02" ::: "r0","r1","r2","r3","memory"); }
INLINE void bios_vblank_wait(void){ asm volatile("swi 0x05" ::: "r0","r1","r2","r3","memory"); }

static inline s32 bios_div(s32 num, s32 den)
{
    register s32 r0 asm("r0") = num;
    register s32 r1 asm("r1") = den;
    asm volatile("swi 0x06" : "+r"(r0), "+r"(r1) :: "r3");
    return r0;
}

extern volatile u32 irq_flags;
extern void (*irq_handler_c)(void);

/* RGB555 */
#define RGB(r,g,b) ((u16)(((r)&31) | (((g)&31)<<5) | (((b)&31)<<10)))
#define RGB8(r,g,b) RGB((r)>>3,(g)>>3,(b)>>3)

/* tiny freestanding helpers (no libc) */
void *ec_memset(void *d, int c, u32 n);
void *ec_memcpy(void *d, const void *s, u32 n);
u32   ec_strlen(const char *s);
void *memcpy(void *d, const void *s, u32 n);
void *memset(void *d, int c, u32 n);
void *memmove(void *d, const void *s, u32 n);

#endif /* GBA_H */
