#include "decoder.h"
#include "config.h"
#include "console.h"
#include "platform.h"
#include "scheduler.h"
#include "stats.h"
#include "util.h"

#include <assert.h>
#include <stdlib.h>

static struct timed_callback expire_event;

static void invalidate_decoder() {
  config.decoder.valid = 0;
  config.decoder.state = DECODER_NOSYNC;
  config.decoder.current_triggers_rpm = 0;
  config.decoder.triggers_since_last_sync = 0;
  invalidate_scheduled_events(config.events, config.num_events);
}

static void handle_decoder_expire() {
  config.decoder.loss = DECODER_EXPIRED;
  invalidate_decoder();
}

static void set_expire_event(timeval_t t) {
  schedule_callback(&expire_event, t);
}

static void push_time(struct decoder *d, timeval_t t) {
  for (int i = MAX_TRIGGERS - 1; i > 0; --i) {
    d->times[i] = d->times[i - 1];
  }
  d->times[0] = t;
}

static unsigned int current_rpm_window_size(unsigned int current_triggers,
                                            unsigned int normal_window_size) {

  /* Use the minimum of rpm_window_size and current previous triggers so that
   * rpm is valid in case our window size is larger than required trigger
   * count */
  if (current_triggers < normal_window_size) {
    return current_triggers;
  } else {
    return normal_window_size;
  }
}

/* Update rpm information and validate */
static void trigger_update_rpm(struct decoder *d) {
  timeval_t diff = d->times[0] - d->times[1];
  unsigned int slicerpm = rpm_from_time_diff(diff, d->degrees_per_trigger);
  unsigned int rpm_window_size =
    current_rpm_window_size(d->current_triggers_rpm, d->rpm_window_size);

  if (rpm_window_size > 1) {
    /* We have at least two data points to draw an rpm from */
    d->rpm = rpm_from_time_diff(d->times[0] - d->times[rpm_window_size - 1],
                                d->degrees_per_trigger * (rpm_window_size - 1));
    if (d->rpm) {
      unsigned int delta =
        (d->rpm > slicerpm) ? d->rpm - slicerpm : slicerpm - d->rpm;
      d->trigger_cur_rpm_change = delta / (float)d->rpm;
    }
  } else {
    d->rpm = 0;
  }

  /* Check for excessive per-tooth variation */
  if ((slicerpm <= d->trigger_min_rpm) ||
      (slicerpm > d->rpm + (d->rpm * d->trigger_max_rpm_change)) ||
      (slicerpm < d->rpm - (d->rpm * d->trigger_max_rpm_change))) {
    d->state = DECODER_NOSYNC;
    d->loss = DECODER_VARIATION;
  }

  /* Check for too many triggers between syncs */
  if (d->triggers_since_last_sync > d->num_triggers) {
    d->state = DECODER_NOSYNC;
    d->loss = DECODER_TRIGGERCOUNT_HIGH;
  }

  /* TODO here is the place to handle a missing tooth */

  /* If we pass 150% of a inter-tooth delay, lose sync */
  d->expiration = d->times[0] + (timeval_t)(diff * 1.5);
  set_expire_event(d->expiration);
}

static void even_tooth_trigger_update(struct decoder *d, timeval_t t) {
  /* Bookkeeping */
  push_time(d, t);
  d->t0_count++;
  d->triggers_since_last_sync++;

  /* Count triggers up until a full wheel */
  if (d->current_triggers_rpm < MAX_TRIGGERS) {
    d->current_triggers_rpm++;
  }

  /* If we get enough triggers, we now have RPM */
  if ((d->state == DECODER_NOSYNC) &&
      (d->current_triggers_rpm >= d->required_triggers_rpm)) {
    d->state = DECODER_RPM;
  }

  if (d->state == DECODER_SYNC) {
    d->last_trigger_angle += d->degrees_per_trigger;
    if (d->last_trigger_angle >= 720) {
      d->last_trigger_angle -= 720;
    }
    d->last_trigger_time = t;
  }

  if (d->state == DECODER_RPM || d->state == DECODER_SYNC) {
    trigger_update_rpm(d);
  }
}

static uint32_t missing_tooth_rpm(struct decoder *d) {
  bool last_tooth_missing =
    (d->state == DECODER_SYNC) && (d->triggers_since_last_sync == 0);
  timeval_t last_tooth_diff = d->times[0] - d->times[1];
  degrees_t rpm_degrees = d->degrees_per_trigger * (last_tooth_missing ? 2 : 1);
  return rpm_from_time_diff(last_tooth_diff, rpm_degrees);
}

