#include <string.h>

#include "fiber.h"
#include "platform.h"

static struct fiber *current_fiber = NULL;

#define N_PRIORITIES 8
#define N_TASKS_PER_PRIO 8

static struct priority_level {
  struct fiber *tasks[N_TASKS_PER_PRIO];
  int current;
  int n_tasks;
} priority_levels[N_PRIORITIES] = {0};

void fibers_init(struct fiber *f, size_t n) {

  for (size_t i = 0; i < n; ++i) {
    priority_levels[f[i].priority].tasks[priority_levels[f[i].priority].n_tasks] = &f[i];
    priority_levels[f[i].priority].n_tasks++;

    memset(f[i].stack, 0xae, sizeof(f[i].stack));
    platform_fiber_spawn(&f[i]);
  }

  platform_fiber_start(&f[0]);
}

void fiber_schedule() {
  struct fiber *new = NULL;
  for (int i = N_PRIORITIES - 1; i >= 0; --i) {
    struct priority_level *prio = &priority_levels[i];
    if (prio->n_tasks == 0) {
      continue;
    }
    int old = prio->current;
    do {
      prio->current = (prio->current + 1) % prio->n_tasks;
    } while (!prio->tasks[prio->current]->runnable &&
             (old != prio->current));
    if (prio->tasks[prio->current]->runnable) {
      new = prio->tasks[prio->current];
      break;
    }
  }
  current_fiber = new ? new : current_fiber;

  current_fiber->times_run++;
}

void fiber_yield() {
  platform_fiber_yield();
}

struct fiber *fiber_current() {
  return current_fiber;
}

void fiber_wait(struct fiber_condition *blocker) {
  if (!blocker->triggered) {
    blocker->waiter = fiber_current();
    if (!blocker->waiter) {
      return;
    }
    blocker->waiter->runnable = 0;
    fiber_yield();
  }
  blocker->triggered = 0;
}

void fiber_notify(struct fiber_condition *blocker) {
  blocker->waiter->runnable = 1;
  blocker->triggered = 1;
  struct fiber *cur = fiber_current();
  if (cur && (blocker->waiter->priority > cur->priority)) {
    fiber_yield();
  }
}
  
