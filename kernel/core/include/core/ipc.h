/*
 * kernel/core/include/core/ipc.h
 *
 * Kernel-space IPC type forwarder.
 * Previously this included <ipc.h> which resolved to the userland header
 * user/sys/include/ipc.h — a namespace boundary violation (plan_fix A1).
 *
 * Now forwards to the kernel-private libkernel/ipc_types.h.
 */

#ifndef _CORE_IPC_H
#define _CORE_IPC_H

#include <libkernel/ipc_types.h>

#endif
