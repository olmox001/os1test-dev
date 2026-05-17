/*
 * kernel/core/include/core/string.h
 * Forwarder to the canonical string declarations in libkernel.
 * Keeps backward compatibility for code using <core/string.h>.
 */
#ifndef _KERNEL_STRING_H
#define _KERNEL_STRING_H

#include <libkernel/string.h>

/* Additional declarations not in libkernel/string.h */
char   *strcat(char *dest, const char *src);
char   *strncat(char *dest, const char *src, size_t n);
size_t  strlcat(char *dest, const char *src, size_t size);
void   *memchr(const void *s, int c, size_t n);
void    bzero(void *s, size_t n);

#endif /* _KERNEL_STRING_H */
