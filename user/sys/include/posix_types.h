#ifndef _POSIX_TYPES_H
#define _POSIX_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* POSIX Standard Types */
typedef int64_t  ssize_t;
typedef int32_t  pid_t;
typedef uint32_t uid_t;
typedef uint32_t gid_t;
typedef uint32_t mode_t;
typedef int64_t  off_t;
typedef uint64_t ino_t;
typedef uint32_t dev_t;
typedef uint32_t nlink_t;
typedef int64_t  time_t;

/* System-specific but POSIX-compatible */
#ifndef EOK
#define EOK 0
#endif

#define EPERM 1
#define ENOENT 2
#define ESRCH 3
#define EINTR 4
#define EIO 5
#define ENXIO 6
#define E2BIG 7
#define ENOEXEC 8
#define EBADF 9
#define ECHILD 10
#define EAGAIN 11
#define ENOMEM 12
#define EACCES 13
#define EFAULT 14
#define ENOTBLK 15
#define EBUSY 16
#define EEXIST 17
#define EXDEV 18
#define ENODEV 19
#define ENOTDIR 20
#define EISDIR 21
#define EINVAL 22

#endif /* _POSIX_TYPES_H */
