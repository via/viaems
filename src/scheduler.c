#include "scheduler.h"
#include "config.h"
#include "decoder.h"
#include "platform.h"
#include "stats.h"
#include "util.h"

#include <assert.h>
#include <string.h>
#include <strings.h>

#define OUTPUT_BUFFER_LEN (512)
static struct output_buffer {
  timeval_t start;
  struct output_slot {
    uint16_t on_mask;  /* On little endian arch, most-significant */
    uint16_t off_mask; /* are last in struct */
  } __attribute__((packed)) slots[OUTPUT_BUFFER_LEN];
} output_buffers[2];

#define MAX_CALLBACKS 32
struct timed_callback *callbacks[MAX_CALLBACKS] = { 0 };
static int n_callbacks = 0;

static int sched_entry_has_fired(struct sched_entry *en) {
  assert(en);

  int ret = 0;
  int ints_on = disable_interrupts();

  if (en->buffer) {
    if (time_in_range(en->time, en->buffer->start, current_time())) {
      ret = 1;
    }
  }
  if (en->fired) {
    ret = 1;
  }
  if (ints_on) {
    enable_interrupts();
  }
  return ret;
}

int event_is_active(struct output_event *ev) {
  return (sched_entry_has_fired(&ev->start) &&
          !sched_entry_has_fired(&ev->stop));
}

int event_has_fired(struct output_event *ev) {
  return (sched_entry_has_fired(&ev->start) &&
          sched_entry_has_fired(&ev->stop));
}

static struct output_buffer *buffer_for_time(timeval_t time) {
  struct output_buffer *buf = NULL;
  if (time_in_range(time,
                    output_buffers[0].start,
                    output_buffers[0].start + OUTPUT_BUFFER_LEN - 1)) {
    buf = &output_buffers[0];
  } else if (time_in_range(time,
                           output_buffers[1].start,
                           output_buffers[1].start + OUTPUT_BUFFER_LEN - 1)) {
    buf = &output_buffers[1];
  }
  return buf;
}

static int fired_if_failed(struct sched_entry *en, int success) {
  en->fired = !success;
  return success;
}

/* Modifies the output buffer to not have the event, returns 0 if event is in
 * the past.
 *
 * Return of 1 means event has not fired. 0 means it couldn't be removed in
 * time.
 *
 * Does no bookkeeping but can set fired flag */
static int sched_entry_disable(const struct sched_entry *en, timeval_t time) {

  assert(!interrupts_enabled());
  /* before either buffer starts */
  if (time_before(time, output_buffers[current_output_buffer()].start)) {
    return 0;
  }

  struct output_buffer *buf = buffer_for_time(time);

  /* In the future, bail out successfully */
  if (!buf) {
    return 1;
  }

  int slot = time - buf->start;
  assert((slot >= 0) && (slot < 512));

  uint16_t *addr =
    en->val ? &buf->slots[slot].on_mask : &buf->slots[slot].off_mask;
  uint16_t value = *addr & ~(1 << en->pin);

  timeval_t before_time = current_time();
  *addr = value;
  timeval_t after_time = current_time();

  if (time_in_range(time, before_time, after_time + 1)) {
    set_output(en->pin, en->val);
    return 0;
  }
  if (time_before(time, before_time) || en->fired) {
    return 0;
  }

  return 1;
}

/* Modifies the output buffer to have the event, returns 0 if event is in the
 * past.
 *
 * Return of 0 means the event did not fire.
 *
 * Does no bookkeeping of sched_entry */
static int sched_entry_enable(const struct sched_entry *en, timeval_t time) {

  assert(!interrupts_enabled());
  /* before either buffer starts */
  if (time_before(time, output_buffers[current_output_buffer()].start)) {
    return 0;
  }

  struct output_buffer *buf = buffer_for_time(time);

  /* In the future, bail out successfully */
  if (!buf) {
    return 1;
  }

  int slot = time - buf->start;
  assert((slot >= 0) && (slot < 512));

  uint16_t *addr =
    en->val ? &buf->slots[slot].on_mask : &buf->slots[slot].off_mask;
  uint16_t value = *addr | (1 << en->pin);

  timeval_t before_time = current_time();
  *addr = value;
  timeval_t after_time = current_time();

  if (time_in_range(time, before_time, after_time + 1)) {
    set_output(en->pin, en->val);
  }

  if (time_before(time, before_time)) {
    return 0;
  }

  return 1;
}

