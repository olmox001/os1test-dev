/*
 * kernel/include/kernel/spinlock.h
 * Simple spinlock implementation for AArch64
 */
#ifndef _KERNEL_SPINLOCK_H
#define _KERNEL_SPINLOCK_H

#include <kernel/types.h>

typedef struct {
  volatile uint32_t lock;
} spinlock_t;

#define SPINLOCK_INIT {0}
#define DEFINE_SPINLOCK(x) spinlock_t x = SPINLOCK_INIT

static inline void spin_lock_init(spinlock_t *lock) { lock->lock = 0; }

static inline void spin_lock(spinlock_t *lock) {
  uint32_t tmp;
  __asm__ __volatile__("   sevl\n"
                       "1: wfe\n"
                       "   ldaxr   %w0, [%1]\n"
                       "   cbnz    %w0, 1b\n"
                       "   stxr    %w0, %w2, [%1]\n"
                       "   cbnz    %w0, 1b\n"
                       : "=&r"(tmp)
                       : "r"(&lock->lock), "r"(1)
                       : "memory");
}

static inline void spin_unlock(spinlock_t *lock) {
  __asm__ __volatile__("   stlr    %w0, [%1]\n"
                       :
                       : "r"(0), "r"(&lock->lock)
                       : "memory");
}

static inline int spin_trylock(spinlock_t *lock) {
  uint32_t tmp;
  __asm__ __volatile__("   ldaxr   %w0, [%1]\n"
                       "   cbnz    %w0, 1f\n"
                       "   stxr    %w0, %w2, [%1]\n"
                       "1:\n"
                       : "=&r"(tmp)
                       : "r"(&lock->lock), "r"(1)
                       : "memory");
  return tmp == 0;
}

/* IRQ-safe spinlock */
static inline void spin_lock_irqsave(spinlock_t *lock, uint64_t *flags) {
  __asm__ __volatile__("mrs %0, daif" : "=r"(*flags));
  __asm__ __volatile__("msr daifset, #3" ::: "memory");
  spin_lock(lock);
}

static inline void spin_unlock_irqrestore(spinlock_t *lock, uint64_t flags) {
  spin_unlock(lock);
  __asm__ __volatile__("msr daif, %0" ::"r"(flags) : "memory");
}

#endif /* _KERNEL_SPINLOCK_H */
