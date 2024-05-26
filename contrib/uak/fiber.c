#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "fiber.h"
#include "fiber-private.h"

static struct executor executor = { 0 };

void itm_debug(const char *);
 __attribute__((weak)) int32_t uak_initialization_failure(const char *msg) {
   itm_debug("initialization failure: ");
   itm_debug(msg);
   itm_debug("\n");
   while (1);
}

 __attribute__((weak)) int32_t uak_runtime_failure(const char *msg) {
   itm_debug("runtime failure: ");
   itm_debug(msg);
   itm_debug("\n");
   while (1);
}

#include "md/cortex-m4f-m7.c"


static uint32_t queue_next_idx(uint32_t current, uint32_t max) {
  if (current == max - 1) {
    return 0;
  } else {
    return current + 1;
  }
}

static void uak_suspend(struct fiber *f, enum uak_fiber_state reason) {
  assert(f->state == UAK_RUNNABLE);

  f->state = reason;
  struct priority *pri_level = &executor.priorities[f->priority];

  uint32_t next_latest = queue_next_idx(pri_level->latest, N_FIBERS_PER_PRIO);
  pri_level->latest = next_latest;

  /* Make sure to mark whole level unrunnable if needed */
  if (pri_level->oldest == pri_level->latest) {
    executor.priority_readiness &= ~(0x80000000UL >> f->priority);
  }
  //emit_trace(FIBER_SUSPEND, 0);
}

static void uak_make_runnable(struct fiber *f) {
  f->state = UAK_RUNNABLE;

  struct priority *pri_level = &executor.priorities[f->priority];

  pri_level->run_queue[pri_level->oldest] = f;
  uint32_t next_oldest = queue_next_idx(pri_level->oldest, N_FIBERS_PER_PRIO);
  pri_level->oldest = next_oldest;

  /* Mark priority level as runnable */
  executor.priority_readiness |= (0x80000000UL >> f->priority);
  //emit_trace(FIBER_BECOMES_RUNNABLE, 0);
}

/* Initializes fiber scheduler with an array of fibers `f` with length `n`. */
int32_t uak_fiber_create(void (*entry)(void *), 
                         void *argument,
                         uint8_t priority,
                         uint32_t *stack,
                         uint32_t stack_size,
                         const struct region *regions) {

  assert(executor.started == false);

  int32_t f_handle = executor.n_fibers;
  struct fiber *f = &executor.fibers[f_handle];
  executor.n_fibers += 1;

  *f = (struct fiber){
    .state = UAK_RUNNABLE,
    .priority = priority,
    .entry = entry,
    .argument = argument,
    .stack = stack,
    .stack_size = stack_size,
  };

  uak_make_runnable(f);

  memset(f->stack, 0xae, f->stack_size);

  uak_md_fiber_create(f);
  uak_md_fiber_add_region(f, &(struct region){
      .start = (uint32_t)stack,
      .size = stack_size,
      .type = STACK_REGION,
      });

  for (const struct region *r = regions; r->size != 0; r++) {
    if (!uak_md_fiber_add_region(f, r)) {
      return -1;
    }
  }
  return f_handle; 
}

void uak_fiber_reschedule() {

  /* First determine highest priority waiting */

  /* TODO use something cross-platform */
  assert(executor.priority_readiness != 0);
  int highest_runnable_pri = __builtin_clz(executor.priority_readiness);
  struct priority *pri_level = &executor.priorities[highest_runnable_pri];

  assert(pri_level->latest != pri_level->oldest);
  executor.current = pri_level->run_queue[pri_level->latest];
}

/* Set the task notification value.
 * If the task is suspended, waiting for non-zero notification value, the task
 * will be made runnable.  If that task is higher priority than the current
 * thread, a context switch is scheduled.
 *
 * This function locks the scheduler, and is safe to call from ISRs
 */
