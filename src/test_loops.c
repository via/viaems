#include <stdint.h>

#include "fiber.h"

int32_t t1 , t2;
volatile uint32_t duration = 0;

void t1_loop(void *) {
  while (true) {
    uint32_t c = uak_syscall_get_cycle_count();
    uak_notify_set(t2, c);
    uak_notify_wait();

  }
}

void t2_loop(void *) {
  while (true) {
    uint32_t value = uak_notify_wait();
    duration = uak_syscall_get_cycle_count() - value;
    __asm__("bkpt\n");
    uak_notify_set(t1, 1);
  }
}
