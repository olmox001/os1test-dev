#include "lib.h"

size_t strlen(const char *s) {
  size_t len = 0;
  while (*s++)
    len++;
  return len;
}

long read(int fd, char *buf, unsigned long count) {
  long ret;
  __asm__ __volatile__("mov x8, %1\n"
                       "mov x0, %2\n"
                       "mov x1, %3\n"
                       "mov x2, %4\n"
                       "svc #0\n"
                       "mov %0, x0\n"
                       : "=r"(ret)
                       : "r"((long)SYS_READ), "r"((long)fd), "r"(buf),
                         "r"((long)count)
                       : "x0", "x1", "x2", "x8", "memory");
  return ret;
}

void write(int fd, const char *buf, size_t count) {
  __asm__ __volatile__("mov x8, %0\n"
                       "mov x0, %1\n"
                       "mov x1, %2\n"
                       "mov x2, %3\n"
                       "svc #0\n"
                       :
                       : "r"((long)SYS_WRITE), "r"((long)fd), "r"(buf),
                         "r"((long)count)
                       : "x0", "x1", "x2", "x8", "memory");
}

long get_time(void) {
  long ret;
  __asm__ __volatile__("mov x8, %1\n"
                       "svc #0\n"
                       "mov %0, x0\n"
                       : "=r"(ret)
                       : "r"((long)SYS_GET_TIME)
                       : "x0", "x8", "memory");
  return ret;
}

int get_pid(void) {
  long ret;
  __asm__ __volatile__("mov x8, %1\n"
                       "svc #0\n"
                       "mov %0, x0\n"
                       : "=r"(ret)
                       : "r"((long)SYS_GETPID)
                       : "x0", "x8", "memory");
  return (int)ret;
}

void exit(int status) {
  __asm__ __volatile__("mov x8, %0\n"
                       "mov x0, %1\n"
                       "svc #0\n"
                       :
                       : "r"((long)SYS_EXIT), "r"((long)status)
                       : "x0", "x8", "memory");
  while (1)
    ;
}

void draw(int x, int y, int w, int h, int color) {
  __asm__ __volatile__("mov x8, %0\n"
                       "mov x0, %1\n"
                       "mov x1, %2\n"
                       "mov x2, %3\n"
                       "mov x3, %4\n"
                       "mov x4, %5\n"
                       "svc #0\n"
                       :
                       : "r"((long)SYS_DRAW), "r"((long)x), "r"((long)y),
                         "r"((long)w), "r"((long)h), "r"((long)color)
                       : "x0", "x1", "x2", "x3", "x4", "x8", "memory");
}

void flush(void) {
  __asm__ __volatile__("mov x8, %0\n"
                       "svc #0\n"
                       :
                       : "r"((long)SYS_FLUSH)
                       : "x8", "memory");
}

int create_window(int x, int y, int w, int h, const char *title) {
  long ret;
  __asm__ __volatile__("mov x8, %1\n"
                       "mov x0, %2\n"
                       "mov x1, %3\n"
                       "mov x2, %4\n"
                       "mov x3, %5\n"
                       "mov x4, %6\n"
                       "svc #0\n"
                       "mov %0, x0\n"
                       : "=r"(ret)
                       : "r"((long)SYS_CREATE_WINDOW), "r"((long)x),
                         "r"((long)y), "r"((long)w), "r"((long)h),
                         "r"((long)title)
                       : "x0", "x1", "x2", "x3", "x4", "x8", "memory");
  return (int)ret;
}

void window_draw(int win_id, int x, int y, int w, int h, unsigned int color) {
  __asm__ __volatile__("mov x8, %0\n"
                       "mov x0, %1\n"
                       "mov x1, %2\n"
                       "mov x2, %3\n"
                       "mov x3, %4\n"
                       "mov x4, %5\n"
                       "mov x5, %6\n"
                       "svc #0\n"
                       :
                       : "r"((long)SYS_WINDOW_DRAW), "r"((long)win_id),
                         "r"((long)x), "r"((long)y), "r"((long)w), "r"((long)h),
                         "r"((long)color)
                       : "x0", "x1", "x2", "x3", "x4", "x5", "x8", "memory");
}

void compositor_render(void) {
  __asm__ __volatile__("mov x8, %0\n"
                       "svc #0\n"
                       :
                       : "r"((long)SYS_COMPOSITOR_RENDER)
                       : "x8", "memory");
}

void print(const char *s) {
  size_t len = 0;
  while (s[len])
    len++;
  write(1, s, len);
}

void print_hex(unsigned long val) {
  char buf[18];
  buf[0] = '0';
  buf[1] = 'x';
  int i;
  for (i = 0; i < 16; i++) {
    int digit = (val >> ((15 - i) * 4)) & 0xF;
    buf[2 + i] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
  }
  write(1, buf, 18);
}

void vsprintf(char *out, const char *fmt, __builtin_va_list args) {
  int out_idx = 0;

  for (const char *p = fmt; *p; p++) {
    if (*p == '%' && *(p + 1)) {
      p++;
      if (*p == 's') {
        const char *s = __builtin_va_arg(args, const char *);
        while (*s)
          out[out_idx++] = *s++;
      } else if (*p == 'd') {
        int d = __builtin_va_arg(args, int);
        if (d < 0) {
          out[out_idx++] = '-';
          d = -d;
        }
        if (d == 0) {
          out[out_idx++] = '0';
        } else {
          char tmp[12];
          int ti = 0;
          while (d > 0) {
            tmp[ti++] = '0' + (d % 10);
            d /= 10;
          }
          while (ti > 0)
            out[out_idx++] = tmp[--ti];
        }
      } else if (*p == 'x') {
        unsigned long x = __builtin_va_arg(args, unsigned long);
        if (x == 0) {
          out[out_idx++] = '0';
        } else {
          char tmp[16];
          int ti = 0;
          while (x > 0) {
            int digit = x % 16;
            tmp[ti++] = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
            x /= 16;
          }
          while (ti > 0)
            out[out_idx++] = tmp[--ti];
        }
      } else {
        out[out_idx++] = '%';
        out[out_idx++] = *p;
      }
    } else {
      out[out_idx++] = *p;
    }
  }
  out[out_idx] = '\0';
}

void printf(const char *fmt, ...) {
  char buf[256];
  __builtin_va_list args;
  __builtin_va_start(args, fmt);
  vsprintf(buf, fmt, args);
  __builtin_va_end(args);
  write(1, buf, strlen(buf));
}

void sprintf(char *out, const char *fmt, ...) {
  __builtin_va_list args;
  __builtin_va_start(args, fmt);
  vsprintf(out, fmt, args);
  __builtin_va_end(args);
}

/*
 * Standard IO Implementation
 */
int getchar(void) {
  char c;
  if (read(0, &c, 1) == 1) {
    return (unsigned char)c;
  }
  return -1;
}

int putchar(int c) {
  char ch = (char)c;
  write(1, &ch, 1);
  return c;
}

char *gets(char *s, int size) {
  int i = 0;
  while (i < size - 1) {
    int c = getchar();
    if (c < 0)
      break; /* Error */

    if (c == '\b' || c == 127) {
      /* Backspace */
      if (i > 0) {
        i--;
        /* Erase on screen */
        write(1, "\b \b", 3);
      }
      continue;
    }

    /* Echo */
    putchar(c);

    if (c == '\n' || c == '\r') {
      s[i] = '\0';
      return s;
    }
    s[i++] = (char)c;
  }
  s[i] = '\0';
  return s;
}
