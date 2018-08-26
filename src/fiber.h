#ifndef FIBER_H
#define FIBER_H

#include <setjmp.h>
#include <stdint.h>
#include <stddef.h>

#define FIBER_STACK_WORDS 512

struct fiber {
  volatile uint8_t runnable;
  uint8_t priority;
  void (*entry)();
  uint32_t times_run;

  void *_md;
  uint32_t stack[FIBER_STACK_WORDS] __attribute__((aligned(8)));
};

struct fiber_condition {
  struct fiber *waiter;
  volatile uint32_t triggered;
};

/* Call to initialize task list */
void fibers_init(struct fiber *, size_t n);
 
struct fiber *fiber_current();

/* Selects a new fiber to run, sets current fiber accordingly */
void fiber_schedule();

/* Notify on a condition.  This sets any fiber that is waiting on a condition to
 * be runnable */
void fiber_notify(struct fiber_condition *);

/* Register a thread with a condition such that it will be made runnable if it
 * is notified */
void fiber_wait(struct fiber_condition *);

/* Yields the thread */
void fiber_yield();

#ifdef UNITTEST
#include <check.h>
TCase *setup_fiber_tests();
#endif

#endif

