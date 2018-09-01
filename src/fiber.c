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

/* Initializes fiber scheduler with an array of fibers `f` with length `n`. */
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

struct fiber *fiber_current() {
  return current_fiber;
}

/* TODO: validate that these functions do things correctly wrt atomicity */
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
  if (!blocker->waiter) {
    return;
  }
  blocker->waiter->runnable = 1;
  blocker->triggered = 1;
#ifndef FIBER_NO_PREEMPT
  struct fiber *cur = fiber_current();
  if (cur && (blocker->waiter->priority > cur->priority)) {
    fiber_yield();
  }
#endif
}
  
#ifdef UNITTEST
#include <check.h>
#include "check_platform.h"

START_TEST(check_fiber_init_priority_insertion) {
  struct fiber f1[] = { 
    { .priority = 0, },
    { .priority = 3, },
    { .priority = 7, },
    { .priority = 7, },
    { .priority = 4, },
    { .priority = 1, },
    { .priority = 0, },
  };        

  fibers_init(f1, 7);
  ck_assert_ptr_eq(&f1[0], priority_levels[0].tasks[0]);
  ck_assert_ptr_eq(&f1[1], priority_levels[3].tasks[0]);
  ck_assert_ptr_eq(&f1[2], priority_levels[7].tasks[0]);
  ck_assert_ptr_eq(&f1[3], priority_levels[7].tasks[1]);
  ck_assert_ptr_eq(&f1[4], priority_levels[4].tasks[0]);
  ck_assert_ptr_eq(&f1[5], priority_levels[1].tasks[0]);
  ck_assert_ptr_eq(&f1[6], priority_levels[0].tasks[1]);

  ck_assert_int_eq(priority_levels[0].n_tasks, 2);
  ck_assert_int_eq(priority_levels[1].n_tasks, 1);
  ck_assert_int_eq(priority_levels[3].n_tasks, 1);
  ck_assert_int_eq(priority_levels[4].n_tasks, 1);
  ck_assert_int_eq(priority_levels[7].n_tasks, 2);
  ck_assert_int_eq(priority_levels[2].n_tasks, 0);
} END_TEST

START_TEST(check_fiber_schedule_switch_to_higher) {
  struct fiber f[] = { 
    { .priority = 0, .runnable = 1 },
    { .priority = 3, .runnable = 1 },
    { .priority = 7, },
  };

  fibers_init(f, 3);

  current_fiber = &f[0];
  fiber_schedule();
  ck_assert_ptr_eq(current_fiber, &f[1]);

  fiber_schedule();
  ck_assert_ptr_eq(current_fiber, &f[1]);

  f[2].runnable = 1;
  fiber_schedule();
  ck_assert_ptr_eq(current_fiber, &f[2]);
  
} END_TEST

START_TEST(check_fiber_schedule_switch_to_lower) {
  struct fiber f[] = { 
    { .priority = 0, .runnable = 1 },
    { .priority = 3, .runnable = 1 },
    { .priority = 7, .runnable = 1 },
  };

  fibers_init(f, 3);

  current_fiber = &f[2];
  fiber_schedule();
  ck_assert_ptr_eq(current_fiber, &f[2]);

  f[2].runnable = 0;
  fiber_schedule();
  ck_assert_ptr_eq(current_fiber, &f[1]);

  f[1].runnable = 0;
  fiber_schedule();
  ck_assert_ptr_eq(current_fiber, &f[0]);
  
} END_TEST

START_TEST(check_fiber_schedule_rr_in_level) {
  struct fiber f[] = { 
    { .priority = 0, .runnable = 1 },
    { .priority = 3, .runnable = 1 },
    { .priority = 3, .runnable = 1 },
    { .priority = 7, .runnable = 0 },
  };

  fibers_init(f, 4);

  current_fiber = &f[0];
  fiber_schedule();
  /* Implementation starts with second task for reasons */
  ck_assert_ptr_eq(current_fiber, &f[2]);

  fiber_schedule();
  ck_assert_ptr_eq(current_fiber, &f[1]);

  fiber_schedule();
  ck_assert_ptr_eq(current_fiber, &f[2]);

  f[3].runnable = 1;
  fiber_schedule();
  ck_assert_ptr_eq(current_fiber, &f[3]);

  f[3].runnable = 0;
  fiber_schedule();
  ck_assert_ptr_eq(current_fiber, &f[1]);

  fiber_schedule();
  ck_assert_ptr_eq(current_fiber, &f[2]);
  
} END_TEST

TCase *setup_fiber_tests() {
  TCase *fiber_tests = tcase_create("fibers");

  tcase_add_test(fiber_tests, check_fiber_init_priority_insertion);
  tcase_add_test(fiber_tests, check_fiber_schedule_switch_to_higher);
  tcase_add_test(fiber_tests, check_fiber_schedule_switch_to_lower);
  tcase_add_test(fiber_tests, check_fiber_schedule_rr_in_level);
  return fiber_tests;
}

#endif

