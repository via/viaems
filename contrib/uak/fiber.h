#ifndef UAK_FIBER_H
#define UAK_FIBER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#if 0
/* Functions that must pass through a system call interface must be directly
 * included and inlined to ensure that they are linked in the same code segment
 * that they are used from.  The actual system calls are defiend in the
 * port-specific header */
#define SYSCALL static inline


/* Create a fiber and return a handle to the fiber.
 * Return values < 0 indicate error.
 */

enum region_type {
  CODE_REGION,
  DATA_REGION,
  STACK_REGION,
  PERIPHERAL_REGION,
  UNCACHED_DATA_REGION,
  CUSTOM,
};

struct region {
  uint32_t start;
  uint32_t size;
  enum region_type type;
  uint32_t _custom;
};


int32_t uak_fiber_create(void (*entry)(void *), 
                         void *argument,
                         uint8_t priority,
                         uint32_t *stack,
                         uint32_t stack_size,
                         const struct region *regions);

#endif

/* Set a fiber's notification value. If it is currently blocked and this sets a
 * non-zero value, the fiber will become runnable. This can only be run from a
 * privileged context (ISR) */
void uak_notify_set_from_privileged(int32_t fiber, uint32_t value);

/* Puts a message onto a queue. This can only be run from a privileged context
 * (ISR) */
bool uak_queue_put_from_privileged(int32_t queue, const void *msg);

#endif

