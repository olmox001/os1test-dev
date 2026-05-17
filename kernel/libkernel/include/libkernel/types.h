#ifndef _LIBKERNEL_TYPES_H
#define _LIBKERNEL_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <core/errno.h>

/* Kernel-specific aliases */
typedef uint64_t phys_addr_t;
typedef uint64_t virt_addr_t;
typedef int32_t  pid_t;
typedef int64_t  ssize_t;

/* Status codes */
typedef enum {
    STATUS_OK = 0,
    STATUS_ERROR = -1,
    STATUS_NOMEM = -2,
    STATUS_BUSY = -3,
    STATUS_AGAIN = -4,
    STATUS_INVALID = -5,
    STATUS_NOT_FOUND = -6,
    STATUS_NOT_SUPPORTED = -7,
} status_t;

/* Helper Macros */
#define UNUSED(x) (void)(x)
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define ALIGN_UP(x, a)   (((x) + ((a) - 1)) & ~((a) - 1))
#define ALIGN_DOWN(x, a) ((x) & ~((a) - 1))

/* Core definitions */
#ifndef offsetof
#define offsetof(type, member) ((size_t) &((type *)0)->member)
#endif
#define container_of(ptr, type, member) ({                      \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetof(type,member) );})

/* Helper Macros */
#define BIT(x) (1ULL << (x))
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* Compiler Attributes */
#define __packed      __attribute__((packed))
#define __aligned(x)  __attribute__((aligned(x)))
#define __section(s)  __attribute__((section(s)))
#define __unused      __attribute__((unused))
#define __noreturn    __attribute__((noreturn))
#define __always_inline inline __attribute__((always_inline))

/* Likely/Unlikely branch hints */
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#endif /* _LIBKERNEL_TYPES_H */
