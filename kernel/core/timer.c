/*
 * kernel/core/timer.c
 * Generic Timer Logic (Jiffies, Software Timers, Scheduling)
 */
#include <drivers/timer.h>
#include <kernel/arch.h>
#include <kernel/cpu.h>
#include <kernel/list.h>
#include <kernel/printk.h>
#include <kernel/sched.h>
#include <kernel/spinlock.h>

extern void compositor_tick(void);
extern volatile int panic_flag;
static uint64_t compositor_interval = 1;
#define COMPOSITOR_TARGET_FPS 30

static LIST_HEAD(timer_list);
static spinlock_t timer_lock = SPINLOCK_INIT;

/* jiffies and timer_freq are defined in drivers/timer/timer.c for AArch64
 * or in arch/amd64/platform/platform.c for AMD64. */
extern volatile uint64_t jiffies;

/* Weak stubs for arch-specific timer functions (can be overridden) */
__attribute__((weak)) void timer_init_percpu(void) {}

__attribute__((weak)) struct pt_regs *timer_handler(struct pt_regs *regs) {
  return regs;
}

struct pt_regs *kernel_timer_tick(struct pt_regs *regs) {
  /* Halt this CPU if another CPU panicked */
  if (panic_flag) {
    arch_timer_control(0);
    arch_cpu_halt();
  }

  struct cpu_info *cpu = get_cpu_info();
  cpu->tick_count++;

  /* Global jiffies incremented only by the primary core */
  if (cpu->cpu_id == 0) {
    jiffies++;
  }

  /* Process software timers and compositor on CPU 0 */
  if (cpu->cpu_id == 0) {
    struct timer *t, *tmp;
    uint64_t flags;
    spin_lock_irqsave(&timer_lock, &flags);
    list_for_each_entry_safe(t, tmp, &timer_list, list) {
      if (jiffies >= t->expires) {
        list_del(&t->list);
        t->pending = false;
        if (t->callback)
          t->callback(t->data);
      }
    }
    spin_unlock_irqrestore(&timer_lock, flags);

    /* Calculate compositor interval once */
    static int interval_init = 0;
    if (!interval_init) {
        if (HZ >= COMPOSITOR_TARGET_FPS) {
            compositor_interval = HZ / COMPOSITOR_TARGET_FPS;
        } else {
            compositor_interval = 1;
        }
        interval_init = 1;
    }

    if ((jiffies % compositor_interval) == 0) {
      compositor_tick();
    }
  }

  /* Call Scheduler for Preemption */
  return schedule(regs);
}

/* Software timer management */
void timer_setup(struct timer *t, timer_callback_t callback, void *data) {
  INIT_LIST_HEAD(&t->list);
  t->callback = callback;
  t->data = data;
  t->pending = false;
}

void timer_add(struct timer *t, uint64_t expires) {
  uint64_t flags;
  spin_lock_irqsave(&timer_lock, &flags);
  t->expires = expires;
  t->pending = true;
  list_add_tail(&t->list, &timer_list);
  spin_unlock_irqrestore(&timer_lock, flags);
}

void timer_del(struct timer *t) {
  uint64_t flags;
  spin_lock_irqsave(&timer_lock, &flags);
  if (t->pending) {
    list_del(&t->list);
    t->pending = false;
  }
  spin_unlock_irqrestore(&timer_lock, flags);
}

bool timer_pending(struct timer *t) { return t->pending; }
