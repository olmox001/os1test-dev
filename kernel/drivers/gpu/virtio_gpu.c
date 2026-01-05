/*
 * kernel/drivers/gpu/virtio_gpu.c
 * VirtIO GPU Driver Implementation
 */
#include <drivers/virtio.h>
#include <drivers/virtio_gpu.h>
#include <kernel/pmm.h>
#include <kernel/printk.h>
#include <kernel/string.h>

/* Helper macros */
#define VIRTIO_REG(base, offset)                                               \
  ((volatile uint32_t *)((uint64_t)(base) + (offset)))
#define VIRTIO_READ(base, offset) (*VIRTIO_REG(base, offset))
#define VIRTIO_WRITE(base, offset, val) (*VIRTIO_REG(base, offset) = (val))

static uint64_t virtio_gpu_base = 0;
static uint32_t virtio_gpu_qsize = 0;

/* Vring structures for Control Queue (Queue 0) */
static struct vring_desc *desc;
static struct vring_avail *avail;
static struct vring_used *used;

/* Global Framebuffer */
struct gpu_framebuffer g_fb = {0};

/*
 * Send a command to the GPU Control Queue (Synchronous)
 * Generic helper that handles one request/response pair.
 */
static int virtio_gpu_send(void *cmd, uint32_t cmd_len, void *resp,
                           uint32_t resp_len) {
  if (!virtio_gpu_base)
    return -1;

  // pr_info("virtio_gpu_send: cmd=%p len=%d resp=%p\n", cmd, cmd_len, resp);

  /* Descriptor 0: Command (Read-only for device) */
  // pr_info("virtio_gpu_send: desc=%p\n", desc);
  desc[0].addr = (uint64_t)cmd;
  desc[0].len = cmd_len;
  desc[0].flags = VRING_DESC_F_NEXT;
  desc[0].next = 1;

  /* Descriptor 1: Response (Write-only for device) */
  desc[1].addr = (uint64_t)resp;
  desc[1].len = resp_len;
  desc[1].flags = VRING_DESC_F_WRITE;
  desc[1].next = 0;

  /* Add to Available Ring */
  uint16_t idx = avail->idx % virtio_gpu_qsize;
  // pr_info("virtio_gpu_send: avail=%p idx=%d\n", avail, idx);
  avail->ring[idx] = 0; /* Head index */

  __asm__ volatile("dmb sy" ::: "memory");
  avail->idx++;
  __asm__ volatile("dmb sy" ::: "memory");

  /* Notify */
  VIRTIO_WRITE(virtio_gpu_base, VIRTIO_MMIO_QUEUE_NOTIFY, 0);

  /* Busy Wait */
  // pr_info("virtio_gpu_send: waiting...\n");
  uint16_t wait_idx = used->idx;
  while (used->idx == wait_idx) {
    __asm__ volatile("nop");
  }
  // pr_info("virtio_gpu_send: done\n");

  return 0;
}

/*
 * Initialize the GPU
 */