static void missing_tooth_trigger_update(struct decoder *d, timeval_t t) {
  push_time(d, t);
  d->t0_count++;

  /* Count triggers up until a full wheel */
  if (d->current_triggers_rpm < MAX_TRIGGERS) {
    d->current_triggers_rpm++;
  }

  if (d->current_triggers_rpm < d->required_triggers_rpm) {
    return;
  }

  /* If we get enough triggers, we now have RPM */
  if (d->state == DECODER_NOSYNC) {
    d->state = DECODER_RPM;
  }

  timeval_t last_tooth_diff = d->times[0] - d->times[1];

  /* Calculate the last N average. If we are synced, and the missing tooth was
   * in the last N teeth, don't forget to include it */
  timeval_t rpm_window_tooth_diff =
    d->times[1] - d->times[d->required_triggers_rpm];
  uint32_t rpm_window_tooth_count = d->required_triggers_rpm - 1;
  if ((d->state == DECODER_SYNC) &&
      (d->triggers_since_last_sync < rpm_window_tooth_count)) {
    rpm_window_tooth_count += 1; /* TODO: check this logic */
  }

  timeval_t rpm_window_average_tooth_diff =
    rpm_window_tooth_diff / rpm_window_tooth_count;

  /* Four possibilities:
   * We are in RPM, looking for a missing tooth:
   *  - Last tooth time is 1.5-2.5x the last N tooth average: We become synced
   *  - Last tooth variance is more than 0.5x-1.5x the last N tooth average,
   *  drop to NOSYNC
   *
   * Or, We are in sync. Stay in sync unless:
   *   - This isn't when we expect a missing tooth: Last tooth time is More than
   * 0.5x-1.5x, go to NOSYNC
   *   - This is when we're expecting to see a tooth, time is not 1.5x to 2.5x
   *   last N average
   */

  float tooth_ratio = last_tooth_diff / (float)rpm_window_average_tooth_diff;
  bool is_acceptable_missing_tooth =
    (tooth_ratio >= 1.5f) && (tooth_ratio <= 2.5f);
  bool is_acceptable_normal_tooth =
    (tooth_ratio >= 0.5f) && (tooth_ratio <= 1.5f);

  if (d->state == DECODER_RPM) {
    if (is_acceptable_missing_tooth) {
      d->state = DECODER_SYNC;
      d->triggers_since_last_sync = 0;
      /* TODO reconcile this with a cam phase sync */
      d->last_trigger_angle = 0;
    } else if (!is_acceptable_normal_tooth) {
      d->state = DECODER_NOSYNC;
      d->loss = DECODER_VARIATION;
    }
  } else if (d->state == DECODER_SYNC) {
    d->triggers_since_last_sync += 1;
    /* Compare against num_triggers - 1, since we're missing a tooth */
    if (d->triggers_since_last_sync == d->num_triggers - 1) {
      if (is_acceptable_missing_tooth) {
        d->triggers_since_last_sync = 0;
      } else {
        d->state = DECODER_NOSYNC;
        d->loss = DECODER_TRIGGERCOUNT_HIGH;
      }
    } else if ((d->triggers_since_last_sync != d->num_triggers - 1) &&
               !is_acceptable_normal_tooth) {
      d->state = DECODER_NOSYNC;
      d->loss = DECODER_TRIGGERCOUNT_LOW;
    }
    degrees_t rpm_degrees =
      d->degrees_per_trigger * (is_acceptable_missing_tooth ? 2 : 1);
    d->last_trigger_time = t;
    d->last_trigger_angle += rpm_degrees;
    if (d->last_trigger_angle >= 720) {
      d->last_trigger_angle -= 720;
    }
  }
}

static void even_tooth_sync_update(struct decoder *d) {
  d->t1_count++;
  if (d->state == DECODER_RPM) {
    d->state = DECODER_SYNC;
    d->loss = DECODER_NO_LOSS;
    d->last_trigger_angle = 0;
  } else if (d->state == DECODER_SYNC) {
    if (d->triggers_since_last_sync == d->num_triggers) {
      d->state = DECODER_SYNC;
      d->loss = DECODER_NO_LOSS;
      d->last_trigger_angle = 0;
    } else {
      d->state = DECODER_NOSYNC;
      d->loss = DECODER_TRIGGERCOUNT_LOW;
    }
  }
  d->triggers_since_last_sync = 0;
}

static void decode_even_with_camsync(struct decoder *d,
                                     struct decoder_event *ev) {
  decoder_state oldstate = d->state;

  if (ev->trigger == 0) {
    even_tooth_trigger_update(d, ev->time);
  } else if (ev->trigger == 1) {
    even_tooth_sync_update(d);
  }

  if (d->state == DECODER_SYNC) {
    d->valid = 1;
    d->loss = DECODER_NO_LOSS;
  } else {
    if (oldstate == DECODER_SYNC) {
      /* We lost sync */
      invalidate_decoder();
    }
  }
}

