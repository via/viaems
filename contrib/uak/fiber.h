#ifndef UAK_FIBER_H
#define UAK_FIBER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "sensors.h"


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
void uak_notify_set(int32_t fiber, uint32_t value);

/* If the fiber's notification slot is non-zero, it will be cleared and this
 * function returns immediately. Otherwise, the fiber will be set to blocking on
 * a notification value, and then any non-zero notification will make the fiber
 * become runnable again */
uint32_t uak_wait_for_notify(void);


/* Create a message queue for messages of the given size and count, backed by an
 * array at data. Returns a handle for the queue, or a negative number to
 * indicate an error */
int32_t uak_queue_create(void *data, uint32_t msg_size, uint32_t msg_count);

/* Gets a pending message from the queue. If no messages are available, suspend
 * the fiber until one is ready. Only one fiber can wait on any queue */
bool uak_queue_get(int32_t queue, void *msg);

/* Gets a pending message from the queue. If no messages are available, returns
 * false */
bool uak_queue_get_nonblock(int32_t queue, void *msg);

/* Puts a message onto a queue. If no space is available in the queue, returns
 * false */
bool uak_queue_put(int32_t queue, const void *msg);

/* Puts a message onto a queue. If the queue is full, the oldest message in the
 * queue is replaced */
bool uak_queue_put_overwrite(int32_t queue, const void *msg);

#endif

