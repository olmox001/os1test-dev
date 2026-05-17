/*
 * kernel/libkernel/include/libkernel/kmalloc.h
 * Kernel heap allocator — implementation in libkernel/src/kmalloc.c.
 */
#ifndef _LIBKERNEL_KMALLOC_H
#define _LIBKERNEL_KMALLOC_H

#include <libkernel/types.h>

void  kmalloc_init(void);
void *kmalloc(size_t size);
void *kcalloc(size_t nmemb, size_t size);
void *krealloc(void *ptr, size_t new_size);
void  kfree(void *ptr);

/* stb_truetype compatibility shims */
#define STBTT_malloc(x, u) ((void)(u), kmalloc(x))
#define STBTT_free(x, u)   ((void)(u), kfree(x))

#endif /* _LIBKERNEL_KMALLOC_H */
