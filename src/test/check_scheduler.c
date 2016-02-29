#include "platform.h"
#include "scheduler.h"
#include "config.h"
#include "check_platform.h"

#include <check.h>

static struct output_event oev;

static void check_scheduler_setup() {
  check_platform_reset();
  config.decoder.last_trigger_angle = 0;
  config.decoder.last_trigger_time = 0;
  config.decoder.rpm = 6000;
  oev = (struct output_event){
    .type = IGNITION_EVENT,
    .angle = 360,
    .output_id = 0,
    .inverted = 0,
  };
}

START_TEST(check_scheduler_inserts_sorted) {
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

START_TEST(check_schedule_ignition) {

  /* Set our current position at 270* for an event at 360* */
  set_current_time(time_from_rpm_diff(6000, 270));
  schedule_ignition_event(&oev, &config.decoder, 10, 1000);
  ck_assert(oev.start.scheduled);
  ck_assert(oev.stop.scheduled);
  ck_assert(!oev.start.fired);
  ck_assert(!oev.stop.fired);

  ck_assert_int_eq(oev.stop.time - oev.start.time, 
    1000 * (TICKRATE / 1000000));
  ck_assert_int_eq(oev.stop.time, 
    time_from_rpm_diff(config.decoder.rpm, 
    oev.angle + config.decoder.offset - 10));
  ck_assert_int_eq(get_output(), 0);
  ck_assert_int_eq(get_event_timer(), oev.start.time);

} END_TEST

START_TEST(check_schedule_ignition_reschedule_later) {

  set_current_time(time_from_rpm_diff(6000, 270));
  schedule_ignition_event(&oev, &config.decoder, 10, 1000);
  /* Reschedule 10 degrees later */
  schedule_ignition_event(&oev, &config.decoder, 0, 1000);

  ck_assert(oev.start.scheduled);
  ck_assert(oev.stop.scheduled);
  ck_assert(!oev.start.fired);
  ck_assert(!oev.stop.fired);

  ck_assert_int_eq(oev.stop.time - oev.start.time, 
    1000 * (TICKRATE / 1000000));
  ck_assert_int_eq(oev.stop.time, 
    time_from_rpm_diff(config.decoder.rpm, 
    oev.angle + config.decoder.offset));
  ck_assert_int_eq(get_output(), 0);
  ck_assert_int_eq(get_event_timer(), oev.start.time);

} END_TEST

START_TEST(check_schedule_ignition_reschedule_while_active) {

  timeval_t prev_start, prev_stop;
  set_current_time(time_from_rpm_diff(6000, 270));
  schedule_ignition_event(&oev, &config.decoder, 10, 1000);
  prev_start = oev.start.time;
  prev_stop = oev.stop.time;
  /* Emulate firing of the event */
  set_current_time(get_event_timer());
  scheduler_execute();
  /* Reschedule 10* later */
  schedule_ignition_event(&oev, &config.decoder, 0, 1000);

  ck_assert(!oev.start.scheduled);
  ck_assert(oev.stop.scheduled);
  ck_assert(oev.start.fired);
  ck_assert(!oev.stop.fired);
  ck_assert_int_eq(oev.stop.time, prev_stop);
  ck_assert_int_eq(oev.start.time, prev_start);

  ck_assert_int_eq(get_output(), 1);
  ck_assert_int_eq(get_event_timer(), oev.stop.time);

} END_TEST

START_TEST(check_schedule_ignition_reschedule_after_finished) {

  set_current_time(time_from_rpm_diff(6000, 270));
  schedule_ignition_event(&oev, &config.decoder, 10, 1000);

  /* Emulate firing of the event */
  set_current_time(get_event_timer());
  scheduler_execute();
  ck_assert_int_eq(get_output(), 1);

  set_current_time(get_event_timer());
  scheduler_execute();
  ck_assert_int_eq(get_output(), 0);

  /* Reschedule 10* later */
  schedule_ignition_event(&oev, &config.decoder, 0, 1000);

  ck_assert(!oev.start.scheduled);
  ck_assert(!oev.stop.scheduled);
  ck_assert(oev.start.fired);
  ck_assert(oev.stop.fired);

  ck_assert_int_eq(get_output(), 0);
  ck_assert_int_eq(get_event_timer(), 0); /*Disabled*/

} END_TEST

START_TEST(check_schedule_ignition_scheduled_late) {

  /* Current time halfway through event */
  set_current_time(time_from_rpm_diff(6000, 
    oev.angle + config.decoder.offset - 10) + 500 * (TICKRATE/1000000));
  schedule_ignition_event(&oev, &config.decoder, 10, 1000);

  /* Fail to schedule */
  ck_assert(!oev.start.scheduled);
  ck_assert(!oev.stop.scheduled);
  ck_assert(!oev.start.fired);
  ck_assert(!oev.stop.fired);

  ck_assert_int_eq(get_output(), 0);
  ck_assert_int_eq(get_event_timer(), 0); /*Disabled*/
} END_TEST

TCase *setup_scheduler_tests() {
  TCase *scheduler_tests = tcase_create("scheduler");
  tcase_add_checked_fixture(scheduler_tests, check_scheduler_setup, NULL);
  tcase_add_test(scheduler_tests, check_scheduler_inserts_sorted);
  tcase_add_test(scheduler_tests, check_schedule_ignition);
  tcase_add_test(scheduler_tests, check_schedule_ignition_reschedule_later);
  tcase_add_test(scheduler_tests, check_schedule_ignition_reschedule_while_active);
  tcase_add_test(scheduler_tests, check_schedule_ignition_reschedule_after_finished);
  tcase_add_test(scheduler_tests, check_schedule_ignition_scheduled_late);
  return scheduler_tests;
}


