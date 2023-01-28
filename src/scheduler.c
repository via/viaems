#include "scheduler.h"
#include "config.h"
#include "decoder.h"
#include "platform.h"
#include "util.h"

#include <assert.h>
#include <string.h>
#include <strings.h>

#define MAX_CALLBACKS 32
struct timer_callback *callbacks[MAX_CALLBACKS] = { 0 };
static int n_callbacks = 0;

/* Returns true if both the start and stop entry have been confirmed to fire */
static bool event_has_fired(struct output_event *ev) {
  return (sched_entry_get_state(&ev->start) == SCHED_FIRED) &&
         (sched_entry_get_state(&ev->stop) == SCHED_FIRED);
}

/* Returns true if both the start and stop entry are unscheduled */
static bool event_is_unscheduled(struct output_event *ev) {
  return (sched_entry_get_state(&ev->start) == SCHED_UNSCHEDULED) &&
         (sched_entry_get_state(&ev->stop) == SCHED_UNSCHEDULED);
}

/* Disables a scheduled entry if it is possible.
 * Returns true for success if it was an entry that was still changable, and
 * false if the entry has already been submitted or fired */
static bool sched_entry_disable(struct sched_entry *en) {

  assert(!interrupts_enabled());
  assert(en->state != SCHED_UNSCHEDULED);
  if (sched_entry_get_state(en) != SCHED_SCHEDULED) {
    return false;
  }

  sched_entry_set_state(en, SCHED_UNSCHEDULED);
  return true;
}

/* Set an entry to fire at a specific time.  If the entry is an
 * already-scheduled entry that has been submitted or fired, it can no longer be
 * changed and must be reset before scheduling again.
 * Returns true if the entry is settable and if the specified time is feasible
 * to schedule */
static bool sched_entry_enable(struct sched_entry *en, timeval_t time) {

  assert(!interrupts_enabled());
  if ((sched_entry_get_state(en) == SCHED_SUBMITTED) ||
      (sched_entry_get_state(en) == SCHED_FIRED)) {
    return false;
  }

  if (time_before(time, platform_output_earliest_schedulable_time())) {
    return false;
  }

  en->time = time;
  sched_entry_set_state(en, SCHED_SCHEDULED);
  return true;
}

/* Set a fired entry to be unscheduled */
static void reset_fired_event(struct output_event *ev) {
  assert(ev->start.state == SCHED_FIRED);
  assert(ev->stop.state == SCHED_FIRED);
  sched_entry_set_state(&ev->start, SCHED_UNSCHEDULED);
  sched_entry_set_state(&ev->stop, SCHED_UNSCHEDULED);
}

/* Attempt to deschedule an output event. If the start event can't be disabled,
 * do nothing. */
void deschedule_event(struct output_event *ev) {
  disable_interrupts();

  int success = sched_entry_disable(&ev->start);
  if (success) {
    sched_entry_disable(&ev->stop);
  }
  enable_interrupts();
}

void invalidate_scheduled_events(struct output_event *evs, int n) {
  for (int i = 0; i < n; ++i) {
    if (evs[i].start.state == SCHED_UNSCHEDULED) {
      continue;
    }
    switch (evs[i].type) {
    case IGNITION_EVENT:
    case FUEL_EVENT:
      deschedule_event(&evs[i]);
      break;
    default:
      break;
    }
  }
}
/* Schedules an output event in a hazard-free manner, assuming
 * that the start and stop times occur at least after curtime
 */
static void schedule_output_event_safely(struct output_event *ev,
                                         timeval_t newstart,
                                         timeval_t newstop) {

  ev->start.pin = ev->pin;
  ev->start.val = ev->inverted ? 0 : 1;
  ev->stop.pin = ev->pin;
  ev->stop.val = ev->inverted ? 1 : 0;

  disable_interrupts();

  if (event_is_unscheduled(ev)) {
    /* Schedule start first, if it succeeds we're gauranteed to be able to
     * schedule the stop and be done, if we failed, leave the event unscheduled
     * */
    if (sched_entry_enable(&ev->start, newstart)) {
      sched_entry_enable(&ev->stop, newstop);
    }
  } else {

    timeval_t oldstart = ev->start.time;

    int success = sched_entry_enable(&ev->start, newstart);
    /* If we failed to move the start time, adjust the stop time to preserve
     * fuel pulse time */
    if (!success && (ev->type == FUEL_EVENT)) {
      newstop += oldstart - newstart;
    }
    sched_entry_enable(&ev->stop, newstop);
  }

  enable_interrupts();
}

