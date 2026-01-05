/*
 * kernel/arch/aarch64/cpu/cpu.c
 * CPU and exception handling for AArch64
 */
#include <drivers/gic.h>
#include <kernel/printk.h>
#include <kernel/types.h>

/* Exception frame structure */
#include <kernel/sched.h>

/* Per-CPU data */
struct cpu_info {
  uint32_t cpu_id;
  uint32_t online;
  uint64_t stack_top;
  void *current_task;
};

/* CPU info array (max 8 CPUs) */
struct cpu_info cpu_data[8];
uint32_t nr_cpus = 0;

/* External functions from assembly */
extern void exception_vectors_install(void);

/*
 * Get current CPU ID
 */
uint32_t cpu_id(void) {
  uint64_t mpidr;
  __asm__ __volatile__("mrs %0, mpidr_el1" : "=r"(mpidr));
  return mpidr & 0xFF;
}

/*
 * Initialize CPU subsystem
 */
void cpu_init(void) {
  uint32_t id = cpu_id();

  cpu_data[id].cpu_id = id;
  cpu_data[id].online = 1;

  if (id == 0) {
    nr_cpus = 1;
    pr_info("CPU: Primary core %u initialized\n", id);
  } else {
    nr_cpus++;
    pr_info("CPU: Secondary core %u online\n", id);
  }

  /* Enable FPU/SIMD (NEON) - set CPACR_EL1.FPEN = 0b11 */
  uint64_t cpacr;
  __asm__ __volatile__("mrs %0, cpacr_el1" : "=r"(cpacr));
  cpacr |= (3 << 20); /* FPEN bits [21:20] = 0b11 */
  __asm__ __volatile__("msr cpacr_el1, %0" : : "r"(cpacr));
  __asm__ __volatile__("isb");

  /* Install exception vector table */
  exception_vectors_install();

  uint64_t vbar;
  __asm__ __volatile__("mrs %0, vbar_el1" : "=r"(vbar));
  pr_info("CPU: VBAR_EL1 set to 0x%lx\n", vbar);
}

/*
 * Synchronous exception handler
 */
struct pt_regs *sync_handler(struct pt_regs *frame) {
  uint64_t esr, far, elr;
  uint32_t ec;

  if (!frame)
    return NULL;

  /* Read exception syndrome */
  __asm__ __volatile__("mrs %0, esr_el1" : "=r"(esr));
  __asm__ __volatile__("mrs %0, far_el1" : "=r"(far));
  elr = frame->elr;

  ec = (esr >> 26) & 0x3F;

  switch (ec) {
  case 0x00: /* Unknown exception */
    pr_err("Unknown exception at 0x%016lx\n", elr);
    break;

  case 0x15: /* SVC instruction (syscall) */
    return syscall_handler(frame);

  case 0x20: /* Instruction abort from lower EL */
  case 0x21: /* Instruction abort from same EL */
    pr_err("Instruction abort at 0x%016lx, FAR=0x%016lx\n", elr, far);
    break;

  case 0x24: /* Data abort from lower EL */
  case 0x25: /* Data abort from same EL */
    pr_err("Data abort at 0x%016lx, FAR=0x%016lx\n", elr, far);
    break;

  case 0x26: /* SP alignment fault */
    pr_err("SP alignment fault at 0x%016lx\n", elr);
    break;

  default:
    pr_err("Unhandled exception EC=0x%x at 0x%016lx\n", ec, elr);
    break;
  }

  if (ec != 0x15) {
    pr_err("SPSR=0x%016lx ESR=0x%016lx\n", frame->spsr, esr);
    panic("Unrecoverable exception");
  }

  return frame;
}

/*
 * System error handler
 */
struct pt_regs *serror_handler(struct pt_regs *frame) {
  uint64_t esr;
  __asm__ __volatile__("mrs %0, esr_el1" : "=r"(esr));
  pr_err("SError at ELR=0x%016lx ESR=0x%016lx\n", frame->elr, esr);
  panic("SError exception");
}

/*
 * Syscall handler is defined in syscall.c
 */
extern struct pt_regs *syscall_handler(struct pt_regs *frame);

/*
 * Enable interrupts (only IRQ, keep SError masked)
 */
void local_irq_enable(void) {
  /* Clear I bit (IRQ) only, keep A bit (SError) masked */
  __asm__ __volatile__("msr daifclr, #2" ::: "memory");
}

/*
 * Disable interrupts
 */
void local_irq_disable(void) {
  __asm__ __volatile__("msr daifset, #2" ::: "memory");
}

/*
 * Save and disable interrupts
 */
uint64_t local_irq_save(void) {
  uint64_t flags;
  __asm__ __volatile__("mrs %0, daif\n"
                       "msr daifset, #2"
                       : "=r"(flags)::"memory");
  return flags;
}

/*
 * Restore interrupt state
 */
void local_irq_restore(uint64_t flags) {
  __asm__ __volatile__("msr daif, %0" ::"r"(flags) : "memory");
}
