/*
 * kernel/arch/amd64/drivers/pic_pit.c
 * Legacy 8259 PIC and 8253 PIT initialization for x86-64
 */
#include <kernel/types.h>
#include <arch/arch.h>
#include <arch/amd64_internal.h>
#include <kernel/printk.h>

#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1

#define PIT_CH0   0x40
#define PIT_CMD   0x43

extern volatile uint64_t jiffies;
extern void timer_tick(void); /* generic scheduler tick */

void pic_init(void) {
  /* Remap PIC IRQs 0-15 to interrupts 32-47 */
  outb(PIC1_CMD, 0x11); /* ICW1: Init + ICW4 */
  outb(PIC2_CMD, 0x11);
  
  outb(PIC1_DATA, 0x20); /* ICW2: PIC1 vector offset 32 */
  outb(PIC2_DATA, 0x28); /* ICW2: PIC2 vector offset 40 */
  
  outb(PIC1_DATA, 0x04); /* ICW3: PIC1 has slave on IRQ2 */
  outb(PIC2_DATA, 0x02); /* ICW3: PIC2 cascade identity */
  
  outb(PIC1_DATA, 0x01); /* ICW4: 8086 mode */
  outb(PIC2_DATA, 0x01);
  
  /* Mask all except IRQ0 (Timer) and IRQ1 (Keyboard) */
  outb(PIC1_DATA, 0xFC);
  outb(PIC2_DATA, 0xFF);
  
  pr_info("PIC Initialized. IRQs 0-1 mapped to 32-33.\n");
}

extern struct pt_regs *kernel_timer_tick(struct pt_regs *regs);

struct pt_regs *amd64_timer_interrupt(struct pt_regs *regs) {
  pic_send_eoi(0);
  return kernel_timer_tick(regs);
}

void pit_init(void) {
  /* Frequency = 1193182 / divisor. For 100 Hz, divisor = 11932 */
  uint16_t divisor = 11932;
  outb(PIT_CMD, 0x36); /* Channel 0, lobyte/hibyte, square wave */
  outb(PIT_CH0, (uint8_t)(divisor & 0xFF));
  outb(PIT_CH0, (uint8_t)((divisor >> 8) & 0xFF));
  
  pr_info("PIT Initialized (100 Hz).\n");
}

void pic_send_eoi(uint8_t irq) {
  if (irq >= 8) {
    outb(PIC2_CMD, 0x20);
  }
  outb(PIC1_CMD, 0x20);
}

struct pt_regs *amd64_keyboard_interrupt(struct pt_regs *regs) {
  uint8_t scancode = inb(0x60);
  /* TODO: Send scancode to keyboard driver / generic input queue */
  (void)scancode;
  pic_send_eoi(1);
  return regs;
}
