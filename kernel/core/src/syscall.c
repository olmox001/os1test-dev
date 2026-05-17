/*
 * kernel/core/src/syscall.c
 * OS1 Microkernel Syscall Dispatcher
 * Syscall numbers match user/arch/ARCH/syscall.S (see user/sys/include/abi.h).
 */

#include <libkernel/types.h>
#include <core/printk.h>
#include <core/sched.h>
#include <core/cpu.h>
#include <core/syscall.h>
#include <core/registry.h>
#include <core/graphics.h>
#include <core/abi.h>

/* Prototype */
struct pt_regs *kernel_syscall_dispatcher(struct pt_regs *frame);

struct pt_regs *kernel_syscall_dispatcher(struct pt_regs *frame) {
    uint64_t num  = pt_regs_syscall_num(frame);
    uint64_t a0   = pt_regs_arg(frame, 0);
    uint64_t a1   = pt_regs_arg(frame, 1);
    uint64_t a2   = pt_regs_arg(frame, 2);
    uint64_t a3   = pt_regs_arg(frame, 3);
    uint64_t a4   = pt_regs_arg(frame, 4);
    uint64_t a5   = pt_regs_arg(frame, 5);

    long ret = -1;

    switch (num) {

    /* ── CORE ─────────────────────────────────────────────── */
    case SYS_EXIT:
        sys_exit((int)a0);
        return schedule(frame);

    case SYS_YIELD:
        return schedule(frame);

    case SYS_GETPID:
        ret = sys_get_pid();
        break;

    case SYS_GETTIME:
        ret = sys_get_time();
        break;

    case SYS_SBRK:
        ret = (long)sys_sbrk((intptr_t)a0);
        break;

    case SYS_SPAWN:
        ret = sys_spawn((const char *)a0);
        break;

    case SYS_KILL:
        ret = sys_kill((int)a0);
        break;

    case SYS_WAIT:
        ret = sys_wait((int)a0);
        break;

    case SYS_GETPROCS:
        ret = sys_getprocs((struct ps_info *)a0, (size_t)a1);
        break;

    /* ── IPC (PID-based) ──────────────────────────────────── */
    case SYS_SEND:
        ret = sys_ipc_send((int)a0, (void *)a1);
        if (ret == 0) return schedule(frame);
        break;

    case SYS_RECV:
        ret = sys_ipc_recv((int)a0, (void *)a1);
        return schedule(frame);

    case SYS_TRY_RECV:
        ret = sys_ipc_try_recv((int)a0, (void *)a1);
        break;

    case SYS_SET_FOCUS: {
        extern int keyboard_focus_pid;
        keyboard_focus_pid = (int)a0;
        ret = 0;
        break;
    }

    /* ── I/O ──────────────────────────────────────────────── */
    case SYS_READ:
        ret = sys_read_fd((int)a0, (void *)a1, (size_t)a2);
        break;

    case SYS_WRITE:
        ret = sys_write_fd((int)a0, (const void *)a1, (size_t)a2);
        break;

    /* ── VFS / Filesystem ─────────────────────────────────── */
    case SYS_FILE_READ:
        ret = sys_file_read((const char *)a0, (void *)a1,
                            (int)a2, (int)a3);
        break;

    case SYS_FILE_WRITE:
        ret = sys_file_write((const char *)a0, (const void *)a1,
                             (int)a2, (int)a3);
        break;

    case SYS_LIST_DIR:
        ret = sys_list_dir((const char *)a0, (char *)a1, (size_t)a2);
        break;

    case SYS_CHDIR:
        ret = sys_chdir((const char *)a0);
        break;

    case SYS_GETCWD:
        ret = sys_getcwd((char *)a0, (size_t)a1);
        break;

    /* ── REGISTRY ─────────────────────────────────────────── */
    case SYS_REGISTRY:
        ret = sys_registry((int)a0, (const char *)a1, (char *)a2, (size_t)a3);
        break;

    case SYS_REG_IPC_SEND:
        ret = sys_reg_ipc_send((const char *)a0,
                               (const struct reg_msg *)a1);
        break;

    case SYS_REG_IPC_RECV:
        ret = sys_reg_ipc_recv((const char *)a0, (struct reg_msg *)a1);
        break;

    case SYS_REG_IPC_PEND:
        ret = sys_reg_ipc_pending((const char *)a0);
        break;

    case SYS_REG_LIST:
        ret = sys_reg_list((const char *)a0, (char *)a1, (size_t)a2);
        break;

    /* ── GRAPHICS (kernel-resident, migrating to user-space) ─ */
    case SYS_FLUSH:
        graphics_swap_buffers();
        ret = 0;
        break;

    case SYS_COMPOSITOR_RENDER:
        compositor_render();
        ret = 0;
        break;

    case SYS_CREATE_WINDOW:
        ret = (long)compositor_create_window((int)a0, (int)a1,
                                             (int)a2, (int)a3,
                                             (const char *)a4,
                                             current_process->pid);
        break;

    case SYS_DESTROY_WINDOW:
        compositor_destroy_window((int)a0);
        ret = 0;
        break;

    case SYS_WINDOW_DRAW:
        compositor_draw_rect((int)a0, (int)a1, (int)a2,
                             (int)a3, (int)a4, (uint32_t)a5,
                             current_process->pid);
        ret = 0;
        break;

    case SYS_WINDOW_BLIT:
        compositor_blit((int)a0, (int)a1, (int)a2,
                        (int)a3, (int)a4, (const uint32_t *)a5,
                        current_process->pid);
        ret = 0;
        break;

    case SYS_WINDOW_SET_FLAGS:
        compositor_set_window_flags((int)a0, (int)a1);
        ret = 0;
        break;

    case SYS_DRAW:
        graphics_draw_rect((uint32_t)a0, (uint32_t)a1,
                           (uint32_t)a2, (uint32_t)a3,
                           (uint32_t)a4);
        ret = 0;
        break;

    case SYS_SET_FONT:
        ret = (long)sys_set_font((void *)a0, (size_t)a1);
        break;

    /* ── POSIX fd-based VFS (Phase 3a) ──────────────────────── */
    case SYS_OPEN:
        ret = sys_open((const char *)a0, (int)a1);
        break;

    case SYS_CLOSE:
        ret = sys_close((int)a0);
        break;

    default:
        pr_warn("PID %d: Unknown syscall %lu\n",
                current_process ? (int)current_process->pid : -1,
                (unsigned long)num);
        ret = -38; /* -ENOSYS */
        break;
    }

    pt_regs_set_return(frame, (uint64_t)ret);
    return frame;
}
