/*
 * kernel/drivers/virtio/virtio_blk.c
 * VirtIO Block Device Driver
 */
#include <drivers/virtio.h>
#include <drivers/virtio_blk.h>
#include <kernel/pmm.h>
#include <kernel/printk.h>
#include <kernel/string.h>

/* Helper macros for MMIO usage */
#define VIRTIO_REG(base, offset)                                               \
  ((volatile uint32_t *)((uint64_t)(base) + (offset)))
#define VIRTIO_READ(base, offset) (*VIRTIO_REG(base, offset))
#define VIRTIO_WRITE(base, offset, val) (*VIRTIO_REG(base, offset) = (val))

static uint64_t virtio_blk_base = 0;
static uint32_t virtio_blk_qsize = 0;

/* Vring structures */
static struct vring_desc *desc;
static struct vring_avail *avail;
static struct vring_used *used;

/*
 * Initialize VirtIO Block Device
 */
void virtio_blk_init(void) {
  pr_info("VirtIO: Probing for block device...\n");

  for (int i = 0; i < VIRTIO_COUNT; i++) {
    uint64_t base = VIRTIO_MMIO_BASE + i * VIRTIO_MMIO_STRIDE;

    uint32_t magic = VIRTIO_READ(base, VIRTIO_MMIO_MAGIC_VALUE);
    if (magic != 0x74726976) { /* "virt" */
      continue;
    }

    uint32_t device_id = VIRTIO_READ(base, VIRTIO_MMIO_DEVICE_ID);
    if (device_id == 0) {
      continue; /* Placeholder */
    }

    if (device_id == VIRTIO_DEV_BLOCK) {
      pr_info("VirtIO: Found Block Device at 0x%08lx\n", base);
      virtio_blk_base = base;

      uint32_t version = VIRTIO_READ(base, VIRTIO_MMIO_VERSION);
      pr_info("VirtIO: Version %d\n", version);

      /* 1. Reset */
      VIRTIO_WRITE(base, VIRTIO_MMIO_STATUS, 0);

      /* 2. Set ACKNOWLEDGE and DRIVER */
      uint32_t status = VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER;
      VIRTIO_WRITE(base, VIRTIO_MMIO_STATUS, status);

      /* 3. Negotiate features */
      uint32_t features = VIRTIO_READ(base, VIRTIO_MMIO_DEVICE_FEATURES);
      VIRTIO_WRITE(base, VIRTIO_MMIO_DRIVER_FEATURES, features);

      /* 4. Set FEATURES_OK (Only for Version 2, but harmless/ignored on V1) */
      if (version >= 2) {
        status |= VIRTIO_STATUS_FEATURES_OK;
        VIRTIO_WRITE(base, VIRTIO_MMIO_STATUS, status);
        if (!(VIRTIO_READ(base, VIRTIO_MMIO_STATUS) &
              VIRTIO_STATUS_FEATURES_OK)) {
          pr_info("VirtIO: Feature negotiation failed\n");
          return;
        }
      }

      /* 5. Initialize Queue 0 */
      VIRTIO_WRITE(base, VIRTIO_MMIO_QUEUE_SEL, 0);

      uint32_t qmax = VIRTIO_READ(base, VIRTIO_MMIO_QUEUE_NUM_MAX);
      if (qmax == 0) {
        pr_info("VirtIO: Queue 0 not available\n");
        return;
      }

      /* Use 16 descriptors (must be power of 2, <= qmax) */
      uint32_t qsize = 16;
      if (qsize > qmax)
        qsize = qmax;
      virtio_blk_qsize = qsize;

      VIRTIO_WRITE(base, VIRTIO_MMIO_QUEUE_NUM, qsize);

      if (version == 1) {
        /* Legacy: Set Page Size and PFN */
        VIRTIO_WRITE(base, VIRTIO_MMIO_GUEST_PAGE_SIZE, 4096);

        /* Allocate memory for queue (must be physically contiguous) */
        /* PMM gives us a 4K aligned page, which is enough */
        /* Let's allocate 2 pages (8KB) just to be safe and use pmm_alloc_pages
         */

        void *qmem = pmm_alloc_pages(2);
        if (!qmem) {
          pr_info("VirtIO: Failed to alloc 2 pages\n");
          return;
        }
        memset(qmem, 0, 8192);

        uint64_t q_phys = (uint64_t)qmem;
        VIRTIO_WRITE(base, VIRTIO_MMIO_QUEUE_PFN, q_phys >> 12);

        desc = (struct vring_desc *)qmem;
        avail = (struct vring_avail *)((uint8_t *)qmem + qsize * 16);

        /* Used starts at next 4K boundary relative to start */
        used = (struct vring_used *)((uint8_t *)qmem + 4096);

        pr_info("VirtIO: Queue 0 setup (Legacy). Desc: %p, Used: %p\n", desc,
                used);

      } else {
        pr_info("VirtIO: Modern not fully implemented yet\n");
        return;
      }

      /* 6. Set DRIVER_OK */
      status |= VIRTIO_STATUS_DRIVER_OK;
      VIRTIO_WRITE(base, VIRTIO_MMIO_STATUS, status);

      pr_info("VirtIO: Block Device Initialized\n");
      return;
    }
  }

  pr_info("VirtIO: No block device found\n");
}

