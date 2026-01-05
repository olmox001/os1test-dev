/*
 * kernel/graphics/compositor.c
 * Window Compositor
 *
 * Manages windows and composites them to the screen.
 */
#include <kernel/graphics.h>
#include <kernel/kmalloc.h>
#include <kernel/printk.h>
#include <kernel/string.h>
#include <kernel/types.h>

#define MAX_WINDOWS 16

/* Window Structure */
struct window {
  int id;
  int x, y;
  int width, height;
  int z_order;
  int visible;
  int pid;
  uint32_t *buffer;  /* Window's pixel buffer */
  uint32_t bg_color; /* Background color */
  char title[64];

  /* Terminal State */
  int cursor_x, cursor_y;
  uint32_t fg_color;
  int escape_state;
  char escape_buf[32];
  int escape_len;
};

/* Global State */
static struct window windows[MAX_WINDOWS];
static int window_count = 0;
static int next_window_id = 1;

/* Mouse State */
static int mouse_x = 400;
static int mouse_y = 300;
// static uint32_t mouse_color = COLOR_WHITE;

/* Dragging State */
static int dragging_window_id = -1;
static int drag_off_x = 0;
static int drag_off_y = 0;

/*
 * Initialize Compositor
 */
void compositor_init(void) {
  memset(windows, 0, sizeof(windows));
  window_count = 0;
  next_window_id = 1;
  pr_info("Compositor: Initialized\n");
}

/*
 * Create Window
 */
int compositor_create_window(int x, int y, int w, int h, const char *title,
                             int pid) {
  if (window_count >= MAX_WINDOWS) {
    pr_err("Compositor: Max windows reached\n");
    return -1;
  }

  /* Find free slot */
  int slot = -1;
  for (int i = 0; i < MAX_WINDOWS; i++) {
    if (windows[i].id == 0) {
      slot = i;
      break;
    }
  }

  if (slot < 0)
    return -1;

  /* Allocate window buffer */
  size_t buffer_size = w * h * sizeof(uint32_t);
  uint32_t *buffer = (uint32_t *)kmalloc(buffer_size);
  if (!buffer) {
    pr_err("Compositor: Failed to allocate window buffer\n");
    return -1;
  }
  /* Initialize clear background */
  for (int i = 0; i < w * h; i++)
    buffer[i] = 0xFF17171A;

  /* Initialize window */
  windows[slot].id = next_window_id++;
  windows[slot].x = x;
  windows[slot].y = y;
  windows[slot].width = w;
  windows[slot].height = h;
  windows[slot].z_order = window_count;
  windows[slot].visible = 1;
  windows[slot].pid = pid;
  windows[slot].buffer = buffer;
  windows[slot].bg_color = 0xFF17171A;
  /* Copy title */
  int len = 0;
  while (title[len] && len < 63) {
    windows[slot].title[len] = title[len];
    len++;
  }
  windows[slot].title[len] = '\0';

  /* Initialize terminal state */
  windows[slot].cursor_x = 0;
  windows[slot].cursor_y = 0;
  windows[slot].fg_color = 0xFFFFFFFF;
  windows[slot].escape_state = 0;
  windows[slot].escape_len = 0;

  /* Clear buffer to background */
  for (int i = 0; i < w * h; i++) {
    buffer[i] = windows[slot].bg_color;
  }

  window_count++;

  pr_info("Compositor: Created window '%s' (%dx%d) at (%d,%d)\n", title, w, h,
          x, y);

  return windows[slot].id;
}

/*
 * Destroy Window
 */
void compositor_destroy_window(int window_id) {
  for (int i = 0; i < MAX_WINDOWS; i++) {
    if (windows[i].id == window_id) {
      if (windows[i].buffer) {
        kfree(windows[i].buffer);
      }
      memset(&windows[i], 0, sizeof(struct window));
      window_count--;
      return;
    }
  }
}

/*
 * Get Window Buffer (for direct drawing)
 */
