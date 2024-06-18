#ifndef UAK_MD_CORTEX_M_SYSCALLS_H
#define UAK_MD_CORTEX_M_SYSCALLS_H

#include "md_cortex_m.h"

enum builtin_syscall_number {
  SYSCALL_NOTIFY_SET = 0x1,
  SYSCALL_NOTIFY_WAIT = 0x2,
  SYSCALL_QUEUE_PUT = 0x3,
  SYSCALL_QUEUE_GET = 0x4,
  SYSCALL_GET_CYCLE_COUNT = 0xf,
};

static inline void uak_notify_set(int32_t fiber, uint32_t val) {
  syscall2(SYSCALL_NOTIFY_SET, fiber, val);
}

static inline uint32_t uak_notify_wait(void) {
  return syscall0(SYSCALL_NOTIFY_WAIT);
}

static inline uint32_t uak_syscall_get_cycle_count() {
  return syscall0(SYSCALL_GET_CYCLE_COUNT);
}

static inline void uak_queue_put(int32_t queue, const void *msg) {
  syscall2(SYSCALL_QUEUE_PUT, queue, (uint32_t)msg);
}

static inline void uak_queue_get(int32_t queue, const void *msg) {
  syscall2(SYSCALL_QUEUE_GET, queue, (uint32_t)msg);
}

#endif