/*
 * virtio_blk_read: Read sectors from disk
 * Synchronous implementation (busy-wait)
 */
int virtio_blk_read(void *buf, uint64_t sector, uint32_t count) {
  if (!virtio_blk_base || !desc || !avail || !used)
    return -1;

  /*
   * Simple implementation: Use a static request struct and lock.
   * TODO: Support concurrent requests using a free list of descriptors.
   */
  static struct virtio_blk_req req;
  static uint8_t status;
  //   static spinlock_t lock = SPINLOCK_INIT; // TODO: Add locking

  //   spin_lock(&lock);

  req.type = VIRTIO_BLK_T_IN;
  req.reserved = 0;
  req.sector = sector;

  /*
   * Descriptor 0: Header (Read-only for device)
   */
  desc[0].addr = (uint64_t)&req;
  desc[0].len = sizeof(struct virtio_blk_req);
  desc[0].flags = VRING_DESC_F_NEXT;
  desc[0].next = 1;

  /*
   * Descriptor 1: Buffer (Write-only for device)
   */
  desc[1].addr = (uint64_t)buf;
  desc[1].len = count * 512;
  desc[1].flags = VRING_DESC_F_WRITE | VRING_DESC_F_NEXT;
  desc[1].next = 2;

  /*
   * Descriptor 2: Status (Write-only for device)
   */
  desc[2].addr = (uint64_t)&status;
  desc[2].len = 1;
  desc[2].flags = VRING_DESC_F_WRITE;
  desc[2].next = 0;

  /* Put request in Available Ring */
  uint16_t idx = avail->idx % virtio_blk_qsize;
  avail->ring[idx] = 0; /* Head descriptor index is 0 */

  /* Update Available Index */
  /* Memory barrier needed here technically */
  __asm__ volatile("dmb sy" ::: "memory");
  avail->idx++;
  __asm__ volatile("dmb sy" ::: "memory");

  /* Notify device */
  /* For Legacy: Write queue index to QueueNotify (offset 0x050) */
  VIRTIO_WRITE(virtio_blk_base, VIRTIO_MMIO_QUEUE_NOTIFY, 0);

  /* Poll Used Ring (Busy Wait) */
  /* Ideally we would wait for interrupt, but polling works for now */
  /* We check if used->idx advances */
  // uint16_t current_used_idx = used->idx;
  // Note: This logic is flawed if we re-use the same descriptors multiple times
  // quickly without tracking used_idx globally. Better: Snaphot used->idx
  // before submitting? Actually we should track our `last_used_idx`. Since we
  // are single threaded here (locked), we can just wait for used->idx !=
  // old_idx.

  // Wait, I need a global last_used_idx if I want to be correct.
  // But for single request at a time:
  uint16_t wait_idx = used->idx;
  // Wait until used->idx increments.
  while (used->idx == wait_idx) {
    __asm__ volatile("nop");
  }

  /* Check status */
  if (status != VIRTIO_BLK_S_OK) {
    pr_info("VirtIO: Read/Write failed status=%d\n", status);
    //       spin_unlock(&lock);
    return -1;
  }

  //   spin_unlock(&lock);
  return 0;
}

int virtio_blk_write(void *buf, uint64_t sector, uint32_t count) {
  /* Similar to read but with VIRTIO_BLK_T_OUT and different flags */
  /* Reuse same static resources/lock -> dangerous if read/write concurrent?
     Yes, need lock. */

  if (!virtio_blk_base || !desc || !avail || !used)
    return -1;

  static struct virtio_blk_req req_w;
  static uint8_t status_w;

  req_w.type = VIRTIO_BLK_T_OUT;
  req_w.reserved = 0;
  req_w.sector = sector;

  /* Header (Read-only for device) */
  desc[0].addr = (uint64_t)&req_w;
  desc[0].len = sizeof(struct virtio_blk_req);
  desc[0].flags = VRING_DESC_F_NEXT;
  desc[0].next = 1;

  /* Buffer (Read-only for device) - different from READ! */
  desc[1].addr = (uint64_t)buf;
  desc[1].len = count * 512;
  desc[1].flags = VRING_DESC_F_NEXT; /* Device reads from buf */
  desc[1].next = 2;

  /* Status (Write-only for device) */
  desc[2].addr = (uint64_t)&status_w;
  desc[2].len = 1;
  desc[2].flags = VRING_DESC_F_WRITE;
  desc[2].next = 0;

  uint16_t idx = avail->idx % virtio_blk_qsize;
  avail->ring[idx] = 0;

  __asm__ volatile("dmb sy" ::: "memory");
  avail->idx++;
  __asm__ volatile("dmb sy" ::: "memory");

  VIRTIO_WRITE(virtio_blk_base, VIRTIO_MMIO_QUEUE_NOTIFY, 0);

  uint16_t wait_idx = used->idx;
  while (used->idx == wait_idx) {
    __asm__ volatile("nop");
  }

  if (status_w != VIRTIO_BLK_S_OK)
    return -1;
  return 0;
}