uint32_t *compositor_get_buffer(int window_id) {
  for (int i = 0; i < MAX_WINDOWS; i++) {
    if (windows[i].id == window_id) {
      return windows[i].buffer;
    }
  }
  return NULL;
}

/*
 * Find window by PID
 */
int compositor_get_window_by_pid(int pid) {
  // pr_info("Compositor: Looking for window for PID %d\n", pid);
  for (int i = 0; i < MAX_WINDOWS; i++) {
    if (windows[i].id != 0) {
      // pr_info("  WinID=%d PID=%d\n", windows[i].id, windows[i].pid);
      if (windows[i].pid == pid) {
        // pr_info("  FOUND!\n");
        return windows[i].id;
      }
    }
  }
  return -1;
}

/*
 * Move Window
 */
void compositor_move_window(int window_id, int x, int y) {
  for (int i = 0; i < MAX_WINDOWS; i++) {
    if (windows[i].id == window_id) {
      windows[i].x = x;
      windows[i].y = y;
      return;
    }
  }
}

/*
 * Alpha blend two colors
 */
static inline uint32_t blend_pixel(uint32_t fg, uint32_t bg) {
  uint32_t alpha = (fg >> 24) & 0xFF;
  if (alpha == 255)
    return fg;
  if (alpha == 0)
    return bg;

  uint32_t inv_alpha = 255 - alpha;
  uint32_t r =
      (((fg >> 16) & 0xFF) * alpha + ((bg >> 16) & 0xFF) * inv_alpha) >> 8;
  uint32_t g =
      (((fg >> 8) & 0xFF) * alpha + ((bg >> 8) & 0xFF) * inv_alpha) >> 8;
  uint32_t b = ((fg & 0xFF) * alpha + (bg & 0xFF) * inv_alpha) >> 8;

  return 0xFF000000 | (r << 16) | (g << 8) | b;
}

/*
 * Draw Window Decorations (Title Bar)
 */
static void draw_window_decorations(struct window *win) {
  struct graphics_context *ctx = graphics_get_context();
  if (!ctx || !ctx->buffer)
    return;

  int title_height = 20;
  uint32_t title_color = 0xFF18181B; // Quasi nero puro
  uint32_t border_color = 0xFF27272A;

  /* Title bar */
  for (int y = 0; y < title_height; y++) {
    for (int x = 0; x < win->width + 2; x++) {
      int px = win->x - 1 + x;
      int py = win->y - title_height + y;
      if (px >= 0 && px < (int)ctx->width && py >= 0 && py < (int)ctx->height) {
        ctx->buffer[py * ctx->width + px] = title_color;
      }
    }
  }

  /* Draw title text */
  int title_len = 0;
  while (win->title[title_len])
    title_len++;

  int char_w = 8;
  int text_w = title_len * char_w;
  int start_x = win->x + (win->width - text_w) / 2;
  int start_y = win->y - title_height + 2;

  for (int i = 0; i < title_len; i++) {
    graphics_draw_char(start_x + i * char_w, start_y, win->title[i],
                       0xFFFFFFFF);
  }

  /* Border */
  for (int y = -title_height; y <= win->height; y++) {
    int px1 = win->x - 1;
    int px2 = win->x + win->width;
    int py = win->y + y;
    if (py >= 0 && py < (int)ctx->height) {
      if (px1 >= 0)
        ctx->buffer[py * ctx->width + px1] = border_color;
      if (px2 < (int)ctx->width)
        ctx->buffer[py * ctx->width + px2] = border_color;
    }
  }
  for (int x = -1; x <= win->width; x++) {
    int px = win->x + x;
    int py1 = win->y - title_height - 1;
    int py2 = win->y + win->height;
    if (px >= 0 && px < (int)ctx->width) {
      if (py1 >= 0)
        ctx->buffer[py1 * ctx->width + px] = border_color;
      if (py2 < (int)ctx->height)
        ctx->buffer[py2 * ctx->width + px] = border_color;
    }
  }
}

/*
 * Process ANSI SGR (Select Graphic Rendition) parameters
 */
