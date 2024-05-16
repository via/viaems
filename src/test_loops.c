#include <stdint.h>

#include "fiber.h"

int32_t t1 , t2;
int32_t q1;
volatile uint32_t duration = 0;
volatile uint32_t count = 0;


#if 0
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
    uak_notify_set(t1, 1);
  }
}

#else

void t1_loop(void *) {
  while (true) {
    uint32_t c = uak_syscall_get_cycle_count();
    uak_queue_put(q1, &c);
    uak_notify_wait();
  }
}

void t2_loop(void *) {
  volatile uint32_t vars[256];
  while (true) {
    uint32_t value;
    uak_queue_get(q1, &value);
    duration = uak_syscall_get_cycle_count() - value;
    uak_notify_set(t1, 1);
    vars[count] = 0x55;
    count++;
  }
}


#endif
