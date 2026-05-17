#include <libkernel/types.h>
#include <core/errno.h>
#include <core/printk.h>

/* Prototypes to satisfy -Wmissing-prototypes */
void compositor_update_mouse(int dx, int dy, int absolute);
void compositor_handle_click(int button, int state);
void compositor_render(void);
long sys_open(const char *path, int flags, int mode);
long sys_read(int fd, void *buf, size_t count);
long sys_write(int fd, const void *buf, size_t count);
long sys_close(int fd);
int ext4_read_inode(uint32_t inode, void *buf, uint32_t size);
uint32_t ext4_find_inode(const char *path);
void compositor_destroy_windows_by_pid(int pid);
int compositor_get_focus_pid(void);
void compositor_tick(void);
/* Compositor stubs — migrating to user-space daemon */
int  compositor_create_window(int x, int y, int w, int h,
                              const char *title, int pid);
void compositor_destroy_window(int win_id);
void compositor_draw_rect(int win_id, int x, int y, int w, int h,
                          uint32_t color, int caller_pid);
void compositor_blit(int win_id, int x, int y, int w, int h,
                     const uint32_t *buf, int caller_pid);
void compositor_set_window_flags(int win_id, int flags);
void compositor_window_write(int win_id, const char *buf, size_t count);
int  compositor_get_window_by_pid(int pid);
void compositor_move_window(int win_id, int x, int y);
uint32_t *compositor_get_buffer(int win_id);
long sys_exit(int status);
long sys_get_pid(void);
long sys_get_time(void);
void kernel_secondary_main(uint64_t cpu_id);
extern volatile uint32_t cpu_boot_ack;
volatile uint32_t cpu_boot_ack = 0;

/* 
 * These are temporary stubs for services that have been moved to .old
 * and are being refactored into user-space processes.
 */

void compositor_update_mouse(int dx, int dy, int absolute) {
    (void)dx; (void)dy; (void)absolute;
}

void compositor_handle_click(int button, int state) {
    (void)button; (void)state;
}

void compositor_render(void) {
}

/* VFS Stubs */
long sys_open(const char *path, int flags, int mode) {
    (void)path; (void)flags; (void)mode;
    return -ENOSYS;
}

long sys_read(int fd, void *buf, size_t count) {
    (void)fd; (void)buf; (void)count;
    return -ENOSYS;
}

long sys_write(int fd, const void *buf, size_t count) {
    (void)fd; (void)buf; (void)count;
    return -ENOSYS;
}

long sys_close(int fd) {
    (void)fd;
    return -ENOSYS;
}

/* Moved to boot_fs.c */

void compositor_destroy_windows_by_pid(int pid) {
    (void)pid;
}

int compositor_get_focus_pid(void) {
    return -1;
}

void compositor_tick(void) {
}

/* Compositor stubs — will be replaced by user-space compositor IPC */

int compositor_create_window(int x, int y, int w, int h,
                             const char *title, int pid) {
    (void)x; (void)y; (void)w; (void)h; (void)title; (void)pid;
    return -1;
}

void compositor_destroy_window(int win_id)       { (void)win_id; }

void compositor_draw_rect(int win_id, int x, int y, int w, int h,
                          uint32_t color, int caller_pid) {
    (void)win_id; (void)x; (void)y; (void)w; (void)h;
    (void)color; (void)caller_pid;
}

void compositor_blit(int win_id, int x, int y, int w, int h,
                     const uint32_t *buf, int caller_pid) {
    (void)win_id; (void)x; (void)y; (void)w; (void)h;
    (void)buf; (void)caller_pid;
}

void compositor_set_window_flags(int win_id, int flags) {
    (void)win_id; (void)flags;
}

void compositor_window_write(int win_id, const char *buf, size_t count) {
    /* Fall back to console when compositor is not running */
    (void)win_id;
    extern void uart_puts(const char *s);
    (void)buf; (void)count;
}

int compositor_get_window_by_pid(int pid)  { (void)pid; return -1; }
void compositor_move_window(int win_id, int x, int y) {
    (void)win_id; (void)x; (void)y;
}
uint32_t *compositor_get_buffer(int win_id) { (void)win_id; return NULL; }

/* Moved to syscall_proc.c */

void kernel_secondary_main(uint64_t cpu_id) {
    (void)cpu_id;
    while(1);
}
