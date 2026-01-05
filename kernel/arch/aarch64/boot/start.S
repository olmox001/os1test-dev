/*
 * kernel/arch/aarch64/boot/start.S
 * Kernel entry point
 */

.section .text.boot
.global _start

/*
 * Kernel entry point
 * Called from bootloader/QEMU with:
 *   x0 = DTB pointer
 *   x1 = 0 (reserved)
 *
 * QEMU virt machine starts kernel at EL1 directly
 */
_start:
    /* Disable interrupts */
    msr daifset, #0xf
    
    /* Get CPU ID */
    mrs x1, mpidr_el1
    and x1, x1, #0xff
    
    /* CPU 0 does primary init */
    cbz x1, primary_init
    
    /* Secondary CPUs wait */
    b secondary_init

/*
 * Primary CPU initialization
 */
primary_init:
    /* Save boot info (DTB pointer) */
    adrp x19, boot_info
    add x19, x19, :lo12:boot_info
    str x0, [x19]
    
    adrp x0, __kernel_stack_top
    add x0, x0, :lo12:__kernel_stack_top
    mov sp, x0
    
    /* Switch to SP_EL1 */
    msr spsel, #1
    /* Re-set SP for EL1 just in case (though mov sp above sets current SP) */
    mov sp, x0
    
    /* Check current exception level */
    mrs x0, CurrentEL
    lsr x0, x0, #2      /* Extract EL bits */
    
    cmp x0, #3
    beq el3_setup
    cmp x0, #2
    beq el2_setup
    /* Already at EL1, continue */
    b el1_setup

/*
 * EL3 setup (if started by firmware at EL3)
 */
el3_setup:
    /* Configure EL3 to drop to EL1 */
    mov x0, #0x531          /* SCR_EL3: RW=1, NS=1 (EL1 AArch64, non-secure) */
    msr scr_el3, x0
    
    mov x0, #0x3c5          /* SPSR: EL1h, all interrupts masked */
    msr spsr_el3, x0
    
    adrp x0, el1_setup
    add x0, x0, :lo12:el1_setup
    msr elr_el3, x0
    eret

/*
 * EL2 setup (if started by hypervisor at EL2)
 */
el2_setup:
    /* Enable EL1 access to timers */
    mrs x0, cnthctl_el2
    orr x0, x0, #3          /* EL1PCEN, EL1PCTEN */
    msr cnthctl_el2, x0
    msr cntvoff_el2, xzr    /* Virtual timer offset = 0 */
    
    /* Configure HCR_EL2 */
    mov x0, #(1 << 31)      /* RW bit: EL1 uses AArch64 */
    msr hcr_el2, x0
    
    /* SPSR_EL2: Return to EL1h with interrupts masked */
    mov x0, #0x3c5
    msr spsr_el2, x0
    
    adrp x0, el1_setup
    add x0, x0, :lo12:el1_setup
    msr elr_el2, x0
    eret

/*
 * EL1 setup
 */
el1_setup:
    /* Reload stack (in case we came from ERET) */
    adrp x0, __kernel_stack_top
    add x0, x0, :lo12:__kernel_stack_top
    mov sp, x0
    
    /* Enable FPU/SIMD (NEON) before anything else */
    /* Set CPACR_EL1.FPEN = 0b11 to enable FP/SIMD at EL0 and EL1 */
    mov x0, #(3 << 20)
    msr cpacr_el1, x0
    isb

    /* Clear BSS */
    adrp x0, __bss_start
    add x0, x0, :lo12:__bss_start
    adrp x1, __bss_end
    add x1, x1, :lo12:__bss_end
clear_bss:
    cmp x0, x1
    bge bss_done
    str xzr, [x0], #8
    b clear_bss
bss_done:

    /* Configure SCTLR_EL1 */
    /* Read current value */
    mov x0, #0
    bic x0, x0, #(1 << 12)   /* I-cache DISABLE */
    bic x0, x0, #(1 << 2)    /* D-cache DISABLE (avoid MMIO cache issues without MMU) */
    bic x0, x0, #(1 << 0)    /* MMU disabled for now */
    msr sctlr_el1, x0
    isb
    
    /* Call kernel main */
    bl kernel_main
    
    /* Should never return */
halt:
    wfe
    b halt

/*
 * Secondary CPU initialization
 */
secondary_init:
    /* Wait for spin table */
    adrp x2, cpu_spin_table
    add x2, x2, :lo12:cpu_spin_table
secondary_wait:
    wfe
    ldr x3, [x2, x1, lsl #3]    /* Load entry for this CPU */
    cbz x3, secondary_wait
    
    /* Setup stack for this CPU */
    adrp x4, cpu_stacks
    add x4, x4, :lo12:cpu_stacks
    ldr x0, [x4, x1, lsl #3]
    mov sp, x0
    
    /* Jump to entry point */
    br x3

/*
 * Wake secondary CPU
 */
.global cpu_wake_secondary
cpu_wake_secondary:
    /* x0 = cpu id, x1 = entry point, x2 = stack */
    adrp x3, cpu_spin_table
    add x3, x3, :lo12:cpu_spin_table
    str x1, [x3, x0, lsl #3]
    
    adrp x3, cpu_stacks
    add x3, x3, :lo12:cpu_stacks
    str x2, [x3, x0, lsl #3]
    
    sev     /* Wake sleeping CPUs */
    ret

/*
 * Data section
 */
.section .data
.balign 8

.global boot_info
boot_info:
    .quad 0

cpu_spin_table:
    .skip 8 * 8     /* 8 CPU entries */

cpu_stacks:
    .skip 8 * 8     /* 8 stack pointers */

/*
 * BSS section
 */
.section .bss
.balign 16

/* Per-CPU stacks (64KB each) */
.global __kernel_stack
__kernel_stack:
    .skip 65536 * 8     /* 8 CPUs * 64KB */
.global __kernel_stack_top
__kernel_stack_top:
