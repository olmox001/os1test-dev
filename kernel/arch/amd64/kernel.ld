/*
 * kernel/arch/amd64/kernel.ld — x86-64 Linker Script
 * Link as 32-bit ELF to satisfy Multiboot loaders.
 */

OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start_32)

SECTIONS {
    . = 1M;
    __kernel_start = .;

    .multiboot : ALIGN(8) {
        KEEP(*(.multiboot))
    }

    .text : ALIGN(4096) {
        _text = .;
        *(.text.boot)
        *(.text*)
        _etext = .;
    }

    .rodata : ALIGN(4096) {
        _rodata = .;
        *(.rodata*)
        . = ALIGN(8);
        __ktests_start = .;
        KEEP(*(.ktests))
        __ktests_end = .;
        _erodata = .;
    }

    .data : ALIGN(4096) {
        _data = .;
        *(.data*)
        _edata = .;
    }

    .bss : ALIGN(4096) {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    }
    __kernel_end = .;

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}

PROVIDE(__kernel_size = __kernel_end - __kernel_start);