static void decode_even_no_sync(struct decoder *d, struct decoder_event *ev) {
  decoder_state oldstate = d->state;
  even_tooth_trigger_update(d, ev->time);
  if (d->state == DECODER_RPM || d->state == DECODER_SYNC) {
    d->state = DECODER_SYNC;
    d->loss = DECODER_NO_LOSS;
    d->valid = 1;
    d->last_trigger_time = ev->time;
    d->triggers_since_last_sync = 0; /* There is no sync */
    ;
  } else {
    if (oldstate == DECODER_SYNC) {
      /* We lost sync */
      invalidate_decoder();
    }
  }
  stats_finish_timing(STATS_DECODE_TIME);
}

static void decode_missing_no_sync(struct decoder *d,
                                   struct decoder_event *ev) {
  decoder_state oldstate = d->state;

  if (ev->trigger == 0) {
    missing_tooth_trigger_update(d, ev->time);
  }

  if (d->state != DECODER_NOSYNC) {
    d->rpm = missing_tooth_rpm(d);
  }

  if (d->state == DECODER_SYNC) {
    d->valid = 1;
    d->loss = DECODER_NO_LOSS;
  } else {
    if (oldstate == DECODER_SYNC) {
      /* We lost sync */
      invalidate_decoder();
    }
  }
}

static void decode_missing_with_camsync(struct decoder *d,
                                        struct decoder_event *ev) {
  bool was_valid = d->valid;
  static bool camsync_seen_this_rotation = false;
  static bool camsync_seen_last_rotation = false;

  if (ev->trigger == 0) {
    missing_tooth_trigger_update(d, ev->time);
    if ((d->state == DECODER_SYNC) && (d->triggers_since_last_sync == 0)) {
      if (!camsync_seen_this_rotation && !camsync_seen_last_rotation) {
        /* We've gone two cycles without a camsync, desync */
        d->valid = 0;
      }
      camsync_seen_last_rotation = camsync_seen_this_rotation;
      camsync_seen_this_rotation = false;
    }
  } else if (ev->trigger == 1) {
    if (d->state == DECODER_SYNC) {
      if (camsync_seen_this_rotation == true) {
        /* We have already seen a cam sync */
        d->valid = 0;
        camsync_seen_this_rotation = false;
      } else {
        camsync_seen_this_rotation = true;
      }
    }
  }

  if (d->state != DECODER_NOSYNC) {
    d->rpm = missing_tooth_rpm(d);
  }

  bool has_seen_camsync = camsync_seen_this_rotation || camsync_seen_last_rotation;
  if (!was_valid && (d->state == DECODER_SYNC) && has_seen_camsync) {
    /* We gained sync */
    d->valid = 1;
    d->loss = DECODER_NO_LOSS;
    d->last_trigger_angle =
      d->degrees_per_trigger * d->triggers_since_last_sync +
      (camsync_seen_this_rotation ? 0 : 360);
  }
  if (was_valid && !d->valid) {
    /* We lost sync */
    invalidate_decoder();
  }
}

void decoder_init(struct decoder *d) {
  d->current_triggers_rpm = 0;
  d->valid = 0;
  d->rpm = 0;
  d->state = DECODER_NOSYNC;
  d->last_trigger_time = 0;
  d->last_trigger_angle = 0;
  d->triggers_since_last_sync = 0;
  d->expiration = 0;
  expire_event.callback = handle_decoder_expire;
}

static void decode(struct decoder *d, struct decoder_event *ev) {
  switch (d->type) {
  case TRIGGER_EVEN_NOSYNC:
    decode_even_no_sync(d, ev);
    break;
  case TRIGGER_EVEN_CAMSYNC:
    decode_even_with_camsync(d, ev);
    break;
  case TRIGGER_MISSING_NOSYNC:
    decode_missing_no_sync(d, ev);
    break;
  case TRIGGER_MISSING_CAMSYNC:
    decode_missing_with_camsync(d, ev);
    break;
  default:
    break;
  }
}

/* When decoder has new information, reschedule everything */
void decoder_update_scheduling(struct decoder_event *events,
                               unsigned int count) {
  stats_start_timing(STATS_SCHEDULE_LATENCY);

  /* TODO Right now this is a thin wrapper for the new decoder interface.
   * Convert the decode() functions to work directly off `decoder_event` structs
   */
  for (struct decoder_event *ev = events; count > 0; count--, ev++) {
    console_record_event((struct logged_event){
      .type = EVENT_TRIGGER,
      .value = ev->trigger,
      .time = ev->time,
    });
    decode(&config.decoder, ev);
  }

  if (config.decoder.valid) {
    calculate_ignition();
    calculate_fueling();
    stats_start_timing(STATS_SCHED_TOTAL_TIME);
    for (unsigned int e = 0; e < config.num_events; ++e) {
      schedule_event(&config.events[e]);
    }
    stats_finish_timing(STATS_SCHED_TOTAL_TIME);
  }
  stats_finish_timing(STATS_SCHEDULE_LATENCY);
}

