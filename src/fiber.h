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

  void *sp;
  uint32_t stack[FIBER_STACK_WORDS] __attribute__((aligned(8)));
};

struct fiber_condition {
  struct fiber *waiter;
  volatile uint32_t triggered;
};

typedef void (*fiber_platform_spawn)(struct fiber *);
typedef int (*fiber_platform_setjmp)();
typedef void (*fiber_platform_longjmp)();
typedef void (*fiber_platform_yield)();

/* Call to initialize task list.  Task initialization if platform-specific, so
 * it is expected that the platform set up the initial jmp_buf for each task,
 * which likely involves setting up stack and calling fiber_spawn */
void fibers_init(struct fiber *, size_t n);
 

struct fiber *fiber_current();

/* Saves current context, determines the correct next context, and switches to
 * it, returning in that new context */
void fiber_schedule();

/* Notify on a condition.  This sets any fiber that is waiting on a condition to
 * be runnable */
void fiber_notify(struct fiber_condition *);

/* Register a thread with a condition such that it will be made runnable if it
 * is notified */
void fiber_wait(struct fiber_condition *);

/* Yields the thread */
void fiber_yield();

#endif

