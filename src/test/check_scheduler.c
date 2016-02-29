#include "platform.h"
#include "scheduler.h"
#include "check_platform.h"

#include <check.h>


START_TEST(check_scheduler_inserts) {
  check_platform_reset();
  struct sched_entry a[] = {
    {5000},
    {8000},
    {6000},
    {2000},
  };
  timeval_t ret;
 
  ret = schedule_insert(0, &a[0]);
  ck_assert(LIST_FIRST(check_get_schedule()) == &a[0]);
  ck_assert(ret == 5000);
 
  ret = schedule_insert(0, &a[1]);
  ck_assert(LIST_FIRST(check_get_schedule()) == &a[0]);
  ck_assert(LIST_NEXT(LIST_FIRST(check_get_schedule()), entries) == &a[1]);
  ck_assert(ret == 5000);
 
  ret = schedule_insert(0, &a[2]);
  ck_assert(LIST_FIRST(check_get_schedule()) == &a[0]);
  ck_assert(LIST_NEXT(LIST_FIRST(check_get_schedule()), entries) == &a[2]);
  ck_assert(LIST_NEXT(LIST_NEXT(LIST_FIRST(check_get_schedule()), entries), entries) == &a[1]);
  ck_assert(ret == 5000);
 
  ret = schedule_insert(0, &a[3]);
  ck_assert(LIST_FIRST(check_get_schedule()) == &a[3]);
  ck_assert(LIST_NEXT(LIST_FIRST(check_get_schedule()), entries) == &a[0]);
  ck_assert(LIST_NEXT(LIST_NEXT(LIST_FIRST(check_get_schedule()), entries), entries) == &a[2]);
  ck_assert(LIST_NEXT(LIST_NEXT(LIST_NEXT(LIST_FIRST(check_get_schedule()), entries), entries), entries) == &a[1]);
  ck_assert(ret == 2000);
 
  a[2].time = 1000;
  ret = schedule_insert(0, &a[2]);
  ck_assert(LIST_FIRST(check_get_schedule()) == &a[2]);
  ck_assert(LIST_NEXT(LIST_FIRST(check_get_schedule()), entries) == &a[3]);
  ck_assert(LIST_NEXT(LIST_NEXT(LIST_FIRST(check_get_schedule()), entries), entries) == &a[0]);
  ck_assert(LIST_NEXT(LIST_NEXT(LIST_NEXT(LIST_FIRST(check_get_schedule()), entries), entries), entries) == &a[1]);
  ck_assert(ret == 1000);

} END_TEST

TCase *setup_scheduler_tests() {
  TCase *scheduler_tests = tcase_create("scheduler");
  tcase_add_test(scheduler_tests, check_scheduler_inserts);
  return scheduler_tests;
}


