/*
 * kernel/graphics/font.c
 * Font rendering and management
 *
 * Role:
 *   Per-glyph alpha-masked blit from a pre-rasterised bitmap font embedded
 *   via <graphics/default_font.h> (Rewir-Light.ttf, compiled to a C array).
 *   Provides gl_draw_char / gl_draw_string for use by compositor.c's terminal
 *   emulator, and exports font metric queries (height, ascent, max width).
 *   Also implements sys_set_font(), the syscall entry point (syscall 253) that
 *   allows userland to replace the active font at runtime.
 *
 * Font state:
 *   The singleton current_font struct holds a font_header (magic, first_char,
 *   num_chars, ascent, descent, bitmap_size), a pointer to the glyph-info
 *   array (glyphs), a pointer to the bitmap data, and an is_dynamic flag.
 *   At startup, glyphs and bitmap point into the statically-linked default
 *   font arrays.  After sys_set_font(), they point into user-space memory.
 *
 * Rendering:
 *   gl_draw_char locates the glyph_info for a codepoint, reads the per-pixel
 *   alpha mask from the bitmap, and blends each non-zero pixel onto the
 *   surface using an >>8 approximation (not exact /255 division).  Fully
 *   opaque pixels (alpha=255) are written directly.
 *
 * Locking & IRQ context:
 *   No lock protects current_font.  gl_draw_char is called from
 *   compositor_window_write (under compositor_lock) and from
 *   compositor_render_internal (also under compositor_lock from
 *   compositor_tick, which fires from a timer IRQ).  sys_set_font is called
 *   from syscall context.  There is no synchronisation between them.
 *
 * Known issues:
 *   GFX-FONT-01 (W4 SECURITY BUG) sys_set_font stores the raw userland
 *               pointer 'data' directly into current_font.glyphs and
 *               current_font.bitmap without copy_from_user, address-space
 *               validation, or a kernel-heap copy.  These pointers are later
 *               dereferenced in gl_draw_char during IRQ-context rendering
 *               (compositor_tick).  Consequences: (a) process exit after
 *               set-font leaves dangling pointers — fault on next render;
 *               (b) a kernel-address passed as 'data' causes kernel-memory
 *               bytes to be rendered to the framebuffer (information
 *               disclosure); (c) if num_chars is near SIZE_MAX the expected
 *               size calculation overflows and size < expected passes
 *               spuriously.  Fix: copy the blob into kmalloc'd kernel memory
 *               inside sys_set_font, validate all internal offsets.
 *   GFX-FONT-02 (W3 BUG, FIXED) graphics_font_height() floors to the built-in
 *               default height when ascent+descent <= 0, so a malformed font
 *               (ascent=descent=0 via sys_set_font) can no longer divide-by-zero
 *               in compositor_create_window (h / char_h, compositor.c:204).
 */
#include <graphics/gl.h>
#include <kernel/graphics.h>
#include <kernel/types.h>
#include <font.h>
#include <drivers/gpu/gpu.h>

/* Include the pre-rasterized Rewir-Light.ttf */
#include <graphics/default_font.h>

/* Forward declarations */
/* utf8_decode: defined in kernel/lib/utf8.c (or equivalent); advances 's' by
 * the byte width of the first codepoint and writes the decoded value to *code.
 * Returns byte count consumed (1..4), or <= 0 on invalid sequence. */
int utf8_decode(const char *s, uint32_t *code);

/* Internal font state */
/*
 * current_font: singleton active font state.
 *   header   — copy of font_header (magic, first_char, num_chars, ascent,
 *              descent, bitmap_size).
 *   glyphs   — pointer to glyph-info array (num_chars entries).
 *   bitmap   — pointer to packed alpha bitmap data.
 *   is_dynamic — 0 if glyphs/bitmap point into static default_font arrays;
 *               1 if they point into a user-supplied buffer (see sys_set_font).
 *
 * NOTE(GFX-FONT-01): when is_dynamic==1, glyphs and bitmap are raw userland
 * virtual addresses stored without copy_from_user; they may become dangling
 * after process exit and are dereferenced from IRQ context.
 */
static struct {
    struct font_header header;
    const struct font_glyph_info *glyphs;
    const uint8_t *bitmap;
    int is_dynamic;
} current_font = {
    .header = {
        .magic = FONT_MAGIC,
        .first_char = FONT_FIRST_CHAR,
        .num_chars = FONT_NUM_CHARS,
        .ascent = FONT_ASCENT,
        .descent = FONT_DESCENT,
        .bitmap_size = 0
    },
    .glyphs = font_glyphs,
    .bitmap = font_bitmap,
    .is_dynamic = 0
};

