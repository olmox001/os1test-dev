#ifndef _KERNEL_DRIVERS_H
#define _KERNEL_DRIVERS_H

#include <libkernel/types.h>
#include <core/ipc.h> /* provides struct reg_msg */

/* Generic Driver Framework Init */
void driver_console_init(void);
void driver_irq_init(void);
void driver_timer_init(void);

/* Message-Based Driver Protocol (Phase 2) */
struct hw_driver {
    const char *name;
    int (*init)(void);
    int (*dispatch)(const struct reg_msg *msg, struct reg_msg *reply);
};

/* Unified API for registering and sending messages to drivers */
int driver_register(struct hw_driver *drv);
int driver_dispatch(const char *name, const struct reg_msg *msg, struct reg_msg *reply);

/* Driver-specific registration entry points called in register_drivers() */
void pl011_driver_register(void);
void uart_16550_driver_register(void);
void virtio_blk_driver_register(void);

#endif /* _KERNEL_DRIVERS_H */
