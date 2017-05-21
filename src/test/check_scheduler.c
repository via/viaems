#include "platform.h"
#include "scheduler.h"
#include "config.h"
#include "check_platform.h"

#include <check.h>
#include <stdio.h>

static struct output_event oev;

static void check_scheduler_setup() {
  check_platform_reset();
  config.decoder.last_trigger_angle = 0;
  config.decoder.last_trigger_time = 0;
  config.decoder.offset = 0;
  config.decoder.rpm = 6000;
  oev = (struct output_event){
    .type = IGNITION_EVENT,
    .angle = 360,
    .output_id = 0,
    .inverted = 0,
  };
}

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
} END_TEST

START_TEST(check_schedule_ignition_reschedule_completely_later) {

  timeval_t old_start, old_stop;

  set_current_time(time_from_rpm_diff(6000, 270));
  schedule_ignition_event(&oev, &config.decoder, 10, 1000);

  set_current_time(oev.start.time - 100);

  /* Reschedule 10 degrees later */
  old_start = oev.start.time;
  old_stop = oev.stop.time;
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

} END_TEST

START_TEST(check_schedule_ignition_reschedule_completely_earlier_still_future) {

  set_current_time(time_from_rpm_diff(6000, 180));
  schedule_ignition_event(&oev, &config.decoder, 10, 1000);
  /* Reschedule 10 earlier later */
  schedule_ignition_event(&oev, &config.decoder, 50, 1000);

  ck_assert(oev.start.scheduled);
  ck_assert(oev.stop.scheduled);
  ck_assert(!oev.start.fired);
  ck_assert(!oev.stop.fired);

  ck_assert_int_eq(oev.stop.time - oev.start.time, 
    1000 * (TICKRATE / 1000000));
  ck_assert_int_eq(oev.stop.time, 
    time_from_rpm_diff(config.decoder.rpm, 
    oev.angle + config.decoder.offset - 50));

} END_TEST

START_TEST(check_schedule_ignition_reschedule_onto_now) {

  set_current_time(time_from_rpm_diff(6000, 340));
  schedule_ignition_event(&oev, &config.decoder, 15, 1000);

  /* Start would fail, stop should schedule */
  ck_assert(!oev.start.scheduled);
  ck_assert(!oev.stop.scheduled); 
  ck_assert(!oev.start.fired);
  ck_assert(!oev.stop.fired);

} END_TEST

START_TEST(check_schedule_ignition_reschedule_active_later) {

  set_current_time(time_from_rpm_diff(6000, 270));
  schedule_ignition_event(&oev, &config.decoder, 10, 1000);

  /* Emulate firing of the event */
  set_current_time(oev.start.time + 1);

  /* Reschedule 10* later */
  schedule_ignition_event(&oev, &config.decoder, 0, 1000);

  ck_assert(oev.stop.scheduled);
  ck_assert_int_eq(oev.stop.time, 
    time_from_rpm_diff(config.decoder.rpm, 
    oev.angle + config.decoder.offset));


} END_TEST

/* Special case where rescheduling before last trigger is
 * reinterpretted as future */
START_TEST(check_schedule_ignition_reschedule_active_too_early) {
  oev.angle = 40;
  set_current_time(time_from_rpm_diff(6000, 0));
  schedule_ignition_event(&oev, &config.decoder, 0, 1000);
  /* Emulate firing of the event */
  set_current_time(oev.start.time + 5);

  timeval_t old_stop = oev.stop.time;
  /* Reschedule 45* earlier, now in past*/
  schedule_ignition_event(&oev, &config.decoder, 45, 1000);

  ck_assert(oev.stop.scheduled);
  ck_assert_int_eq(oev.stop.time, old_stop);

} END_TEST

START_TEST(check_event_is_active) {
  ck_assert(!event_is_active(&oev));

  oev.start.fired = 1;
  ck_assert(event_is_active(&oev));

  oev.stop.fired = 1;
  ck_assert(!event_is_active(&oev));

  oev.start.fired = 0;
  ck_assert(!event_is_active(&oev));
} END_TEST

START_TEST(check_event_has_fired) {


} END_TEST
  
START_TEST(check_invalidate_events_when_active) {
  /* Schedule an event, get in the middle of it */
  set_current_time(time_from_rpm_diff(6000, 270));
  schedule_ignition_event(&oev, &config.decoder, 10, 1000);
  set_current_time(time_from_rpm_diff(6000, 
    oev.angle + config.decoder.offset - 10) - 500 * (TICKRATE/1000000));
 
  invalidate_scheduled_events(&oev, 1);

  ck_assert(oev.stop.scheduled);

} END_TEST

START_TEST(check_deschedule_event) {

} END_TEST


TCase *setup_scheduler_tests() {
  TCase *scheduler_tests = tcase_create("scheduler");
  tcase_add_checked_fixture(scheduler_tests, check_scheduler_setup, NULL);
  tcase_add_test(scheduler_tests, check_schedule_ignition);
  tcase_add_test(scheduler_tests, check_schedule_ignition_reschedule_completely_later);
  tcase_add_test(scheduler_tests, check_schedule_ignition_reschedule_completely_earlier_still_future);
  tcase_add_test(scheduler_tests, check_schedule_ignition_reschedule_onto_now);
  tcase_add_test(scheduler_tests, check_schedule_ignition_reschedule_active_later);
  tcase_add_test(scheduler_tests, check_schedule_ignition_reschedule_active_too_early);
  tcase_add_test(scheduler_tests, check_event_is_active);
  tcase_add_test(scheduler_tests, check_event_has_fired);
  tcase_add_test(scheduler_tests, check_invalidate_events_when_active);
  tcase_add_test(scheduler_tests, check_deschedule_event);
  check_add_buffer_tests(scheduler_tests);
  return scheduler_tests;
}


