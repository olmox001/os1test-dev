/*
 * kernel/kernel.c
 * Main kernel initialization and entry point
 */
#include <drivers/gic.h>
#include <drivers/keyboard.h>
#include <drivers/timer.h>
#include <drivers/uart.h>
#include <drivers/virtio_blk.h>
#include <drivers/virtio_gpu.h>
#include <kernel/buffer.h>
#include <kernel/ext4.h>
#include <kernel/gpt.h>
#include <kernel/graphics.h>
#include <kernel/pmm.h>
#include <kernel/printk.h>
#include <kernel/sched.h>
#include <kernel/string.h>
#include <kernel/types.h>
#include <kernel/vmm.h>

/* Version */
#define KERNEL_VERSION_MAJOR 0
#define KERNEL_VERSION_MINOR 1
#define KERNEL_VERSION_PATCH 0
#define KERNEL_NAME "AArch64 Microkernel"

/* External symbols */
extern uint64_t boot_info;
extern void cpu_init(void);
extern void local_irq_enable(void);

/* Forward declarations */
static void print_banner(void);
static void init_memory(void);
static void init_scheduler(void);

/*
 * Kernel main entry point
 */
void kernel_main(void) {
  /* Initialize UART first for debug output */
  uart_init();

  /* Print kernel banner */
  print_banner();

  /* CPU initialization (exception vectors, per-CPU data) */
  pr_info("Initializing CPU...\n");
  cpu_init();

  /* Interrupt controller */
  pr_info("Initializing GIC...\n");
  gic_init();
  gic_init_percpu();

  /* System timer */
  pr_info("Initializing timer...\n");
  timer_init();
  timer_init_percpu();

  /* Memory management */
  pr_info("Initializing memory...\n");
  init_memory();

  /* Scheduler */
  init_scheduler();

  /* Enable interrupts */
  pr_info("Enabling interrupts...\n");
  local_irq_enable();

  pr_info("Kernel initialized successfully!\n");
  pr_info("Boot info at: 0x%016lx\n", boot_info);

  /* Main kernel loop */
  pr_info("Entering idle loop...\n");

  uint64_t last_jiffies = 0;
  while (1) {
    /* Print heartbeat every second */
    if (jiffies != last_jiffies && (jiffies % HZ) == 0) {
      pr_info("Tick: %lu seconds\n", jiffies / HZ);
      last_jiffies = jiffies;
    }

    /* Wait for interrupt */
    __asm__ __volatile__("wfi");
  }
}

/*
 * Print kernel banner
 */
static void print_banner(void) {
  printk("\n");
  printk("========================================\n");
  printk("  %s v%d.%d.%d\n", KERNEL_NAME, KERNEL_VERSION_MAJOR,
         KERNEL_VERSION_MINOR, KERNEL_VERSION_PATCH);
  printk("  Production-Ready AArch64 Kernel\n");
  printk("========================================\n");
  printk("\n");
}

/*
 * Initialize memory subsystem
 */
static void init_memory(void) {
  /* Initialize physical memory manager */
  pmm_init(NULL, 0); /* Use default 1GB memory detection */

  /* Initialize virtual memory manager */
  vmm_init();

  /* Initialize VirtIO Block Driver */
  virtio_blk_init();

  /* Initialize VirtIO GPU Driver */
  virtio_gpu_init();
  pr_info("VirtIO-GPU: Done.\n");

  /* Initialize Graphics Subsystem */
  graphics_init();

  /* Initialize GPT */
  gpt_init();
  pr_info("GPT: Done.\n");

  /* Initialize Buffer Cache */
  buffer_init();
  pr_info("Buffer: Done.\n");

  /* Initialize Ext4 */
  ext4_init();
  pr_info("Ext4: Done.\n");

  /* Initialize Keyboard */
  keyboard_init();

  /* TODO: Initialize slab allocator */
}

/*
 * Initialize scheduler (placeholder)
 */
static void init_scheduler(void) {
  pr_info("Scheduler: Initializing...\n");

  /* Initialize Compositor */
  compositor_init();

  /* Spawn Init Process (Splash) */
  struct process *init = process_create("init");
  if (init && process_load_elf(init, "/init") == 0) {
    pr_info("Scheduler: Loaded /init\n");
  }

  /* Spawn Shell Process 1 */
  struct process *shell1 = process_create("shell1");
  if (shell1 && process_load_elf(shell1, "/shell") == 0) {
    pr_info("Scheduler: Loaded /shell (1)\n");
  }

  /* Spawn Shell Process 2 */
  struct process *shell2 = process_create("shell2");
  if (shell2 && process_load_elf(shell2, "/shell") == 0) {
    pr_info("Scheduler: Loaded /shell (2)\n");
  }

  /* This function will not return if successful */
  if (init)
    start_user_process(init);
}

/*
 * Secondary CPU entry point
 */
void secondary_cpu_entry(void) {
  uint32_t cpu = 0;

  /* Get CPU ID */
  __asm__ __volatile__("mrs %0, mpidr_el1" : "=r"(cpu));
  cpu &= 0xFF;

  pr_info("Secondary CPU %u starting...\n", cpu);

  /* Initialize per-CPU state */
  cpu_init();
  gic_init_percpu();
  timer_init_percpu();

  /* Enable interrupts */
  local_irq_enable();

  pr_info("Secondary CPU %u online\n", cpu);

  /* Enter idle loop */
  while (1) {
    __asm__ __volatile__("wfi");
  }
}
