/*
 * kernel/arch/amd64/drivers/uart_16550.c
 * COM1 Serial Driver (16550 compatible)
 *
 * Provides uart_init/uart_putc/uart_puts for serial console output.
 */
#include <libkernel/types.h>
#include <core/arch.h>
#include <hal/arch/amd64_internal.h>
#include <hal/drivers/uart.h>

#define COM1_PORT 0x3F8

/* Register offsets */
#define UART_THR 0 /* Transmit Holding Register */
#define UART_RBR 0 /* Receive Buffer Register */
#define UART_IER 1 /* Interrupt Enable Register */
#define UART_FCR 2 /* FIFO Control Register */
#define UART_LCR 3 /* Line Control Register */
#define UART_MCR 4 /* Modem Control Register */
#define UART_LSR 5 /* Line Status Register */
#define UART_DLL 0 /* Divisor Latch Low (when DLAB=1) */
#define UART_DLH 1 /* Divisor Latch High (when DLAB=1) */

/* LSR bits */
#define LSR_DATA_READY  0x01
#define LSR_THRE        0x20 /* Transmit Holding Register Empty */

void uart_init(void) {
  /* Disable interrupts */
  outb(COM1_PORT + UART_IER, 0x00);

  /* Enable DLAB (set baud rate divisor) */
  outb(COM1_PORT + UART_LCR, 0x80);

  /* Set divisor: 115200 baud (divisor = 1) */
  outb(COM1_PORT + UART_DLL, 0x01);
  outb(COM1_PORT + UART_DLH, 0x00);

  /* 8 bits, no parity, one stop bit (8N1) */
  outb(COM1_PORT + UART_LCR, 0x03);

  /* Enable FIFO, clear them, 14-byte threshold */
  outb(COM1_PORT + UART_FCR, 0xC7);

  /* IRQs enabled, RTS/DSR set */
  outb(COM1_PORT + UART_MCR, 0x0B);

  /* Enable receive data available interrupt */
  outb(COM1_PORT + UART_IER, 0x01);
}

void uart_putc(char c) {
  /* Wait for transmit buffer to be empty */
  while ((inb(COM1_PORT + UART_LSR) & LSR_THRE) == 0)
    ;
  outb(COM1_PORT + UART_THR, (uint8_t)c);
}

void uart_puts(const char *s) {
  while (*s) {
    if (*s == '\n')
      uart_putc('\r');
    uart_putc(*s++);
  }
}

char uart_getc(void) {
  /* Wait for data to be ready */
  while ((inb(COM1_PORT + UART_LSR) & LSR_DATA_READY) == 0) {
    arch_idle();
  }
  return (char)inb(COM1_PORT + UART_RBR);
}

int uart_getc_nonblock(void) {
  if (inb(COM1_PORT + UART_LSR) & LSR_DATA_READY) {
    return (int)inb(COM1_PORT + UART_RBR);
  }
  return -1;
}

void uart_puthex(uint64_t val) {
  static const char hex[] = "0123456789abcdef";
  char buf[17];
  int i;

  buf[16] = '\0';
  for (i = 15; i >= 0; i--) {
    buf[i] = hex[val & 0xF];
    val >>= 4;
  }

  uart_puts("0x");
  uart_puts(buf);
}

/* Phase 2: Message-Based Driver Dispatch Implementation */
#include <core/drivers.h>
#include <core/ipc.h>

static int uart_16550_dispatch(const struct reg_msg *msg, struct reg_msg *reply) {
  if (!msg || !reply)
    return -1;

  reply->from = 0; /* Kernel */
  reply->type = REG_MSG_NAK;
  reply->d0 = 0;
  reply->d1 = 0;

  switch (msg->type) {
    case REG_MSG_MMIO_WRITE: {
      if (msg->d1 > 0) {
        /* Message d1 holds length; treat d0 as a pointer to string */
        const char *s = (const char *)msg->d0;
        if (s) {
          uart_puts(s);
          reply->type = REG_MSG_ACK;
        }
      } else {
        /* Treat d0 as a single character */
        uart_putc((char)msg->d0);
        reply->type = REG_MSG_ACK;
      }
      break;
    }
    case REG_MSG_MMIO_READ: {
      if (msg->d1 == 0) {
        /* Blocking read */
        reply->d0 = (uint64_t)uart_getc();
        reply->type = REG_MSG_ACK;
      } else {
        /* Non-blocking read */
        int c = uart_getc_nonblock();
        if (c >= 0) {
          reply->d0 = (uint64_t)c;
          reply->type = REG_MSG_ACK;
        } else {
          reply->type = REG_MSG_NAK;
        }
      }
      break;
    }
    default:
      return -1;
  }
  return 0;
}

static struct hw_driver uart_16550_driver = {
  .name = "uart",
  .init = NULL, /* Pre-initialized during early boot */
  .dispatch = uart_16550_dispatch
};

void uart_16550_driver_register(void) {
  driver_register(&uart_16550_driver);
}