degrees_t current_angle() {
  if (!config.decoder.rpm) {
    return config.decoder.last_trigger_angle;
  }
  degrees_t angle_since_last_tooth = degrees_from_time_diff(
    current_time() - config.decoder.last_trigger_time, config.decoder.rpm);

  return clamp_angle(config.decoder.last_trigger_angle + angle_since_last_tooth,
                     720);
}

#ifdef UNITTEST
#include "config.h"
#include "decoder.h"
#include "platform.h"

#include <check.h>

static struct decoder_event *find_last_trigger_event(
  struct decoder_event **entries) {
  struct decoder_event *entry;
  for (entry = *entries; entry->next; entry = entry->next)
    ;
  return entry;
}

static void append_trigger_event(struct decoder_event *last,
                                 struct decoder_event new) {
  last->next = malloc(sizeof(struct decoder_event));
  *(last->next) = new;
}

static void add_trigger_event(struct decoder_event **entries,
                              timeval_t duration,
                              unsigned int trigger) {
  if (!*entries) {
    *entries = malloc(sizeof(struct decoder_event));
    **entries = (struct decoder_event){
      .trigger = trigger,
      .time = duration,
      .state = DECODER_NOSYNC,
    };
    return;
  }

  struct decoder_event *last = find_last_trigger_event(entries);
  struct decoder_event new = (struct decoder_event){
    .trigger = trigger,
    .time = last->time + duration,
    .state = last->state,
    .valid = last->valid,
    .reason = last->reason,
  };
  append_trigger_event(last, new);
}

static void free_trigger_list(struct decoder_event *entries) {
  struct decoder_event *current = entries;
  struct decoder_event *next;
  while (current) {
    next = current->next;
    free(current);
    current = next;
  }
}

static void add_trigger_event_transition_rpm(struct decoder_event **entries,
                                             timeval_t duration,
                                             unsigned int trigger) {
  ck_assert(*entries);

  struct decoder_event *last = find_last_trigger_event(entries);
  struct decoder_event new = (struct decoder_event){
    .trigger = trigger,
    .time = last->time + duration,
    .state = DECODER_RPM,
    .valid = last->valid,
    .reason = last->reason,
  };
  append_trigger_event(last, new);
}

static void add_trigger_event_transition_sync(struct decoder_event **entries,
                                              timeval_t duration,
                                              unsigned int trigger) {
  ck_assert(*entries);

  struct decoder_event *last = find_last_trigger_event(entries);
  struct decoder_event new = (struct decoder_event){
    .trigger = trigger,
    .time = last->time + duration,
    .state = DECODER_SYNC,
    .valid = 1,
    .reason = 0,
  };
  append_trigger_event(last, new);
}

static void add_trigger_event_transition_loss(struct decoder_event **entries,
                                              timeval_t duration,
                                              unsigned int trigger,
                                              decoder_loss_reason reason) {
  ck_assert(*entries);

  struct decoder_event *last = find_last_trigger_event(entries);
  struct decoder_event new = (struct decoder_event){
    .trigger = trigger,
    .time = last->time + duration,
    .state = DECODER_NOSYNC,
    .valid = 0,
    .reason = reason,
  };
  append_trigger_event(last, new);
}

static void validate_decoder_sequence(struct decoder_event *ev) {
  for (; ev; ev = ev->next) {
    set_current_time(ev->time);
    decoder_update_scheduling(ev, 1);
    ck_assert_msg(
      config.decoder.state == ev->state,
      "expected event at %d: (state=%d, valid=%d). Got (state=%d, valid=%d)",
      ev->time,
      ev->state,
      ev->valid,
      config.decoder.state,
      config.decoder.valid);
    if (!ev->valid) {
      ck_assert_msg(config.decoder.loss == ev->reason,
                    "reason mismatch at %d: %d. Got %d",
                    ev->time,
                    ev->reason,
                    config.decoder.loss);
    }
  }
}

static void prepare_ford_tfi_decoder() {
  config.decoder.type = TRIGGER_EVEN_NOSYNC;
  config.decoder.required_triggers_rpm = 4;
  config.decoder.degrees_per_trigger = 90;
  config.decoder.rpm_window_size = 3;
  config.decoder.num_triggers = 8;
  decoder_init(&config.decoder);
}

static void prepare_7mgte_cps_decoder() {
  config.decoder.type = TRIGGER_EVEN_CAMSYNC;
  config.decoder.required_triggers_rpm = 8;
  config.decoder.degrees_per_trigger = 30;
  config.decoder.rpm_window_size = 8;
  config.decoder.num_triggers = 24;
  decoder_init(&config.decoder);
}

