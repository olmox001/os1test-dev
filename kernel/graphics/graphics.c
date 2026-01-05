/*
 * kernel/graphics/graphics.c
 * Basic Graphics Primitives and Double Buffering
 */
#include <drivers/virtio_gpu.h>
#include <kernel/graphics.h>
#include <kernel/pmm.h>
#include <kernel/printk.h>
#include <kernel/string.h>

/* Global Graphics Context (Backbuffer) */
static struct graphics_context g_ctx = {0};

/*
 * Initialize Graphics Subsystem
 * Allocates a backbuffer for double buffering.
 */
void graphics_init(void) {
  if (!g_fb.base_addr) {
    pr_err("Graphics: No GPU framebuffer found.\n");
    return;
  }

  g_ctx.width = g_fb.width;
  g_ctx.height = g_fb.height;
  g_ctx.bpp = g_fb.bpp;
  g_ctx.stride = g_fb.width * 4; // Assuming 32bpp for now

  /* Allocate Backbuffer (Size = width * height * 4) */
  uint32_t size = g_ctx.width * g_ctx.height * 4;
  uint32_t pages = (size + 4095) / 4096;

  g_ctx.buffer = (uint32_t *)pmm_alloc_pages(pages);
  if (!g_ctx.buffer) {
    pr_err("Graphics: Failed to allocate backbuffer.\n");
    return;
  }

  /* Clear Backbuffer to Black */
  memset(g_ctx.buffer, 0, size);

  pr_info("Graphics: Initialized. Backbuffer at %p (%dx%d)\n", g_ctx.buffer,
          g_ctx.width, g_ctx.height);
}

struct graphics_context *graphics_get_context(void) { return &g_ctx; }

/*
 * Put Pixel (Clipping included)
 */
void graphics_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
  if (!g_ctx.buffer)
    return;
  if (x >= g_ctx.width || y >= g_ctx.height)
    return;

  g_ctx.buffer[y * g_ctx.width + x] = color;
}

/*
 * Fill Rectangle
 */
void graphics_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                        uint32_t color) {
  if (!g_ctx.buffer)
    return;

  /* Clip to screen */
  if (x >= g_ctx.width || y >= g_ctx.height)
    return;
  if (x + w > g_ctx.width)
    w = g_ctx.width - x;
  if (y + h > g_ctx.height)
    h = g_ctx.height - y;

  for (uint32_t row = 0; row < h; row++) {
    uint32_t *line = g_ctx.buffer + (y + row) * g_ctx.width + x;
    for (uint32_t col = 0; col < w; col++) {
      line[col] = color;
    }
  }
}

/*
 * Clear screen to color
 */
void graphics_clear(uint32_t color) {
  if (!g_ctx.buffer)
    return;
  for (uint32_t i = 0; i < g_ctx.width * g_ctx.height; i++) {
    g_ctx.buffer[i] = color;
  }
}

/*
 * Swap Buffers
 * Copies backbuffer to frontbuffer (physical framebuffer) and flushes GPU.
 */
void graphics_swap_buffers(void) {
  if (!g_ctx.buffer || !g_fb.base_addr)
    return;

  /* Memcpy Backbuffer -> Frontbuffer */
  /* Accessing g_fb.base_addr directly assumes it's mapped and writable */
  memcpy(g_fb.base_addr, g_ctx.buffer, g_ctx.width * g_ctx.height * 4);

  /* Flush GPU */
  virtio_gpu_flush(0, 0, g_ctx.width, g_ctx.height);
}
