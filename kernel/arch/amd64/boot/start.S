#include <kernel/cpu.h>

.section .text.boot
.code32

/* Multiboot 1 Header */
.align 4
.long 0x1BADB002
.long 0x00000003
.long -(0x1BADB002 + 0x00000003)

/* Multiboot 2 Header */
.align 8
multiboot2_header:
    .long 0xe85250d6
    .long 0
    .long multiboot2_header_end - multiboot2_header
    .long -(0xe85250d6 + 0 + (multiboot2_header_end - multiboot2_header))

    /* Tags */
    .short 0
    .short 0
    .long 8
multiboot2_header_end:

/* PVH Header */
.section .note.elf.pvh
.align 4
    .long 4             /* Namesz */
    .long 4             /* Descsz */
    .long 18            /* Type (XEN_ELFNOTE_PHYS32_ENTRY) */
    .ascii "Xen\0"      /* Name */
    .long _start_32     /* Desc (Entry point) */

.section .text.boot
.global _start_32
_start_32:
    cli
    cld

    /* Save magic and info for later */
    movl %eax, %ebp
    movl %ebx, %edx

    /* 1. Setup temporary stack */
    movl $boot_stack_top, %esp

    /* 2. Enable PAE */
    movl %cr4, %eax
    orl $(1 << 5), %eax
    movl %eax, %cr4

    /* 3. Setup Page Tables (Identity map first 1GB) */
    /* PML4 -> PDPT -> PD (2MB pages) */
    
    /* PML4[0] = boot_pdpt */
    movl $boot_pdpt, %eax
    orl $3, %eax /* Present | RW */
    movl %eax, (boot_pml4)

    /* PDPT[0] = boot_pd */
    movl $boot_pd, %eax
    orl $3, %eax
    movl %eax, (boot_pdpt)

    /* PDPT[3] = boot_pd_high (For 4th GB / LAPIC) */
    movl $boot_pd_high, %eax
    orl $3, %eax
    movl %eax, (boot_pdpt + 24) /* Entry 3 (3 * 8 bytes) */

    /* PD[0..511] = 0, 2MB, 4MB, ... 1GB */
    movl $boot_pd, %edi
    movl $0x83, %eax /* Present | RW | PageSize */
    movl $512, %ecx
.fill_pd:
    movl %eax, (%edi)
    addl $0x200000, %eax
    addl $8, %edi
    loop .fill_pd

    /* PD_HIGH (3GB-4GB) -> 512 * 2MB pages */
    movl $boot_pd_high, %edi
    movl $0xC0000083, %eax /* Starting at 3GB, Present | RW | PageSize */
    movl $512, %ecx
.fill_pd_high:
    movl %eax, (%edi)
    addl $0x200000, %eax
    addl $8, %edi
    loop .fill_pd_high

    /* 4. Load PML4 into CR3 */
    movl $boot_pml4, %eax
    movl %eax, %cr3

    /* 5. Enable Long Mode */
    movl $0xC0000080, %ecx
    rdmsr
    orl $(1 << 8), %eax
    wrmsr

    /* 6. Enable Paging */
    movl %cr0, %eax
    orl $(1 << 31 | 1 << 16), %eax
    movl %eax, %cr0

    /* 7. Load GDT and jump to 64-bit */
    lgdt (boot_gdtr)
    pushl $0x08
    pushl $_start_64
    lret

.code64
_start_64:
    /* Segment registers to NULL */
    xorl %eax, %eax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    /* Clear BSS */
    movabsq $__bss_start, %rdi
    movabsq $__bss_end, %rcx
    subq %rdi, %rcx
    jz .bss_done
    xorq %rax, %rax
    rep stosb
.bss_done:

    /* 5. Load saved boot information into kernel variables */
    movl %ebp, %eax
    movq %rax, mb_magic(%rip)
    movl %edx, %eax
    movq %rax, mb_info_ptr(%rip)

    /* Setup initial stack (CPU 0) */
    movabsq $__kernel_stack, %rsp
    addq $131072, %rsp
    movq %rsp, %rbp

    /* Load final 64-bit GDT */
    lgdt (boot_gdtr)
    
    /* 6. Handover to C: RDI = info, RSI = magic */
    movq mb_info_ptr(%rip), %rdi
    movq mb_magic(%rip), %rsi
    call kernel_main

.halt:
    hlt
    jmp .halt

.global secondary_cpu_entry
secondary_cpu_entry:
    /* Secondary CPUs enter here from trampoline in 64-bit mode.
       Stack is already set up by trampoline. */
    cli
    
    /* Segment registers to NULL */
    xorl %eax, %eax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    /* TTBR0 for secondary core should be already set by trampoline.S
       if it loaded kernel_pgd_phys (boot_pml4). */

    /* Setup local CPU state */
    call arch_cpu_init
    
    /* Enter scheduler */
    call kernel_secondary_main

.global arch_yield
arch_yield:
    pause
    ret

.global arch_impl_idle
arch_impl_idle:
    hlt
    ret

.section .data
.align 16
boot_gdt:
    .quad 0x0000000000000000 /* Null */
    .quad 0x00af9a000000ffff /* Kernel Code 64 */
    .quad 0x00cf92000000ffff /* Kernel Data 64 */
boot_gdt_end:

boot_gdtr:
    .word boot_gdt_end - boot_gdt - 1
    .quad boot_gdt

.global mb_magic, mb_info_ptr
mb_magic:    .quad 0
mb_info_ptr: .quad 0

.global kernel_pgd_phys
kernel_pgd_phys: .quad boot_pml4

.section .bss
.align 4096
boot_stack:
    .skip 4096
boot_stack_top:

.align 4096
.global boot_pml4
boot_pml4: .skip 4096
.global boot_pdpt
boot_pdpt: .skip 4096
.global boot_pd
boot_pd:   .skip 4096
.global boot_pd_high
boot_pd_high: .skip 4096

.section .bss
.align 4096
.global __kernel_stack
__kernel_stack:
    .skip 131072 * MAX_CPUS     /* 128KB * MAX_CPUS */
.global __kernel_stack_top
__kernel_stack_top:
