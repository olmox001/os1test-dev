/*
 * user/bin/hello.c
 * Minimal windowless CLI demo (USR-TTY-01 #123).
 *
 * Opens NO compositor window, so when launched from the shell it runs as a
 * foreground job: its stdout lands in the shell's window (the controlling
 * terminal, resolved kernel-side), and Ctrl+C terminates it.  It loops
 * forever printing a heartbeat so the in-shell + Ctrl+C path is observable.
 */
#include <os1.h>

int main(void) {
  printf("hello: running in the shell (PID %d) — press Ctrl+C to stop\n",
         get_pid());

  unsigned long i = 0;
  while (1) {
    printf("hello #%lu\n", i++);
    /* Slow the heartbeat down: yield a lot between lines. */
    for (int d = 0; d < 400; d++)
      yield();
  }
  return 0;
}
