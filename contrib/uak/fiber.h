#ifndef UAK_FIBER_H
#define UAK_FIBER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "sensors.h"

/* Functions that must pass through a system call interface must be directly
 * included and inlined to ensure that they are linked in the same code segment
 * that they are used from.  The actual system calls are defiend in the
 * port-specific header */
#define SYSCALL static inline


/* Create a fiber and return a handle to the fiber.
 * Return values < 0 indicate error.
 */

int32_t uak_fiber_create(void (*entry)(void *), 
                         void *argument,
                         uint8_t priority,
                         uint32_t *stack,
                         uint32_t stack_size);
 
/* Set a fiber's notification value. If it is currently blocked on a
 * notification, and the value set is non-zero, the fiber will become runnable */
SYSCALL void uak_notify_set(int32_t fiber, uint32_t value);

/* Same as uak_notify_set, but intended to run from non-fiber privileged
 * contexts like ISRs */
void uak_notify_set_from_privileged(int32_t fiber, uint32_t value);

/* If the fiber's notification slot is non-zero, it will be cleared and this
 * function returns immediately. Otherwise, the fiber will be set to blocking on
 * a notification value, and then any non-zero notification will make the fiber
 * become runnable again */
SYSCALL uint32_t uak_notify_wait(void);


/* Create a message queue for messages of the given size and count, backed by an
 * array at data. Returns a handle for the queue, or a negative number to
 * indicate an error */
int32_t uak_queue_create(char *data, uint32_t msg_size, uint32_t msg_count);

/* Gets a pending message from the queue. If no messages are available, suspend
 * the fiber until one is ready. Only one fiber can wait on any queue */
SYSCALL void uak_queue_get(int32_t queue, const char *msg);

/* Puts a message onto a queue. If no space is available in the queue, returns
 * false */
SYSCALL void uak_queue_put(int32_t queue, const char *msg);

bool uak_queue_put_from_privileged(int32_t queue, const char *msg);

#include "cortex-m4f-m7.h"

#endif

