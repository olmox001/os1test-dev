#include "lib.h"

int main(void) {
  print("Counter Process Started\n");

  int i = 0;
  while (1) {
    /* Only print every 256 iterations to reduce spam */
    if ((i & 0xFF) == 0) {
      print("Count: ");
      print_hex(i);
      print("\n");
    }
    i++;

    /* Busy wait to slow down output */
    for (volatile int j = 0; j < 1000000; j++) {
    }

    /* Yield is not implemented yet, so we rely on Preemption */
  }

  return 0;
}
