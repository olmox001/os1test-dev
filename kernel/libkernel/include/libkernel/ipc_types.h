/*
 * libkernel/include/libkernel/ipc_types.h
 *
 * Kernel-private IPC type definitions.
 * These are ABI-level structs shared between kernel and user space.
 * The user-facing copy lives in user/sys/include/ipc.h — both must
 * remain byte-identical for ABI compatibility.
 *
 * This header uses only libkernel/types.h and NEVER pulls in userland
 * headers, preventing the namespace leakage documented in plan_fix.md A1.
 */

#ifndef _LIBKERNEL_IPC_TYPES_H
#define _LIBKERNEL_IPC_TYPES_H

#include <libkernel/types.h>

/* ---------------------------------------------------------------
   PID-based IPC message types (sys_send / sys_recv)
   --------------------------------------------------------------- */
#define IPC_TYPE_RAW     0   /* Raw binary payload */
#define IPC_TYPE_INPUT   1   /* Keyboard input event */
#define IPC_TYPE_MOUSE   2   /* Mouse movement/click */
#define IPC_TYPE_NOTIFY  3   /* Notification string */
#define IPC_TYPE_RPC     4   /* Generic RPC call */

/* ---------------------------------------------------------------
   Registry-queue IPC message types (sys_reg_ipc_send/recv)
   These are used for driver command routing (Phase 2 / Plan 9 style).
   --------------------------------------------------------------- */

/* MMIO device commands */
#define REG_MSG_MMIO_READ    0x10
#define REG_MSG_MMIO_WRITE   0x11

/* PCI bus commands */
#define REG_MSG_PCI_PROBE    0x20
#define REG_MSG_PCI_READ_CFG 0x21
#define REG_MSG_PCI_WRITE_CFG 0x22

/* Block device commands */
#define REG_MSG_BLK_READ     0x30
#define REG_MSG_BLK_WRITE    0x31
#define REG_MSG_BLK_FLUSH    0x32

/* GPU commands */
#define REG_MSG_GPU_BLIT     0x40
#define REG_MSG_GPU_FLUSH    0x41
#define REG_MSG_GPU_MODE     0x42

/* Generic status reply */
#define REG_MSG_ACK          0xFF
#define REG_MSG_NAK          0xFE

/* ---------------------------------------------------------------
   PID-based IPC message (payload = 256 bytes)
   --------------------------------------------------------------- */
struct ipc_message {
    int32_t  from;         /* Source PID (filled by kernel) */
    int32_t  type;         /* Message type (IPC_TYPE_*) */
    uint64_t data1;
    uint64_t data2;
    char     payload[256];
} __attribute__((packed));

/* ---------------------------------------------------------------
   Registry IPC message (smaller, for per-key queues)
   --------------------------------------------------------------- */
struct reg_msg {
    int32_t  from;         /* Sender PID (filled by kernel on send) */
    int32_t  type;         /* REG_MSG_* */
    uint64_t d0, d1;       /* Generic data words */
    char     payload[64];
} __attribute__((packed));

#endif /* _LIBKERNEL_IPC_TYPES_H */
