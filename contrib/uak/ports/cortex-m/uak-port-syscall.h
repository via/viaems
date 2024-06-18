#ifndef UAK_PORTS_CORTEX_M_UAK_PORT_SYSCALL_H
#define UAK_PORTS_CORTEX_M_UAK_PORT_SYSCALL_H

#include <stdint.h>

/* System call primitives for Cortex-M */

static inline uint32_t uak_syscall(uint32_t number,
                      void *arg) {
  volatile register uint32_t _r0 __asm__ ("r0") = number;
  register void *_r1 __asm__ ("r1") = arg;

  __asm__ volatile ("svc 0" : "=r"(_r0) : "r"(_r0), "r"(_r1) : "memory");
  return _r0;
}

#endif