void virtio_gpu_init(void) {
  pr_info("VirtIO-GPU: Probing...\n");

  for (int i = 0; i < VIRTIO_COUNT; i++) {
    uint64_t base = VIRTIO_MMIO_BASE + i * VIRTIO_MMIO_STRIDE;
    uint32_t magic = VIRTIO_READ(base, VIRTIO_MMIO_MAGIC_VALUE);
    if (magic != 0x74726976)
      continue;
    uint32_t device_id = VIRTIO_READ(base, VIRTIO_MMIO_DEVICE_ID);
    if (device_id == VIRTIO_DEV_GPU) {
      pr_info("VirtIO-GPU: Found at 0x%08lx\n", base);
      virtio_gpu_base = base;
      break;
    }
  }

  if (!virtio_gpu_base) {
    pr_info("VirtIO-GPU: Not found.\n");
    return;
  }

  /* Reset */
  VIRTIO_WRITE(virtio_gpu_base, VIRTIO_MMIO_STATUS, 0);

  /* Set ACKNOWLEDGE and DRIVER */
  uint32_t status = VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER;
  VIRTIO_WRITE(virtio_gpu_base, VIRTIO_MMIO_STATUS, status);

  /* Negotiate Features */
  uint32_t features = VIRTIO_READ(virtio_gpu_base, VIRTIO_MMIO_DEVICE_FEATURES);
  VIRTIO_WRITE(virtio_gpu_base, VIRTIO_MMIO_DRIVER_FEATURES, features);

  status |= VIRTIO_STATUS_FEATURES_OK;
  VIRTIO_WRITE(virtio_gpu_base, VIRTIO_MMIO_STATUS, status);

  if (!(VIRTIO_READ(virtio_gpu_base, VIRTIO_MMIO_STATUS) &
        VIRTIO_STATUS_FEATURES_OK)) {
    pr_info("VirtIO-GPU: Feature negotiation failed\n");
    return;
  }

  pr_info("VirtIO-GPU: Features OK. Setting up queues...\n");

  /* Queue Setup */
  VIRTIO_WRITE(virtio_gpu_base, VIRTIO_MMIO_QUEUE_SEL, 0); /* Control Queue */
  uint32_t qmax = VIRTIO_READ(virtio_gpu_base, VIRTIO_MMIO_QUEUE_NUM_MAX);
  uint32_t qsize = (qmax > 16) ? 16 : qmax;
  virtio_gpu_qsize = qsize;
  VIRTIO_WRITE(virtio_gpu_base, VIRTIO_MMIO_QUEUE_NUM, qsize);
  VIRTIO_WRITE(virtio_gpu_base, VIRTIO_MMIO_GUEST_PAGE_SIZE, 4096);

  /* Alloc Ring */
  void *qmem = pmm_alloc_pages(2); // 8KB
  if (!qmem)
    return;
  memset(qmem, 0, 8192);
  VIRTIO_WRITE(virtio_gpu_base, VIRTIO_MMIO_QUEUE_PFN, (uint64_t)qmem >> 12);

  desc = (struct vring_desc *)qmem;
  avail = (struct vring_avail *)((uint8_t *)qmem + qsize * 16);
  used = (struct vring_used *)((uint8_t *)qmem + 4096);

  /* DRIVER_OK */
  VIRTIO_WRITE(virtio_gpu_base, VIRTIO_MMIO_STATUS,
               VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER |
                   VIRTIO_STATUS_DRIVER_OK);

  pr_info("VirtIO-GPU: Driver Initialized.\n");

  /* ALLOCATE TEMPORARY CMD BUFFERS ON HEAP TO AVOID STACK OVERFLOW */
  /* We use 2 pages: one for command, one for response */
  void *cmd_page = pmm_alloc_page();
  void *resp_page = pmm_alloc_page();
  if (!cmd_page || !resp_page) {
    pr_info("VirtIO-GPU: Failed to alloc cmd buffers\n");
    return;
  }
  memset(cmd_page, 0, 4096);
  memset(resp_page, 0, 4096);

  /* 1. Get Display Info */
  struct virtio_gpu_ctrl_hdr *get_info_cmd =
      (struct virtio_gpu_ctrl_hdr *)cmd_page;
  struct virtio_gpu_resp_display_info *info_resp =
      (struct virtio_gpu_resp_display_info *)resp_page;

  get_info_cmd->type = VIRTIO_GPU_CMD_GET_DISPLAY_INFO;

  virtio_gpu_send(get_info_cmd, sizeof(*get_info_cmd), info_resp,
                  sizeof(*info_resp));

  if (info_resp->hdr.type == VIRTIO_GPU_RESP_OK_DISPLAY_INFO) {
    pr_info("VirtIO-GPU: Display 0: %dx%d enabled=%d\n",
            info_resp->pmodes[0].r.width, info_resp->pmodes[0].r.height,
            info_resp->pmodes[0].enabled);
  }

  /* 2. Create 2D Resource */
  uint32_t width = 800;
  uint32_t height = 600;
  uint32_t size = width * height * 4;
  uint32_t pages = (size + 4095) / 4096;

  /* Allocate Backing Store */
  void *backing_store = pmm_alloc_pages(pages);
  if (!backing_store) {
    pr_info("VirtIO-GPU: Failed to allocate framebuffer memory.\n");
    return;
  }
  pr_info("VirtIO-GPU: Backing Store at %p (Size: %d)\n", backing_store, size);
  memset(backing_store, 0xFF, size); // White background

  /* SETUP GLOBAL FB STRUCT */
  g_fb.width = width;
  g_fb.height = height;
  g_fb.stride = width * 4;
  g_fb.bpp = 32;
  g_fb.base_addr = backing_store;
  g_fb.resource_id = 1;

  /* Reuse buffers */
  memset(cmd_page, 0, 4096);
  memset(resp_page, 0, 4096);

  struct virtio_gpu_resource_create_2d *create_cmd =
      (struct virtio_gpu_resource_create_2d *)cmd_page;
  struct virtio_gpu_ctrl_hdr *generic_resp =
      (struct virtio_gpu_ctrl_hdr *)resp_page;

  create_cmd->hdr.type = VIRTIO_GPU_CMD_RESOURCE_CREATE_2D;
  create_cmd->resource_id = 1;
  create_cmd->format = VIRTIO_GPU_FORMAT_B8G8R8A8_UNORM;
  create_cmd->width = width;
  create_cmd->height = height;

  virtio_gpu_send(create_cmd, sizeof(*create_cmd), generic_resp,
                  sizeof(*generic_resp));

  /* 3. Attach Backing */
  memset(cmd_page, 0, 4096);
  memset(resp_page, 0, 4096);

  /* We construct attach struct manually in buffer */
  struct virtio_gpu_resource_attach_backing *attach_hdr =
      (struct virtio_gpu_resource_attach_backing *)cmd_page;
  struct virtio_gpu_mem_entry *entries =
      (struct virtio_gpu_mem_entry
           *)(cmd_page + sizeof(struct virtio_gpu_resource_attach_backing));

  attach_hdr->hdr.type = VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING;
  attach_hdr->resource_id = 1;
  attach_hdr->nr_entries = 1;

  entries[0].addr = (uint64_t)backing_store;
  entries[0].length = size;

  virtio_gpu_send(attach_hdr,
                  sizeof(*attach_hdr) + sizeof(struct virtio_gpu_mem_entry),
                  generic_resp, sizeof(*generic_resp));

  /* 4. Set Scanout */
  memset(cmd_page, 0, 4096);
  memset(resp_page, 0, 4096);

  struct virtio_gpu_set_scanout *scanout_cmd =
      (struct virtio_gpu_set_scanout *)cmd_page;

  scanout_cmd->hdr.type = VIRTIO_GPU_CMD_SET_SCANOUT;
  scanout_cmd->r.x = 0;
  scanout_cmd->r.y = 0;
  scanout_cmd->r.width = width;
  scanout_cmd->r.height = height;
  scanout_cmd->scanout_id = 0;
  scanout_cmd->resource_id = 1;

  virtio_gpu_send(scanout_cmd, sizeof(*scanout_cmd), generic_resp,
                  sizeof(*generic_resp));

  /* 5. Initial Flush */
  virtio_gpu_flush(0, 0, width, height);

  pr_info("VirtIO-GPU: Mode Set 800x600 OK. Backing at %p\n", backing_store);

  /* Clean up cmd buffers */
  pmm_free_page(cmd_page);
  pmm_free_page(resp_page);
}

