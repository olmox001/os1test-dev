/*
 * kernel/core/src/drivers.c
 * Unified Message-Based Driver Framework Implementation
 */
#include <core/drivers.h>
#include <libkernel/string.h>
#include <core/printk.h>

#define MAX_DRIVERS 16

static struct hw_driver *registered_drivers[MAX_DRIVERS];
static int registered_driver_count = 0;

int driver_register(struct hw_driver *drv) {
    if (!drv) return -1;
    if (registered_driver_count >= MAX_DRIVERS) {
        pr_err("Driver Framework: Max drivers limit reached (%d)\n", MAX_DRIVERS);
        return -1;
    }

    /* Prevent duplicate names */
    for (int i = 0; i < registered_driver_count; i++) {
        if (strcmp(registered_drivers[i]->name, drv->name) == 0) {
            pr_warn("Driver Framework: Driver '%s' already registered\n", drv->name);
            return -2;
        }
    }

    registered_drivers[registered_driver_count++] = drv;
    pr_info("Driver Framework: Registered message-based driver '%s'\n", drv->name);

    if (drv->init) {
        int err = drv->init();
        if (err != 0) {
            pr_err("Driver Framework: Init failed for driver '%s' with code %d\n", drv->name, err);
            return err;
        }
    }

    return 0;
}

int driver_dispatch(const char *name, const struct reg_msg *msg, struct reg_msg *reply) {
    if (!name || !msg || !reply) return -1;

    for (int i = 0; i < registered_driver_count; i++) {
        if (strcmp(registered_drivers[i]->name, name) == 0) {
            if (registered_drivers[i]->dispatch) {
                return registered_drivers[i]->dispatch(msg, reply);
            }
            return -2;
        }
    }

    pr_warn("Driver Framework: Dispatch failed, driver '%s' not found\n", name);
    return -1;
}
