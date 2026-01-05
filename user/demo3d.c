/*
 * user/demo3d.c
 * 3D Rotating Cube Demo
 */
#include "lib.h"

#define WIN_W 400
#define WIN_H 300

/* Fixed-point math (16.16) */
#define FP_SHIFT 16
#define FP_ONE (1 << FP_SHIFT)

/* Sin/Cos lookup (pre-computed for 0-359 degrees) */
static int sin_table[360];
static int cos_table[360];

static void init_trig(void) {
  /* Approximate sin/cos using Taylor series or simple table */
  /* For simplicity, just use a basic approximation */
  for (int i = 0; i < 360; i++) {
    /* sin(x) ≈ x - x^3/6 for small x */
    /* Convert degrees to fixed-point radians: rad = deg * PI / 180 */
    /* PI ≈ 205887 in 16.16 (3.14159 * 65536) */
    int rad = (i * 1144) >> 6; /* i * PI / 180 in FP */

    /* Simple sin approximation for small angles */
    int x = rad;
    int x3 = ((x * x) >> FP_SHIFT);
    x3 = (x3 * x) >> FP_SHIFT;
    sin_table[i] = x - (x3 / 6);

    /* cos = sin(90 - x) */
    int cos_deg = (90 - i + 360) % 360;
    int cos_rad = (cos_deg * 1144) >> 6;
    x = cos_rad;
    x3 = ((x * x) >> FP_SHIFT);
    x3 = (x3 * x) >> FP_SHIFT;
    cos_table[i] = x - (x3 / 6);
  }

  /* Normalize to FP_ONE range */
  /* For degrees 0-90, sin goes 0->1, cos goes 1->0 */
}

static int my_sin(int deg) {
  while (deg < 0)
    deg += 360;
  while (deg >= 360)
    deg -= 360;
  return sin_table[deg];
}

static int my_cos(int deg) {
  while (deg < 0)
    deg += 360;
  while (deg >= 360)
    deg -= 360;
  return cos_table[deg];
}

/* 3D Point */
typedef struct {
  int x, y, z; /* Fixed-point */
} vec3_t;

/* Cube vertices (8 corners) in fixed-point */
static vec3_t cube_verts[8];

static void init_cube(int size) {
  int s = size;
  cube_verts[0] = (vec3_t){-s, -s, -s};
  cube_verts[1] = (vec3_t){s, -s, -s};
  cube_verts[2] = (vec3_t){s, s, -s};
  cube_verts[3] = (vec3_t){-s, s, -s};
  cube_verts[4] = (vec3_t){-s, -s, s};
  cube_verts[5] = (vec3_t){s, -s, s};
  cube_verts[6] = (vec3_t){s, s, s};
  cube_verts[7] = (vec3_t){-s, s, s};
}

/* Cube edges (12 edges) */
static int edges[12][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0}, /* Front face */
    {4, 5}, {5, 6}, {6, 7}, {7, 4}, /* Back face */
    {0, 4}, {1, 5}, {2, 6}, {3, 7}  /* Connecting edges */
};

/* Rotate point around Y axis */
static vec3_t rotate_y(vec3_t p, int angle) {
  int c = my_cos(angle);
  int s = my_sin(angle);
  vec3_t r;
  r.x = ((p.x * c) >> FP_SHIFT) - ((p.z * s) >> FP_SHIFT);
  r.y = p.y;
  r.z = ((p.x * s) >> FP_SHIFT) + ((p.z * c) >> FP_SHIFT);
  return r;
}

/* Rotate point around X axis */
static vec3_t rotate_x(vec3_t p, int angle) {
  int c = my_cos(angle);
  int s = my_sin(angle);
  vec3_t r;
  r.x = p.x;
  r.y = ((p.y * c) >> FP_SHIFT) - ((p.z * s) >> FP_SHIFT);
  r.z = ((p.y * s) >> FP_SHIFT) + ((p.z * c) >> FP_SHIFT);
  return r;
}

/* Project 3D to 2D */
static void project(vec3_t p, int *sx, int *sy) {
  int z = p.z + (3 * FP_ONE); /* Move camera back */
  if (z < FP_ONE / 4)
    z = FP_ONE / 4; /* Avoid division by zero */

  int scale = (2 * FP_ONE);
  *sx = WIN_W / 2 + ((p.x * scale / z) >> (FP_SHIFT - 6));
  *sy = WIN_H / 2 - ((p.y * scale / z) >> (FP_SHIFT - 6));
}

/* Draw line using Bresenham */
static void draw_line(int win, int x0, int y0, int x1, int y1,
                      unsigned int color) {
  int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
  int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx - dy;

  while (1) {
    if (x0 >= 0 && x0 < WIN_W && y0 >= 0 && y0 < WIN_H) {
      window_draw(win, x0, y0, 1, 1, color);
    }

    if (x0 == x1 && y0 == y1)
      break;

    int e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dx) {
      err += dx;
      y0 += sy;
    }
  }
}

int main(void) {
  int pid = get_pid();
  char title[32];
  sprintf(title, "3D Demo PID %d", pid);

  int win = create_window(50, 50, WIN_W, WIN_H, title);
  if (win <= 0) {
    print("[Demo3D] Error creating window\n");
    exit(1);
  }

  init_trig();
  init_cube(FP_ONE / 2); /* Cube with size 0.5 */

  int angle_y = 0;
  int angle_x = 0;

  printf("[Demo3D] Running at PID %d\n", pid);

  while (1) {
    /* Clear window */
    window_draw(win, 0, 0, WIN_W, WIN_H, 0xFF000020);

    /* Transform and project vertices */
    int screen_x[8], screen_y[8];
    for (int i = 0; i < 8; i++) {
      vec3_t v = cube_verts[i];
      v = rotate_y(v, angle_y);
      v = rotate_x(v, angle_x);
      project(v, &screen_x[i], &screen_y[i]);
    }

    /* Draw edges */
    for (int i = 0; i < 12; i++) {
      int v0 = edges[i][0];
      int v1 = edges[i][1];
      unsigned int color = 0xFF00FF00; /* Green */
      if (i < 4)
        color = 0xFFFF0000; /* Front = Red */
      if (i >= 4 && i < 8)
        color = 0xFF0000FF; /* Back = Blue */
      draw_line(win, screen_x[v0], screen_y[v0], screen_x[v1], screen_y[v1],
                color);
    }

    compositor_render();

    /* Rotate */
    angle_y = (angle_y + 2) % 360;
    angle_x = (angle_x + 1) % 360;

    /* Small delay */
    for (volatile int i = 0; i < 500000; i++)
      ;
  }

  exit(0);
  return 0;
}
