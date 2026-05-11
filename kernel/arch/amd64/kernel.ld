/*
 * kernel/arch/amd64/kernel.ld — x86-64 Linker Script
 * Kernel loaded at 1MB by Multiboot2
 */

OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS {
    /* Multiboot2 loads at 1MB */
    . = 1M;

    .multiboot2 : ALIGN(8) {
        KEEP(*(.multiboot2))
    }

    .text : ALIGN(4096) {
        *(.text.boot)
        *(.text*)
    }

    .rodata : ALIGN(4096) {
        *(.rodata*)
    }

    .data : ALIGN(4096) {
        *(.data*)
    }

    .bss : ALIGN(4096) {
        _bss_start = .;
        *(.bss*)
        *(COMMON)
        _bss_end = .;
    }

    /* Kernel stack (defined in boot.S BSS) */
    /* _stack_bottom and _stack_top are provided by boot.S */

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}
