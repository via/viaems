#include "scheduler.h"
#include "config.h"
#include "decoder.h"
#include "platform.h"
#include "util.h"

#include <assert.h>
#include <string.h>
#include <strings.h>

/* Returns true if both the start and stop entry have been confirmed to fire */
static bool event_has_fired(struct output_event_schedule_state *ev) {
  return (ev->start.state == SCHED_FIRED) && (ev->stop.state == SCHED_FIRED);
}

/* Returns true if both the start and stop entry are unscheduled */
static bool event_is_unscheduled(struct output_event_schedule_state *ev) {
  return (ev->start.state == SCHED_UNSCHEDULED) &&
         (ev->stop.state == SCHED_UNSCHEDULED);
}

/* Disables a scheduled entry if it is possible.
 * Returns true for success if it was an entry that was still changable, and
 * false if the entry has already been submitted or fired */
static bool sched_entry_disable(struct schedule_entry *en) {

  assert(en->state != SCHED_UNSCHEDULED);
  if (en->state != SCHED_SCHEDULED) {
    return false;
  }

  en->state = SCHED_UNSCHEDULED;
  return true;
}

/* Set an entry to fire at a specific time.  If the entry is an
 * already-scheduled entry that has been submitted or fired, it can no longer be
 * changed and must be reset before scheduling again.
 * Returns true if the entry is settable and if the specified time is feasible
 * to schedule */
static bool sched_entry_enable(struct schedule_entry *en,
                               timeval_t time,
                               timeval_t earliest_schedulable_time) {

  if ((en->state == SCHED_SUBMITTED) || (en->state == SCHED_FIRED)) {
    return false;
  }

  if (time_before(time, earliest_schedulable_time)) {
    return false;
  }

  en->time = time;
  en->state = SCHED_SCHEDULED;

  return true;
}

/* Set a fired entry to be unscheduled */
static void reset_fired_event(struct output_event_schedule_state *ev) {
  assert(ev->start.state == SCHED_FIRED);
  assert(ev->stop.state == SCHED_FIRED);
  ev->start.state = SCHED_UNSCHEDULED;
  ev->stop.state = SCHED_UNSCHEDULED;
}

/* Attempt to deschedule an output event. If the start event can't be disabled,
 * do nothing. */
void deschedule_event(struct output_event_schedule_state *ev) {

  int success = sched_entry_disable(&ev->start);
  if (success) {
    sched_entry_disable(&ev->stop);
  }
}

