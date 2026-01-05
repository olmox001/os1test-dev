/*
 * user/lib.h
 * User-space library header
 */
#ifndef _LIB_H
#define _LIB_H

#include <stddef.h>

/* Syscall constants (Linux AArch64 compatible) */
#define PROCESS_NAME_MAX 32
#define STACK_SIZE 16384
#define SYS_READ 63
#define SYS_WRITE 64
#define SYS_EXIT 93
#define SYS_GET_TIME 169
#define SYS_GETPID 172
#define SYS_DRAW 200
#define SYS_FLUSH 201
#define SYS_CREATE_WINDOW 210
#define SYS_WINDOW_DRAW 211
#define SYS_COMPOSITOR_RENDER 212

/* Basic syscalls */
long read(int fd, char *buf, unsigned long count);
void write(int fd, const char *buf, size_t count);
long get_time(void);
int get_pid(void);
void exit(int status);

/* Graphics syscalls (direct framebuffer) */
void draw(int x, int y, int w, int h, int color);
void flush(void);

/* Compositor syscalls */
int create_window(int x, int y, int w, int h, const char *title);
void window_draw(int win_id, int x, int y, int w, int h, unsigned int color);
void compositor_render(void);

/* Helper functions */
void print(const char *s);
void print_hex(unsigned long val);
void printf(const char *fmt, ...);
void sprintf(char *out, const char *fmt, ...);
size_t strlen(const char *s);

/* Standard IO */
int getchar(void);
int putchar(int c);
char *gets(char *s, int size);

#endif /* _LIB_H */
