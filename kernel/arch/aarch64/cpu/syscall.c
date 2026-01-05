/*
 * kernel/arch/aarch64/cpu/syscall.c
 * System Call Handler
 */
#include <kernel/graphics.h>
#include <kernel/printk.h>
#include <kernel/sched.h>
#include <kernel/string.h>
#include <kernel/types.h>

extern volatile uint64_t jiffies;

extern int compositor_get_window_by_pid(int pid);
extern void compositor_window_write(int win_id, const char *buf, size_t count);

long sys_get_time(void) { return (long)jiffies; }

/* Syscall Implementations */
long sys_write(int fd, const char *buf, size_t count);
long sys_read(int fd, char *buf, size_t count);
void sys_exit(int status);

extern char uart_getc(void);

extern int keyboard_read_char_nonblock(void);

long sys_read(int fd, char *buf, size_t count) {
  if (fd != 0 || count == 0)
    return 0;

  while (1) {
    /* Priority 1: Virtual Keyboard (VirtIO-Input) */
    int c = keyboard_read_char_nonblock();
    if (c >= 0) {
      buf[0] = (char)c;
      /* pr_info("sys_read: got kbd char %d\n", c); */
      return 1;
    }

    /* Priority 2: Serial Input (UART) */
    /* TODO: Implement non-blocking UART check. For now, skip to avoid blocking
     * shell. */

    /* Wait for interrupt (Power Save) */
    /* This will wake up when VirtIO interrupt fires (key pressed) */
    __asm__ volatile("wfe");
  }
}

long sys_write(int fd, const char *buf, size_t count) {
  /* stdout (1) or stderr (2) */
  if (fd == 1 || fd == 2) {
    /* Try to find window for current process */
    int pid = current_process ? current_process->pid : 0;
    int win_id = compositor_get_window_by_pid(pid);

    /* Debug window lookup */
    if (count > 0 && buf[0] == '\n')
      pr_info("sys_write: pid=%d win_id=%d\n", pid, win_id);

    if (win_id >= 0) {
      compositor_window_write(win_id, buf, count);
      return count;
    }
  }

  // Fallback: Write to UART/Console
  for (size_t i = 0; i < count; i++) {
    printk("%c", buf[i]);
  }
  return count;
}

void sys_exit(int status) {
  pr_info("\nProcess exited with status %d\n", status);
  // In a real kernel: schedule next process, free resources
  // Here: Halt
  while (1) {
    __asm__ __volatile__("wfe");
  }
}

/* Handler */
extern void local_irq_enable(void);

struct pt_regs *syscall_handler(struct pt_regs *frame) {
  /* Enable Interrupts to allow Preemption and I/O */
  local_irq_enable();

  uint64_t esr;
  __asm__ __volatile__("mrs %0, esr_el1" : "=r"(esr));

  /* Check Exception Class checking bits 31:26 */
  uint32_t ec = (esr >> 26) & 0x3F;

  if (ec != 0x15) {
    uint64_t far;
    __asm__ __volatile__("mrs %0, far_el1" : "=r"(far));
    pr_err("USER FAULT: ESR=0x%lx (EC=0x%x) FAR=0x%lx ELR=0x%lx\n", esr, ec,
           far, frame->elr);

    sys_exit(-1);
    return frame;
  }

  /* Argument parsing (x0-x7 args, x8 syscall num) */
  uint64_t syscall_num = frame->regs[8];

  uint64_t arg0 = frame->regs[0];
  uint64_t arg1 = frame->regs[1];
  uint64_t arg2 = frame->regs[2];

  switch (syscall_num) {
  case 63: /* READ */
    frame->regs[0] = sys_read((int)arg0, (char *)arg1, (size_t)arg2);
    break;
  case 64: /* WRITE */
    frame->regs[0] = sys_write((int)arg0, (const char *)arg1, (size_t)arg2);
    break;
  case 169: /* GET_TIME */
    frame->regs[0] = sys_get_time();
    break;
  case 93: /* EXIT */
    pr_info("SYS_EXIT: status=%ld\n", arg0);
    sys_exit((int)arg0);
    break;
  case 200: /* DRAW (Custom) - Draw to backbuffer */
    /* args: x, y, w, h, color */
    {
      uint32_t x = (uint32_t)frame->regs[0];
      uint32_t y = (uint32_t)frame->regs[1];
      uint32_t w = (uint32_t)frame->regs[2];
      uint32_t h = (uint32_t)frame->regs[3];
      uint32_t color = (uint32_t)frame->regs[4];
      graphics_fill_rect(x, y, w, h, color);
      frame->regs[0] = 0;
    }
    break;
  case 201: /* FLUSH (Custom) - Swap buffers */
    graphics_swap_buffers();
    frame->regs[0] = 0;
    break;
  case 210: /* CREATE_WINDOW */
    /* args: x, y, w, h, title_ptr */
    {
      int x = (int)frame->regs[0];
      int y = (int)frame->regs[1];
      int w = (int)frame->regs[2];
      int h = (int)frame->regs[3];
      const char *title = (const char *)frame->regs[4];
      int pid = current_process ? current_process->pid : 0;
      pr_info("sys_create_window: current_pid=%d req_pid=%d\n", pid, pid);
      frame->regs[0] = compositor_create_window(x, y, w, h, title, pid);
    }
    break;
  case 211: /* WINDOW_DRAW */
    /* args: window_id, x, y, w, h, color */
    {
      int win_id = (int)frame->regs[0];
      int x = (int)frame->regs[1];
      int y = (int)frame->regs[2];
      int w = (int)frame->regs[3];
      int h = (int)frame->regs[4];
      uint32_t color = (uint32_t)frame->regs[5];
      compositor_draw_rect(win_id, x, y, w, h, color);
      frame->regs[0] = 0;
    }
    break;
  case 212: /* COMPOSITOR_RENDER */
    compositor_render();
    frame->regs[0] = 0;
    break;
  default:
    pr_warn("Unknown syscall: %ld\n", syscall_num);
    frame->regs[0] = -1;
    break;
  }

  return frame;
}