static void handle_sgr(struct window *win) {
  if (win->escape_len == 0) {
    win->fg_color = 0xFFFFFFFF;
    return;
  }

  int val = 0;
  for (int i = 0; i < win->escape_len; i++) {
    if (win->escape_buf[i] >= '0' && win->escape_buf[i] <= '9') {
      val = val * 10 + (win->escape_buf[i] - '0');
    }
  }

  if (val == 0) {
    win->fg_color = 0xFFFFFFFF;
  } else if (val >= 30 && val <= 37) {
    uint32_t colors[] = {0xFF000000, 0xFFBB0000, 0xFF00BB00, 0xFFBBBB00,
                         0xFF0000BB, 0xFFBB00BB, 0xFF00BBBB, 0xFFBBBBBB};
    win->fg_color = colors[val - 30];
  } else if (val >= 90 && val <= 97) {
    uint32_t colors[] = {0xFF555555, 0xFFFF5555, 0xFF55FF55, 0xFFFFFF55,
                         0xFF5555FF, 0xFFFF55FF, 0xFF55FFFF, 0xFFFFFFFF};
    win->fg_color = colors[val - 90];
  }
}

/*
 * Write text to a window (Terminal Emulator)
 */
void compositor_window_write(int win_id, const char *buf, size_t count) {
  struct window *win = NULL;
  for (int i = 0; i < MAX_WINDOWS; i++) {
    if (windows[i].id == win_id) {
      win = &windows[i];
      break;
    }
  }
  if (!win || !win->buffer)
    return;

  int char_w = 8;
  int char_h = 16;
  int cols = win->width / char_w;
  int rows = win->height / char_h;

  /* Temporarily swap context buffer to window buffer for drawing */
  struct graphics_context *ctx = graphics_get_context();
  uint32_t *screen_buffer = ctx->buffer;
  uint32_t screen_width = ctx->width;
  uint32_t screen_height = ctx->height;

  ctx->buffer = win->buffer;
  ctx->width = win->width;
  ctx->height = win->height;

  for (size_t i = 0; i < count; i++) {
    char c = buf[i];

    if (win->escape_state == 0) {
      if (c == '\033') {
        win->escape_state = 1;
        win->escape_len = 0;
      } else if (c == '\n') {
        win->cursor_x = 0;
        win->cursor_y++;
      } else if (c == '\r') {
        win->cursor_x = 0;
      } else if (c == '\b' || c == 127) {
        if (win->cursor_x > 0)
          win->cursor_x--;
      } else if (c >= 32 && c < 127) {
        /* Clear char background */
        compositor_draw_rect(win_id, win->cursor_x * char_w,
                             win->cursor_y * char_h, char_w, char_h,
                             win->bg_color);

        graphics_draw_char(win->cursor_x * char_w, win->cursor_y * char_h, c,
                           win->fg_color);

        win->cursor_x++;
      }
    } else if (win->escape_state == 1) {
      if (c == '[')
        win->escape_state = 2;
      else
        win->escape_state = 0;
    } else if (win->escape_state == 2) {
      if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
        if (c == 'm') {
          handle_sgr(win);
        } else if (c == 'J') {
          for (int p = 0; p < win->width * win->height; p++)
            win->buffer[p] = win->bg_color;
          win->cursor_x = 0;
          win->cursor_y = 0;
        } else if (c == 'H') {
          win->cursor_x = 0;
          win->cursor_y = 0;
        }
        win->escape_state = 0;
      } else if (win->escape_len < 31) {
        win->escape_buf[win->escape_len++] = c;
      } else {
        win->escape_state = 0;
      }
    }

    if (win->cursor_x >= cols) {
      win->cursor_x = 0;
      win->cursor_y++;
    }
    if (win->cursor_y >= rows) {
      /* Scroll buffer up by one line */
      size_t line_size = win->width * char_h;
      memmove(win->buffer, win->buffer + line_size,
              win->width * (win->height - char_h) * 4);
      /* Clear last line */
      for (int p = win->width * (win->height - char_h);
           p < win->width * win->height; p++) {
        win->buffer[p] = win->bg_color;
      }
      win->cursor_y = rows - 1;
    }
  }

  /* Restore screen buffer */
  ctx->buffer = screen_buffer;
  ctx->width = screen_width;
  ctx->height = screen_height;

  /* Force compositor to refresh */
  compositor_render();
}

