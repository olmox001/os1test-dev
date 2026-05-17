/*
 * kernel/libkernel/include/libkernel/printk.h
 * Kernel logging — implementation in libkernel/src/printk.c.
 */
#ifndef _LIBKERNEL_PRINTK_H
#define _LIBKERNEL_PRINTK_H

#include <libkernel/types.h>
#include <stdarg.h>

#define KERN_EMERG   0
#define KERN_ALERT   1
#define KERN_CRIT    2
#define KERN_ERR     3
#define KERN_WARNING 4
#define KERN_NOTICE  5
#define KERN_INFO    6
#define KERN_DEBUG   7

extern int console_loglevel;

int printk(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
int vprintk(const char *fmt, va_list args);
int snprintf(char *buf, size_t size, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);

#define pr_emerg(fmt, ...)  do { if (console_loglevel >= KERN_EMERG)   printk("[EMERG] "  fmt, ##__VA_ARGS__); } while (0)
#define pr_alert(fmt, ...)  do { if (console_loglevel >= KERN_ALERT)   printk("[ALERT] "  fmt, ##__VA_ARGS__); } while (0)
#define pr_crit(fmt, ...)   do { if (console_loglevel >= KERN_CRIT)    printk("[CRIT] "   fmt, ##__VA_ARGS__); } while (0)
#define pr_err(fmt, ...)    do { if (console_loglevel >= KERN_ERR)     printk("[ERROR] "  fmt, ##__VA_ARGS__); } while (0)
#define pr_warn(fmt, ...)   do { if (console_loglevel >= KERN_WARNING) printk("[WARN] "   fmt, ##__VA_ARGS__); } while (0)
#define pr_notice(fmt, ...) do { if (console_loglevel >= KERN_NOTICE)  printk("[NOTICE] " fmt, ##__VA_ARGS__); } while (0)
#define pr_info(fmt, ...)   do { if (console_loglevel >= KERN_INFO)    printk("[INFO] "   fmt, ##__VA_ARGS__); } while (0)
#define pr_debug(fmt, ...)  do { if (console_loglevel >= KERN_DEBUG)   printk("[DEBUG] "  fmt, ##__VA_ARGS__); } while (0)

void panic(const char *fmt, ...) __noreturn __attribute__((format(printf, 1, 2)));

#endif /* _LIBKERNEL_PRINTK_H */