static int schedule_ignition_event(struct output_event *ev,
                                   struct decoder *d,
                                   degrees_t advance,
                                   unsigned int usecs_dwell) {

  timeval_t stop_time;
  timeval_t start_time;
  degrees_t firing_angle;

  if (!d->rpm || !config.decoder.valid) {
    return 0;
  }

  firing_angle =
    clamp_angle(ev->angle - advance - d->last_trigger_angle + d->offset, 720);

  stop_time = d->last_trigger_time + time_from_rpm_diff(d->rpm, firing_angle);
  start_time = stop_time - time_from_us(usecs_dwell);

  if (event_has_fired(ev)) {

    /* Don't reschedule until we've passed at least 90*/
    if ((time_diff(stop_time, ev->stop.time) <
         time_from_rpm_diff(d->rpm, 90))) {
      return 0;
    }
    reset_fired_event(ev);
  }

  /* Don't let the stop time move more than 180*
   * forward once it is scheduled */
  if (sched_entry_get_state(&ev->stop) == SCHED_SCHEDULED &&
      time_before(ev->stop.time, stop_time) &&
      ((time_diff(stop_time, ev->stop.time) >
        time_from_rpm_diff(d->rpm, 180)))) {
    return 0;
  }

  if (time_diff(start_time, ev->stop.time) <
      time_from_us(config.ignition.min_fire_time_us)) {
    /* Too little time since last fire */
    return 0;
  }

  schedule_output_event_safely(ev, start_time, stop_time);

  return 1;
}

static int schedule_fuel_event(struct output_event *ev,
                               struct decoder *d,
                               unsigned int usecs_pw) {

  timeval_t stop_time;
  timeval_t start_time;
  degrees_t firing_angle;

  if (!d->rpm || !config.decoder.valid) {
    return 0;
  }

  firing_angle =
    clamp_angle(ev->angle - d->last_trigger_angle + d->offset, 720);

  stop_time = d->last_trigger_time + time_from_rpm_diff(d->rpm, firing_angle);
  start_time = stop_time - (TICKRATE / 1000000) * usecs_pw;

  if (event_has_fired(ev)) {

    /* Don't reschedule until we've passed at least 90*/
    if ((time_diff(stop_time, ev->stop.time) <
         time_from_rpm_diff(d->rpm, 90))) {
      return 0;
    }

    reset_fired_event(ev);
  }

  /* Don't let the stop time move more than 180*
   * forward once it is scheduled
   * TODO evaluate if this is necessary for fueling */

  if (sched_entry_get_state(&ev->stop) == SCHED_SCHEDULED &&
      time_before(ev->stop.time, stop_time) &&
      ((time_diff(stop_time, ev->stop.time) >
        time_from_rpm_diff(d->rpm, 180)))) {
    return 0;
  }

  schedule_output_event_safely(ev, start_time, stop_time);

  return 1;
}

void schedule_event(struct output_event *ev) {
  switch (ev->type) {
  case IGNITION_EVENT:
    if (ignition_cut() || !config.decoder.valid) {
      invalidate_scheduled_events(config.events, MAX_EVENTS);
      return;
    }
    schedule_ignition_event(ev,
                            &config.decoder,
                            (degrees_t)calculated_values.timing_advance,
                            calculated_values.dwell_us);
    break;

  case FUEL_EVENT:
    if (fuel_cut() || !config.decoder.valid) {
      invalidate_scheduled_events(config.events, MAX_EVENTS);
      return;
    }
    schedule_fuel_event(ev, &config.decoder, calculated_values.fueling_us);
    break;

  default:
    break;
  }
}

static void callback_remove(struct timer_callback *tcb) {
  int i;
  int tcb_pos;
  for (i = 0; i < n_callbacks; ++i) {
    if (tcb == callbacks[i]) {
      break;
    }
  }
  tcb_pos = i;
  assert(tcb_pos < n_callbacks);

  for (i = tcb_pos; i < n_callbacks - 1; ++i) {
    callbacks[i] = callbacks[i + 1];
  }
  callbacks[n_callbacks - 1] = NULL;
  n_callbacks--;

  tcb->scheduled = 0;
}

static void callback_insert(struct timer_callback *tcb) {

  int i;
  int tcb_pos;
  for (i = 0; i < n_callbacks; ++i) {
    if (time_before(tcb->time, callbacks[i]->time)) {
      break;
    }
  }
  tcb_pos = i;

  for (i = n_callbacks; i > tcb_pos; --i) {
    callbacks[i] = callbacks[i - 1];
  }

  callbacks[tcb_pos] = tcb;
  n_callbacks++;
  tcb->scheduled = 1;
}