/*
 * Draw simple mouse cursor
 */

static void draw_mouse_cursor(struct graphics_context *ctx) {
  /* Definiamo la forma del cursore in stile "Apple/Modern OS".
     Legenda:
     ' ' = Trasparente
     'X' = Bordo Bianco (Outline) - Per visibilità su scuro
     '.' = Riempimento Nero (Fill) - Colore principale
  */
  static const char *cursor_shape[] = {
      "X           ", "XX          ", "X.X         ", "X..X        ",
      "X...X       ", "X....X      ", "X.....X     ", "X......X    ",
      "X.......X   ", "X........X  ", "X.....XXXXX ", "X..X..X     ",
      "X.X X..X    ", "XX  X..X    ", "X    XX     ", "     XX     "};

  int height = 16; // Altezza della matrice sopra
  int width = 12;  // Larghezza massima della matrice

  // Colori in formato ARGB (Assumendo 0xAARRGGBB)
  // Se il tuo sistema usa un formato diverso, modifica questi valori.
  int border_color = 0xFFFFFFFF; // Bianco puro
  int fill_color = 0xFF000000;   // Nero puro

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      // Calcola la posizione assoluta nel buffer
      int px = mouse_x + x;
      int py = mouse_y + y;

      // Controllo Bounds (per non scrivere fuori dallo schermo)
      if (px >= 0 && px < (int)ctx->width && py >= 0 && py < (int)ctx->height) {

        char pixel_type = cursor_shape[y][x];

        if (pixel_type == 'X') {
          // Disegna Bordo
          ctx->buffer[py * ctx->width + px] = border_color;
        } else if (pixel_type == '.') {
          // Disegna Riempimento
          ctx->buffer[py * ctx->width + px] = fill_color;
        }
        // Se è ' ', non facciamo nulla (trasparenza)
      }
    }
  }
}

/*
 * Handle Mouse Click
 */
void compositor_handle_click(int button, int state) {
  (void)button;

  if (state == 0) {
    /* Release - Stop dragging */
    dragging_window_id = -1;
    return;
  }

  if (state != 1) /* Only handle press down */
    return;

  /* Debug click */
  /* pr_info("Click: %d, %d\n", mouse_x, mouse_y); */

  /* Check for window hit (front to back in z-order) */
  struct window *hit = NULL;
  int max_z = -1;

  for (int i = 0; i < MAX_WINDOWS; i++) {
    if (windows[i].id != 0 && windows[i].visible) {
      if (mouse_x >= windows[i].x &&
          mouse_x < windows[i].x + windows[i].width &&
          mouse_y >= windows[i].y &&
          mouse_y < windows[i].y + windows[i].height) {
        if (windows[i].z_order > max_z) {
          max_z = windows[i].z_order;
          hit = &windows[i];
        }
      }
    }
  }

  if (hit) {
    /* Bring to front */
    int top_z = 0;
    for (int i = 0; i < MAX_WINDOWS; i++) {
      if (windows[i].id != 0 && windows[i].z_order > top_z)
        top_z = windows[i].z_order;
    }
    hit->z_order = top_z + 1;

    /* Check for drag start (Title bar area, approx top 20 pixels) */
    if (mouse_y < hit->y + 20) {
      dragging_window_id = hit->id;
      drag_off_x = mouse_x - hit->x;
      drag_off_y = mouse_y - hit->y;
    }

    compositor_render();
  }
}

/*
 * Update Mouse Position
 */