static void prepare_36minus1_cam_wheel_decoder() {
  config.decoder.type = TRIGGER_MISSING_NOSYNC;
  config.decoder.required_triggers_rpm = 4;
  config.decoder.degrees_per_trigger = 20;
  config.decoder.num_triggers = 36;
  decoder_init(&config.decoder);
}

static void prepare_36minus1_crank_wheel_plus_cam_decoder() {
  config.decoder.type = TRIGGER_MISSING_CAMSYNC;
  config.decoder.required_triggers_rpm = 4;
  config.decoder.degrees_per_trigger = 10;
  config.decoder.num_triggers = 36;
  decoder_init(&config.decoder);
}

START_TEST(check_tfi_decoder_startup_normal) {
  struct decoder_event *entries = NULL;
  prepare_ford_tfi_decoder();

  /* Triggers to get RPM */
  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }
  add_trigger_event_transition_sync(&entries, 25000, 0);
  add_trigger_event(&entries, 25000, 0);

  validate_decoder_sequence(entries);

  ck_assert_int_eq(config.decoder.last_trigger_angle, 90);

  free_trigger_list(entries);
}
END_TEST

START_TEST(check_tfi_decoder_syncloss_variation) {
  struct decoder_event *entries = NULL;
  prepare_ford_tfi_decoder();

  /* Triggers to get RPM */
  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }
  add_trigger_event_transition_sync(&entries, 25000, 0);
  add_trigger_event(&entries, 25000, 0);
  /* Trigger far too soon */
  add_trigger_event_transition_loss(&entries, 10000, 0, DECODER_VARIATION);

  validate_decoder_sequence(entries);

  ck_assert_int_eq(config.decoder.current_triggers_rpm, 0);

  free_trigger_list(entries);
}
END_TEST

START_TEST(check_tfi_decoder_syncloss_expire) {
  struct decoder_event *entries = NULL;
  prepare_ford_tfi_decoder();

  /* Triggers to get RPM */
  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }
  add_trigger_event_transition_sync(&entries, 25000, 0);
  add_trigger_event(&entries, 25000, 0);

  validate_decoder_sequence(entries);

  /* Currently expiration is not dependent on variation setting, fixed 1.5x
   * trigger time */
  timeval_t expected_expiration =
    find_last_trigger_event(&entries)->time + (1.5 * 25000);
  ck_assert_int_eq(config.decoder.expiration, expected_expiration);

  set_current_time(expected_expiration + 500);
  handle_decoder_expire(&config.decoder);
  ck_assert(!config.decoder.valid);
  ck_assert_int_eq(0, config.decoder.current_triggers_rpm);
  ck_assert_int_eq(DECODER_EXPIRED, config.decoder.loss);
  free_trigger_list(entries);
}
END_TEST

/* Gets decoder up to sync, plus an additional trigger */
static void cam_nplusone_normal_startup_to_sync(
  struct decoder_event **entries) {
  /* Triggers to get RPM */
  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm - 1; ++i) {
    add_trigger_event(entries, 25000, 0);
  }
  add_trigger_event_transition_rpm(entries, 25000, 0);

  /* Wait a few triggers before sync */
  add_trigger_event(entries, 25000, 0);
  add_trigger_event(entries, 25000, 0);

  /* Get a sync pulse and then continue to maintain sync for 3 triggers */
  add_trigger_event_transition_sync(entries, 500, 1);
  add_trigger_event(entries, 24500, 0);
}

START_TEST(check_cam_nplusone_startup_normal) {
  struct decoder_event *entries = NULL;
  prepare_7mgte_cps_decoder();

  cam_nplusone_normal_startup_to_sync(&entries);

  /* Two additional triggers, three total after sync pulse */
  add_trigger_event(&entries, 25000, 0);
  add_trigger_event(&entries, 25000, 0);

  validate_decoder_sequence(entries);

  ck_assert_int_eq(config.decoder.last_trigger_angle,
                   3 * config.decoder.degrees_per_trigger);

  free_trigger_list(entries);
}
END_TEST

START_TEST(check_cam_nplusone_startup_normal_then_early_sync) {
  struct decoder_event *entries = NULL;
  prepare_7mgte_cps_decoder();

  cam_nplusone_normal_startup_to_sync(&entries);

  /* Two additional triggers, three total after sync pulse */
  add_trigger_event(&entries, 25000, 0);
  add_trigger_event(&entries, 25000, 0);

  /* Spurious early sync */
  add_trigger_event_transition_loss(&entries, 500, 1, DECODER_TRIGGERCOUNT_LOW);

  validate_decoder_sequence(entries);

  ck_assert(!config.decoder.valid);

  free_trigger_list(entries);
}
END_TEST

