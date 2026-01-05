/*
 * kernel/drivers/timer/timer.c
 * ARM Generic Timer driver
 *
 * Uses EL1 virtual timer (CNTV) - works in QEMU -kernel mode
 */
#include <drivers/gic.h>
#include <drivers/timer.h>
#include <kernel/list.h>
#include <kernel/printk.h>
#include <kernel/types.h>

/* Timer frequency */
uint64_t timer_freq;

/* System tick counter */
volatile uint64_t jiffies = 0;

/* Software timer list */
static LIST_HEAD(timer_list);

/*
 * Read counter frequency
 */
static inline uint64_t read_cntfrq(void) {
  uint64_t freq;
  __asm__ __volatile__("mrs %0, cntfrq_el0" : "=r"(freq));
  return freq;
}

/*
 * Read virtual counter
 */
static inline uint64_t read_cntvct(void) {
  uint64_t cnt;
  __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(cnt));
  return cnt;
}

/*
 * Set virtual timer compare value (EL1 virtual timer)
 */
static inline void write_cntv_cval(uint64_t val) {
  __asm__ __volatile__("msr cntv_cval_el0, %0" ::"r"(val));
}

/*
 * Set virtual timer control (EL1 virtual timer)
 */
static inline void write_cntv_ctl(uint64_t val) {
  __asm__ __volatile__("msr cntv_ctl_el0, %0" ::"r"(val));
}

#include <kernel/sched.h>

/*
 * Timer interrupt handler (Internal C handler called by GIC)
 * Now accepts regs for Preemption.
 */
struct pt_regs *timer_handler(struct pt_regs *regs) {
  uint64_t next;
  struct timer *t, *tmp;

  /* Increment jiffies */
  jiffies++;

  /* Schedule next timer interrupt */
  next = read_cntvct() + (timer_freq / HZ);
  write_cntv_cval(next);

  /* Process expired software timers */
  list_for_each_entry_safe(t, tmp, &timer_list, list) {
    if (jiffies >= t->expires) {
      list_del(&t->list);
      t->pending = false;
      if (t->callback)
        t->callback(t->data);
    }
  }

  /* Call Scheduler for Preemption and return resulting context */
  return schedule(regs);
}

/* Legacy static handler used by irq_register?
   No, we call timer_handler directly from GIC now.
   We can remove the static wrapper or keep it for compatibility if other IRQs
   use it. But GIC logic above handles IRQ 27 explicitly.
*/

/*
 * Initialize timer (called once on boot CPU)
 */
void timer_init(void) {
  timer_freq = read_cntfrq();

  pr_info("Timer: Frequency %lu Hz\n", timer_freq);
  pr_info("Timer: System tick rate %d Hz\n", HZ);

  /* Register virtual timer interrupt (IRQ 27 on QEMU virt) */
  /* We handle IRQ 27 explicitly in gic.c dispatch to pass regs */
  gic_enable_irq(IRQ_TIMER_VIRT);
}

/*
 * Initialize timer on each CPU
 * NOTE: Timer disabled temporarily - causes SError in QEMU -kernel mode
 * Timer works correctly with UEFI boot (./build_iso.sh run)
 */
void timer_init_percpu(void) {
  uint64_t next;

  /* Set up first timer interrupt using virtual timer */
  next = read_cntvct() + (timer_freq / HZ);
  write_cntv_cval(next);

  /* Enable virtual timer (ENABLE=1, IMASK=0) */
  write_cntv_ctl(1);

  /* Enable virtual timer IRQ in GIC (PPI, already per-CPU) */
  gic_enable_irq(IRQ_TIMER_VIRT);

  pr_info(
      "Timer: Per-CPU virtual timer enabled (IRQ %d). Next: 0x%lx, Ctl: 0x1\n",
      IRQ_TIMER_VIRT, next);
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

/*
 * Initialize a software timer
 */
void timer_setup(struct timer *t, timer_callback_t callback, void *data) {
  INIT_LIST_HEAD(&t->list);
  t->callback = callback;
  t->data = data;
  t->pending = false;
}

/*
 * Add a software timer
 */
void timer_add(struct timer *t, uint64_t expires) {
  t->expires = expires;
  t->pending = true;
  list_add_tail(&t->list, &timer_list);
}

/*
 * Delete a software timer
 */
void timer_del(struct timer *t) {
  if (t->pending) {
    list_del(&t->list);
    t->pending = false;
  }
}

/*
 * Check if timer is pending
 */
bool timer_pending(struct timer *t) { return t->pending; }