void compositor_update_mouse(int dx, int dy, int absolute) {
  struct graphics_context *ctx = graphics_get_context();
  if (!ctx)
    return;

  if (absolute) {
    mouse_x = dx;
    mouse_y = dy;
  } else {
    mouse_x += dx;
    mouse_y += dy;
  }

  /* Handle Dragging */
  if (dragging_window_id != -1) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
      if (windows[i].id == dragging_window_id) {
        windows[i].x = mouse_x - drag_off_x;
        windows[i].y = mouse_y - drag_off_y;
        /* Simple bounds check to keep title bar somewhat visible */
        if (windows[i].y < 0)
          windows[i].y = 0;
        break;
      }
    }
  }

  /* pr_info("Mouse update: dx=%d, dy=%d -> (%d, %d)\n", dx, dy, mouse_x,
   * mouse_y); */

  /* Clamp to screen */
  if (mouse_x < 0)
    mouse_x = 0;
  if (mouse_x >= (int)ctx->width)
    mouse_x = (int)ctx->width - 1;
  if (mouse_y < 0)
    mouse_y = 0;
  if (mouse_y >= (int)ctx->height)
    mouse_y = (int)ctx->height - 1;
}

/*
 * Composite All Windows to Screen
 */
void compositor_render(void) {
  struct graphics_context *ctx = graphics_get_context();
  if (!ctx || !ctx->buffer)
    return;

  /* Clear to desktop background (gradient) */
  for (uint32_t y = 0; y < ctx->height; y++) {
    for (uint32_t x = 0; x < ctx->width; x++) {
      /* Gradient from dark blue to lighter blue */
      uint32_t r = 20;
      uint32_t g = 40 + (y * 40 / ctx->height);
      uint32_t b = 80 + (y * 80 / ctx->height);
      ctx->buffer[y * ctx->width + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
    }
  }

  /* Sort windows by z-order (simple bubble sort) */
  struct window *sorted[MAX_WINDOWS];
  int count = 0;
  for (int i = 0; i < MAX_WINDOWS; i++) {
    if (windows[i].id != 0 && windows[i].visible) {
      sorted[count++] = &windows[i];
    }
  }

  for (int i = 0; i < count - 1; i++) {
    for (int j = 0; j < count - i - 1; j++) {
      if (sorted[j]->z_order > sorted[j + 1]->z_order) {
        struct window *tmp = sorted[j];
        sorted[j] = sorted[j + 1];
        sorted[j + 1] = tmp;
      }
    }
  }

  /* Render windows back to front */
  for (int i = 0; i < count; i++) {
    struct window *win = sorted[i];

    /* Draw decorations */
    draw_window_decorations(win);

    /* Blit window content */
    for (int wy = 0; wy < win->height; wy++) {
      for (int wx = 0; wx < win->width; wx++) {
        int px = win->x + wx;
        int py = win->y + wy;

        if (px >= 0 && px < (int)ctx->width && py >= 0 &&
            py < (int)ctx->height) {
          uint32_t src = win->buffer[wy * win->width + wx];
          uint32_t dst = ctx->buffer[py * ctx->width + px];
          ctx->buffer[py * ctx->width + px] = blend_pixel(src, dst);
        }
      }
    }
  }

  /* pr_info("Compositor: Rendering frame (mouse at %d,%d)\n", mouse_x,
   * mouse_y); */

  /* Draw Cursor */
  draw_mouse_cursor(ctx);

  /* Flush to screen */
  graphics_swap_buffers();
}

/*
 * Draw to Window
 */
void compositor_draw_rect(int window_id, int x, int y, int w, int h,
                          uint32_t color) {
  for (int i = 0; i < MAX_WINDOWS; i++) {
    if (windows[i].id == window_id && windows[i].buffer) {
      for (int dy = 0; dy < h; dy++) {
        for (int dx = 0; dx < w; dx++) {
          int px = x + dx;
          int py = y + dy;
          if (px >= 0 && px < windows[i].width && py >= 0 &&
              py < windows[i].height) {
            windows[i].buffer[py * windows[i].width + px] = color;
          }
        }
      }
      return;
    }
  }
}
