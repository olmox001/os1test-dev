#ifndef _OS1_ABI_H
#define _OS1_ABI_H

/*
 * OS1 Microkernel ABI — Syscall Numbers
 * Matches user/arch/aarch64/syscall.S and user/arch/amd64/syscall.S exactly.
 */

/* POSIX-compatible I/O */
#define SYS_READ               63
#define SYS_WRITE              64
#define SYS_EXIT               93
#define SYS_GETTIME            169
#define SYS_GETPID             172

/* GRAPHICS — kernel-resident, migrating to user-space compositor */
#define SYS_DRAW               200
#define SYS_FLUSH              201
#define SYS_CREATE_WINDOW      210
#define SYS_WINDOW_DRAW        211
#define SYS_COMPOSITOR_RENDER  212
#define SYS_WINDOW_BLIT        213
#define SYS_WINDOW_SET_FLAGS   214
#define SYS_DESTROY_WINDOW     215

/* CORE */
#define SYS_SBRK               216
#define SYS_SPAWN              220
#define SYS_KILL               221
#define SYS_GETPROCS           222
#define SYS_YIELD              223

/* IPC (PID-based) */
#define SYS_SEND               230
#define SYS_RECV               231
#define SYS_SET_FOCUS          232
#define SYS_TRY_RECV           32

/* PROCESS */
#define SYS_WAIT               247

/* REGISTRY + VFS */
#define SYS_REGISTRY           250
#define SYS_FILE_WRITE         251
#define SYS_FILE_READ          252
#define SYS_SET_FONT           253
#define SYS_LIST_DIR           254
#define SYS_CHDIR              255
#define SYS_GETCWD             256

/* REGISTRY IPC QUEUES — Plan 9 style per-key message queues */
#define SYS_REG_IPC_SEND       260
#define SYS_REG_IPC_RECV       261
#define SYS_REG_IPC_PEND       262
#define SYS_REG_LIST           263

#endif /* _OS1_ABI_H */
