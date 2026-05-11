/*
 * kernel/arch/amd64/boot/start.S
 * AMD64 Kernel Entry Point - Self-booting via Multiboot2
 */

.set MB2_MAGIC,        0xE85250D6
.set MB2_ARCH_I386,    0
.set HEADER_LENGTH,    40
.set CHECKSUM,         0x17ADAF2A

.section .multiboot
.align 8
mb2_header_start:
    .long MB2_MAGIC
    .long MB2_ARCH_I386
    .long HEADER_LENGTH
    .long CHECKSUM

    /* Entry Address Tag (Type 5) */
    .align 8
    .short 5
    .short 0
    .long 12
    .long _start_32

    /* End Tag */
    .align 8
    .short 0
    .short 0
    .long 8
mb2_header_end:

.code32
.section .text.boot
.global _start_32
_start_32:
    cli
    movl %ebx, (mb_info_ptr_phys)

    /* Transition to 64-bit */
    movl $boot_pml4, %edi
    xorl %eax, %eax
    movl $4096, %ecx
    rep stosl

    movl $boot_pml4, %edi
    movl $boot_pdpt, %eax
    orl  $0x3, %eax
    movl %eax, (%edi)

    movl $boot_pdpt, %edi
    movl $boot_pd, %eax
    orl  $0x3, %eax
    movl %eax, (%edi)

    movl $boot_pd, %edi
    movl $0x83, %eax
    movl $512, %ecx
.fill_pd:
    movl %eax, (%edi)
    addl $0x200000, %eax
    addl $8, %edi
    loop .fill_pd

    movl %cr4, %eax
    orl  $(1 << 5), %eax
    movl %eax, %cr4

    movl $boot_pml4, %eax
    movl %eax, %cr3

    movl $0xC0000080, %ecx
    rdmsr
    orl  $(1 << 8), %eax
    wrmsr

    movl %cr0, %eax
    orl  $(1 << 31), %eax
    movl %eax, %cr0

    lgdt (boot_gdtr_phys)
    ljmp $0x08, $_start_64

.code64
.global _start_64
_start_64:
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    movabsq $_stack_top, %rsp
    movq %rsp, %rbp

    /* Clear BSS */
    movabsq $__bss_start, %rdi
    movabsq $__bss_end, %rcx
    subq %rdi, %rcx
    shr $3, %rcx
    xorq %rax, %rax
    rep stosq

    lgdt (boot_gdtr)
    movq (mb_info_ptr), %rdi
    call kernel_main

.halt:
    hlt
    jmp .halt

.section .data
.align 8
.global mb_info_ptr
mb_info_ptr:
mb_info_ptr_phys: .quad 0

.align 16
boot_gdt:
    .quad 0x0000000000000000
    .quad 0x00AF9A000000FFFF
    .quad 0x00CF92000000FFFF
    .quad 0x00CFF2000000FFFF
    .quad 0x00AFFA000000FFFF
boot_gdt_end:

.global boot_gdtr
boot_gdtr:
boot_gdtr_phys:
    .word boot_gdt_end - boot_gdt - 1
    .quad boot_gdt

.section .bss
.align 4096
.global boot_pml4
boot_pml4: .skip 4096
boot_pdpt: .skip 4096
boot_pd:   .skip 4096

.align 4096
.global _stack_bottom
_stack_bottom:
    .skip 0x20000
.global _stack_top
_stack_top:
