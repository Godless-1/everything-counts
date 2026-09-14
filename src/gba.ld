/* SPDX-License-Identifier: AGPL-3.0-or-later */
OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

MEMORY
{
    rom   (rx)  : ORIGIN = 0x08000000, LENGTH = 32M
    iwram (rwx) : ORIGIN = 0x03000000, LENGTH = 32K
    ewram (rwx) : ORIGIN = 0x02000000, LENGTH = 256K
}

__iwram_top = ORIGIN(iwram) + LENGTH(iwram);
__sp_irq    = __iwram_top - 0x060;   /* 0x03007FA0 */
__sp_usr    = __sp_irq    - 0x0A0;   /* 0x03007F00 */

SECTIONS
{
    . = ORIGIN(rom);

    .crt0 : { KEEP(*(.crt0)) . = ALIGN(4); } > rom

    .text : {
        *(.text .text.*)
        *(.gnu.linkonce.t.*)
        . = ALIGN(4);
    } > rom

    .rodata : {
        *(.rodata .rodata.*)
        *(.gnu.linkonce.r.*)
        . = ALIGN(4);
    } > rom

    .ARM.exidx : { *(.ARM.exidx* .gnu.linkonce.armexidx.*) . = ALIGN(4); } > rom

    __iwram_lma = .;
    .iwram : AT (__iwram_lma) {
        __iwram_start = ABSOLUTE(.);
        *(.iwram .iwram.*)
        *(.text.iwram*)
        . = ALIGN(4);
        __iwram_end = ABSOLUTE(.);
    } > iwram
    __iwram_size = __iwram_end - __iwram_start;

    .iwram_bss (NOLOAD) : {
        __iwbss_start = ABSOLUTE(.);
        *(.iwram_bss .iwram_bss.*)
        . = ALIGN(4);
        __iwbss_end = ABSOLUTE(.);
    } > iwram
    __iwbss_size = __iwbss_end - __iwbss_start;

    __data_lma = __iwram_lma + __iwram_size;
    .data : AT (__data_lma) {
        __data_start = ABSOLUTE(.);
        *(.data .data.*)
        *(.gnu.linkonce.d.*)
        . = ALIGN(4);
        __data_end = ABSOLUTE(.);
    } > ewram
    __data_size = __data_end - __data_start;

    .bss (NOLOAD) : {
        __bss_start = ABSOLUTE(.);
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end = ABSOLUTE(.);
    } > ewram
    __bss_size = __bss_end - __bss_start;

    __heap_start = __bss_end;

    /DISCARD/ : { *(.comment) *(.note*) *(.ARM.attributes) }
}