START_TEST(check_cam_nplusone_startup_normal_sustained) {
  struct decoder_event *entries = NULL;
  prepare_7mgte_cps_decoder();

  cam_nplusone_normal_startup_to_sync(&entries);

  /* continued wheel of additional triggers */
  for (unsigned int i = 0; i < config.decoder.num_triggers - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }
  /* Plus another sync and trigger */
  add_trigger_event(&entries, 500, 1);
  add_trigger_event(&entries, 24500, 0);

  validate_decoder_sequence(entries);

  ck_assert_int_eq(config.decoder.last_trigger_angle,
                   1 * config.decoder.degrees_per_trigger);

  free_trigger_list(entries);
}
END_TEST

START_TEST(check_bulk_decoder_updates) {
  struct decoder_event *entries = NULL;
  prepare_7mgte_cps_decoder();

  cam_nplusone_normal_startup_to_sync(&entries);

  /* Turn entries linked list into array usable by interface */
  struct decoder_event *entry_list = malloc(sizeof(struct decoder_event));
  struct decoder_event *ev;
  int len;
  for (len = 0, ev = entries; ev != NULL; ++len, ev = ev->next) {
    entry_list[len] = *ev;
    entry_list = realloc(entry_list, sizeof(struct decoder) * (len + 1));
  }
  decoder_update_scheduling(entry_list, len);

  ck_assert(config.decoder.valid);
  ck_assert_int_eq(config.decoder.last_trigger_angle,
                   1 * config.decoder.degrees_per_trigger);
  free_trigger_list(entries);
  free(entry_list);
}
END_TEST

START_TEST(check_cam_nplusone_startup_normal_no_second_trigger) {
  struct decoder_event *entries = NULL;
  prepare_7mgte_cps_decoder();

  cam_nplusone_normal_startup_to_sync(&entries);

  /* continued wheel of additional triggers */
  for (unsigned int i = 0; i < config.decoder.num_triggers - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }
  /* Another trigger, no sync when there should be one */
  add_trigger_event_transition_loss(
    &entries, 25000, 0, DECODER_TRIGGERCOUNT_HIGH);

  validate_decoder_sequence(entries);
  ck_assert(!config.decoder.valid);
  free_trigger_list(entries);
}
END_TEST

START_TEST(check_nplusone_decoder_syncloss_expire) {
  struct decoder_event *entries = NULL;
  prepare_7mgte_cps_decoder();

  cam_nplusone_normal_startup_to_sync(&entries);

  /* Three extra triggers */
  add_trigger_event(&entries, 25000, 0);
  add_trigger_event(&entries, 25000, 0);
  add_trigger_event(&entries, 25000, 0);

  validate_decoder_sequence(entries);
  /* Currently expiration is not dependent on variation setting, fixed 1.5x
   * trigger time */
  timeval_t expected_expiration =
    find_last_trigger_event(&entries)->time + (1.5 * 25000);
  ck_assert_int_eq(config.decoder.expiration, expected_expiration);

  set_current_time(expected_expiration + 500);
  handle_decoder_expire(&config.decoder);
  ck_assert(!config.decoder.valid);
  ck_assert_int_eq(0, config.decoder.current_triggers_rpm);
  ck_assert_int_eq(DECODER_EXPIRED, config.decoder.loss);
  free_trigger_list(entries);
}
END_TEST

START_TEST(check_missing_tooth_wrong_wheel) {
  struct decoder_event *entries = NULL;
  prepare_36minus1_cam_wheel_decoder();

  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };
  add_trigger_event_transition_rpm(&entries, 25000, 0);

  /* Create logs of triggers, we should never get sync */
  for (unsigned int i = 0; i < config.decoder.num_triggers * 4; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }
  validate_decoder_sequence(entries);

  ck_assert_int_eq(config.decoder.valid, 0);
  free_trigger_list(entries);
}
END_TEST

START_TEST(check_missing_tooth_startup_normal) {
  struct decoder_event *entries = NULL;
  prepare_36minus1_cam_wheel_decoder();

  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };
  add_trigger_event_transition_rpm(&entries, 25000, 0);

  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }

  /* Missing tooth */
  add_trigger_event_transition_sync(&entries, 50000, 0);

  add_trigger_event(&entries, 25000, 0);
  validate_decoder_sequence(entries);

  ck_assert_int_eq(config.decoder.valid, 1);
  ck_assert_int_eq(config.decoder.last_trigger_angle,
                   config.decoder.degrees_per_trigger);
  free_trigger_list(entries);
}
END_TEST

START_TEST(check_missing_tooth_startup_then_early) {
  struct decoder_event *entries = NULL;
  prepare_36minus1_cam_wheel_decoder();

  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };
  add_trigger_event_transition_rpm(&entries, 25000, 0);

  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }

  /* Missing tooth */
  add_trigger_event_transition_sync(&entries, 50000, 0);

  for (unsigned int i = 0; i < 5; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }

  /* Another early missing tooth */
  add_trigger_event_transition_loss(
    &entries, 50000, 0, DECODER_TRIGGERCOUNT_LOW);

  validate_decoder_sequence(entries);

  ck_assert_int_eq(config.decoder.valid, 0);
  free_trigger_list(entries);
}
END_TEST