static void sched_entry_update(struct sched_entry *en, timeval_t time) {
  en->buffer = buffer_for_time(time);
  en->scheduled = 1;
  en->time = time;
}

static void sched_entry_off(struct sched_entry *en) {
  en->scheduled = 0;
  en->buffer = NULL;
}

void deschedule_event(struct output_event *ev) {
  int ints_en = disable_interrupts();

  if (ev->start.fired) {
    enable_interrupts();
    return;
  }

  int success;
  success = fired_if_failed(&ev->start,
                            sched_entry_disable(&ev->start, ev->start.time));
  if (success) {
    fired_if_failed(&ev->stop, sched_entry_disable(&ev->stop, ev->stop.time));
    sched_entry_off(&ev->start);
    sched_entry_off(&ev->stop);
  }
  if (ints_en) {
    enable_interrupts();
  }
}

void invalidate_scheduled_events(struct output_event *evs, int n) {
  for (int i = 0; i < n; ++i) {
    if (!evs[i].start.scheduled) {
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

static void reschedule_end(struct sched_entry *s,
                           timeval_t old,
                           timeval_t new) {
  if (new == old)
    return;

  int success;
  success = sched_entry_enable(s, new);
  if (success) {
    success = fired_if_failed(s, sched_entry_disable(s, old));
    if (!success) {
      sched_entry_disable(s, new);
      sched_entry_update(s, old);
    } else {
      sched_entry_update(s, new);
    }
  }
}

/* Schedules an output event in a hazard-free manner, assuming
 * that the start and stop times occur at least after curtime
 */
void schedule_output_event_safely(struct output_event *ev,
                                  timeval_t newstart,
                                  timeval_t newstop,
                                  int preserve_duration) {

  stats_start_timing(STATS_SCHED_SINGLE_TIME);
  int success;

  timeval_t oldstart = ev->start.time;
  timeval_t oldstop = ev->stop.time;

  ev->start.pin = ev->pin;
  ev->start.val = ev->inverted ? 0 : 1;
  ev->stop.pin = ev->pin;
  ev->stop.val = ev->inverted ? 1 : 0;

  if (!ev->start.scheduled && !ev->stop.scheduled) {
    int ints_on = disable_interrupts();
    if (sched_entry_enable(&ev->stop, newstop)) {
      sched_entry_update(&ev->stop, newstop);
      if (sched_entry_enable(&ev->start, newstart)) {
        sched_entry_update(&ev->start, newstart);
      } else {
        sched_entry_disable(&ev->start, newstart);
        sched_entry_off(&ev->start);
        sched_entry_disable(&ev->stop, newstop);
        sched_entry_off(&ev->stop);
      }
    }
    if (ints_on) {
      enable_interrupts();
    }
    stats_finish_timing(STATS_SCHED_SINGLE_TIME);
    return;
  }

  int ints_on = disable_interrupts();
  if (oldstart == newstart) {
    if (time_before(ev->start.time, newstop) || preserve_duration) {
      reschedule_end(&ev->stop, oldstop, newstop);
    }
  } else if (time_before(newstart, oldstart)) {
    success = sched_entry_enable(&ev->start, newstart);
    if (!success && preserve_duration) {
      newstop += oldstart - newstart;
    }
    if (success) {
      sched_entry_disable(&ev->start, oldstart);
      sched_entry_update(&ev->start, newstart);
    } else {
      fired_if_failed(&ev->start, sched_entry_disable(&ev->start, newstart));
    }
    if (time_before(ev->start.time, newstop) || preserve_duration) {
      reschedule_end(&ev->stop, oldstop, newstop);
    }
  } else {
    success =
      fired_if_failed(&ev->start, sched_entry_disable(&ev->start, oldstart));
    if (!success && preserve_duration) {
      newstop += oldstart - newstart;
    }
    reschedule_end(&ev->stop, oldstop, newstop);
    if (success) {
      sched_entry_enable(&ev->start, newstart);
      sched_entry_update(&ev->start, newstart);
    }
  }

  if (ints_on) {
    enable_interrupts();
  }
  stats_finish_timing(STATS_SCHED_SINGLE_TIME);
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

    ev->start.fired = 0;
    ev->stop.fired = 0;
    ev->start.scheduled = 0;
    ev->stop.scheduled = 0;
  }

  /* Don't let the stop time move more than 180*
   * forward once it is scheduled */
  if (ev->stop.scheduled && time_before(ev->stop.time, stop_time) &&
      ((time_diff(stop_time, ev->stop.time) >
        time_from_rpm_diff(d->rpm, 180)))) {
    return 0;
  }

  if (time_diff(start_time, ev->stop.time) <
      time_from_us(config.ignition.min_fire_time_us)) {
    /* Too little time since last fire */
    return 0;
  }

  schedule_output_event_safely(ev, start_time, stop_time, 0);

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

    ev->start.fired = 0;
    ev->stop.fired = 0;
    ev->start.scheduled = 0;
    ev->stop.scheduled = 0;
  }

  /* Don't let the stop time move more than 180*
   * forward once it is scheduled
   * TODO evaluate if this is necessary for fueling */

  if (ev->stop.scheduled && time_before(ev->stop.time, stop_time) &&
      ((time_diff(stop_time, ev->stop.time) >
        time_from_rpm_diff(d->rpm, 180)))) {
    return 0;
  }

  schedule_output_event_safely(ev, start_time, stop_time, 1);

  /* Schedule a callback to reschedule this immediately after it fires */
  if (ev->stop.scheduled) {
    ev->callback.callback = (void (*)(void *))schedule_event;
    ev->callback.data = ev;
    schedule_callback(&ev->callback, ev->stop.time);
  }

  return 1;
}

void schedule_event(struct output_event *ev) {
  switch (ev->type) {
  case IGNITION_EVENT:
    if (ignition_cut() || !config.decoder.valid) {
      invalidate_scheduled_events(config.events, config.num_events);
      return;
    }
    schedule_ignition_event(ev,
                            &config.decoder,
                            (degrees_t)calculated_values.timing_advance,
                            calculated_values.dwell_us);
    break;

  case FUEL_EVENT:
    if (fuel_cut() || !config.decoder.valid) {
      invalidate_scheduled_events(config.events, config.num_events);
      return;
    }
    schedule_fuel_event(ev, &config.decoder, calculated_values.fueling_us);
    break;

  default:
    break;
  }
}

static void callback_remove(struct timed_callback *tcb) {
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

static void callback_insert(struct timed_callback *tcb) {

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

int schedule_callback(struct timed_callback *tcb, timeval_t time) {

  int ints_on = disable_interrupts();
  if (tcb->scheduled) {
    callback_remove(tcb);
  }

  tcb->time = time;
  callback_insert(tcb);

  if (callbacks[0]->time != get_event_timer()) {
    set_event_timer(callbacks[0]->time);

    if (time_before(callbacks[0]->time, current_time())) {
      /* Handle now */
      scheduler_callback_timer_execute();
    }
  }
  if (ints_on) {
    enable_interrupts();
  }

  return 0;
}

void scheduler_callback_timer_execute() {
  while (n_callbacks && time_before(callbacks[0]->time, current_time())) {
    clear_event_timer();
    struct timed_callback *cb = callbacks[0];
    callback_remove(cb);
    if (cb->callback) {
      cb->callback(cb->data);
    }
    if (!n_callbacks) {
      disable_event_timer();
    } else {
      set_event_timer(callbacks[0]->time);
    }
  }
}

void scheduler_buffer_swap() {
  int newbuf = (current_output_buffer() + 1) % 2;
  struct output_buffer *obuf = &output_buffers[newbuf];

  disable_interrupts();
  struct output_event *oev;
  int i;
  for (i = 0; i < MAX_EVENTS; ++i) {
    oev = &config.events[i];

    /* OEVs that were in the old buffer are no longer */
    if (oev->start.buffer == obuf) {
      oev->start.buffer = NULL;
      oev->start.fired = 1;
    }
    if (oev->stop.buffer == obuf) {
      oev->stop.buffer = NULL;
      oev->stop.fired = 1;
    }
  }

  memset(obuf->slots, 0, sizeof(struct output_slot) * OUTPUT_BUFFER_LEN);
  obuf->start += 2 * OUTPUT_BUFFER_LEN;
  timeval_t end = obuf->start + OUTPUT_BUFFER_LEN - 1;

  for (i = 0; i < MAX_EVENTS; ++i) {
    oev = &config.events[i];
    /* Is this an event that is scheduled for this time window? */
    if (oev->start.scheduled &&
        time_in_range(oev->start.time, obuf->start, end)) {
      sched_entry_enable(&oev->start, oev->start.time);
      sched_entry_update(&oev->start, oev->start.time);
    }
    if (oev->stop.scheduled &&
        time_in_range(oev->stop.time, obuf->start, end)) {
      sched_entry_enable(&oev->stop, oev->stop.time);
      sched_entry_update(&oev->stop, oev->stop.time);
    }
  }
  enable_interrupts();
}

void initialize_scheduler() {
  memset(&output_buffers, 0, sizeof(output_buffers));

  output_buffers[0].start =
    init_output_thread((uint32_t *)output_buffers[0].slots,
                       (uint32_t *)output_buffers[1].slots,
                       OUTPUT_BUFFER_LEN);
  output_buffers[1].start = output_buffers[0].start + OUTPUT_BUFFER_LEN;

  n_callbacks = 0;
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
  config.decoder.valid = 1;
  config.num_events = 1;
  *oev = (struct output_event){
    .type = IGNITION_EVENT,
    .angle = 360,
    .pin = 0,
    .inverted = 0,
  };
}

START_TEST(check_schedule_ignition) {

  /* Set our current position at 270* for an event at 360* */
  set_current_time(time_from_rpm_diff(6000, 270));
  schedule_ignition_event(oev, &config.decoder, 10, 1000);
  ck_assert(oev->start.scheduled);
  ck_assert(oev->stop.scheduled);
  ck_assert(!oev->start.fired);
  ck_assert(!oev->stop.fired);

  ck_assert_int_eq(oev->stop.time - oev->start.time,
                   1000 * (TICKRATE / 1000000));
  ck_assert_int_eq(oev->stop.time,
                   time_from_rpm_diff(config.decoder.rpm,
                                      oev->angle + config.decoder.offset - 10));
}
END_TEST

START_TEST(check_schedule_ignition_reschedule_completely_later) {

  set_current_time(time_from_rpm_diff(6000, 270));
  schedule_ignition_event(oev, &config.decoder, 10, 1000);

  set_current_time(oev->start.time - 100);

  /* Reschedule 10 degrees later */
  schedule_ignition_event(oev, &config.decoder, 0, 1000);

  ck_assert(oev->start.scheduled);
  ck_assert(oev->stop.scheduled);
  ck_assert(!oev->start.fired);
  ck_assert(!oev->stop.fired);

  ck_assert_int_eq(oev->stop.time - oev->start.time,
                   1000 * (TICKRATE / 1000000));
  ck_assert_int_eq(
    oev->stop.time,
    time_from_rpm_diff(config.decoder.rpm, oev->angle + config.decoder.offset));
}
END_TEST

START_TEST(check_schedule_ignition_reschedule_completely_earlier_still_future) {

  set_current_time(time_from_rpm_diff(6000, 180));
  schedule_ignition_event(oev, &config.decoder, 10, 1000);
  /* Reschedule 10 earlier later */
  schedule_ignition_event(oev, &config.decoder, 50, 1000);

  ck_assert(oev->start.scheduled);
  ck_assert(oev->stop.scheduled);
  ck_assert(!oev->start.fired);
  ck_assert(!oev->stop.fired);

  ck_assert_int_eq(oev->stop.time - oev->start.time,
                   1000 * (TICKRATE / 1000000));
  ck_assert_int_eq(oev->stop.time,
                   time_from_rpm_diff(config.decoder.rpm,
                                      oev->angle + config.decoder.offset - 50));
}
END_TEST

START_TEST(check_schedule_ignition_reschedule_onto_now) {

  set_current_time(time_from_rpm_diff(6000, 340));
  schedule_ignition_event(oev, &config.decoder, 15, 1000);

  /* Start would fail, stop should schedule */
  ck_assert(!oev->start.scheduled);
  ck_assert(!oev->stop.scheduled);
  ck_assert(!oev->start.fired);
  ck_assert(!oev->stop.fired);
}
END_TEST

START_TEST(check_schedule_ignition_reschedule_active_later) {

  set_current_time(time_from_rpm_diff(6000, 270));
  schedule_ignition_event(oev, &config.decoder, 10, 1000);

  /* Emulate firing of the event */
  set_current_time(oev->start.time + 1);

  /* Reschedule 10* later */
  schedule_ignition_event(oev, &config.decoder, 0, 1000);

  ck_assert(oev->stop.scheduled);
  ck_assert_int_eq(
    oev->stop.time,
    time_from_rpm_diff(config.decoder.rpm, oev->angle + config.decoder.offset));
}
END_TEST

/* Special case where rescheduling before last trigger is
 * reinterpretted as future */
START_TEST(check_schedule_ignition_reschedule_active_too_early) {
  oev->angle = 60;
  set_current_time(time_from_rpm_diff(6000, 0));
  schedule_ignition_event(oev, &config.decoder, 0, 1000);
  /* Emulate firing of the event */
  set_current_time(oev->start.time + 5);

  timeval_t old_stop = oev->stop.time;
  /* Reschedule 45* earlier, now in past*/
  schedule_ignition_event(oev, &config.decoder, 45, 1000);

  ck_assert(oev->stop.scheduled);
  ck_assert_int_eq(oev->stop.time, old_stop);
}
END_TEST

START_TEST(check_schedule_fuel_immediately_after_finish) {
  oev->angle = 60;
  config.decoder.rpm = 6000;
  schedule_fuel_event(oev, &config.decoder, 1000);

  /* Emulate firing of the event */
  set_current_time(oev->stop.time + 5);

  /* Reschedule same event */
  ck_assert(!schedule_fuel_event(oev, &config.decoder, 1000));
}
END_TEST

START_TEST(check_event_is_active) {
  ck_assert(!event_is_active(oev));

  oev->start.fired = 1;
  ck_assert(event_is_active(oev));

  oev->stop.fired = 1;
  ck_assert(!event_is_active(oev));

  oev->start.fired = 0;
  ck_assert(!event_is_active(oev));
}
END_TEST

START_TEST(check_event_has_fired) {}
END_TEST

START_TEST(check_invalidate_events_when_active) {
  /* Schedule an event, get in the middle of it */
  set_current_time(time_from_rpm_diff(6000, 270));
  schedule_ignition_event(oev, &config.decoder, 10, 1000);
  set_current_time(oev->start.time + 500);

  invalidate_scheduled_events(oev, 1);

  ck_assert(oev->stop.scheduled);
}
END_TEST

START_TEST(check_deschedule_event) {}
END_TEST

START_TEST(check_buffer_insert_totally_after) {
  struct output_event oev = { 0 };

  schedule_output_event_safely(&oev, 20, 40, 0);

  schedule_output_event_safely(&oev, 80, 100, 0);
  ck_assert_int_eq(output_buffers[0].slots[20].on_mask, 0);
  ck_assert_int_eq(output_buffers[0].slots[40].off_mask, 0);
  ck_assert_int_eq(output_buffers[0].slots[80].on_mask, 1);
  ck_assert_int_eq(output_buffers[0].slots[100].off_mask, 1);
  ck_assert_int_eq(oev.start.scheduled, 1);
  ck_assert_int_eq(oev.stop.scheduled, 1);

  schedule_output_event_safely(&oev, 100, 150, 1);
  ck_assert_int_eq(output_buffers[0].slots[80].on_mask, 0);
  ck_assert_int_eq(output_buffers[0].slots[100].off_mask, 0);
  ck_assert_int_eq(output_buffers[0].slots[100].on_mask, 1);
  ck_assert_int_eq(output_buffers[0].slots[150].off_mask, 1);
  ck_assert_int_eq(oev.start.scheduled, 1);
  ck_assert_int_eq(oev.stop.scheduled, 1);
}
END_TEST

START_TEST(check_buffer_insert_totally_before) {
  struct output_event oev = { 0 };

  schedule_output_event_safely(&oev, 80, 100, 0);

  schedule_output_event_safely(&oev, 20, 40, 0);
  ck_assert_int_eq(output_buffers[0].slots[80].on_mask, 0);
  ck_assert_int_eq(output_buffers[0].slots[100].off_mask, 0);
  ck_assert_int_eq(output_buffers[0].slots[20].on_mask, 1);
  ck_assert_int_eq(output_buffers[0].slots[40].off_mask, 1);
  ck_assert_int_eq(oev.start.scheduled, 1);
  ck_assert_int_eq(oev.stop.scheduled, 1);

  schedule_output_event_safely(&oev, 10, 15, 1);
  ck_assert_int_eq(output_buffers[0].slots[20].on_mask, 0);
  ck_assert_int_eq(output_buffers[0].slots[40].off_mask, 0);
  ck_assert_int_eq(output_buffers[0].slots[10].on_mask, 1);
  ck_assert_int_eq(output_buffers[0].slots[15].off_mask, 1);
  ck_assert_int_eq(oev.start.scheduled, 1);
  ck_assert_int_eq(oev.stop.scheduled, 1);
}
END_TEST

START_TEST(check_buffer_insert_totally_inside) {
  struct output_event oev = { 0 };

  schedule_output_event_safely(&oev, 20, 100, 0);

  schedule_output_event_safely(&oev, 30, 90, 0);
  ck_assert_int_eq(output_buffers[0].slots[20].on_mask, 0);
  ck_assert_int_eq(output_buffers[0].slots[100].off_mask, 0);
  ck_assert_int_eq(output_buffers[0].slots[30].on_mask, 1);
  ck_assert_int_eq(output_buffers[0].slots[90].off_mask, 1);
  ck_assert_int_eq(oev.start.scheduled, 1);
  ck_assert_int_eq(oev.stop.scheduled, 1);

  schedule_output_event_safely(&oev, 30, 80, 0);
  ck_assert_int_eq(output_buffers[0].slots[90].off_mask, 0);
  ck_assert_int_eq(output_buffers[0].slots[30].on_mask, 1);
  ck_assert_int_eq(output_buffers[0].slots[80].off_mask, 1);
  ck_assert_int_eq(oev.start.scheduled, 1);
  ck_assert_int_eq(oev.stop.scheduled, 1);

  schedule_output_event_safely(&oev, 40, 80, 0);
  ck_assert_int_eq(output_buffers[0].slots[30].on_mask, 0);
  ck_assert_int_eq(output_buffers[0].slots[40].on_mask, 1);
  ck_assert_int_eq(output_buffers[0].slots[80].off_mask, 1);
  ck_assert_int_eq(oev.start.scheduled, 1);
  ck_assert_int_eq(oev.stop.scheduled, 1);
}
END_TEST

START_TEST(check_buffer_insert_totally_outside) {
  struct output_event oev = { 0 };

  schedule_output_event_safely(&oev, 40, 80, 0);

  schedule_output_event_safely(&oev, 30, 90, 0);
  ck_assert_int_eq(output_buffers[0].slots[40].on_mask, 0);
  ck_assert_int_eq(output_buffers[0].slots[80].off_mask, 0);
  ck_assert_int_eq(output_buffers[0].slots[30].on_mask, 1);
  ck_assert_int_eq(output_buffers[0].slots[90].off_mask, 1);
  ck_assert_int_eq(oev.start.scheduled, 1);
  ck_assert_int_eq(oev.stop.scheduled, 1);

  schedule_output_event_safely(&oev, 30, 100, 0);
  ck_assert_int_eq(output_buffers[0].slots[90].off_mask, 0);
  ck_assert_int_eq(output_buffers[0].slots[30].on_mask, 1);
  ck_assert_int_eq(output_buffers[0].slots[100].off_mask, 1);
  ck_assert_int_eq(oev.start.scheduled, 1);
  ck_assert_int_eq(oev.stop.scheduled, 1);

  schedule_output_event_safely(&oev, 20, 100, 0);
  ck_assert_int_eq(output_buffers[0].slots[30].on_mask, 0);
  ck_assert_int_eq(output_buffers[0].slots[20].on_mask, 1);
  ck_assert_int_eq(output_buffers[0].slots[100].off_mask, 1);
  ck_assert_int_eq(oev.start.scheduled, 1);
  ck_assert_int_eq(oev.stop.scheduled, 1);
}
END_TEST

START_TEST(check_buffer_insert_partially_later) {
  struct output_event oev = { 0 };

  schedule_output_event_safely(&oev, 40, 80, 0);

  schedule_output_event_safely(&oev, 50, 90, 0);
  ck_assert_int_eq(output_buffers[0].slots[40].on_mask, 0);
  ck_assert_int_eq(output_buffers[0].slots[80].off_mask, 0);
  ck_assert_int_eq(output_buffers[0].slots[50].on_mask, 1);
  ck_assert_int_eq(output_buffers[0].slots[90].off_mask, 1);
  ck_assert_int_eq(oev.start.scheduled, 1);
  ck_assert_int_eq(oev.stop.scheduled, 1);
}
END_TEST

START_TEST(check_buffer_insert_partially_earlier) {
  struct output_event oev = { 0 };

  schedule_output_event_safely(&oev, 40, 80, 0);

  schedule_output_event_safely(&oev, 30, 70, 0);
  ck_assert_int_eq(output_buffers[0].slots[40].on_mask, 0);
  ck_assert_int_eq(output_buffers[0].slots[80].off_mask, 0);
  ck_assert_int_eq(output_buffers[0].slots[30].on_mask, 1);
  ck_assert_int_eq(output_buffers[0].slots[70].off_mask, 1);
  ck_assert_int_eq(oev.start.scheduled, 1);
  ck_assert_int_eq(oev.stop.scheduled, 1);
}
END_TEST

START_TEST(check_buffer_insert_active_later) {
  struct output_event oev = { 0 };

  schedule_output_event_safely(&oev, 40, 80, 0);

  set_current_time(50);
  /* If we don't preserve duration, it'll stay fired through */
  schedule_output_event_safely(&oev, 100, 120, 0);
  ck_assert_int_eq(output_buffers[0].slots[80].off_mask, 0);
  ck_assert_int_eq(output_buffers[0].slots[120].off_mask, 1);
  ck_assert_int_eq(oev.stop.scheduled, 1);
}
END_TEST

START_TEST(check_buffer_insert_active_later_preserve_duration) {
  struct output_event oev = { 0 };

  schedule_output_event_safely(&oev, 40, 80, 0);

  set_current_time(50);
  /* We want to preserve the pulse width at the expense
   * of end time */
  schedule_output_event_safely(&oev, 100, 120, 1);
  ck_assert_int_eq(output_buffers[0].slots[60].off_mask, 1);
  ck_assert_int_eq(output_buffers[0].slots[100].on_mask, 0);
  ck_assert_int_eq(oev.stop.scheduled, 1);
}
END_TEST

START_TEST(check_buffer_insert_active_earlier) {
  struct output_event oev = { 0 };

  schedule_output_event_safely(&oev, 40, 80, 0);

  set_current_time(50);
  /* Currently in this situation we give up, leave it the same.
   * The naive fix to this causes errors in other situations */
  schedule_output_event_safely(&oev, 30, 70, 0);
  ck_assert_int_eq(output_buffers[0].slots[40].off_mask, 0);
  ck_assert_int_eq(output_buffers[0].slots[80].off_mask, 0);
  ck_assert_int_eq(output_buffers[0].slots[70].off_mask, 1);
  ck_assert_int_eq(oev.stop.scheduled, 1);
}
END_TEST

START_TEST(check_buffer_insert_active_earlier_repeated) {
  struct output_event oev = { 0 };

  schedule_output_event_safely(&oev, 40, 80, 0);

  set_current_time(50);
  schedule_output_event_safely(&oev, 30, 70, 0);
  ck_assert_int_eq(output_buffers[0].slots[40].off_mask, 0);
  ck_assert_int_eq(output_buffers[0].slots[80].off_mask, 0);
  ck_assert_int_eq(output_buffers[0].slots[70].off_mask, 1);
  ck_assert_int_eq(oev.stop.scheduled, 1);

  set_current_time(55);
  schedule_output_event_safely(&oev, 30, 60, 0);
  ck_assert_int_eq(output_buffers[0].slots[70].off_mask, 0);
  ck_assert_int_eq(output_buffers[0].slots[60].off_mask, 1);
  ck_assert_int_eq(oev.stop.scheduled, 1);

  set_current_time(58);
  schedule_output_event_safely(&oev, 30, 55, 0);
  ck_assert_int_eq(output_buffers[0].slots[60].off_mask, 1);
  ck_assert_int_eq(oev.stop.scheduled, 1);
}
END_TEST

START_TEST(check_buffer_insert_active_earlier_longer) {
  struct output_event oev = { 0 };

  schedule_output_event_safely(&oev, 40, 80, 0);

  set_current_time(50);
  schedule_output_event_safely(&oev, 30, 90, 0);
  ck_assert_int_eq(output_buffers[0].slots[80].off_mask, 0);
  ck_assert_int_eq(output_buffers[0].slots[90].off_mask, 1);
  ck_assert_int_eq(oev.stop.scheduled, 1);
}
END_TEST

START_TEST(check_buffer_insert_active_too_earlier) {
  struct output_event oev = { 0 };

  schedule_output_event_safely(&oev, 40, 80, 0);

  set_current_time(50);
  schedule_output_event_safely(&oev, 30, 45, 0);
  ck_assert_int_eq(output_buffers[0].slots[80].off_mask, 1);
  ck_assert_int_eq(oev.stop.scheduled, 1);
}
END_TEST

START_TEST(check_buffer_insert_active_earlier_not_yet_started) {
  struct output_event oev = { 0 };

  schedule_output_event_safely(&oev, 40, 80, 0);

  set_current_time(50);
  schedule_output_event_safely(&oev, 60, 70, 0);
  ck_assert_int_eq(output_buffers[0].slots[80].off_mask, 0);
  ck_assert_int_eq(output_buffers[0].slots[70].off_mask, 1);
  ck_assert_int_eq(oev.stop.scheduled, 1);
}
END_TEST

START_TEST(check_buffer_insert_active_earlier_preserve_duration) {
  struct output_event oev = { 0 };

  schedule_output_event_safely(&oev, 40, 80, 0);

  set_current_time(50);
  /* We want to preserve the pulse width at the expense
   * of end time */
  schedule_output_event_safely(&oev, 30, 60, 1);
  ck_assert_int_eq(output_buffers[0].slots[80].off_mask, 0);
  ck_assert_int_eq(output_buffers[0].slots[60].off_mask, 0);
  ck_assert_int_eq(output_buffers[0].slots[70].off_mask, 1);
  ck_assert_int_eq(oev.stop.scheduled, 1);
}
END_TEST

START_TEST(check_callback_insert) {

  struct timed_callback tc1 = { .time = 50 };
  struct timed_callback tc2 = { .time = 100 };
  struct timed_callback tc3 = { .time = 150 };

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

  struct timed_callback tc1 = { .time = 50 };
  struct timed_callback tc2 = { .time = 100 };
  struct timed_callback tc3 = { .time = 150 };

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

START_TEST(check_callback_execute) {

  int count = 0;

  void _increase_count(void *_c) {
    int *c = (int *)_c;
    (*c)++;
  }

  struct timed_callback tc1 = {
    .time = 95,
    .callback = _increase_count,
    .data = &count,
  };
  struct timed_callback tc2 = {
    .time = 100,
    .callback = _increase_count,
    .data = &count,
  };
  struct timed_callback tc3 = {
    .time = 150,
    .callback = _increase_count,
    .data = &count,
  };

  callback_insert(&tc1);
  callback_insert(&tc2);
  callback_insert(&tc3);

  set_current_time(101);
  scheduler_callback_timer_execute();

  ck_assert_int_eq(count, 2);
  ck_assert_int_eq(n_callbacks, 1);
  ck_assert(callbacks[0] == &tc3);

  set_current_time(151);

  scheduler_callback_timer_execute();
  ck_assert_int_eq(count, 3);
  ck_assert_int_eq(n_callbacks, 0);
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
  tcase_add_test(tc, check_schedule_ignition_reschedule_active_too_early);
  tcase_add_test(tc, check_schedule_fuel_immediately_after_finish);
  tcase_add_test(tc, check_event_is_active);
  tcase_add_test(tc, check_event_has_fired);
  tcase_add_test(tc, check_invalidate_events_when_active);
  tcase_add_test(tc, check_deschedule_event);
  tcase_add_test(tc, check_buffer_insert_totally_after);
  tcase_add_test(tc, check_buffer_insert_totally_before);
  tcase_add_test(tc, check_buffer_insert_totally_inside);
  tcase_add_test(tc, check_buffer_insert_totally_outside);
  tcase_add_test(tc, check_buffer_insert_partially_later);
  tcase_add_test(tc, check_buffer_insert_partially_earlier);

  tcase_add_test(tc, check_buffer_insert_active_later);
  tcase_add_test(tc, check_buffer_insert_active_later_preserve_duration);
  tcase_add_test(tc, check_buffer_insert_active_earlier);
  tcase_add_test(tc, check_buffer_insert_active_earlier_repeated);
  tcase_add_test(tc, check_buffer_insert_active_earlier_longer);
  tcase_add_test(tc, check_buffer_insert_active_too_earlier);
  tcase_add_test(tc, check_buffer_insert_active_earlier_not_yet_started);
  tcase_add_test(tc, check_buffer_insert_active_earlier_preserve_duration);

  tcase_add_test(tc, check_callback_insert);
  tcase_add_test(tc, check_callback_remove);
  tcase_add_test(tc, check_callback_execute);
  return tc;
}
#endif
