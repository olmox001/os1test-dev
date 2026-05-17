#ifndef _CORE_SYSCALL_H
#define _CORE_SYSCALL_H

#include <libkernel/types.h>

/* ---------------------------------------------------------------
   Core syscalls
   --------------------------------------------------------------- */
long sys_exit(int status);
long sys_yield(void);
long sys_get_pid(void);
long sys_get_time(void);
long sys_sbrk(intptr_t increment);
long sys_spawn(const char *path);
long sys_wait(int pid);
long sys_kill(int pid);
/* ---------------------------------------------------------------
   IPC (PID-based)
   --------------------------------------------------------------- */
int  sys_ipc_send(int target_pid, void *msg_ptr);
int  sys_ipc_recv(int src_pid, void *msg_ptr);
int  sys_ipc_try_recv(int src_pid, void *msg_ptr);

/* ---------------------------------------------------------------
   VFS / Filesystem
   --------------------------------------------------------------- */
long sys_file_read(const char *path, void *buf, int size, int offset);
long sys_file_write(const char *path, const void *buf, int size, int offset);
long sys_list_dir(const char *path, char *buf, size_t size);
long sys_chdir(const char *path);
long sys_getcwd(char *buf, size_t size);

/* ---------------------------------------------------------------
   Console I/O  (fd=1 → UART, fd>2 → compositor window)
   --------------------------------------------------------------- */
long sys_write_fd(int fd, const void *buf, size_t count);
long sys_read_fd(int fd, void *buf, size_t count);

/* ---------------------------------------------------------------
   Graphics / Compositor (kernel-resident for now)
   --------------------------------------------------------------- */
int  sys_set_font(void *data, size_t size);

#endif /* _CORE_SYSCALL_H */