int schedule_callback(struct timer_callback *tcb, timeval_t time) {

  disable_interrupts();
  if (tcb->scheduled) {
    callback_remove(tcb);
  }

  tcb->time = time;
  callback_insert(tcb);

  schedule_event_timer(callbacks[0]->time);
  enable_interrupts();

  return 0;
}

void scheduler_callback_timer_execute() {
  assert(n_callbacks > 0);
  assert(time_before_or_equal(callbacks[0]->time, current_time()));

  struct timer_callback *cb = callbacks[0];
  callback_remove(cb);
  xTaskNotifyFromISR(cb->task, 0, eNoAction, NULL);

  if (n_callbacks > 0) {
    schedule_event_timer(callbacks[0]->time);
  }
}

#ifdef UNITTEST
#include <check.h>
#include <stdio.h>

static struct output_event *oev = &config.events[0];

static void check_scheduler_setup() {
  decoder_init(&config.decoder);
  check_platform_reset();
  config.decoder.last_trigger_angle = 0;
  config.decoder.last_trigger_time = 0;
  config.decoder.offset = 0;
  config.decoder.rpm = 6000;
  config.decoder.tooth_rpm = 6000;
  config.decoder.valid = 1;
  *oev = (struct output_event){
    .type = IGNITION_EVENT,
    .angle = 360,
    .pin = 0,
    .inverted = false,
  };
}

START_TEST(check_schedule_ignition) {

  schedule_ignition_event(oev, &config.decoder, 10, 1000);
  ck_assert(oev->start.state == SCHED_SCHEDULED);
  ck_assert(oev->stop.state == SCHED_SCHEDULED);

  ck_assert_int_eq(oev->stop.time - oev->start.time,
                   1000 * (TICKRATE / 1000000));
  ck_assert_int_eq(oev->stop.time,
                   time_from_rpm_diff(config.decoder.rpm,
                                      oev->angle + config.decoder.offset - 10));

  set_current_time(oev->stop.time + 512);
}
END_TEST

START_TEST(check_schedule_ignition_reschedule_completely_later) {

  schedule_ignition_event(oev, &config.decoder, 10, 1000);

  /* Reschedule 10 degrees later */
  schedule_ignition_event(oev, &config.decoder, 0, 1000);

  ck_assert(oev->start.state == SCHED_SCHEDULED);
  ck_assert(oev->stop.state == SCHED_SCHEDULED);

  ck_assert_int_eq(oev->stop.time - oev->start.time,
                   1000 * (TICKRATE / 1000000));
  ck_assert_int_eq(
    oev->stop.time,
    time_from_rpm_diff(config.decoder.rpm, oev->angle + config.decoder.offset));
}
END_TEST

START_TEST(check_schedule_ignition_reschedule_completely_earlier_still_future) {

  schedule_ignition_event(oev, &config.decoder, 10, 1000);
  /* Reschedule 10 earlier later */
  schedule_ignition_event(oev, &config.decoder, 20, 1000);
  ck_assert(oev->start.state == SCHED_SCHEDULED);
  ck_assert(oev->stop.state == SCHED_SCHEDULED);

  ck_assert_int_eq(oev->stop.time - oev->start.time,
                   1000 * (TICKRATE / 1000000));
  ck_assert_int_eq(oev->stop.time,
                   time_from_rpm_diff(config.decoder.rpm,
                                      oev->angle + config.decoder.offset - 20));
}
END_TEST

START_TEST(check_schedule_ignition_reschedule_onto_now) {

  set_current_time(time_from_rpm_diff(6000, 340));
  schedule_ignition_event(oev, &config.decoder, 15, 1000);

  ck_assert(oev->start.state == SCHED_UNSCHEDULED);
  ck_assert(oev->stop.state == SCHED_UNSCHEDULED);
}
END_TEST

START_TEST(check_schedule_ignition_reschedule_active_later) {

  schedule_ignition_event(oev, &config.decoder, 10, 1000);
  timeval_t start_time = oev->start.time;

  /* Move to a time we're sure the event can no longer reschedule */
  set_current_time(oev->start.time + 1);
  ck_assert(oev->start.state == SCHED_SUBMITTED ||
            oev->start.state == SCHED_FIRED);

  /* metatest: make sure we chose numbers that allow us to still change the stop
   * */
  ck_assert(
    !time_before(oev->stop.time, platform_output_earliest_schedulable_time()));

  /* Reschedule 10* later */
  schedule_ignition_event(oev, &config.decoder, 0, 1000);

  ck_assert(oev->stop.state == SCHED_SCHEDULED);
  /* We shouldn't have changed the start */
  ck_assert_int_eq(oev->start.time, start_time);
  ck_assert_int_eq(
    oev->stop.time,
    time_from_rpm_diff(config.decoder.rpm, oev->angle + config.decoder.offset));
}
END_TEST

