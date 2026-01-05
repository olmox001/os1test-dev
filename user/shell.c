/*
 * user/shell.c
 * Standalone Graphical Shell for AArch64 OS
 * Each process creates its own TTY window in the compositor.
 */
#include "lib.h"

/* Window dimensions */
#define WIN_W 640
#define WIN_H 480

/* Colors */
#define COLOR_BG 0xFF1a1a2e
#define COLOR_FG 0xFFe0e0e0
#define COLOR_PROMPT 0xFF00ff88

/* Shell state */
static int my_window = -1;
static int running = 1;
static char cmd_buf[128];
static int cmd_len = 0;

/*
 * Compare strings
 */
static int str_eq(const char *a, const char *b) {
  while (*a && *b) {
    if (*a != *b)
      return 0;
    a++;
    b++;
  }
  return *a == *b;
}

/*
 * Redraw window background
 */
static void shell_redraw(void) {
  if (my_window < 0)
    return;

  /* Just clear the window */
  window_draw(my_window, 0, 0, WIN_W, WIN_H, COLOR_BG);

  /* Top accent line */
  window_draw(my_window, 0, 0, WIN_W, 2, COLOR_PROMPT);

  compositor_render();
}

/*
 * Process command
 */
static void process_command(void) {
  cmd_buf[cmd_len] = '\0';
  if (cmd_len == 0)
    return;

  print("\n");

  if (str_eq(cmd_buf, "help") || str_eq(cmd_buf, "?")) {
    print("\nCommands: help, clear, time, info, demo, about, exit\n");
  } else if (str_eq(cmd_buf, "clear")) {
    print("\033[2J\033[H");
    shell_redraw();
  } else if (str_eq(cmd_buf, "time")) {
    printf("Uptime: %d seconds (%x jiffies)\n", (int)(get_time() / 100),
           get_time());
  } else if (str_eq(cmd_buf, "demo")) {
    print("Drawing demo shapes in window...\n");
    for (int i = 0; i < 5; i++) {
      unsigned int colors[] = {0xFFff4444, 0xFF44ff44, 0xFF4444ff, 0xFFffff44,
                               0xFFff44ff};
      window_draw(my_window, 50 + i * 100, 100, 80, 80, colors[i]);
    }
    compositor_render();
  } else if (str_eq(cmd_buf, "about")) {
    print("\n\033[1;36mNeXs OS v0.0.1\033[0m\n");
    print("\033[33mGraphics:\033[0m Window Compositor + ANSI Terminal "
          "Emulator\n");
    print("\033[35mInput:\033[0m Interrupt-driven VirtIO Mouse/Keyboard\n");
    print("\033[32mLibrary:\033[0m POSIX-like userlib with printf support\n");
    print("\nSystem reported: OK\n");
  } else if (str_eq(cmd_buf, "exit")) {
    print("Exiting shell...\n");
    running = 0;
  } else {
    printf("Unknown command: %s\n", cmd_buf);
  }

  cmd_len = 0;
}

/*
 * Main shell entry
 */
int main(void) {
  /* Create a unique window for this shell instance */
  /* Start at different positions depending on some "randomness" or just fixed
   * for now */
  /* Use time as a seed-like thing for position? */
  int pid = get_pid();
  char title[32];
  sprintf(title, "Shell PID %d", pid);

  int x_off = (pid * 40) % 200;
  int y_off = (pid * 40) % 200;
  my_window = create_window(100 + x_off, 100 + y_off, WIN_W, WIN_H, title);

  if (my_window <= 0) {
    print("[Shell] Error creating window\n");
    exit(1);
  }

  shell_redraw();

  print("\n[Shell] TTY Window ");
  print_hex(my_window);
  printf(" active (PID %d).\n", get_pid());
  print("\033[32mshell\033[0m> ");

  char buf[2] = {0, 0};
  while (running) {
    long n = read(0, buf, 1);
    if (n <= 0)
      continue;

    char c = buf[0];
    if (c == '\n' || c == '\r') {
      process_command();
      if (running)
        print("\033[32mshell\033[0m> ");
    } else if (c == '\b' || c == 127) {
      if (cmd_len > 0) {
        cmd_len--;
        print("\b \b");
      }
    } else if (c >= 32 && c < 127 && cmd_len < 126) {
      cmd_buf[cmd_len++] = c;
      buf[0] = c;
      buf[1] = 0;
      print(buf);
    }
  }

  exit(0);
  return 0;
}
