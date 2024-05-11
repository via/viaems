#include <stdint.h>

#include "fiber.h"

int32_t t1 , t2;
volatile uint32_t duration = 0;

static inline uint32_t syscall0(uint32_t number) {
  volatile register uint32_t _r0 __asm__ ("r0") = number;

  __asm__ volatile ("svc 0" : "=r"(_r0) : "r"(_r0));
  return _r0;
}

static inline uint32_t syscall1(uint32_t number,
                      uint32_t arg1) {
  volatile register uint32_t _r0 __asm__ ("r0") = number;
  register uint32_t _r1 __asm__ ("r1") = arg1;

  __asm__ volatile ("svc 0" : "=r"(_r0) : "r"(_r0), "r"(_r1) : "memory");
  return _r0;
}

static inline uint32_t syscall2(uint32_t number,
                      uint32_t arg1,
                      uint32_t arg2) {
  volatile register uint32_t _r0 __asm__ ("r0") = number;
  register uint32_t _r1 __asm__ ("r1") = arg1;
  register uint32_t _r2 __asm__ ("r2") = arg2;

  __asm__ volatile ("svc 0" : "=r"(_r0) : "r"(_r0), "r"(_r1), "r"(_r2) : "memory");
  return _r0;
}

static void uak_syscall_set_notify(int32_t fiber, uint32_t val) {
  syscall2(1, fiber, val);
}

static uint32_t uak_syscall_wait_notify() {
  return syscall0(2);
}

static uint32_t uak_syscall_get_cycle_count() {
  return syscall0(15);
}

void t1_loop(void *) {
  while (true) {
    uint32_t c = uak_syscall_get_cycle_count();
    uak_syscall_set_notify(t2, c);
    uak_syscall_wait_notify();

  }
}

void t2_loop(void *) {
  while (true) {
    uint32_t value = uak_syscall_wait_notify();
    duration = uak_syscall_get_cycle_count() - value;
    uak_syscall_set_notify(t1, 1);
  }
}
