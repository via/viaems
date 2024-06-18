#ifndef UAK_FIBER_PRIVATE_H
#define UAK_FIBER_PRIVATE_H

#include "uak-port-private.h"
#include "queue.h"

enum uak_fiber_state {
  UAK_RUNNABLE,
  UAK_BLOCK_ON_NOTIFY,
  UAK_BLOCK_ON_QUEUE,
  UAK_KILLED,
};

struct fiber {
  enum uak_fiber_state state;
  union {
    /* UAK_BLOCK_ON_NOTIFY */
    uint32_t *notification_value;

    /* UAK_BLOCK_ON_QUEUE */
    char *queue_get_dest;
  } blockers;
  uint8_t priority;
  void (*entry)(void *);
  void *argument;

  uint32_t notification_value;

  struct uak_port_context context;
  uint32_t *stack;
  uint32_t stack_size;

  STAILQ_ENTRY(fiber, entries);
};

struct queue {
  uint32_t n_msgs;
  uint8_t msg_size;
  char *data;

  uint32_t read;
  uint32_t write;

  struct fiber *waiter;
};

/* TODO move to config */
#define UAK_NUM_PRIORITIES X
#define UAK_NUM_FIBERS X
#define UAK_NUM_QUEUES X

STAILQ_HEAD(fiber_queue, fiber);

struct executor {
  /* Pointer to currently running fiber */
  struct fiber *current;

  /* List of fibers managed by executor */
  struct fiber fibers[UAK_NUM_FIBERS];

  /* List of priority levels managed by executor */
  fiber_queue priorities[N_PRIORITIES];

  /* Bitfield representing individual priority level readiness. Priority 0 is
   * highest priority, and is represented by the least significant bit */
  uint32_t priority_readiness;

  /* True if executor has been started */
  bool started;

  /* Internal state for queues */
  struct queue queues[N_QUEUES];
};

struct fiber *uak_current_fiber(void);
void uak_fiber_reschedule(void);

/* *** MD-layer API *** */

bool uak_internal_notify_wait(uint32_t *result);
bool uak_internal_queue_get(int32_t queue, void *msg);

/* Pend a scheduler call:
 * - If this is called from an ISR, it is expected that the ISR
 *   will continue until it exits
 * - If this is called from a fiber (indirectly via a uak API call), it may
 *   cause the fiber to be descheduled, if and only if, there are higher
 *   priority fibers to run, or if the current fiber has been made not runnable
 *   */
void uak_md_request_reschedule();

#endif