START_TEST(check_missing_tooth_startup_then_late) {
  struct decoder_event *entries = NULL;
  prepare_36minus1_cam_wheel_decoder();

  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };
  add_trigger_event_transition_rpm(&entries, 25000, 0);

  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }

  /* Missing tooth */
  add_trigger_event_transition_sync(&entries, 50000, 0);

  for (unsigned int i = 0; i < config.decoder.num_triggers - 2; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }

  /* Add a normal tooth when we should have a missing tooth */
  add_trigger_event_transition_loss(
    &entries, 25000, 0, DECODER_TRIGGERCOUNT_HIGH);

  /* Now a late missing tooth, we shouldn't immediately regain sync */
  add_trigger_event(&entries, 50000, 0);

  validate_decoder_sequence(entries);

  ck_assert_int_eq(config.decoder.valid, 0);
  free_trigger_list(entries);
}
END_TEST

START_TEST(check_missing_tooth_false_starts) {
  struct decoder_event *entries = NULL;
  prepare_36minus1_cam_wheel_decoder();

  add_trigger_event(&entries, 25000, 0);
  add_trigger_event(&entries, 50000, 0);
  add_trigger_event(&entries, 25000, 0);
  /* This does go immediately to sync, as we do no checking for rpm variance as
   * we're in RPM waiting for a missing tooth. Maybe this isn't ideal? */
  add_trigger_event_transition_sync(&entries, 50000, 0);
  add_trigger_event(&entries, 25000, 0);
  add_trigger_event_transition_loss(
    &entries, 50000, 0, DECODER_TRIGGERCOUNT_LOW);

  validate_decoder_sequence(entries);

  ck_assert_int_eq(config.decoder.valid, 0);
  free_trigger_list(entries);
}
END_TEST

START_TEST(check_missing_tooth_camsync_startup_no_cam_at_first) {
  struct decoder_event *entries = NULL;
  prepare_36minus1_crank_wheel_plus_cam_decoder();

  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };
  add_trigger_event_transition_rpm(&entries, 25000, 0);

  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }

  /* Missing tooth */
  add_trigger_event_transition_sync(&entries, 50000, 0);

  add_trigger_event(&entries, 25000, 0);
  validate_decoder_sequence(entries);

  /* We never saw a cam, so we can't be valid */
  ck_assert_int_eq(config.decoder.valid, 0);
  free_trigger_list(entries);
}
END_TEST

START_TEST(check_missing_tooth_camsync_startup_no_cam_long_time) {
  struct decoder_event *entries = NULL;
  prepare_36minus1_crank_wheel_plus_cam_decoder();

  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };
  add_trigger_event_transition_rpm(&entries, 25000, 0);

  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }

  /* Missing tooth */
  add_trigger_event_transition_sync(&entries, 50000, 0);

  for (unsigned int i = 0; i < config.decoder.num_triggers - 2; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }

  add_trigger_event(&entries, 50000, 0);

  validate_decoder_sequence(entries);

  /* We still expect it to not be valid even though we've passed two missing
   * teeth, because we still haven't see a cam cycle yet */
  ck_assert_int_eq(config.decoder.valid, 0);
  free_trigger_list(entries);
}
END_TEST

START_TEST(check_missing_tooth_camsync_startup_normal) {
  struct decoder_event *entries = NULL;
  prepare_36minus1_crank_wheel_plus_cam_decoder();

  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };
  add_trigger_event_transition_rpm(&entries, 25000, 0);

  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }

  /* Missing tooth */
  add_trigger_event_transition_sync(&entries, 50000, 0);
  add_trigger_event(&entries, 25000, 0);

  /* Sync tooth indicating 0-360* (first) revolution */
  add_trigger_event(&entries, 5000, 1);

  validate_decoder_sequence(entries);

  ck_assert_int_eq(config.decoder.valid, 1);
  ck_assert_int_eq(config.decoder.last_trigger_angle,
                   config.decoder.degrees_per_trigger);
  free_trigger_list(entries);
}
END_TEST

