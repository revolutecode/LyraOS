/* SPDX-License-Identifier: 0BSD */
/* LyraOS — Linker script for x86-64 Limine kernel */

OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

/* Limine loads the kernel at a high-half virtual address. */
/* The physical load address is decided by the bootloader. */
KERNEL_VIRT_BASE = 0xFFFFFFFF80000000;

PHDRS
{
    text    PT_LOAD FLAGS((1 << 0) | (1 << 2));  /* PF_X | PF_R  */
    rodata  PT_LOAD FLAGS((1 << 2));             /* PF_R         */
    data    PT_LOAD FLAGS((1 << 1) | (1 << 2));  /* PF_W | PF_R  */
}

SECTIONS
{
    . = KERNEL_VIRT_BASE;

    /* --- Limine requests (must be scanned by the bootloader) --- */
    .limine_requests_start : {
        KEEP(*(.limine_requests_start))
    } :text

    .limine_requests : {
        KEEP(*(.limine_requests))
    } :text

    .limine_requests_end : {
        KEEP(*(.limine_requests_end))
    } :text

    /* --- Executable code --- */
    .text : ALIGN(0x1000) {
        *(.text .text.*)
    } :text

    /* --- Read-only data --- */
    .rodata : ALIGN(0x1000) {
        *(.rodata .rodata.*)
    } :rodata

    /* --- Read-write data --- */
    .data : ALIGN(0x1000) {
        *(.data .data.*)
    } :data

    .bss : ALIGN(0x1000) {
        *(COMMON)
        *(.bss .bss.*)
    } :data

    /* Discard unwanted sections */
    /DISCARD/ : {
        *(.eh_frame)
        *(.note .note.*)
        *(.comment)
    }
}
