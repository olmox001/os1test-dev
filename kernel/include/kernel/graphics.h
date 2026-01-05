/*
 * kernel/include/kernel/graphics.h
 * Graphics Subsystem Definitions
 */
#ifndef _KERNEL_GRAPHICS_H
#define _KERNEL_GRAPHICS_H

#include <kernel/types.h>

/* Color Macros (ARGB 8888) */
#define COLOR_BLACK 0xFF000000
#define COLOR_WHITE 0xFFFFFFFF
#define COLOR_RED 0xFFFF0000
#define COLOR_GREEN 0xFF00FF00
#define COLOR_BLUE 0xFF0000FF
#define COLOR_YELLOW 0xFFFFFF00
#define COLOR_CYAN 0xFF00FFFF
#define COLOR_MAGENTA 0xFFFF00FF
#define COLOR_GRAY 0xFF808080
#define COLOR_DARK_GRAY 0xFF404040

/* RGB/ARGB Helpers */
#define RGB(r, g, b) (0xFF000000 | ((r) << 16) | ((g) << 8) | (b))
#define ARGB(a, r, g, b) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))

/* Basic Graphics Context */
struct graphics_context {
  uint32_t width;
  uint32_t height;
  uint32_t bpp;
  uint32_t stride;
  uint32_t *buffer; /* Pixel buffer (Backbuffer or Framebuffer) */
};

/* ============================================
 * Core Graphics API
 * ============================================ */
void graphics_init(void);
struct graphics_context *graphics_get_context(void);

/* Basic Primitives */
void graphics_put_pixel(uint32_t x, uint32_t y, uint32_t color);
void graphics_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                        uint32_t color);
void graphics_clear(uint32_t color);

/* Double Buffering */
void graphics_swap_buffers(void);

/* ============================================
 * 2D Drawing API (draw2d.c)
 * ============================================ */
void graphics_draw_line(int x0, int y0, int x1, int y1, uint32_t color);
void graphics_draw_circle(int cx, int cy, int r, uint32_t color);
void graphics_fill_circle(int cx, int cy, int r, uint32_t color);
void graphics_draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2,
                            uint32_t color);
void graphics_fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2,
                            uint32_t color);
void graphics_draw_rounded_rect(int x, int y, int w, int h, int r,
                                uint32_t color);
void graphics_draw_gradient_h(int x, int y, int w, int h, uint32_t color_left,
                              uint32_t color_right);
uint32_t graphics_blend(uint32_t fg, uint32_t bg);

/* ============================================
 * Bitmap Font API (font.c)
 * ============================================ */
void graphics_draw_char(uint32_t x, uint32_t y, char c, uint32_t color);
void graphics_draw_string(uint32_t x, uint32_t y, const char *str,
                          uint32_t color);

/* ============================================
 * TrueType Font API (ttf.c) - Future
 * ============================================ */
/* int ttf_load_font(const uint8_t *data, size_t size);
 * void ttf_render_string(int x, int y, const char *str, int size, uint32_t
 * color);
 */

/* ============================================
 * 3D Rendering API (draw3d.c)
 * ============================================ */
typedef struct {
  float x, y, z, w;
} vec4_t;
typedef struct {
  float m[4][4];
} mat4_t;

void render3d_init(uint32_t width, uint32_t height);
void render3d_clear_zbuffer(void);

mat4_t mat4_identity(void);
mat4_t mat4_translate(float x, float y, float z);
mat4_t mat4_scale(float x, float y, float z);
mat4_t mat4_rotate_y(float angle);
mat4_t mat4_mul(mat4_t a, mat4_t b);
vec4_t mat4_mul_vec(mat4_t m, vec4_t v);
mat4_t mat4_perspective(float fov, float aspect, float near, float far);

void render3d_triangle(vec4_t v0, vec4_t v1, vec4_t v2, mat4_t mvp,
                       uint32_t color, int screen_w, int screen_h);
void render3d_cube(float x, float y, float z, float size, mat4_t view_proj,
                   uint32_t color, int screen_w, int screen_h);

/* ============================================
 * Compositor API (compositor.c)
 * ============================================ */
void compositor_init(void);
int compositor_create_window(int x, int y, int w, int h, const char *title,
                             int pid);
void compositor_destroy_window(int window_id);
uint32_t *compositor_get_buffer(int window_id);
void compositor_move_window(int window_id, int x, int y);
void compositor_handle_click(int button, int state);
void compositor_render(void);
void compositor_draw_rect(int window_id, int x, int y, int w, int h,
                          uint32_t color, int caller_pid);

#endif /* _KERNEL_GRAPHICS_H */