void invalidate_scheduled_events(struct output_event_schedule_state *evs,
                                 int n) {
  for (int i = 0; i < n; ++i) {
    if (evs[i].start.state == SCHED_UNSCHEDULED) {
      continue;
    }
    switch (evs[i].config->type) {
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
static void schedule_output_event_safely(struct output_event_schedule_state *ev,
                                         timeval_t earliest_schedulable_time,
                                         timeval_t newstart,
                                         timeval_t newstop) {

  ev->start.pin = ev->config->pin;
  ev->start.val = ev->config->inverted ? 0 : 1;
  ev->stop.pin = ev->config->pin;
  ev->stop.val = ev->config->inverted ? 1 : 0;

  if (event_is_unscheduled(ev)) {
    /* Schedule start first, if it succeeds we're gauranteed to be able to
     * schedule the stop and be done, if we failed, leave the event unscheduled
     * */
    if (sched_entry_enable(&ev->start, newstart, earliest_schedulable_time)) {
      sched_entry_enable(&ev->stop, newstop, earliest_schedulable_time);
    }
  } else {

    timeval_t oldstart = ev->start.time;

    int success =
      sched_entry_enable(&ev->start, newstart, earliest_schedulable_time);
    /* If we failed to move the start time, adjust the stop time to preserve
     * fuel pulse time */
    if (!success && (ev->config->type == FUEL_EVENT)) {
      newstop += oldstart - newstart;
    }
    sched_entry_enable(&ev->stop, newstop, earliest_schedulable_time);
  }
}

static bool schedule_ignition_event(const struct config *config,
                                    struct output_event_schedule_state *ev,
                                    timeval_t earliest_schedulable_time,
                                    const struct engine_position *d,
                                    degrees_t advance,
                                    unsigned int usecs_dwell) {

  degrees_t firing_angle =
    clamp_angle(clamp_angle(ev->config->angle - advance, 720) -
                  d->last_trigger_angle + config->decoder.offset,
                720);

  timeval_t stop_time =
    d->time + time_from_rpm_diff(d->tooth_rpm, firing_angle);
  timeval_t start_time = stop_time - time_from_us(usecs_dwell);

  if (event_has_fired(ev)) {

    /* Prevent rescheduling the same event after its fired by ensuring the new
     * stop time is at least 90 degrees later than the just-fired stop time */
    if ((time_diff(stop_time, ev->stop.time) <
         time_from_rpm_diff(d->rpm, 90))) {
      return false;
    }
    reset_fired_event(ev);
  }

  /* Don't let the stop time move more than 180*
   * forward once it is scheduled */
  if (ev->stop.state == SCHED_SCHEDULED &&
      time_before(ev->stop.time, stop_time) &&
      ((time_diff(stop_time, ev->stop.time) >
        time_from_rpm_diff(d->rpm, 180)))) {
    return false;
  }

  if (time_diff(start_time, ev->stop.time) <
      time_from_us(config->ignition.min_fire_time_us)) {
    /* Too little time since last fire */
    return false;
  }

  schedule_output_event_safely(
    ev, earliest_schedulable_time, start_time, stop_time);

  return true;
}

static bool schedule_fuel_event(const struct config *config,
                                struct output_event_schedule_state *ev,
                                timeval_t earliest_schedulable_time,
                                const struct engine_position *pos,
                                unsigned int usecs_pw) {

  degrees_t firing_angle = clamp_angle(
    ev->config->angle - pos->last_trigger_angle + config->decoder.offset, 720);

  timeval_t stop_time =
    pos->time + time_from_rpm_diff(pos->tooth_rpm, firing_angle);
  timeval_t start_time = stop_time - time_from_us(usecs_pw);

  if (event_has_fired(ev)) {

    /* Prevent rescheduling the same event after its fired by ensuring the new
     * stop time is at least 90 degrees later than the just-fired stop time */
    if ((time_diff(stop_time, ev->stop.time) <
         time_from_rpm_diff(pos->rpm, 90))) {
      return false;
    }

    reset_fired_event(ev);
  }

  /* Don't let the stop time move more than 180*
   * forward once it is scheduled */

  if (ev->stop.state == SCHED_SCHEDULED &&
      time_before(ev->stop.time, stop_time) &&
      ((time_diff(stop_time, ev->stop.time) >
        time_from_rpm_diff(pos->rpm, 180)))) {
    return false;
  }

  schedule_output_event_safely(
    ev, earliest_schedulable_time, start_time, stop_time);

  return true;
}

void schedule_events(const struct config *config,
                     const struct calculated_values *calcs,
                     const struct engine_position *pos,
                     struct output_event_schedule_state *ev,
                     size_t n_outputs,
                     timeval_t earliest_scheduable_time) {

  for (size_t i = 0; i < n_outputs; i++) {
    switch (ev[i].config->type) {
    case IGNITION_EVENT:
      schedule_ignition_event(config,
                              &ev[i],
                              earliest_scheduable_time,
                              pos,
                              (degrees_t)calcs->timing_advance,
                              calcs->dwell_us);
      break;

    case FUEL_EVENT:
      schedule_fuel_event(
        config, &ev[i], earliest_scheduable_time, pos, calcs->fueling_us);
      break;

    default:
      break;
    }
  }
}

#ifdef UNITTEST
#include <check.h>
#include <stdio.h>

static const struct output_event_config ignition_ev_conf = {
  .type = IGNITION_EVENT,
  .angle = 360,
  .pin = 0,
  .inverted = false,
};

static const struct output_event_schedule_state ignition_ev = {
  .config = &ignition_ev_conf,
};

START_TEST(check_schedule_ignition) {

  struct engine_position pos = { .has_position = true,
                                 .has_rpm = true,
                                 .rpm = 6000,
                                 .tooth_rpm = 6000,
                                 .valid_until = -1 };

  struct output_event_schedule_state ev = ignition_ev;

  const uint32_t dwell = 1000;
  const float advance = 10;

  schedule_ignition_event(&default_config, &ev, 0, &pos, advance, dwell);
  ck_assert(ev.start.state == SCHED_SCHEDULED);
  ck_assert(ev.stop.state == SCHED_SCHEDULED);

  ck_assert_int_eq(ev.stop.time - ev.start.time, time_from_us(dwell));
  ck_assert_int_eq(
    ev.stop.time,
    time_from_rpm_diff(
      pos.rpm, ev.config->angle + default_config.decoder.offset - advance));
}
END_TEST

START_TEST(check_schedule_ignition_reschedule_completely_later) {

  struct engine_position pos = { .has_position = true,
                                 .has_rpm = true,
                                 .rpm = 6000,
                                 .tooth_rpm = 6000,
                                 .valid_until = -1 };

  struct output_event_schedule_state ev = ignition_ev;

  const uint32_t dwell = 1000;
  const float advance = 10;

  schedule_ignition_event(&default_config, &ev, 0, &pos, advance, dwell);

  /* Reschedule 10 degrees later */
  const float new_advance = 0;
  schedule_ignition_event(&default_config, &ev, 0, &pos, new_advance, dwell);

  ck_assert(ev.start.state == SCHED_SCHEDULED);
  ck_assert(ev.stop.state == SCHED_SCHEDULED);

  ck_assert_int_eq(ev.stop.time - ev.start.time, time_from_us(dwell));
  ck_assert_int_eq(
    ev.stop.time,
    time_from_rpm_diff(
      pos.rpm, ev.config->angle + default_config.decoder.offset - new_advance));

  ck_assert(ev.start.state == SCHED_SCHEDULED);
  ck_assert(ev.stop.state == SCHED_SCHEDULED);
}
END_TEST

START_TEST(check_schedule_ignition_reschedule_completely_earlier_still_future) {
  struct engine_position pos = { .has_position = true,
                                 .has_rpm = true,
                                 .rpm = 6000,
                                 .tooth_rpm = 6000,
                                 .valid_until = -1 };

  struct output_event_schedule_state ev = ignition_ev;

  const uint32_t dwell = 1000;
  const float advance = 10;

  schedule_ignition_event(&default_config, &ev, 0, &pos, advance, dwell);

  /* Reschedule 10 degrees later */
  const float new_advance = 20;
  schedule_ignition_event(&default_config, &ev, 0, &pos, new_advance, dwell);

  ck_assert(ev.start.state == SCHED_SCHEDULED);
  ck_assert(ev.stop.state == SCHED_SCHEDULED);

  ck_assert_int_eq(ev.stop.time - ev.start.time, time_from_us(dwell));
  ck_assert_int_eq(
    ev.stop.time,
    time_from_rpm_diff(
      pos.rpm, ev.config->angle + default_config.decoder.offset - new_advance));

  ck_assert(ev.start.state == SCHED_SCHEDULED);
  ck_assert(ev.stop.state == SCHED_SCHEDULED);
}
END_TEST

START_TEST(check_schedule_ignition_reschedule_onto_now) {

  struct engine_position pos = { .has_position = true,
                                 .has_rpm = true,
                                 .rpm = 6000,
                                 .tooth_rpm = 6000,
                                 .valid_until = -1 };

  timeval_t time = time_from_rpm_diff(6000, 340);

  const uint32_t dwell = 1000;
  const float advance = 15;

  struct output_event_schedule_state ev = ignition_ev;
  schedule_ignition_event(&default_config, &ev, time, &pos, advance, dwell);

  ck_assert(ev.start.state == SCHED_UNSCHEDULED);
  ck_assert(ev.stop.state == SCHED_UNSCHEDULED);
}
END_TEST

START_TEST(check_schedule_ignition_reschedule_active_later) {

  struct engine_position pos = { .has_position = true,
                                 .has_rpm = true,
                                 .rpm = 6000,
                                 .tooth_rpm = 6000,
                                 .valid_until = -1 };

  struct output_event_schedule_state ev = ignition_ev;

  const uint32_t dwell = 1000;
  const float advance = 10;

  schedule_ignition_event(&default_config, &ev, 0, &pos, advance, dwell);

  /* Move to a time we're sure the event can no longer reschedule */
  timeval_t original_start_time = ev.start.time;
  timeval_t time = original_start_time + 1;
  ev.start.state = SCHED_SUBMITTED;

  /* Reschedule 10* later */
  float new_advance = 0;
  schedule_ignition_event(&default_config, &ev, time, &pos, new_advance, dwell);

  ck_assert(ev.stop.state == SCHED_SCHEDULED);
  /* We shouldn't have changed the start */
  ck_assert_int_eq(ev.start.time, original_start_time);

  /* The end time should have rescheduled though */
  ck_assert_int_eq(
    ev.stop.time,
    time_from_rpm_diff(pos.rpm,
                       ev.config->angle + default_config.decoder.offset));
}
END_TEST

START_TEST(check_schedule_fuel_reschedule_active_later) {
  struct engine_position pos = { .has_position = true,
                                 .has_rpm = true,
                                 .rpm = 6000,
                                 .tooth_rpm = 6000,
                                 .valid_until = -1 };

  struct output_event_config ev_conf = {
    .type = FUEL_EVENT,
    .angle = 360,
    .pin = 0,
    .inverted = false,
  };

  struct output_event_schedule_state ev = {
    .config = &ev_conf,
  };

  const unsigned int pw = 1000;

  schedule_fuel_event(&default_config, &ev, 0, &pos, pw);

  /* Move to a time we're sure the event can no longer reschedule */
  timeval_t original_start_time = ev.start.time;
  timeval_t time = original_start_time + 1;
  ev.start.state = SCHED_SUBMITTED;

  /* Reschedule 10* later, and one microsecond longer */
  const unsigned int new_pw = pw + 1;
  ev_conf.angle += 10;
  schedule_fuel_event(&default_config, &ev, time, &pos, new_pw);

  ck_assert(ev.stop.state == SCHED_SCHEDULED);
  /* We shouldn't have changed the start */
  ck_assert_int_eq(ev.start.time, original_start_time);
  /* and total time should be the new 1001 time */
  ck_assert_int_eq(ev.stop.time, original_start_time + time_from_us(new_pw));
}
END_TEST

/* Special case where rescheduling before last trigger is
 * reinterpretted as future */
START_TEST(check_schedule_ignition_reschedule_active_too_early) {
  struct engine_position pos = { .has_position = true,
                                 .has_rpm = true,
                                 .rpm = 6000,
                                 .tooth_rpm = 6000,
                                 .valid_until = -1 };

  struct output_event_schedule_state ev = ignition_ev;

  const uint32_t dwell = 1000;
  const float advance = 10;

  schedule_ignition_event(&default_config, &ev, 0, &pos, advance, dwell);
  timeval_t original_stop = ev.stop.time;

  timeval_t time = time_from_rpm_diff(6000, 340);
  ck_assert_int_gt(time, ev.start.time); // Make sure we have started the event
  ev.start.state = SCHED_SUBMITTED;

  /* Reschedule 30 degrees earlier, to before the current time */
  const float new_advance = 40;
  schedule_ignition_event(&default_config, &ev, time, &pos, new_advance, dwell);

  /* Should result in no changes */
  ck_assert(ev.stop.state == SCHED_SCHEDULED);
  ck_assert(ev.stop.time == original_stop);
}
END_TEST

START_TEST(check_schedule_fuel_immediately_after_finish) {

  struct engine_position pos = { .has_position = true,
                                 .has_rpm = true,
                                 .rpm = 6000,
                                 .tooth_rpm = 6000,
                                 .valid_until = -1 };

  struct output_event_config ev_conf = {
    .type = FUEL_EVENT,
    .angle = 360,
    .pin = 0,
    .inverted = false,
  };

  struct output_event_schedule_state ev = {
    .config = &ev_conf,
  };

  const unsigned int pw = 1000;

  schedule_fuel_event(&default_config, &ev, 0, &pos, pw);

  ev.start.state = SCHED_FIRED;
  ev.stop.state = SCHED_FIRED;

  /* Attempt to reschedule to immediately after it fired */
  timeval_t time = ev.stop.time + 5;
  pos.last_trigger_angle = 360;
  schedule_fuel_event(&default_config, &ev, time, &pos, pw);

  ck_assert(ev.start.state != SCHED_SCHEDULED);
  ck_assert(ev.stop.state != SCHED_SCHEDULED);

  /* Wait a bit longer and reschedule */
  time += time_from_rpm_diff(pos.rpm, 180);
  pos.last_trigger_angle = 540;
  pos.time = time_from_rpm_diff(pos.rpm, 540);
  schedule_fuel_event(&default_config, &ev, time, &pos, pw);

  ck_assert(ev.start.state == SCHED_SCHEDULED);
  ck_assert(ev.stop.state == SCHED_SCHEDULED);
}
END_TEST

START_TEST(check_deschedule_event) {
  /* Test descheduling scheduled event in the future */
  struct output_event_config conf = {
    .type = FUEL_EVENT,
  };

  struct output_event_schedule_state ev = {
    .config = &conf,
    .start = { .time = 10, .state = SCHED_SCHEDULED },
    .stop = { .time = 20, .state = SCHED_SCHEDULED },
  };

  deschedule_event(&ev);
  ck_assert(ev.start.state == SCHED_UNSCHEDULED);
  ck_assert(ev.stop.state == SCHED_UNSCHEDULED);

  /* Test a submitted start */
  struct output_event_schedule_state ev2 = {
    .config = &conf,
    .start = { .time = 10, .state = SCHED_SUBMITTED },
    .stop = { .time = 20, .state = SCHED_SCHEDULED },
  };
  deschedule_event(&ev2);
  ck_assert(ev2.start.state == SCHED_SUBMITTED);
  ck_assert(ev2.stop.state == SCHED_SCHEDULED);

  /* Test descheduling scheduled event with already-fired start */
  struct output_event_schedule_state ev3 = {
    .config = &conf,
    .start = { .time = 10, .state = SCHED_FIRED },
    .stop = { .time = 20, .state = SCHED_SCHEDULED },
  };
  deschedule_event(&ev3);
  ck_assert(ev3.start.state == SCHED_FIRED);
  ck_assert(ev3.stop.state == SCHED_SCHEDULED);
}
END_TEST

TCase *setup_scheduler_tests() {
  TCase *tc = tcase_create("scheduler");
  tcase_add_test(tc, check_schedule_ignition);
  tcase_add_test(tc, check_schedule_ignition_reschedule_completely_later);
  tcase_add_test(
    tc, check_schedule_ignition_reschedule_completely_earlier_still_future);
  tcase_add_test(tc, check_schedule_ignition_reschedule_onto_now);
  tcase_add_test(tc, check_schedule_ignition_reschedule_active_later);
  tcase_add_test(tc, check_schedule_fuel_reschedule_active_later);
  tcase_add_test(tc, check_schedule_ignition_reschedule_active_too_early);
  tcase_add_test(tc, check_schedule_fuel_immediately_after_finish);
  tcase_add_test(tc, check_deschedule_event);
  return tc;
}
#endif
