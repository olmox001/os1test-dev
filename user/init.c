/*
 * user/init.c
 * Init Process - System Boot Splash
 */
#include "lib.h"

int main(void) {
  /* Splash Screen */
  draw(0, 0, 800, 600, 0xFF1a1a2e); /* Dark Background */

  print("\n[Init] System Booting...\n");

  /* Progress Bar */
  for (int i = 0; i < 100; i++) {
    draw(200, 400, i * 4, 10, 0xFF00ff88);
    if (i % 20 == 0)
      flush();
    /* Small delay */
    for (volatile int j = 0; j < 1000000; j++)
      ;
  }

  print("[Init] Handing over to Shell Processes\n");
  flush();

  /* Init stays alive as an idle/supervisor process */
  while (1) {
    long t = get_time();
    if (t % 100 == 0) {
      /* Optional: heartbeat? */
    }
  }

  return 0;
}
