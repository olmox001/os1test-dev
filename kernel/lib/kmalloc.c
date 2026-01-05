/*
 * kernel/lib/kmalloc.c
 * Simple Kernel Heap Allocator
 *
 * This is a very basic bump allocator with free list.
 * Suitable for small allocations in kernel context.
 */
#include <kernel/pmm.h>
#include <kernel/printk.h>
#include <kernel/string.h>
#include <kernel/types.h>

#define HEAP_SIZE (8 * 1024 * 1024) /* 8 MB heap */
#define BLOCK_MAGIC 0xDEADBEEF
#define BLOCK_FREE 0xFREEFREE
#define MIN_ALLOC 16

/* Block header */
struct block_header {
  uint32_t magic;
  uint32_t size; /* Size including header */
  struct block_header *next;
  uint32_t reserved;
} __attribute__((aligned(16)));

/* Global heap state */
static uint8_t *heap_base = NULL;
static uint8_t *heap_end = NULL;
static uint8_t *heap_ptr = NULL;
static struct block_header *free_list = NULL;
static int heap_initialized = 0;

/*
 * Initialize kernel heap
 */
void kmalloc_init(void) {
  if (heap_initialized)
    return;

  /* Allocate heap pages from PMM */
  uint32_t pages = (HEAP_SIZE + 4095) / 4096;
  heap_base = (uint8_t *)pmm_alloc_pages(pages);

  if (!heap_base) {
    pr_err("kmalloc: Failed to allocate heap\n");
    return;
  }

  heap_end = heap_base + HEAP_SIZE;
  heap_ptr = heap_base;
  free_list = NULL;
  heap_initialized = 1;

  pr_info("kmalloc: Heap initialized at %p (%u KB)\n", heap_base,
          HEAP_SIZE / 1024);
}

/*
 * Allocate memory
 */
void *kmalloc(size_t size) {
  if (!heap_initialized) {
    kmalloc_init();
  }

  if (size == 0)
    return NULL;

  /* Align size to 16 bytes */
  size = (size + 15) & ~15;
  size_t total = size + sizeof(struct block_header);

  /* Check free list first */
  struct block_header **pp = &free_list;
  while (*pp) {
    if ((*pp)->size >= total) {
      struct block_header *block = *pp;
      *pp = block->next;
      block->magic = BLOCK_MAGIC;
      block->next = NULL;
      return (void *)(block + 1);
    }
    pp = &(*pp)->next;
  }

  /* Allocate from bump pointer */
  if (heap_ptr + total > heap_end) {
    pr_err("kmalloc: Out of memory\n");
    return NULL;
  }

  struct block_header *block = (struct block_header *)heap_ptr;
  block->magic = BLOCK_MAGIC;
  block->size = total;
  block->next = NULL;

  heap_ptr += total;

  return (void *)(block + 1);
}

/*
 * Allocate zeroed memory
 */
void *kcalloc(size_t nmemb, size_t size) {
  size_t total = nmemb * size;
  void *ptr = kmalloc(total);
  if (ptr) {
    memset(ptr, 0, total);
  }
  return ptr;
}

/*
 * Free memory
 */
void kfree(void *ptr) {
  if (!ptr)
    return;

  struct block_header *block = ((struct block_header *)ptr) - 1;

  if (block->magic != BLOCK_MAGIC) {
    pr_err("kfree: Invalid block at %p (magic=0x%x)\n", ptr, block->magic);
    return;
  }

  /* Add to free list */
  block->magic = 0xFEEEFEEE; /* Mark as freed */
  block->next = free_list;
  free_list = block;
}

/*
 * Reallocate memory
 */
void *krealloc(void *ptr, size_t new_size) {
  if (!ptr)
    return kmalloc(new_size);
  if (new_size == 0) {
    kfree(ptr);
    return NULL;
  }

  struct block_header *block = ((struct block_header *)ptr) - 1;
  size_t old_size = block->size - sizeof(struct block_header);

  void *new_ptr = kmalloc(new_size);
  if (new_ptr) {
    size_t copy_size = (old_size < new_size) ? old_size : new_size;
    memcpy(new_ptr, ptr, copy_size);
    kfree(ptr);
  }

  return new_ptr;
}
