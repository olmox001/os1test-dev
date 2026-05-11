/*
 * kernel/drivers/timer/timer.c
 * ARM Generic Timer driver
 *
 * Uses EL1 virtual timer (CNTV) - works in QEMU -kernel mode
 */
#include <kernel/irq.h>
#include <drivers/timer.h>
#include <kernel/arch.h>
#include <kernel/cpu.h>
#include <kernel/list.h>
#include <kernel/printk.h>
#include <kernel/sched.h>
#include <kernel/spinlock.h>
#include <kernel/types.h>

/* Disable optimization */

#include <kernel/vmm.h>

/* Timer frequency */
/* Timer frequency */
uint64_t timer_freq;

/* System tick counter */
volatile uint64_t jiffies = 0;

/*
 * Read counter frequency
 */
static inline uint64_t read_cntfrq(void) { return arch_timer_get_freq(); }

/*
 * Read virtual counter
 */
static inline uint64_t read_cntvct(void) { return arch_timer_get_count(); }

/*
 * Set virtual timer compare value (EL1 virtual timer)
 */
static inline void write_cntv_cval(uint64_t val) {
  arch_timer_set_compare(val);
}

/*
 * Set virtual timer control (EL1 virtual timer)
 */
static inline void write_cntv_ctl(uint64_t val) {
  arch_timer_control(val);
}

/*
 * Timer interrupt handler (Internal C handler called by GIC)
 * Now accepts regs for Preemption.
 */
extern void compositor_tick(void);

/* Global panic flag set by panic() to halt all CPUs */
extern volatile int panic_flag;

extern struct pt_regs *kernel_timer_tick(struct pt_regs *regs);

struct pt_regs *timer_handler(struct pt_regs *regs) {
  struct cpu_info *cpu = get_cpu_info();

  /* Precision Tick Logic for ARM Generic Timer */
  extern uint64_t timer_tick_interval;
  extern uint64_t timer_tick_remainder;

  cpu->tick_error_acc += timer_tick_remainder;
  uint64_t interval = timer_tick_interval;
  if (cpu->tick_error_acc >= HZ) {
    interval += 1;
    cpu->tick_error_acc -= HZ;
  }

  cpu->next_tick_target += interval;
  uint64_t now = read_cntvct();

  /* Catch up logic */
  if (cpu->next_tick_target <= now) {
    cpu->next_tick_target = now + interval;
  }

  write_cntv_cval(cpu->next_tick_target);

  /* Call generic tick logic */
  return kernel_timer_tick(regs);
}

/* Legacy static handler used by irq_register?
   No, we call timer_handler directly from GIC now.
   We can remove the static wrapper or keep it for compatibility if other IRQs
   use it. But GIC logic above handles IRQ 27 explicitly.
*/

/*
 * Initialize timer (called once on boot CPU)
 */
/* Precomputed timer values */
uint64_t timer_tick_interval = 0;
uint64_t timer_tick_remainder = 0;

/*
 * Initialize timer (called once on boot CPU)
 */
void timer_init(void) {
  timer_freq = read_cntfrq();
  timer_tick_interval = timer_freq / HZ;
  timer_tick_remainder = timer_freq % HZ;

  pr_info("Timer: Frequency %lu Hz\n", timer_freq);
  pr_info("Timer: System tick rate %d Hz\n", HZ);

  /* Register virtual timer interrupt (IRQ 27 on QEMU virt) */
  /* We handle IRQ 27 explicitly in gic.c dispatch to pass regs */
  irq_enable(IRQ_TIMER);
}

/*
 * Initialize timer on each CPU
 * NOTE: Timer disabled temporarily - causes SError in QEMU -kernel mode
 * Timer works correctly with UEFI boot (./build_iso.sh run)
 */
void timer_init_percpu(void) {
  struct cpu_info *cpu = get_cpu_info();

  /* Set up first timer interrupt using virtual timer */
  cpu->next_tick_target = read_cntvct() + (timer_freq / HZ);
  cpu->tick_error_acc = 0;
  write_cntv_cval(cpu->next_tick_target);

  /* Enable virtual timer (ENABLE=1, IMASK=0) */
  write_cntv_ctl(1);

  /* Enable virtual timer IRQ in GIC (PPI, already per-CPU) */
  irq_enable(IRQ_TIMER);

  pr_info("Timer: Per-CPU virtual timer enabled (IRQ %d). Next: 0x%lx, Ctl: "
          "0x1\n",
          IRQ_TIMER, cpu->next_tick_target);
}

/*
 * Get current timer ticks
 */
uint64_t timer_get_ticks(void) { return read_cntvct(); }

/*
 * Get microseconds since boot
 */
uint64_t timer_get_us(void) {
  return (read_cntvct() * USEC_PER_SEC) / timer_freq;
}

/*
 * Delay for specified microseconds
 */
void timer_delay_us(uint64_t us) {
  uint64_t start = read_cntvct();
  uint64_t ticks = (us * timer_freq) / USEC_PER_SEC;

  while ((read_cntvct() - start) < ticks)
    ;
}

/*
 * Delay for specified milliseconds
 */
void timer_delay_ms(uint64_t ms) { timer_delay_us(ms * 1000); }

/* Software timer functions are now in kernel/core/timer.c */
