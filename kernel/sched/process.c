#include <kernel/pmm.h>
#include <kernel/printk.h>
#include <kernel/sched.h>
#include <kernel/string.h>
#include <kernel/vmm.h>

#define MAX_PROCESSES 16
static struct process *process_pool[MAX_PROCESSES];
static int process_count = 0;
static int current_pid = -1; /* Index in pool */

struct process *current_process = NULL;

struct process *process_create(const char *name) {
  struct process *proc = (struct process *)pmm_alloc_page();
  if (!proc)
    return NULL;

  memset(proc, 0, 4096);
  strcpy(proc->name, name);

  /* Assign PID and add to pool */
  if (process_count < MAX_PROCESSES) {
    proc->pid = process_count + 1; // 1-based PID
    process_pool[process_count++] = proc;
  } else {
    pr_err("Process pool full!\n");
    return NULL;
  }

  proc->page_table = vmm_create_pgd();

  /* Allocate and Setup Kernel Stack */
  void *kstack_page = pmm_alloc_page();
  proc->kernel_stack = (uint64_t)kstack_page + 4096;

  /* Initial frame on kernel stack */
  proc->context =
      (struct pt_regs *)(proc->kernel_stack - sizeof(struct pt_regs));
  memset(proc->context, 0, sizeof(struct pt_regs));

  proc->state = PROC_CREATED;
  return proc;
}

/* Assembly helper to jump to user mode */
extern void enter_user_mode(uint64_t entry, uint64_t sp, uint64_t ksp);

/*
 * This is called for the FIRST run of a process.
 * We don't return from here.
 * We must set it as 'current'.
 */
void start_user_process(struct process *proc) {
  pr_info("Starting process '%s' at 0x%lx (SP: 0x%lx)\n", proc->name,
          proc->user_entry, proc->user_stack);

  current_process = proc;
  /* Find index for current_pid */
  for (int i = 0; i < process_count; i++) {
    if (process_pool[i] == proc) {
      current_pid = i;
      break;
    }
  }

  uint64_t ttbr0 = virt_to_phys(proc->page_table);
  __asm__ __volatile__("msr ttbr0_el1, %0" ::"r"(ttbr0));
  __asm__ __volatile__("tlbi vmalle1is");
  __asm__ __volatile__("dsb ish");
  __asm__ __volatile__("isb");

  proc->state = PROC_RUNNING;
  enter_user_mode(proc->user_entry, proc->user_stack, proc->kernel_stack);
}

/*
 * Schedule Next Process
 * Called from Timer Interrupt Handler.
 * 'regs' points to the stack where registers were saved by exception entry.
 * We modify 'regs' in-place to effect valid context switch upon 'eret'.
 */
struct pt_regs *schedule(struct pt_regs *regs) {
  if (process_count == 0)
    return regs;

  /* Save Current Context pointer */
  if (current_process) {
    current_process->context = regs;
    current_process->state = PROC_RUNNING;
  }

  /* Pick Next (Round Robin) */
  current_pid = (current_pid + 1) % process_count;
  struct process *next = process_pool[current_pid];
  current_process = next;

  /* Switch Page Table */
  uint64_t ttbr0 = virt_to_phys(next->page_table);
  __asm__ __volatile__("msr ttbr0_el1, %0" ::"r"(ttbr0));
  __asm__ __volatile__("tlbi vmalle1is\n"
                       "dsb ish\n"
                       "isb");

  return next->context;
}