/*
 * gl_draw_char - render one Unicode codepoint from the active font onto surf.
 *
 * Params: surf — target surface; x, y — top-left origin for the glyph cell;
 *         codepoint — Unicode scalar value; color — ARGB8888 foreground.
 *
 * Algorithm:
 *   1. Compute glyph index: idx = codepoint - first_char.  Out-of-range
 *      codepoints (idx < 0 or >= num_chars) are silently skipped.
 *   2. Look up font_glyph_info gi from current_font.glyphs[idx].
 *   3. Compute pixel origin: start_x = x + gi->x0,
 *      start_y = y + ascent + gi->y0  (baseline offset).
 *   4. Walk the glyph's alpha mask (gi->height rows x gi->width cols):
 *      - alpha==0: skip (transparent pixel).
 *      - alpha==255: direct write (no blend arithmetic).
 *      - otherwise: blend using >>8 approximation (not exact /255).
 *        Note: >>8 differs from /255 by at most 1 LSB; visible only at
 *        specific alpha values (e.g. alpha=128, dst=255 → 127 vs 128).
 *   5. Per-pixel bounds check against surf->width/height before any write.
 *
 * NOTE(GFX-FONT-01): current_font.glyphs and current_font.bitmap may point
 *   into userland memory if sys_set_font was called; dereferencing them here
 *   from IRQ context (compositor_tick) is unsafe.
 *
 * Locking: none; called under compositor_lock from compositor_window_write
 *          and compositor_render_internal.
 * Side effects: writes pixels to surf->buffer.
 */
/*
 * Draw character using GL
 */
void gl_draw_char(struct gl_surface *surf, int x, int y, uint32_t codepoint,
                  uint32_t color) {
  if (!surf || !current_font.bitmap)
    return;

  int idx = (int)codepoint - current_font.header.first_char;
  if (idx < 0 || idx >= current_font.header.num_chars)
    return;

  const struct font_glyph_info *gi = &current_font.glyphs[idx];
  const uint8_t *bitmap = current_font.bitmap + gi->data_offset;

  int start_x = x + gi->x0;
  int start_y = y + current_font.header.ascent + gi->y0;

  uint32_t r_color = (color >> 16) & 0xFF;
  uint32_t g_color = (color >> 8) & 0xFF;
  uint32_t b_color = color & 0xFF;

  for (int gy = 0; gy < gi->height; gy++) {
    for (int gx = 0; gx < gi->width; gx++) {
      uint8_t alpha = bitmap[gy * gi->width + gx];
      if (alpha == 0)
        continue;

      int px = start_x + gx;
      int py = start_y + gy;

      if (px >= 0 && px < (int)surf->width && py >= 0 && py < (int)surf->height) {
        if (alpha == 255) {
          surf->buffer[py * surf->stride + px] = color;
          continue;
        }

        /* Alpha blending approximation using shifts */
        uint32_t bg = surf->buffer[py * surf->stride + px];
        uint32_t inv_alpha = 255 - alpha;

        uint32_t r = (r_color * alpha + ((bg >> 16) & 0xFF) * inv_alpha) >> 8;
        uint32_t gr = (g_color * alpha + ((bg >> 8) & 0xFF) * inv_alpha) >> 8;
        uint32_t b = (b_color * alpha + (bg & 0xFF) * inv_alpha) >> 8;

        surf->buffer[py * surf->stride + px] = 0xFF000000 | (r << 16) | (gr << 8) | b;
      }
    }
  }
}

/*
 * graphics_char_width - return the horizontal advance width for a codepoint.
 *
 * Param: codepoint — Unicode scalar value.
 * Returns advance width in pixels from the active font's glyph_info, or 0 if
 * the codepoint is outside [first_char, first_char+num_chars).
 * Used by gl_draw_string and graphics_string_width to advance the cursor.
 *
 * Locking: none; reads current_font.glyphs (see NOTE GFX-FONT-01).
 */
/*
 * Get character advance width
 */
int graphics_char_width(uint32_t codepoint) {
  int idx = (int)codepoint - current_font.header.first_char;
  if (idx < 0 || idx >= current_font.header.num_chars)
    return 0;
  return current_font.glyphs[idx].advance;
}

