@ EVERYTHING COUNTS - a card counting story for the Game Boy Advance
@ Copyright (C) 2026 Godless-1
@ SPDX-License-Identifier: AGPL-3.0-or-later
@ =========================================================================
@  EVERYTHING COUNTS  -  GBA cartridge header + C runtime startup
@ =========================================================================
        .section .crt0, "ax"
        .arm
        .align 2
        .global _start
_start:
        b       rom_header_end

@ --- 156 byte Nintendo logo area -----------------------------------------
@ Left zeroed on purpose: that data is Nintendo's trademarked boot logo and
@ we do not redistribute it.  Every emulator (mGBA, VBA-M, no$gba, NanoBoyA)
@ boots this ROM fine.  For real hardware, splice the logo in with any
@ standard `gbafix`-style utility.
        .space  156, 0

@ --- 0xA0 game title (12 bytes) -----------------------------------------
        .byte   'E','V','E','R','Y','C','O','U','N','T','S',0
@ --- 0xAC game code (4) --------------------------------------------------
        .byte   'C','E','V','E'
@ --- 0xB0 maker code (2) -------------------------------------------------
        .byte   '0','0'
@ --- 0xB2 fixed value ----------------------------------------------------
        .byte   0x96
@ --- 0xB3 main unit code -------------------------------------------------
        .byte   0x00
@ --- 0xB4 device type ----------------------------------------------------
        .byte   0x00
@ --- 0xB5 reserved (7) ---------------------------------------------------
        .space  7, 0
@ --- 0xBC software version -----------------------------------------------
        .byte   0x00
@ --- 0xBD complement check (patched post-link by tools/gbafix.py) --------
        .byte   0x00
@ --- 0xBE reserved (2) ---------------------------------------------------
        .space  2, 0

@ --- 0xC0 multiboot header (unused, kept for layout compatibility) -------
        .space  32, 0

@ --- Emulator save-type autodetect signature ----------------------------
        .align 2
        .global __save_signature
__save_signature:
        .asciz  "SRAM_V113"
        .align 2

rom_header_end:
        @ ---- switch to supervisor-free state, set up stacks -------------
        mov     r0, #0x12                @ IRQ mode, IRQ+FIQ disabled
        msr     cpsr_c, r0
        ldr     sp, =__sp_irq
        mov     r0, #0x1F                @ System mode
        msr     cpsr_c, r0
        ldr     sp, =__sp_usr

        @ ---- install our IRQ dispatcher with the BIOS -------------------
        ldr     r0, =irq_dispatch
        ldr     r1, =0x03007FFC
        str     r0, [r1]

        @ ---- copy .iwram (LMA -> IWRAM) ---------------------------------
        ldr     r0, =__iwram_lma
        ldr     r1, =__iwram_start
        ldr     r2, =__iwram_size
        bl      copy_words

        @ ---- copy .data (LMA -> EWRAM) ----------------------------------
        ldr     r0, =__data_lma
        ldr     r1, =__data_start
        ldr     r2, =__data_size
        bl      copy_words

        @ ---- zero .iwram_bss --------------------------------------------
        ldr     r1, =__iwbss_start
        ldr     r2, =__iwbss_size
        bl      zero_words

        @ ---- zero .bss ---------------------------------------------------
        ldr     r1, =__bss_start
        ldr     r2, =__bss_size
        bl      zero_words

        @ ---- waitstate control: 3/1 ROM, prefetch on ---------------------
        ldr     r0, =0x04000204
        ldr     r1, =0x4317
        strh    r1, [r0]

        @ ---- into C (thumb) ---------------------------------------------
        ldr     r0, =main
        mov     lr, pc
        bx      r0
1:      b       1b

@ r0=src r1=dst r2=byte count (multiple of 4)
copy_words:
        cmp     r2, #0
        bxeq    lr
2:      ldr     r3, [r0], #4
        str     r3, [r1], #4
        subs    r2, r2, #4
        bgt     2b
        bx      lr

@ r1=dst r2=byte count
zero_words:
        cmp     r2, #0
        bxeq    lr
        mov     r3, #0
3:      str     r3, [r1], #4
        subs    r2, r2, #4
        bgt     3b
        bx      lr

@ =========================================================================
@  IRQ dispatcher.  Acks the interrupt, updates the BIOS IntrWait flags and
@  calls the C hook (void hook(void)).  Nested interrupts stay disabled.
@ =========================================================================
        .section .iwram, "ax"
        .arm
        .align 2
        .global irq_dispatch
irq_dispatch:
        ldr     r3, =0x04000200          @ IE / IF block
        ldr     r1, [r3]                 @ r1 = IE | IF<<16
        and     r1, r1, r1, lsr #16      @ pending & enabled
        strh    r1, [r3, #2]             @ ack hardware IF

        ldr     r2, =0x03FFFFF8          @ BIOS IntrWait flags
        ldrh    r0, [r2]
        orr     r0, r0, r1
        strh    r0, [r2]

        ldr     r2, =irq_flags           @ publish flags to the C side
        ldr     r0, [r2]
        orr     r0, r0, r1
        str     r0, [r2]

        ldr     r2, =irq_handler_c
        ldr     r2, [r2]
        cmp     r2, #0
        bxeq    lr

        stmfd   sp!, {r4, lr}
        mov     lr, pc
        bx      r2
        ldmfd   sp!, {r4, lr}
        bx      lr

        .section .iwram_bss, "aw", %nobits
        .align 2
        .global irq_handler_c
irq_handler_c:
        .space 4
        .global irq_flags
irq_flags:
        .space 4
