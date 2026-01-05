/*
 * kernel/include/kernel/kmalloc.h
 * Kernel Heap Allocator
 */
#ifndef _KERNEL_KMALLOC_H
#define _KERNEL_KMALLOC_H

#include <kernel/types.h>

void kmalloc_init(void);
void *kmalloc(size_t size);
void *kcalloc(size_t nmemb, size_t size);
void *krealloc(void *ptr, size_t new_size);
void kfree(void *ptr);

/* Macros for stb_truetype compatibility */
#define STBTT_malloc(x, u) ((void)(u), kmalloc(x))
#define STBTT_free(x, u) ((void)(u), kfree(x))

#endif /* _KERNEL_KMALLOC_H */