/*
 * gl_draw_string - render a UTF-8 string left-to-right onto surf.
 *
 * Params: surf — target surface; x, y — origin of the first glyph cell;
 *         str — null-terminated UTF-8 string; color — ARGB8888 foreground.
 *
 * Decodes each codepoint via utf8_decode, renders it with gl_draw_char at the
 * current cursor_x, then advances cursor_x by graphics_char_width.  Invalid
 * UTF-8 sequences (consumed <= 0) consume one byte and continue.
 *
 * Locking: none; called under compositor_lock in compositor_render_internal.
 * Side effects: writes pixels to surf->buffer.
 */
/*
 * Draw string using GL (UTF-8 supported)
 */
void gl_draw_string(struct gl_surface *surf, int x, int y, const char *str,
                    uint32_t color) {
  if (!surf || !str)
    return;

  int cursor_x = x;
  uint32_t codepoint;
  int consumed;

  while (*str) {
    consumed = utf8_decode(str, &codepoint);
    if (consumed <= 0) {
        str++;
        continue;
    }
    gl_draw_char(surf, cursor_x, y, codepoint, color);
    cursor_x += graphics_char_width(codepoint);
    str += consumed;
  }
}

/*
 * graphics_string_width - compute pixel width of a UTF-8 string.
 *
 * Param: str — null-terminated UTF-8 string (NULL-safe: returns 0).
 * Sums graphics_char_width for each decoded codepoint.  Mirrors the cursor
 * advance in gl_draw_string so callers can centre text (e.g. title bar).
 *
 * Locking: none; reads current_font.glyphs (see NOTE GFX-FONT-01).
 */
/*
 * Get string width in pixels (UTF-8 supported)
 */
int graphics_string_width(const char *str) {
  if (!str)
    return 0;

  int width = 0;
  uint32_t codepoint;
  int consumed;

  while (*str) {
    consumed = utf8_decode(str, &codepoint);
    if (consumed <= 0) {
        str++;
        continue;
    }
    width += graphics_char_width(codepoint);
    str += consumed;
  }
  return width;
}

/*
 * graphics_font_height - return the total line height of the active font.
 *
 * Returns ascent + descent in pixels.  Used as char_h in compositor for row
 * count and scroll arithmetic.
 *
 * GFX-FONT-02 (fixed): floors to the built-in default height when
 *   ascent+descent <= 0, so a malformed font (both fields zero, via
 *   sys_set_font) can no longer cause a divide-by-zero at
 *   compositor_create_window (h / char_h).
 *
 * Locking: none; reads current_font.header (not IRQ-safe under sys_set_font
 *          race, see GFX-FONT-01).
 */
/*
 * Get font height
 */
int graphics_font_height(void) { 
    /* GFX-FONT-02: floor to the built-in default height so a malformed font
     * (ascent=descent=0 via sys_set_font) cannot make char_h==0 and trigger a
     * divide-by-zero in compositor row/scroll arithmetic — mirrors the
     * graphics_font_max_width() floor. */
    int h = current_font.header.ascent + current_font.header.descent;
    return h > 0 ? h : (FONT_ASCENT + FONT_DESCENT); 
}

/*
 * graphics_font_ascent - return the ascent of the active font in pixels.
 *
 * Ascent is the distance from the baseline to the top of the tallest glyph.
 * Used in gl_draw_char to compute start_y = y + ascent + gi->y0.
 *
 * Locking: none.
 */
/*
 * Get font ascent
 */
int graphics_font_ascent(void) {
    return current_font.header.ascent;
}

/*
 * Get max character width (for grid systems)
 */
int graphics_font_max_width(void) {
    int max_w = 0;
    for (int i = 0; i < current_font.header.num_chars; i++) {
        if (current_font.glyphs[i].advance > max_w)
            max_w = current_font.glyphs[i].advance;
    }
    return max_w > 0 ? max_w : 8;
}

/*
 * System Call: Set Font
 */
int sys_set_font(void *data, size_t size) {
    if (!data || size < sizeof(struct font_header)) return -1;

    struct font_header *h = (struct font_header *)data;
    if (h->magic != FONT_MAGIC) return -2;

    size_t expected = sizeof(struct font_header) + 
                     h->num_chars * sizeof(struct font_glyph_info) + 
                     h->bitmap_size;
    if (size < expected) return -3;

    current_font.header = *h;
    current_font.glyphs = (struct font_glyph_info *)((uint8_t *)data + sizeof(struct font_header));
    current_font.bitmap = (uint8_t *)current_font.glyphs + h->num_chars * sizeof(struct font_glyph_info);
    current_font.is_dynamic = 1;

    return 0;
}
