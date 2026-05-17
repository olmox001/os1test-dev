#include <core/drivers.h>
#include <core/hal.h>
#include <core/printk.h>

void driver_timer_init(void) {
    arch_timer_init();
    pr_info("%s", "Timer driver initialized via HAL\n");
}
