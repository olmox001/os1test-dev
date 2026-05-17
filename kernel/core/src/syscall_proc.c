/*
 * kernel/core/src/syscall_proc.c
 * Process, VFS, and Console Syscall Implementations
 */

#include <libkernel/types.h>
#include <core/printk.h>
#include <core/sched.h>
#include <core/vmm.h>
#include <core/kmalloc.h>
#include <core/boot_fs.h>
#include <core/syscall.h>
#include <libkernel/string.h>

/* Extern from boot_fs.c */
extern uint32_t ext4_find_inode(const char *path);
extern int      ext4_read_inode(uint32_t ino, uint64_t offset,
                                uint8_t *buf, uint32_t size);
extern int      ext4_list_dir(const char *path, char *buf, size_t size);

/* ================================================================
   Process Management
   ================================================================ */

long sys_spawn(const char *path) {
    char k_path[128];
    if (vmm_copy_string_from_user(k_path, path, sizeof(k_path)) != 0)
        return -EFAULT;

    pr_info("Syscall: SPAWN %s\n", k_path);

    const char *name = k_path;
    const char *slash = strrchr(k_path, '/');
    if (slash) name = slash + 1;

    struct process *proc = process_create(name, PROC_PRIO_USER, PROC_PERM_USER);
    if (!proc) return -ENOMEM;

    if (process_load_elf(proc, k_path) != 0)
        return -ENOENT;

    enqueue_task(proc);
    return (long)proc->pid;
}

long sys_kill(int pid) {
    return process_terminate(pid);
}

long sys_wait(int pid) {
    return process_wait(pid);
}

long sys_get_pid(void) {
    return (long)current_process->pid;
}

long sys_exit(int status) {
    pr_info("Process %d exiting (status %d)\n",
            current_process->pid, status);
    process_terminate(current_process->pid);
    return 0;
}

long sys_get_time(void) {
    extern volatile uint64_t jiffies;
    return (long)jiffies;
}

/* ================================================================
   Console I/O  (SYS_WRITE / SYS_READ)
   ================================================================ */

long sys_write_fd(int fd, const void *buf, size_t count) {
    (void)fd;
    if (!buf || count == 0) return 0;

    /* Copy from user space */
    char *k_buf = (char *)kmalloc(count + 1);
    if (!k_buf) return -ENOMEM;

    if (vmm_copy_from_user(k_buf, buf, count) != 0) {
        kfree(k_buf);
        return -EFAULT;
    }
    k_buf[count] = '\0';

    if (fd == 1 || fd == 2) {
        /* stdout / stderr → UART console */
        extern void uart_puts(const char *s);
        uart_puts(k_buf);
    } else if (fd > 2) {
        /* Window ID → compositor terminal */
        extern void compositor_window_write(int win_id, const char *buf,
                                            size_t count);
        compositor_window_write(fd, k_buf, count);
    }

    kfree(k_buf);
    return (long)count;
}

long sys_read_fd(int fd, void *buf, size_t count) {
    if (fd != 0) return -ENOSYS;

    /* stdin: try to pull a keyboard event from the current process msg queue */
    struct ipc_message msg;
    if (sys_ipc_try_recv(-1, &msg) < 0) return 0;

    if (msg.type == IPC_TYPE_INPUT) {
        uint8_t key = (uint8_t)(msg.data1 & 0xFF);
        if (count >= 1) {
            vmm_copy_to_user(buf, &key, 1);
            return 1;
        }
    }
    return 0;
}

/* ================================================================
   VFS — read-only Ext4 via boot_fs.c
   ================================================================ */

long sys_file_read(const char *path, void *buf, int size, int offset) {
    char k_path[256];
    if (vmm_copy_string_from_user(k_path, path, sizeof(k_path)) != 0)
        return -EFAULT;

    /* size == 0 → return file size (stat-like) */
    uint32_t ino = ext4_find_inode(k_path);
    if (!ino) return -ENOENT;

    if (size == 0 || buf == NULL) {
        /* Return inode data size as a rough "file size" estimate */
        uint8_t tmp[4];
        int r = ext4_read_inode(ino, 0, tmp, 4);
        return (r >= 0) ? 65536 : -1; /* Simplified: tell caller "file exists" */
    }

    uint8_t *k_buf = (uint8_t *)kmalloc((size_t)size);
    if (!k_buf) return -ENOMEM;

    int bytes = ext4_read_inode(ino, (uint64_t)offset, k_buf, (uint32_t)size);
    if (bytes > 0)
        vmm_copy_to_user(buf, k_buf, (size_t)bytes);

    kfree(k_buf);
    return (long)bytes;
}

long sys_file_write(const char *path, const void *buf, int size, int offset) {
    (void)path; (void)buf; (void)size; (void)offset;
    return -30; /* -EROFS — filesystem is read-only */
}

long sys_list_dir(const char *path, char *buf, size_t size) {
    char k_path[256];
    if (vmm_copy_string_from_user(k_path, path, sizeof(k_path)) != 0)
        return -EFAULT;

    char *k_buf = (char *)kmalloc(size < 4096 ? size : 4096);
    if (!k_buf) return -ENOMEM;

    int ret = ext4_list_dir(k_path, k_buf, size < 4096 ? size : 4096);
    if (ret >= 0)
        vmm_copy_to_user(buf, k_buf, (size_t)(ret + 1));

    kfree(k_buf);
    return (long)ret;
}

long sys_chdir(const char *path) {
    if (!current_process) return -1;

    char k_path[128];
    if (vmm_copy_string_from_user(k_path, path, sizeof(k_path)) != 0)
        return -EFAULT;

    /* Verify it's a valid directory by finding its inode */
    if (k_path[0] != '\0' && strcmp(k_path, "/") != 0) {
        uint32_t ino = ext4_find_inode(k_path);
        if (!ino) return -ENOENT;
    }

    strncpy(current_process->cwd, k_path, sizeof(current_process->cwd) - 1);
    current_process->cwd[sizeof(current_process->cwd) - 1] = '\0';
    return 0;
}

long sys_getcwd(char *buf, size_t size) {
    if (!current_process || !buf || size == 0) return -1;

    const char *cwd = current_process->cwd;
    if (cwd[0] == '\0') cwd = "/";

    size_t len = strlen(cwd) + 1;
    if (len > size) return -34; /* -ERANGE */

    return vmm_copy_to_user(buf, cwd, len);
}
