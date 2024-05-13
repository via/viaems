#ifndef UAK_MD_CORTEX_M4F_M7
#define UAK_MD_CORTEX_M4F_M7

#include <stdint.h>

enum syscall_number {
  SYSCALL_NOTIFY_SET = 0x1,
  SYSCALL_NOTIFY_WAIT = 0x2,
  SYSCALL_GET_CYCLE_COUNT = 0xf,
};

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

static inline void uak_notify_set(int32_t fiber, uint32_t val) {
  syscall2(SYSCALL_NOTIFY_SET, fiber, val);
}

static inline uint32_t uak_notify_wait(void) {
  return syscall0(SYSCALL_NOTIFY_WAIT);
}

static uint32_t uak_syscall_get_cycle_count() {
  return syscall0(SYSCALL_GET_CYCLE_COUNT);
}



#endif
