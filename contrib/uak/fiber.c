#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "fiber.h"
#include "fiber-private.h"

static struct executor executor = { 0 };

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
}

static void uak_make_runnable(struct fiber *f) {
  f->state = UAK_RUNNABLE;

  struct priority *pri_level = &executor.priorities[f->priority];

  pri_level->run_queue[pri_level->oldest] = f;
  uint32_t next_oldest = queue_next_idx(pri_level->oldest, N_FIBERS_PER_PRIO);
  pri_level->oldest = next_oldest;

  /* Mark priority level as runnable */
  executor.priority_readiness |= (0x80000000UL >> f->priority);
}

/* Initializes fiber scheduler with an array of fibers `f` with length `n`. */
int32_t uak_fiber_create(void (*entry)(void *), 
                         void *argument,
                         uint8_t priority,
                         uint32_t *stack,
                         uint32_t stack_size) {

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

uint32_t uak_wait_for_notify() {
  uint32_t posture = uak_md_lock_scheduler();
  struct fiber *f = executor.current;
  if (f->notification_value == 0) {
    uak_suspend(f, UAK_BLOCK_ON_NOTIFY);
    uak_md_request_reschedule();
    uak_md_unlock_scheduler(posture);
    posture = uak_md_lock_scheduler();
  }
  uint32_t result = f->notification_value;
  f->notification_value = 0;
  uak_md_unlock_scheduler(posture);
  return result;
}


void uak_notify_set(int32_t fiber, uint32_t value) {
  struct fiber *f = &executor.fibers[fiber];
  uint32_t posture = uak_md_lock_scheduler();
  f->notification_value = value;
  if (f->state == UAK_BLOCK_ON_NOTIFY) {
    uak_make_runnable(f);
    if (f->priority < executor.current->priority) {
      uak_md_request_reschedule();
    }
  };
  uak_md_unlock_scheduler(posture);
}

void internal_uak_notify_set(int32_t fiber, uint32_t value) {
  struct fiber *f = &executor.fibers[fiber];
  f->notification_value = value;
  if (f->state == UAK_BLOCK_ON_NOTIFY) {
    uak_make_runnable(f);
    uak_md_set_return_value(f, value);
    f->notification_value = 0;
    if (f->priority < executor.current->priority) {
      uak_md_request_reschedule();
    }
  };
}

int32_t internal_wait_on_notify() {
  struct fiber *f = executor.current;
  if (f->notification_value != 0) {
    return f->notification_value;
  }
  uak_suspend(f, UAK_BLOCK_ON_NOTIFY);
  uak_md_request_reschedule();
  __asm__("dsb\n");
  __asm__("isb\n");
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

bool uak_queue_get_nonblock(int32_t queue_handle, void *msg) {
  struct queue *q = &executor.queues[queue_handle];
  bool valid = false;
  uak_md_lock_scheduler();
  if (q->read != q->write) {
    char *cdata = q->data;
    uint32_t read = q->read;
    memcpy(msg, &q->data[read * q->msg_size], q->msg_size);
    valid = true;

    read += 1;
    if (read >= q->n_msgs) {
      read = 0;
    }
    q->read = read;
  }
  uak_md_unlock_scheduler(0);

  return valid;
}

bool uak_queue_get(int32_t queue_handle, void *msg) {
  struct queue *q = &executor.queues[queue_handle];
  uint32_t lock = uak_md_lock_scheduler();
  if (q->read == q->write) {
    assert(q->waiter == NULL);

    /* Block */
    q->waiter = executor.current;
    uak_suspend(executor.current, UAK_BLOCK_ON_QUEUE);
    uak_md_request_reschedule();
    uak_md_unlock_scheduler(lock);

    lock = uak_md_lock_scheduler();
    q->waiter = NULL;
  }

  assert(q->read != q->write);
  uint32_t read = q->read;
  memcpy(msg, &q->data[read * q->msg_size], q->msg_size);

  q->read = queue_next_idx(read, q->n_msgs);

  uak_md_unlock_scheduler(lock);
  return true;
}

bool uak_queue_put(int32_t queue_handle, const void *msg) {
  struct queue *q = &executor.queues[queue_handle];
  uint32_t lock = uak_md_lock_scheduler();

  uint32_t write = q->write;
  uint32_t next_write = queue_next_idx(write, q->n_msgs);

  if (next_write == q->read) {
    /* full! */
    uak_md_unlock_scheduler(lock);
    return false;
  }

  memcpy(&q->data[write * q->msg_size], msg, q->msg_size);
  q->write = next_write;

  if (q->waiter && q->waiter->state == UAK_BLOCK_ON_QUEUE) {
    uak_make_runnable(q->waiter);
    if (q->waiter->priority < executor.current->priority) {
      uak_md_request_reschedule();
    }
  }

  uak_md_unlock_scheduler(lock);
  return true;
}