void uak_notify_set_from_privileged(int32_t fiber, uint32_t value) {
  uint32_t posture = uak_md_lock_scheduler();
  struct fiber *f = &executor.fibers[fiber];
  if (f->state == UAK_BLOCK_ON_NOTIFY) {
    uak_md_set_return_value(f, value);
    uak_make_runnable(f);
    if (f->priority < executor.current->priority) {
      uak_md_request_reschedule();
    }
  } else {
    f->notification_value = value;
  }

  uak_md_unlock_scheduler(posture);
}

/* Check task notify and either:
 *   If already set, set result and return true
 *   If not set, suspend the fiber and return false
 *
 *  This task should only be called from a system-call context. */
bool uak_internal_notify_wait(uint32_t *result) {
  uint32_t posture = uak_md_lock_scheduler();
  struct fiber *f = executor.current;

  if (f->notification_value != 0) {
    *result = f->notification_value;
    f->notification_value = 0;
    uak_md_unlock_scheduler(posture);
    return true;
  }

  uak_suspend(f, UAK_BLOCK_ON_NOTIFY);
  uak_md_request_reschedule();
  uak_md_unlock_scheduler(posture);
  return false;
}

/* Initializes fiber scheduler with an array of fibers `f` with length `n`. */
int32_t uak_queue_create(void *data,
                         uint32_t msg_size,
                         uint32_t msg_count) {

  assert(executor.started == false);

  int32_t q_handle = executor.n_queues;
  struct queue *q = &executor.queues[q_handle];
  executor.n_queues += 1;

  *q = (struct queue){
    .n_msgs = msg_count,
    .msg_size = msg_size,
    .data = data,
    .read = 0,
    .write = 0,
    .waiter = NULL,
  };

  return q_handle; 
}

bool uak_internal_queue_get(int32_t queue_handle, void *msg) {
  struct queue *q = &executor.queues[queue_handle];
  uint32_t lock = uak_md_lock_scheduler();
  bool immediate_result = false;
  if (q->read == q->write) {
    /* Block */
    q->waiter = executor.current;
    q->waiter->blockers.queue_get_dest = msg;
    uak_suspend(executor.current, UAK_BLOCK_ON_QUEUE);
    uak_md_request_reschedule();
    immediate_result = false;
  } else {
    /* Read and return */
    uint32_t read = q->read;
    memcpy(msg, &q->data[read * q->msg_size], q->msg_size);
    q->read = queue_next_idx(read, q->n_msgs);
    immediate_result = true;
  }

  uak_md_unlock_scheduler(lock);
  return immediate_result;
}

bool uak_queue_put_from_privileged(int32_t queue_handle, const void *msg) {
  struct queue *q = &executor.queues[queue_handle];
  uint32_t lock = uak_md_lock_scheduler();

  uint32_t write = q->write;
  uint32_t next_write = queue_next_idx(write, q->n_msgs);

  if (next_write == q->read) {
    /* full! */
    uak_md_unlock_scheduler(lock);
    return false;
  }

  if (q->waiter && q->waiter->state == UAK_BLOCK_ON_QUEUE) {
    assert(q->read == write); /* It shouldn't be possible to be blocked with 
                                 non-zero entries in the queue */

    /* Copy directly into result */
    memcpy(q->waiter->blockers.queue_get_dest, msg, q->msg_size);
    uak_make_runnable(q->waiter);
    if (q->waiter->priority < executor.current->priority) {
      uak_md_request_reschedule();
    }
      
  } else {
    /* Copy into the queue */
    memcpy(&q->data[write * q->msg_size], msg, q->msg_size);
    q->write = next_write;
  }

  uak_md_unlock_scheduler(lock);
  return true;
}

bool uak_fiber_add_regions(int32_t f_handle, const struct region *regions) {
  struct fiber *f = &executor.fibers[f_handle];
  for (const struct region *r = regions; r->size != 0; r++) {
    if (!uak_md_fiber_add_region(f, r)) {
      return false;
    }
  }
  return true;
}
