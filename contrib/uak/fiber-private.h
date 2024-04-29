#ifndef UAK_FIBER_PRIVATE_H
#define UAK_FIBER_PRIVATE_H

#include "fiber.h"

enum uak_fiber_state {
  UAK_RUNNABLE,
  UAK_BLOCK_ON_NOTIFY,
  UAK_BLOCK_ON_QUEUE,
};


struct fiber {
  enum uak_fiber_state state;
  uint8_t priority;
  void (*entry)(void *);
  void *argument;

  uint32_t notification_value;

  void *_md;
  uint32_t *stack;
  uint32_t stack_size;
};

struct queue {
  uint32_t n_msgs;
  uint8_t msg_size;
  void *data;

  uint32_t read;
  uint32_t write;

  struct fiber *waiter;
};

#define N_PRIORITIES 4
#define N_FIBERS_PER_PRIO 5
#define N_QUEUES 8

struct priority {
  struct fiber *run_queue[N_FIBERS_PER_PRIO];
  uint32_t latest;
  uint32_t oldest;
};

struct executor {
  /* Pointer to currently running fiber */
  struct fiber *current;

  /* Count of fibers in fibers array */
  size_t n_fibers;

  /* List of fibers managed by executor */
  struct fiber fibers[N_PRIORITIES * N_FIBERS_PER_PRIO];

  /* List of priority levels managed by executor */
  struct priority priorities[N_PRIORITIES];

  /* Bitfield representing individual priority level readiness. Priority 0 is
   * highest priority, and is represented by the least significant bit */
  uint32_t priority_readiness;

  /* True if executor has been started */
  bool started;

  /* Internal state for queues */
  struct queue queues[N_QUEUES];

  /* Count of queues in queues array */
  size_t n_queues;
};

struct fiber *uak_current_fiber(void);
void uak_fiber_reschedule(void);

/* *** MD-layer API *** */

/* Selects a new fiber to run, sets current fiber accordingly */
void uak_md_fiber_create(struct fiber *);


/* Global scheduler lock implementation */
void uak_md_lock_scheduler(void);
void uak_md_unlock_scheduler(void);

/* Pend a scheduler call:
 * - If this is called from an ISR, it is expected that the ISR
 *   will continue until it exits
 * - If this is called from a fiber (indirectly via a uak API call), it may
 *   cause the fiber to be descheduled, if and only if, there are higher
 *   priority fibers to run, or if the current fiber has been made not runnable
 *   */
void uak_md_request_reschedule();

#endif

