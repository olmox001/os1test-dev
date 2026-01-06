/*
 * user/init.c
 * Init Process - System Boot Splash
 */
#include "lib.h"

int main(void) {
  /* Splash Screen */
  draw(0, 0, 800, 600, 0xFF1a1a2e); /* Dark Background */

  printf("\n[Init] System Booting (PID %d)...\n", get_pid());
  print("[Init] Splash Screen Active.\n");

  /* Progress Bar */
  for (int i = 0; i < 100; i++) {
    draw(200, 400, i * 4, 10, 0xFF00ff88);
    if (i % 20 == 0)
      flush();
    /* Small delay */
    for (volatile int j = 0; j < 1000000; j++)
      ;
  }

  print("[Init] Handing over to Shell Processes...\n");

  /* Spawn Shells Dinamically (Second Stage) */
  int pid1 = spawn("/shell");
  // int pid2 = spawn("/demo3d");
  printf("[Init] Spawning IPC Test...\n");
  int pid2 = spawn("/ipc_send");

  flush();

  /* Supervisor loop: Monitor and respawn shells if they die */
  while (1) {
    /* Poll Shell 1 */
    if (wait(pid1) == pid1) {
      print("[Init] Shell 1 (PID %d) terminated! Respawning...\n");
      pid1 = spawn("/shell");
    }

    /* Poll Shell 2 */
    if (wait(pid2) == pid2) {
      print("[Init] Demo3d (PID %d) terminated! Respawning...\n");
      pid2 = spawn("/demo3d");
    }

    /* Wait a bit before next poll to avoid busy-waiting too much
       (Though wait() is non-blocking, we don't want to hog the CPU) */
    /* Wait a bit before next poll to avoid busy-waiting too much
       Use yield to be cooperative! */
    yield();
  }

  return 0;
}