void virtio_gpu_flush(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
  if (!virtio_gpu_base)
    return;

  // pr_info("VirtIO-GPU: Flushing %d,%d %dx%d\n", x, y, w, h);

  /* Use heap buffers */
  void *cmd_page = pmm_alloc_page();
  void *resp_page = pmm_alloc_page();
  if (!cmd_page || !resp_page)
    return;
  memset(cmd_page, 0, 4096);
  memset(resp_page, 0, 4096);

  /* 1. Transfer To Host */
  struct virtio_gpu_transfer_to_host_2d *cmd =
      (struct virtio_gpu_transfer_to_host_2d *)cmd_page;
  struct virtio_gpu_ctrl_hdr *resp = (struct virtio_gpu_ctrl_hdr *)resp_page;

  cmd->hdr.type = VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D;
  cmd->r.x = x;
  cmd->r.y = y;
  cmd->r.width = w;
  cmd->r.height = h;
  cmd->offset = (y * g_fb.width + x) * 4;
  cmd->resource_id = 1;

  virtio_gpu_send(cmd, sizeof(*cmd), resp, sizeof(*resp));

  /* 2. Resource Flush */
  memset(cmd_page, 0, 4096);
  memset(resp_page, 0, 4096);

  struct virtio_gpu_resource_flush *flush_cmd =
      (struct virtio_gpu_resource_flush *)cmd_page;

  flush_cmd->hdr.type = VIRTIO_GPU_CMD_RESOURCE_FLUSH;
  flush_cmd->r.x = x;
  flush_cmd->r.y = y;
  flush_cmd->r.width = w;
  flush_cmd->r.height = h;
  flush_cmd->resource_id = 1;

  virtio_gpu_send(flush_cmd, sizeof(*flush_cmd), resp, sizeof(*resp));

  pmm_free_page(cmd_page);
  pmm_free_page(resp_page);
}