START_TEST(check_schedule_fuel_reschedule_active_later) {

  oev->angle = 360;
  oev->type = FUEL_EVENT;
  schedule_fuel_event(oev, &config.decoder, 1000);
  timeval_t start_time = oev->start.time;

  /* Move to a time we're sure the event can no longer reschedule */
  set_current_time(oev->start.time + 1);
  ck_assert(oev->start.state == SCHED_SUBMITTED ||
            oev->start.state == SCHED_FIRED);

  /* metatest: make sure we chose numbers that allow us to still change the stop
   * */
  ck_assert(
    !time_before(oev->stop.time, platform_output_earliest_schedulable_time()));

  /* Reschedule 10* later, but one tick longer */
  oev->angle = 370;
  schedule_fuel_event(oev, &config.decoder, 1001);

  ck_assert(oev->stop.state == SCHED_SCHEDULED);
  /* We shouldn't have changed the start */
  ck_assert_int_eq(oev->start.time, start_time);
  /* and total time should be the new 1001 time */
  ck_assert_int_eq(oev->stop.time, start_time + time_from_us(1001));
}
END_TEST

/* Special case where rescheduling before last trigger is
 * reinterpretted as future */
START_TEST(check_schedule_ignition_reschedule_active_too_early) {
  oev->angle = 60;
  schedule_ignition_event(oev, &config.decoder, 0, 1000);
  ck_assert(oev->start.state == SCHED_SCHEDULED);
  ck_assert(oev->stop.state == SCHED_SCHEDULED);

  /* Emulate firing of the event */
  set_current_time(oev->start.time + 5);

  /* metatest: make sure we chose numbers that allow us to still change the stop
   * */
  ck_assert(
    !time_before(oev->stop.time, platform_output_earliest_schedulable_time()));

  timeval_t old_stop = oev->stop.time;
  /* Reschedule 45* earlier, now in past*/
  schedule_ignition_event(oev, &config.decoder, 45, 1000);

  ck_assert(oev->stop.state == SCHED_SCHEDULED);
  ck_assert_int_eq(oev->stop.time, old_stop);
}
END_TEST

START_TEST(check_schedule_fuel_immediately_after_finish) {
  oev->angle = 60;
  config.decoder.rpm = 6000;
  schedule_fuel_event(oev, &config.decoder, 1000);
  ck_assert(oev->start.state == SCHED_SCHEDULED);
  ck_assert(oev->stop.state == SCHED_SCHEDULED);

  /* Emulate firing of the event */
  set_current_time(oev->stop.time + 5);

  /* Reschedule same event */
  schedule_fuel_event(oev, &config.decoder, 1000);
  ck_assert(oev->start.state != SCHED_SCHEDULED);
  ck_assert(oev->stop.state != SCHED_SCHEDULED);
}
END_TEST

START_TEST(check_invalidate_events_when_active) {
  /* Schedule an event, get in the middle of it */
  schedule_ignition_event(oev, &config.decoder, 10, 1000);
  set_current_time(oev->start.time + 5);

  invalidate_scheduled_events(oev, 1);

  ck_assert(oev->stop.state != SCHED_UNSCHEDULED);
}
END_TEST

START_TEST(check_deschedule_event) {
  /* Test descheduling scheduled event in the future */
  oev->start = (struct sched_entry){ .time = current_time() + 10,
                                     .state = SCHED_SCHEDULED };
  oev->stop = (struct sched_entry){ .time = current_time() + 20,
                                    .state = SCHED_SCHEDULED };
  deschedule_event(oev);
  ck_assert(oev->start.state == SCHED_UNSCHEDULED);
  ck_assert(oev->stop.state == SCHED_UNSCHEDULED);

  /* Test descheduling scheduled event with already-submitted start */
  oev->start = (struct sched_entry){ .time = current_time() + 10,
                                     .state = SCHED_SUBMITTED };
  oev->stop = (struct sched_entry){ .time = current_time() + 20,
                                    .state = SCHED_SCHEDULED };
  deschedule_event(oev);
  ck_assert(oev->start.state != SCHED_UNSCHEDULED);
  ck_assert(oev->stop.state != SCHED_UNSCHEDULED);

  /* Test descheduling scheduled event with already-fired start */
  oev->start =
    (struct sched_entry){ .time = current_time() - 10, .state = SCHED_FIRED };
  oev->stop = (struct sched_entry){ .time = current_time() + 20,
                                    .state = SCHED_SCHEDULED };
  deschedule_event(oev);
  ck_assert(oev->start.state != SCHED_UNSCHEDULED);
  ck_assert(oev->stop.state != SCHED_UNSCHEDULED);
}
END_TEST