START_TEST(check_missing_tooth_camsync_startup_normal_nextrev) {
  struct decoder_event *entries = NULL;
  prepare_36minus1_crank_wheel_plus_cam_decoder();

  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };
  add_trigger_event_transition_rpm(&entries, 25000, 0);

  /* Missing tooth */
  add_trigger_event_transition_sync(&entries, 50000, 0);

  for (unsigned int i = 0; i < config.decoder.num_triggers - 2; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };

  /* Missing tooth */
  add_trigger_event(&entries, 50000, 0);

  /* Three teeth */
  add_trigger_event(&entries, 25000, 0);
  add_trigger_event(&entries, 25000, 0);
  add_trigger_event(&entries, 25000, 0);

  /* Sync tooth indicating 0-360* (first) revolution */
  add_trigger_event(&entries, 5000, 1);

  /* Three teeth */
  add_trigger_event(&entries, 25000, 0);
  add_trigger_event(&entries, 25000, 0);
  add_trigger_event(&entries, 25000, 0);

  validate_decoder_sequence(entries);

  ck_assert_int_eq(config.decoder.valid, 1);
  ck_assert_int_eq(config.decoder.last_trigger_angle,
                   config.decoder.degrees_per_trigger * 6);
  free_trigger_list(entries);
}
END_TEST

START_TEST(check_update_rpm_single_point) {
  struct decoder d = {
    .times = { 100 },
    .degrees_per_trigger = 30,
    .current_triggers_rpm = 1,
    .rpm_window_size = 4,
    .trigger_max_rpm_change = 0.5,
  };

  trigger_update_rpm(&d);
  ck_assert_int_eq(d.rpm, 0);
  ck_assert_int_eq(d.state, DECODER_NOSYNC);
}
END_TEST

START_TEST(check_update_rpm_sufficient_points) {
  struct decoder d = {
    .times = { 400, 300, 200, 100 },
    .degrees_per_trigger = 30,
    .current_triggers_rpm = 4,
    .rpm_window_size = 4,
    .num_triggers = 24,
    .trigger_max_rpm_change = 0.5,
  };

  trigger_update_rpm(&d);
  ck_assert_int_eq(d.rpm, rpm_from_time_diff(100, d.degrees_per_trigger));
}
END_TEST

START_TEST(check_update_rpm_window_larger) {
  struct decoder d = {
    .times = { 800, 700, 700, 500, 400, 300, 200, 100 },
    .degrees_per_trigger = 30,
    .current_triggers_rpm = 4,
    .rpm_window_size = 8,
    .num_triggers = 24,
    .trigger_max_rpm_change = 0.5,
  };

  trigger_update_rpm(&d);
  ck_assert_int_eq(d.rpm, rpm_from_time_diff(100, d.degrees_per_trigger));
}
END_TEST

START_TEST(check_update_rpm_window_smaller) {
  struct decoder d = {
    .times = { 800, 700, 700, 500, 400, 300, 200, 100 },
    .degrees_per_trigger = 30,
    .current_triggers_rpm = 8,
    .rpm_window_size = 4,
    .num_triggers = 24,
    .trigger_max_rpm_change = 0.5,
  };

  trigger_update_rpm(&d);
  ck_assert_int_eq(d.rpm, rpm_from_time_diff(100, d.degrees_per_trigger));
}
END_TEST

TCase *setup_decoder_tests() {
  TCase *decoder_tests = tcase_create("decoder");
  tcase_add_test(decoder_tests, check_tfi_decoder_startup_normal);
  tcase_add_test(decoder_tests, check_tfi_decoder_syncloss_variation);
  tcase_add_test(decoder_tests, check_tfi_decoder_syncloss_expire);
  tcase_add_test(decoder_tests, check_cam_nplusone_startup_normal);
  tcase_add_test(decoder_tests,
                 check_cam_nplusone_startup_normal_then_early_sync);
  tcase_add_test(decoder_tests, check_cam_nplusone_startup_normal_sustained);
  tcase_add_test(decoder_tests,
                 check_cam_nplusone_startup_normal_no_second_trigger);

  tcase_add_test(decoder_tests, check_missing_tooth_wrong_wheel);
  tcase_add_test(decoder_tests, check_missing_tooth_startup_normal);
  tcase_add_test(decoder_tests, check_missing_tooth_startup_then_early);
  tcase_add_test(decoder_tests, check_missing_tooth_startup_then_late);
  tcase_add_test(decoder_tests, check_missing_tooth_false_starts);

  tcase_add_test(decoder_tests,
                 check_missing_tooth_camsync_startup_no_cam_at_first);
  tcase_add_test(decoder_tests,
                 check_missing_tooth_camsync_startup_no_cam_long_time);
  tcase_add_test(decoder_tests, check_missing_tooth_camsync_startup_normal);
  tcase_add_test(decoder_tests,
                 check_missing_tooth_camsync_startup_normal_nextrev);

  tcase_add_test(decoder_tests, check_nplusone_decoder_syncloss_expire);
  tcase_add_test(decoder_tests, check_bulk_decoder_updates);

  tcase_add_test(decoder_tests, check_update_rpm_single_point);
  tcase_add_test(decoder_tests, check_update_rpm_sufficient_points);
  tcase_add_test(decoder_tests, check_update_rpm_window_larger);
  tcase_add_test(decoder_tests, check_update_rpm_window_smaller);
  return decoder_tests;
}
#endif
