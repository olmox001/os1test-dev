#ifndef _KERNEL_SCHED_H
#define _KERNEL_SCHED_H

#include <kernel/types.h>

#define PROCESS_NAME_MAX 32
#define STACK_SIZE 4096

/* Task Context (Saved Registers) */
/* Full Register State (Matching stack layout in exception.S) */
struct pt_regs {
  uint64_t regs[31]; /* x0-x30 */
  uint64_t unused;   /* Padding to align to 16 bytes (32 * 8 = 256) */
  uint64_t elr;      /* 256 */
  uint64_t spsr;     /* 264 */
  uint64_t sp_el0;   /* 272 */
  uint64_t padding;  /* 280-288 */
};

/* Process Control Block */
struct process {
  uint32_t pid;
  char name[PROCESS_NAME_MAX];

  /* Memory */
  uint64_t *page_table;  /* Physical address of TTBR0_EL1 */
  uint64_t kernel_stack; /* Kernel stack top */

  /* Context for switching */
  struct pt_regs *context;

  /* User Context (Initial) */
  uint64_t user_entry;
  uint64_t user_stack;

  /* State */
  int state;
  struct process *next; /* Linked list */
};

/* Process States */
#define PROC_UNUSED 0
#define PROC_CREATED 1
#define PROC_RUNNING 2
#define PROC_ZOMBIE 3

/* API */
extern struct process *current_process;
struct process *process_create(const char *name);
int process_load_elf(struct process *proc, const char *path);
void start_user_process(struct process *proc);
struct pt_regs *schedule(struct pt_regs *regs);

/* Exception Handlers */
struct pt_regs *syscall_handler(struct pt_regs *frame);
struct pt_regs *irq_handler(struct pt_regs *frame);
struct pt_regs *sync_handler(struct pt_regs *frame);

#endif