START_TEST(check_callback_insert) {

  struct timer_callback tc1 = { .time = 50 };
  struct timer_callback tc2 = { .time = 100 };
  struct timer_callback tc3 = { .time = 150 };

  callback_insert(&tc2);
  ck_assert_int_eq(tc2.scheduled, 1);
  ck_assert_int_eq(n_callbacks, 1);
  ck_assert(callbacks[0] == &tc2);

  callback_insert(&tc1);
  ck_assert_int_eq(n_callbacks, 2);
  ck_assert(callbacks[0] == &tc1);
  ck_assert(callbacks[1] == &tc2);

  callback_insert(&tc3);
  ck_assert_int_eq(n_callbacks, 3);
  ck_assert(callbacks[0] == &tc1);
  ck_assert(callbacks[1] == &tc2);
  ck_assert(callbacks[2] == &tc3);
}
END_TEST

START_TEST(check_callback_remove) {

  struct timer_callback tc1 = { .time = 50 };
  struct timer_callback tc2 = { .time = 100 };
  struct timer_callback tc3 = { .time = 150 };

  callback_insert(&tc1);
  callback_insert(&tc2);
  callback_insert(&tc3);
  ck_assert_int_eq(n_callbacks, 3);

  callback_remove(&tc2);
  ck_assert_int_eq(n_callbacks, 2);
  ck_assert_int_eq(tc2.scheduled, 0);
  ck_assert(callbacks[0] == &tc1);
  ck_assert(callbacks[1] == &tc3);

  callback_remove(&tc3);
  ck_assert_int_eq(n_callbacks, 1);
  ck_assert(callbacks[0] == &tc1);

  callback_remove(&tc1);
  ck_assert_int_eq(n_callbacks, 0);
}
END_TEST

static void _increase_count(void *_c) {
  int *c = (int *)_c;
  (*c)++;
}

extern bool event_timer_pending;
START_TEST(check_callback_execute) {

  int count = 0;

  struct timer_callback tc1 = {
    .time = 95,
    .callback = _increase_count,
    .data = &count,
  };
  struct timer_callback tc2 = {
    .time = 100,
    .callback = _increase_count,
    .data = &count,
  };
  struct timer_callback tc3 = {
    .time = 150,
    .callback = _increase_count,
    .data = &count,
  };

  callback_insert(&tc1);
  callback_insert(&tc2);
  callback_insert(&tc3);

  set_current_time(101);
  scheduler_callback_timer_execute();

  /* Only one gets processed, but interrupt immediately marked pending */
  ck_assert_int_eq(count, 1);
  ck_assert_int_eq(n_callbacks, 2);
  ck_assert(callbacks[0] == &tc2);
  ck_assert(event_timer_pending == true);

  scheduler_callback_timer_execute();

  ck_assert_int_eq(count, 2);
  ck_assert_int_eq(n_callbacks, 1);
  ck_assert(callbacks[0] == &tc3);
  ck_assert(event_timer_pending == false);

  set_current_time(151);

  scheduler_callback_timer_execute();
  ck_assert_int_eq(count, 3);
  ck_assert_int_eq(n_callbacks, 0);
  ck_assert(event_timer_pending == false);
}
END_TEST

TCase *setup_scheduler_tests() {
  TCase *tc = tcase_create("scheduler");
  tcase_add_checked_fixture(tc, check_scheduler_setup, NULL);
  tcase_add_test(tc, check_schedule_ignition);
  tcase_add_test(tc, check_schedule_ignition_reschedule_completely_later);
  tcase_add_test(
    tc, check_schedule_ignition_reschedule_completely_earlier_still_future);
  tcase_add_test(tc, check_schedule_ignition_reschedule_onto_now);
  tcase_add_test(tc, check_schedule_ignition_reschedule_active_later);
  tcase_add_test(tc, check_schedule_fuel_reschedule_active_later);
  tcase_add_test(tc, check_schedule_ignition_reschedule_active_too_early);
  tcase_add_test(tc, check_schedule_fuel_immediately_after_finish);
  tcase_add_test(tc, check_invalidate_events_when_active);
  tcase_add_test(tc, check_deschedule_event);
  tcase_add_test(tc, check_callback_insert);
  tcase_add_test(tc, check_callback_remove);
  tcase_add_test(tc, check_callback_execute);
  return tc;
}
#endif
