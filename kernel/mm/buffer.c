/*
 * kernel/mm/buffer.c
 * Zero-Copy Buffer Cache Implementation
 */
#include <drivers/virtio_blk.h>
#include <kernel/buffer.h>
#include <kernel/pmm.h>
#include <kernel/printk.h>
#include <kernel/string.h>

#define HASH_BUCKETS 64

static struct list_head lru_list;
static struct list_head hash_table[HASH_BUCKETS];

/* Simple hash function */
static uint32_t hash_block(uint64_t block) { return block % HASH_BUCKETS; }

void buffer_init(void) {
  pr_info("BufferCache: Initializing...\n");
  INIT_LIST_HEAD(&lru_list);
  for (int i = 0; i < HASH_BUCKETS; i++) {
    INIT_LIST_HEAD(&hash_table[i]);
  }
}

static struct block_buffer *__lookup(uint64_t block) {
  uint32_t bucket = hash_block(block);
  struct block_buffer *pos;

  list_for_each_entry(pos, &hash_table[bucket], hash) {
    if (pos->block == block) {
      return pos;
    }
  }
  return NULL;
}

struct block_buffer *buffer_get(uint64_t block) {
  /* 1. Check Cache */
  struct block_buffer *buf = __lookup(block);

  if (buf) {
    /* Move to head of LRU (most recently used) */
    if (!list_empty(&buf->list)) {
      list_move(&buf->list, &lru_list);
    }
    buf->ref_count++;
    return buf;
  }

  /* 2. Allocate New Buffer (Metadata) */
  /* TODO: use slab allocator for struct block_buffer */
  /* For now, hacking it by allocating a full page for one struct (WASTEFUL)
     OR just allocate a small chunk if we had malloc.
     Let's use a static pool or allow one page to hold many structs?
     Simpler: For this stage, just alloc pointers.
     Actually, `pmm_alloc_page()` gives 4KB. That's fine for DATA.
     But for `struct block_buffer`?
     Let's hack: Put the struct AT THE END of the data page?
     No, data page must be 4KB aligned for hardware DMA?
     VirtIO doesn't STRICTLY require 4KB alignment for buffers, but it's good
     practice.

     Let's implement a very simple `zalloc` (kernel malloc) placeholder or just
     use 1 page for metadata for now (wasteful but works).
  */

  /* HACK: We need a way to alloc `struct block_buffer`.
     Let's just use `pmm_alloc_page` for the struct too (checking size).
  */
  buf = (struct block_buffer *)pmm_alloc_page();
  if (!buf)
    return NULL;
  memset(buf, 0, sizeof(*buf));

  /* 3. Allocate Data Page */
  buf->data = (uint8_t *)pmm_alloc_page();
  if (!buf->data) {
    pmm_free_page(buf);
    return NULL;
  }

  /* 4. Read from Disk */
  /* Block is 4KB = 8 sectors */
  if (virtio_blk_read(buf->data, block * SECTORS_PER_BLOCK,
                      SECTORS_PER_BLOCK) != 0) {
    pr_info("BufferCache: Disk read error block %ld\n", block);
    pmm_free_page(buf->data);
    pmm_free_page(buf);
    return NULL;
  }

  buf->block = block;
  buf->flags = BUFFER_UPTODATE;
  buf->ref_count = 1;

  /* 5. Insert into Hash and LRU */
  uint32_t bucket = hash_block(block);
  list_add(&buf->hash, &hash_table[bucket]);
  list_add(&buf->list, &lru_list);

  // pr_info("BufferCache: Read block %ld\n", block);
  return buf;
}

void buffer_put(struct block_buffer *buf) {
  if (!buf)
    return;
  if (buf->ref_count > 0)
    buf->ref_count--;
}

void buffer_sync(void) {
  struct block_buffer *buf;
  /* Iterate LRU */
  list_for_each_entry(buf, &lru_list, list) {
    if (buf->flags & BUFFER_DIRTY) {
      virtio_blk_write(buf->data, buf->block * SECTORS_PER_BLOCK,
                       SECTORS_PER_BLOCK);
      buf->flags &= ~BUFFER_DIRTY;
    }
  }
}
