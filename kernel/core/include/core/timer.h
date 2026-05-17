#ifndef _KERNEL_TIMER_H
#define _KERNEL_TIMER_H

#include <core/sched.h>

void timer_init(void);
struct pt_regs *timer_handler(struct pt_regs *regs);

#endif
