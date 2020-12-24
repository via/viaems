#include "decoder.h"
#include "config.h"
#include "console.h"
#include "platform.h"
#include "scheduler.h"
#include "stats.h"
#include "util.h"

#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#define MAX_TRIGGERS 36

typedef enum {
  DECODER_NOSYNC,
  DECODER_RPM,
  DECODER_SYNC,
} decoder_state;

struct decoder_status decoder_status;

static timeval_t tooth_times[MAX_TRIGGERS];
static decoder_state state;
static uint32_t current_triggers_rpm;
static uint32_t triggers_since_last_sync;

static struct timed_callback expire_event;
static timeval_t last_trigger_time;
static degrees_t last_trigger_angle;
static timeval_t expiration;

static void invalidate_decoder() {
  decoder_status.valid = 0;
  state = DECODER_NOSYNC;
  current_triggers_rpm = 0;
  triggers_since_last_sync = 0;
  invalidate_scheduled_events(config.events, config.num_events);
}

static void handle_decoder_expire() {
  decoder_status.loss = DECODER_EXPIRED;
  invalidate_decoder();
}

static void set_expire_event(timeval_t t) {
  schedule_callback(&expire_event, t);
}

static void push_time(timeval_t t) {
  for (int i = MAX_TRIGGERS - 1; i > 0; --i) {
    tooth_times[i] = tooth_times[i - 1];
  }
  tooth_times[0] = t;
}

static void even_tooth_trigger_update(timeval_t t) {
  push_time(t);
  triggers_since_last_sync++;
  last_trigger_time = t;

  /* Count triggers up until a full wheel */
  if (current_triggers_rpm < MAX_TRIGGERS) {
    current_triggers_rpm += 1;
  }

  timeval_t diff = tooth_times[0] - tooth_times[1];

  if (current_triggers_rpm > 1) {
    /* We have at least two data points to draw an rpm from */
    decoder_status.tooth_rpm = rpm_from_time_diff(diff,
                                config.decoder.degrees_per_trigger);
  }

  if (current_triggers_rpm > 2) {
    /* We have enough to draw a variance */
    timeval_t previous = tooth_times[2] - tooth_times[1];
    if (previous) {
      decoder_status.cur_variance = (float)(diff - previous) / previous;
    }
  }

  /* If we get enough triggers, we now have RPM */
  if ((state == DECODER_NOSYNC) &&
      (current_triggers_rpm >= config.decoder.required_triggers_rpm)) {
    state = DECODER_RPM;
  }

  /* Check for excessive per-tooth variation */
  if (fabs(decoder_status.cur_variance) > config.decoder.max_variance) {
    state = DECODER_NOSYNC;
    decoder_status.loss = DECODER_VARIATION;
  }

  expiration = tooth_times[0] + diff * (1 + config.decoder.max_variance);
  set_expire_event(expiration);
}

static uint32_t missing_tooth_instantaneous_rpm() {
  bool last_tooth_missing =
    (state == DECODER_SYNC) && (triggers_since_last_sync == 0);
  timeval_t last_tooth_diff = tooth_times[0] - tooth_times[1];
  degrees_t rpm_degrees = config.decoder.degrees_per_trigger * (last_tooth_missing ? 2 : 1);
  return rpm_from_time_diff(last_tooth_diff, rpm_degrees);
}

static uint32_t average_rpm() {
  uint32_t n_triggers;
  switch (config.decoder.type) {
    case TRIGGER_MISSING_CAMSYNC:
    case TRIGGER_MISSING_NOSYNC:
      n_triggers = config.decoder.num_triggers - 1;
      break;
    case TRIGGER_EVEN_CAMSYNC:
    case TRIGGER_EVEN_NOSYNC:
    default:
      n_triggers = config.decoder.num_triggers;
      break;
  }

  if (current_triggers_rpm >= n_triggers) {
    if (triggers_since_last_sync == 0) {
      return rpm_from_time_diff(tooth_times[0] - tooth_times[n_triggers],
        config.decoder.num_triggers * config.decoder.degrees_per_trigger);
    } else {
      return decoder_status.rpm;
    }
  }
  return decoder_status.tooth_rpm;
}

static void missing_tooth_trigger_update(timeval_t t) {
  push_time(t);

  /* Count triggers up until a full wheel */
  if (current_triggers_rpm < MAX_TRIGGERS) {
    current_triggers_rpm++;
  }

  if (current_triggers_rpm < config.decoder.required_triggers_rpm) {
    return;
  }

  /* If we get enough triggers, we now have RPM */
  if (state == DECODER_NOSYNC) {
    state = DECODER_RPM;
  }


  timeval_t last_tooth_diff = tooth_times[0] - tooth_times[1];

  /* Calculate the last N average. If we are synced, and the missing tooth was
   * in the last N teeth, don't forget to include it */
  timeval_t rpm_window_tooth_diff =
    tooth_times[1] - tooth_times[config.decoder.required_triggers_rpm];
  uint32_t rpm_window_tooth_count = config.decoder.required_triggers_rpm - 1;
  if ((state == DECODER_SYNC) &&
      (triggers_since_last_sync < rpm_window_tooth_count)) {
    rpm_window_tooth_count += 1;
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
   *   - This isn't when we expect a missing tooth, but we have a gap
   *   - This is when we're expecting to see a tooth, but no gap
   */

  float tooth_ratio = last_tooth_diff / (float)rpm_window_average_tooth_diff;
  float max_variance = config.decoder.max_variance;
  bool is_acceptable_missing_tooth = (tooth_ratio >= 2.0f - max_variance) &&
                                     (tooth_ratio <= 2.0f + max_variance);
  bool is_acceptable_normal_tooth = (tooth_ratio >= 1.0f - max_variance) &&
                                    (tooth_ratio <= 1.0f + max_variance);

  /* Preserve meaning from old even tooth code */
  decoder_status.cur_variance = tooth_ratio < 1.0f ? 1.0f - tooth_ratio : tooth_ratio - 1.0f;

  if (state == DECODER_RPM) {
    if (is_acceptable_missing_tooth) {
      state = DECODER_SYNC;
      triggers_since_last_sync = 0;
      last_trigger_angle = 0;
    } else if (!is_acceptable_normal_tooth) {
      state = DECODER_NOSYNC;
      decoder_status.loss = DECODER_VARIATION;
    }
  } else if (state == DECODER_SYNC) {
    triggers_since_last_sync += 1;
    /* Compare against num_triggers - 1, since we're missing a tooth */
    if (triggers_since_last_sync == config.decoder.num_triggers - 1) {
      if (is_acceptable_missing_tooth) {
        triggers_since_last_sync = 0;
      } else {
        state = DECODER_NOSYNC;
        decoder_status.loss = DECODER_TRIGGERCOUNT_HIGH;
      }
    } else if ((triggers_since_last_sync != config.decoder.num_triggers - 1) &&
               !is_acceptable_normal_tooth) {
      state = DECODER_NOSYNC;
      decoder_status.loss = DECODER_TRIGGERCOUNT_LOW;
    }
    degrees_t rpm_degrees =
      config.decoder.degrees_per_trigger * (is_acceptable_missing_tooth ? 2 : 1);
    last_trigger_time = t;
    last_trigger_angle += rpm_degrees;
    if (last_trigger_angle >= 720) {
      last_trigger_angle -= 720;
    }
  }
  if (state != DECODER_NOSYNC) {
    decoder_status.tooth_rpm = missing_tooth_instantaneous_rpm();
    /* Are we expecting this to be the gap? */
    degrees_t expected_gap =
      config.decoder.degrees_per_trigger *
      ((triggers_since_last_sync == config.decoder.num_triggers - 2) ? 2 : 1);
    timeval_t expected_time = time_from_rpm_diff(decoder_status.tooth_rpm, expected_gap);
    expiration =
      tooth_times[0] +
      (timeval_t)(expected_time * (1.0f + config.decoder.max_variance));
    //set_expire_event(expiration);
  }
}

static void even_tooth_sync_update() {
  if (state == DECODER_RPM) {
    state = DECODER_SYNC;
    decoder_status.loss = DECODER_NO_LOSS;
    last_trigger_angle = 0;
  } else if (state == DECODER_SYNC) {
    if (triggers_since_last_sync == config.decoder.num_triggers) {
      state = DECODER_SYNC;
      decoder_status.loss = DECODER_NO_LOSS;
      last_trigger_angle = 0;
    } else {
      state = DECODER_NOSYNC;
      decoder_status.loss = DECODER_TRIGGERCOUNT_LOW;
    }
  }
  triggers_since_last_sync = 0;
}

static void decode_even_with_camsync(struct decoder_event *ev) {
  decoder_state oldstate = state;

  if (ev->trigger == 0) {
    even_tooth_trigger_update(ev->time);
  } else if (ev->trigger == 1) {
    even_tooth_sync_update();
  }

  /* Check for too many triggers between syncs */
  if (triggers_since_last_sync > config.decoder.num_triggers) {
    state = DECODER_NOSYNC;
    decoder_status.loss = DECODER_TRIGGERCOUNT_HIGH;
  }

  decoder_status.rpm = average_rpm();

  if (state == DECODER_SYNC) {
    decoder_status.valid = 1;
    decoder_status.loss = DECODER_NO_LOSS;
    last_trigger_angle = config.decoder.degrees_per_trigger * triggers_since_last_sync;
  } else {
    if (oldstate == DECODER_SYNC) {
      /* We lost sync */
      invalidate_decoder();
    }
  }
}

static void decode_even_no_sync(struct decoder_event *ev) {
  decoder_state oldstate = state;
  even_tooth_trigger_update(ev->time);
  if (state == DECODER_RPM || state == DECODER_SYNC) {
    state = DECODER_SYNC;
    decoder_status.loss = DECODER_NO_LOSS;
    decoder_status.valid = 1;
    decoder_status.rpm = decoder_status.tooth_rpm;
    last_trigger_time = ev->time;
    last_trigger_angle += config.decoder.degrees_per_trigger;
    triggers_since_last_sync = 0; /* There is no sync */
  } else {
    if (oldstate == DECODER_SYNC) {
      /* We lost sync */
      invalidate_decoder();
    }
  }
  if (last_trigger_angle > 720) {
    last_trigger_angle -= 720;
  }
  stats_finish_timing(STATS_DECODE_TIME);
}

static void decode_missing_no_sync(struct decoder_event *ev) {
  decoder_state oldstate = state;

  if (ev->trigger == 0) {
    missing_tooth_trigger_update(ev->time);
  }

  decoder_status.rpm = average_rpm();

  if (state == DECODER_SYNC) {
    decoder_status.valid = 1;
    decoder_status.loss = DECODER_NO_LOSS;
  } else {
    if (oldstate == DECODER_SYNC) {
      /* We lost sync */
      invalidate_decoder();
    }
  }
}

static void decode_missing_with_camsync(struct decoder_event *ev) {
  bool was_valid = decoder_status.valid;
  static bool camsync_seen_this_rotation = false;
  static bool camsync_seen_last_rotation = false;

  if (ev->trigger == 0) {
    missing_tooth_trigger_update(ev->time);
    if ((state == DECODER_SYNC) && (triggers_since_last_sync == 0)) {
      if (!camsync_seen_this_rotation && !camsync_seen_last_rotation) {
        /* We've gone two cycles without a camsync, desync */
        decoder_status.valid = 0;
      }
      camsync_seen_last_rotation = camsync_seen_this_rotation;
      camsync_seen_this_rotation = false;
    }
  } else if (ev->trigger == 1) {
    if (state == DECODER_SYNC) {
      if (camsync_seen_this_rotation || camsync_seen_last_rotation) {
        decoder_status.valid = 0;
        camsync_seen_this_rotation = false;
      } else {
        camsync_seen_this_rotation = true;
      }
    }
  }

  decoder_status.rpm = average_rpm();

  bool has_seen_camsync =
    camsync_seen_this_rotation || camsync_seen_last_rotation;
  if (state != DECODER_SYNC) {
    decoder_status.valid = 0;
  } else if (!was_valid && (state == DECODER_SYNC) && has_seen_camsync) {
    /* We gained sync */
    decoder_status.valid = 1;
    decoder_status.loss = DECODER_NO_LOSS;
    last_trigger_angle =
      config.decoder.degrees_per_trigger * triggers_since_last_sync +
      (camsync_seen_this_rotation ? 0 : 360);
  }
  if (was_valid && !decoder_status.valid) {
    /* We lost sync */
    invalidate_decoder();
  }
}

void decoder_init() {
  current_triggers_rpm = 0;
  decoder_status.valid = 0;
  decoder_status.rpm = 0;
  decoder_status.tooth_rpm = 0;
  state = DECODER_NOSYNC;
  last_trigger_time = 0;
  last_trigger_angle = 0;
  triggers_since_last_sync = 0;
  expiration = 0;
  if (config.decoder.max_variance == 0.0f) {
    config.decoder.max_variance = 0.5f;
  }
  expire_event.callback = handle_decoder_expire;
}

static void decode(struct decoder_event *ev) {
  switch (config.decoder.type) {
  case TRIGGER_EVEN_NOSYNC:
    decode_even_no_sync(ev);
    break;
  case TRIGGER_EVEN_CAMSYNC:
    decode_even_with_camsync(ev);
    break;
  case TRIGGER_MISSING_NOSYNC:
    decode_missing_no_sync(ev);
    break;
  case TRIGGER_MISSING_CAMSYNC:
    decode_missing_with_camsync(ev);
    break;
  default:
    break;
  }
}

/* When decoder has new information, reschedule everything */
void decoder_update_scheduling(struct decoder_event *events,
                               unsigned int count) {
  stats_start_timing(STATS_SCHEDULE_LATENCY);

  for (struct decoder_event *ev = events; count > 0; count--, ev++) {
    console_record_event((struct logged_event){
      .type = EVENT_TRIGGER,
      .value = ev->trigger,
      .time = ev->time,
    });
    if (ev->trigger == 0) {
      decoder_status.t0_count++;
    } else if (ev->trigger == 1) {
      decoder_status.t1_count++;
    }

    decode(ev);
  }

  if (decoder_status.valid) {
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
  if (!decoder_status.rpm) {
    return last_trigger_angle;
  }
  degrees_t angle_since_last_tooth = degrees_from_time_diff(
    current_time() - last_trigger_time, decoder_status.rpm);

  return clamp_angle(last_trigger_angle + angle_since_last_tooth,
                     720);
}

timeval_t next_time_of_angle(degrees_t angle) {
  degrees_t relative_angle = clamp_angle(angle - last_trigger_angle + config.decoder.offset, 720);
  return last_trigger_time + time_from_rpm_diff(decoder_status.tooth_rpm, relative_angle);
}

#ifdef UNITTEST
#include "config.h"
#include "decoder.h"
#include "platform.h"

#include <check.h>

struct decoder_test_event {
  unsigned int trigger;
  timeval_t time;
  decoder_state state;
  uint32_t valid;
  decoder_loss_reason reason;
  struct decoder_test_event *next;
};

struct decoder_event event_from_test_event(struct decoder_test_event ev) {
  return (struct decoder_event){.time = ev.time, .trigger = ev.trigger};
}

static struct decoder_test_event *find_last_trigger_event(
  struct decoder_test_event **entries) {
  struct decoder_test_event *entry;
  for (entry = *entries; entry->next; entry = entry->next)
    ;
  return entry;
}

static void append_trigger_event(struct decoder_test_event *last,
                                 struct decoder_test_event new) {
  last->next = malloc(sizeof(struct decoder_test_event));
  *(last->next) = new;
}

static void add_trigger_event(struct decoder_test_event **entries,
                              timeval_t duration,
                              unsigned int trigger) {
  if (!*entries) {
    *entries = malloc(sizeof(struct decoder_test_event));
    **entries = (struct decoder_test_event){
      .trigger = trigger,
      .time = duration,
      .state = DECODER_NOSYNC,
    };
    return;
  }

  struct decoder_test_event *last = find_last_trigger_event(entries);
  struct decoder_test_event new = (struct decoder_test_event){
    .trigger = trigger,
    .time = last->time + duration,
    .state = last->state,
    .valid = last->valid,
    .reason = last->reason,
  };
  append_trigger_event(last, new);
}

static void free_trigger_list(struct decoder_test_event *entries) {
  struct decoder_test_event *current = entries;
  struct decoder_test_event *next;
  while (current) {
    next = current->next;
    free(current);
    current = next;
  }
}

static void add_trigger_event_transition(struct decoder_test_event **entries,
                                         timeval_t duration,
                                         unsigned int trigger,
                                         uint32_t validity,
                                         decoder_state state,
                                         decoder_loss_reason reason) {
  ck_assert(*entries);
  struct decoder_test_event *last = find_last_trigger_event(entries);
  struct decoder_test_event new = (struct decoder_test_event){
    .trigger = trigger,
    .time = last->time + duration,
    .state = state,
    .valid = validity,
    .reason = reason,
  };
  append_trigger_event(last, new);
}

static void validate_decoder_sequence(struct decoder_test_event *ev) {
  for (; ev; ev = ev->next) {
    set_current_time(ev->time);
    struct decoder_event dev = event_from_test_event(*ev);
    decoder_update_scheduling(&dev, 1);
    ck_assert_msg(
      (config.decoder.state == ev->state) &&
        (config.decoder.valid == ev->valid),
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
  config.decoder.num_triggers = 8;
  decoder_init(&config.decoder);
}

static void prepare_7mgte_cps_decoder() {
  config.decoder.type = TRIGGER_EVEN_CAMSYNC;
  config.decoder.required_triggers_rpm = 8;
  config.decoder.degrees_per_trigger = 30;
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
  struct decoder_test_event *entries = NULL;
  prepare_ford_tfi_decoder();

  /* Triggers to get RPM */
  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }
  add_trigger_event_transition(
    &entries, 25000, 0, 1, DECODER_SYNC, DECODER_NO_LOSS);
  add_trigger_event(&entries, 25000, 0);

  validate_decoder_sequence(entries);

  ck_assert_int_eq(last_trigger_angle, 90);

  free_trigger_list(entries);
}
END_TEST

START_TEST(check_tfi_decoder_syncloss_variation) {
  struct decoder_test_event *entries = NULL;
  prepare_ford_tfi_decoder();

  /* Triggers to get RPM */
  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }
  add_trigger_event_transition(
    &entries, 25000, 0, 1, DECODER_SYNC, DECODER_NO_LOSS);
  add_trigger_event(&entries, 25000, 0);
  /* Trigger far too soon */
  add_trigger_event_transition(
    &entries, 10000, 0, 0, DECODER_NOSYNC, DECODER_VARIATION);

  validate_decoder_sequence(entries);

  ck_assert_int_eq(config.decoder.current_triggers_rpm, 0);

  free_trigger_list(entries);
}
END_TEST

START_TEST(check_tfi_decoder_syncloss_expire) {
  struct decoder_test_event *entries = NULL;
  prepare_ford_tfi_decoder();

  /* Triggers to get RPM */
  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }
  add_trigger_event_transition(
    &entries, 25000, 0, 1, DECODER_SYNC, DECODER_NO_LOSS);
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
  struct decoder_test_event **entries) {
  /* Triggers to get RPM */
  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm - 1; ++i) {
    add_trigger_event(entries, 25000, 0);
  }
  add_trigger_event_transition(
    entries, 25000, 0, 0, DECODER_RPM, DECODER_NO_LOSS);

  /* Wait a few triggers before sync */
  add_trigger_event(entries, 25000, 0);
  add_trigger_event(entries, 25000, 0);

  /* Get a sync pulse and then continue to maintain sync for 3 triggers */
  add_trigger_event_transition(
    entries, 500, 1, 1, DECODER_SYNC, DECODER_NO_LOSS);
  add_trigger_event(entries, 24500, 0);
}

START_TEST(check_cam_nplusone_startup_normal) {
  struct decoder_test_event *entries = NULL;
  prepare_7mgte_cps_decoder();

  cam_nplusone_normal_startup_to_sync(&entries);

  /* Two additional triggers, three total after sync pulse */
  add_trigger_event(&entries, 25000, 0);
  add_trigger_event(&entries, 25000, 0);

  validate_decoder_sequence(entries);

  ck_assert_int_eq(last_trigger_angle,
                   3 * config.decoder.degrees_per_trigger);

  free_trigger_list(entries);
}
END_TEST

START_TEST(check_cam_nplusone_startup_normal_then_early_sync) {
  struct decoder_test_event *entries = NULL;
  prepare_7mgte_cps_decoder();

  cam_nplusone_normal_startup_to_sync(&entries);

  /* Two additional triggers, three total after sync pulse */
  add_trigger_event(&entries, 25000, 0);
  add_trigger_event(&entries, 25000, 0);

  /* Spurious early sync */
  add_trigger_event_transition(
    &entries, 500, 1, 0, DECODER_NOSYNC, DECODER_TRIGGERCOUNT_LOW);

  validate_decoder_sequence(entries);

  ck_assert(!config.decoder.valid);

  free_trigger_list(entries);
}
END_TEST

START_TEST(check_cam_nplusone_startup_normal_sustained) {
  struct decoder_test_event *entries = NULL;
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

  ck_assert_int_eq(last_trigger_angle,
                   1 * config.decoder.degrees_per_trigger);

  free_trigger_list(entries);
}
END_TEST

START_TEST(check_bulk_decoder_updates) {
  struct decoder_test_event *entries = NULL;
  prepare_7mgte_cps_decoder();

  cam_nplusone_normal_startup_to_sync(&entries);

  /* Turn entries linked list into array usable by interface */
  struct decoder_event *entry_list = malloc(sizeof(struct decoder_event));
  struct decoder_test_event *ev;
  int len;
  for (len = 0, ev = entries; ev != NULL; ++len, ev = ev->next) {
    entry_list[len] = event_from_test_event(*ev);
    entry_list = realloc(entry_list, sizeof(struct decoder_event) * (len + 1));
  }
  decoder_update_scheduling(entry_list, len);

  ck_assert(config.decoder.valid);
  ck_assert_int_eq(last_trigger_angle,
                   1 * config.decoder.degrees_per_trigger);
  free_trigger_list(entries);
  free(entry_list);
}
END_TEST

START_TEST(check_cam_nplusone_startup_normal_no_second_trigger) {
  struct decoder_test_event *entries = NULL;
  prepare_7mgte_cps_decoder();

  cam_nplusone_normal_startup_to_sync(&entries);

  /* continued wheel of additional triggers */
  for (unsigned int i = 0; i < config.decoder.num_triggers - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }
  /* Another trigger, no sync when there should be one */
  add_trigger_event_transition(
    &entries, 25500, 0, 0, DECODER_NOSYNC, DECODER_TRIGGERCOUNT_HIGH);

  validate_decoder_sequence(entries);
  ck_assert(!config.decoder.valid);
  free_trigger_list(entries);
}
END_TEST

START_TEST(check_nplusone_decoder_syncloss_expire) {
  struct decoder_test_event *entries = NULL;
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
  struct decoder_test_event *entries = NULL;
  prepare_36minus1_cam_wheel_decoder();

  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };
  add_trigger_event_transition(
    &entries, 25000, 0, 0, DECODER_RPM, DECODER_NO_LOSS);

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
  struct decoder_test_event *entries = NULL;
  prepare_36minus1_cam_wheel_decoder();

  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };
  add_trigger_event_transition(
    &entries, 25000, 0, 0, DECODER_RPM, DECODER_NO_LOSS);

  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }

  /* Missing tooth */
  add_trigger_event_transition(
    &entries, 50000, 0, 1, DECODER_SYNC, DECODER_NO_LOSS);

  add_trigger_event(&entries, 25000, 0);
  validate_decoder_sequence(entries);

  ck_assert_int_eq(config.decoder.valid, 1);
  ck_assert_int_eq(last_trigger_angle,
                   config.decoder.degrees_per_trigger);
  free_trigger_list(entries);
}
END_TEST

START_TEST(check_missing_tooth_rpm_after_gap) {
  struct decoder_test_event *entries = NULL;
  prepare_36minus1_cam_wheel_decoder();

  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };
  add_trigger_event_transition(
    &entries, 25000, 0, 0, DECODER_RPM, DECODER_NO_LOSS);

  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }

  /* Missing tooth */
  add_trigger_event_transition(
    &entries, 50000, 0, 1, DECODER_SYNC, DECODER_NO_LOSS);
  validate_decoder_sequence(entries);

  ck_assert_int_eq(config.decoder.valid, 1);
  ck_assert_int_eq(last_trigger_angle, 0);
  ck_assert_int_eq(
    config.decoder.rpm,
    rpm_from_time_diff(25000, config.decoder.degrees_per_trigger));
  free_trigger_list(entries);
}
END_TEST

START_TEST(check_missing_tooth_startup_then_early) {
  struct decoder_test_event *entries = NULL;
  prepare_36minus1_cam_wheel_decoder();

  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };
  add_trigger_event_transition(
    &entries, 25000, 0, 0, DECODER_RPM, DECODER_NO_LOSS);

  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }

  /* Missing tooth */
  add_trigger_event_transition(
    &entries, 50000, 0, 1, DECODER_SYNC, DECODER_NO_LOSS);

  for (unsigned int i = 0; i < 5; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }

  /* Another early missing tooth */
  add_trigger_event_transition(
    &entries, 50000, 0, 0, DECODER_NOSYNC, DECODER_TRIGGERCOUNT_LOW);

  validate_decoder_sequence(entries);

  ck_assert_int_eq(config.decoder.valid, 0);
  free_trigger_list(entries);
}
END_TEST

START_TEST(check_missing_tooth_startup_then_late) {
  struct decoder_test_event *entries = NULL;
  prepare_36minus1_cam_wheel_decoder();

  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };
  add_trigger_event_transition(
    &entries, 25000, 0, 0, DECODER_RPM, DECODER_NO_LOSS);

  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }

  /* Missing tooth */
  add_trigger_event_transition(
    &entries, 50000, 0, 1, DECODER_SYNC, DECODER_NO_LOSS);

  for (unsigned int i = 0; i < config.decoder.num_triggers - 2; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }

  /* Add a normal tooth when we should have a missing tooth */
  add_trigger_event_transition(
    &entries, 25000, 0, 0, DECODER_NOSYNC, DECODER_TRIGGERCOUNT_HIGH);

  /* Now a late missing tooth, we shouldn't immediately regain sync */
  add_trigger_event(&entries, 50000, 0);

  validate_decoder_sequence(entries);

  ck_assert_int_eq(config.decoder.valid, 0);
  free_trigger_list(entries);
}
END_TEST

START_TEST(check_missing_tooth_false_starts) {
  struct decoder_test_event *entries = NULL;
  prepare_36minus1_cam_wheel_decoder();

  add_trigger_event(&entries, 25000, 0);
  add_trigger_event(&entries, 50000, 0);
  add_trigger_event(&entries, 25000, 0);
  add_trigger_event_transition(
    &entries, 50000, 0, 0, DECODER_NOSYNC, DECODER_VARIATION);
  add_trigger_event(&entries, 25000, 0);
  add_trigger_event(&entries, 50000, 0);

  validate_decoder_sequence(entries);

  ck_assert_int_eq(config.decoder.valid, 0);
  free_trigger_list(entries);
}
END_TEST

START_TEST(check_missing_tooth_camsync_startup_no_cam_long_time) {
  struct decoder_test_event *entries = NULL;
  prepare_36minus1_crank_wheel_plus_cam_decoder();

  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };
  add_trigger_event_transition(
    &entries, 25000, 0, 0, DECODER_RPM, DECODER_NO_LOSS);

  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }

  /* Missing tooth, remaining invalid */
  add_trigger_event_transition(
    &entries, 50000, 0, 0, DECODER_SYNC, DECODER_NO_LOSS);

  for (unsigned int i = 0; i < config.decoder.num_triggers - 2; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }

  add_trigger_event(&entries, 50000, 0);

  validate_decoder_sequence(entries);
  free_trigger_list(entries);
}
END_TEST

START_TEST(check_missing_tooth_camsync_startup_normal_cam_first_rev) {
  struct decoder_test_event *entries = NULL;
  prepare_36minus1_crank_wheel_plus_cam_decoder();

  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };
  add_trigger_event_transition(
    &entries, 25000, 0, 0, DECODER_RPM, DECODER_NO_LOSS);

  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }

  /* Missing tooth, but remainings not valid due to lack of sync pulse */
  add_trigger_event_transition(
    &entries, 50000, 0, 0, DECODER_SYNC, DECODER_NO_LOSS);
  add_trigger_event(&entries, 25000, 0);

  /* Sync tooth indicating 0-360* (first) revolution, becomes valid */
  add_trigger_event_transition(
    &entries, 5000, 1, 1, DECODER_SYNC, DECODER_NO_LOSS);

  validate_decoder_sequence(entries);

  ck_assert_int_eq(last_trigger_angle,
                   config.decoder.degrees_per_trigger);
  free_trigger_list(entries);
}
END_TEST

START_TEST(check_missing_tooth_camsync_startup_normal_cam_second_rev) {
  struct decoder_test_event *entries = NULL;
  prepare_36minus1_crank_wheel_plus_cam_decoder();

  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };
  add_trigger_event_transition(
    &entries, 25000, 0, 0, DECODER_RPM, DECODER_NO_LOSS);

  /* Missing tooth */
  add_trigger_event_transition(
    &entries, 50000, 0, 0, DECODER_SYNC, DECODER_NO_LOSS);

  for (unsigned int i = 0; i < config.decoder.num_triggers - 2; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };

  /* Missing tooth */
  add_trigger_event(&entries, 50000, 0);

  /* Three teeth */
  add_trigger_event(&entries, 25000, 0);
  add_trigger_event(&entries, 25000, 0);
  add_trigger_event(&entries, 25000, 0);

  /* Sync tooth indicating 0-360* (first) revolution, becomes valid */
  add_trigger_event_transition(
    &entries, 5000, 1, 1, DECODER_SYNC, DECODER_NO_LOSS);

  /* Three teeth */
  add_trigger_event(&entries, 25000, 0);
  add_trigger_event(&entries, 25000, 0);
  add_trigger_event(&entries, 25000, 0);

  validate_decoder_sequence(entries);

  ck_assert_int_eq(last_trigger_angle,
                   config.decoder.degrees_per_trigger * 6);
  free_trigger_list(entries);
}
END_TEST

START_TEST(check_missing_tooth_camsync_startup_normal_cam_two_revs_in_row) {
  struct decoder_test_event *entries = NULL;
  prepare_36minus1_crank_wheel_plus_cam_decoder();

  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };
  add_trigger_event_transition(
    &entries, 25000, 0, 0, DECODER_RPM, DECODER_NO_LOSS);

  /* Missing tooth */
  add_trigger_event_transition(
    &entries, 50000, 0, 0, DECODER_SYNC, DECODER_NO_LOSS);

  /* Three teeth */
  add_trigger_event(&entries, 25000, 0);
  add_trigger_event(&entries, 25000, 0);
  add_trigger_event(&entries, 25000, 0);

  /* Sync tooth indicating 0-360* (first) revolution, becomes valid */
  add_trigger_event_transition(
    &entries, 5000, 1, 1, DECODER_SYNC, DECODER_NO_LOSS);

  for (unsigned int i = 0; i < config.decoder.num_triggers - 5; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };

  /* Missing tooth */
  add_trigger_event(&entries, 50000, 0);

  /* Three teeth */
  add_trigger_event(&entries, 25000, 0);
  add_trigger_event(&entries, 25000, 0);
  add_trigger_event(&entries, 25000, 0);

  /* Sync tooth when we shouldn't have one: lose sync */
  add_trigger_event_transition(
    &entries, 5000, 1, 0, DECODER_NOSYNC, DECODER_NO_LOSS);

  validate_decoder_sequence(entries);

  free_trigger_list(entries);
}
END_TEST

START_TEST(check_missing_tooth_camsync_startup_normal_cam_stops_working) {
  struct decoder_test_event *entries = NULL;
  prepare_36minus1_crank_wheel_plus_cam_decoder();

  for (unsigned int i = 0; i < config.decoder.required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };
  add_trigger_event_transition(
    &entries, 25000, 0, 0, DECODER_RPM, DECODER_NO_LOSS);

  /* Missing tooth */
  add_trigger_event_transition(
    &entries, 50000, 0, 0, DECODER_SYNC, DECODER_NO_LOSS);

  /* Three teeth */
  add_trigger_event(&entries, 25000, 0);
  add_trigger_event(&entries, 25000, 0);
  add_trigger_event(&entries, 25000, 0);

  /* Sync tooth indicating 0-360* (first) revolution, becomes valid */
  add_trigger_event_transition(
    &entries, 5000, 1, 1, DECODER_SYNC, DECODER_NO_LOSS);

  for (unsigned int i = 0; i < config.decoder.num_triggers - 5; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };

  /* Missing tooth */
  add_trigger_event(&entries, 50000, 0);

  for (unsigned int i = 0; i < config.decoder.num_triggers - 2; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };

  /* Missing tooth */
  add_trigger_event(&entries, 50000, 0);

  for (unsigned int i = 0; i < config.decoder.num_triggers - 2; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };

  /* Missing tooth: now we've not seen a cam sync in two cycles, lose sync */
  add_trigger_event_transition(
    &entries, 50000, 0, 0, DECODER_NOSYNC, DECODER_NO_LOSS);

  validate_decoder_sequence(entries);

  free_trigger_list(entries);
}
END_TEST

START_TEST(check_update_rpm_single_point) {
  struct decoder d = {
    .degrees_per_trigger = 30,
    .current_triggers_rpm = 1,
    .trigger_max_rpm_change = 0.5,
  };
  tooth_times = { 100 },

  trigger_update_rpm(&d);
  ck_assert_int_eq(d.rpm, 0);
  ck_assert_int_eq(d.state, DECODER_NOSYNC);
}
END_TEST

START_TEST(check_update_rpm_sufficient_points) {
  struct decoder d = {
    .degrees_per_trigger = 30,
    .required_triggers_rpm = 4,
    .num_triggers = 24,
    .trigger_max_rpm_change = 0.5,
  };
  tooth_times = { 400, 300, 200, 100 },

  trigger_update_rpm(&d);
  ck_assert_int_eq(d.rpm, rpm_from_time_diff(100, d.degrees_per_trigger));
}
END_TEST

START_TEST(check_update_rpm_window_larger) {
  struct decoder d = {
    .degrees_per_trigger = 30,
    .required_triggers_rpm = 4,
    .num_triggers = 24,
    .trigger_max_rpm_change = 0.5,
  };
  tooth_times = { 800, 700, 700, 500, 400, 300, 200, 100 },

  trigger_update_rpm(&d);
  ck_assert_int_eq(d.rpm, rpm_from_time_diff(100, d.degrees_per_trigger));
}
END_TEST

START_TEST(check_update_rpm_window_smaller) {
  struct decoder d = {
    .degrees_per_trigger = 30,
    .required_triggers_rpm = 8,
    .num_triggers = 24,
    .trigger_max_rpm_change = 0.5,
  };
  tooth_times = { 800, 700, 700, 500, 400, 300, 200, 100 },

  trigger_update_rpm(&d);
  ck_assert_int_eq(d.rpm, rpm_from_time_diff(100, d.degrees_per_trigger));
}
END_TEST

TCase *setup_decoder_tests() {
  TCase *decoder_tests = tcase_create("decoder");
  /* Even teeth with no sync */
  tcase_add_test(decoder_tests, check_tfi_decoder_startup_normal);
  tcase_add_test(decoder_tests, check_tfi_decoder_syncloss_variation);
  tcase_add_test(decoder_tests, check_tfi_decoder_syncloss_expire);

  /* Even teeth with cam sync */
  tcase_add_test(decoder_tests, check_cam_nplusone_startup_normal);
  tcase_add_test(decoder_tests,
                 check_cam_nplusone_startup_normal_then_early_sync);
  tcase_add_test(decoder_tests, check_cam_nplusone_startup_normal_sustained);
  tcase_add_test(decoder_tests,
                 check_cam_nplusone_startup_normal_no_second_trigger);
  tcase_add_test(decoder_tests, check_nplusone_decoder_syncloss_expire);

  /* Missing tooth with no sync */
  tcase_add_test(decoder_tests, check_missing_tooth_wrong_wheel);
  tcase_add_test(decoder_tests, check_missing_tooth_startup_normal);
  tcase_add_test(decoder_tests, check_missing_tooth_rpm_after_gap);
  tcase_add_test(decoder_tests, check_missing_tooth_startup_then_early);
  tcase_add_test(decoder_tests, check_missing_tooth_startup_then_late);
  tcase_add_test(decoder_tests, check_missing_tooth_false_starts);

  /* Missing tooth with cam sync */
  tcase_add_test(decoder_tests,
                 check_missing_tooth_camsync_startup_no_cam_long_time);
  tcase_add_test(decoder_tests,
                 check_missing_tooth_camsync_startup_normal_cam_first_rev);
  tcase_add_test(decoder_tests,
                 check_missing_tooth_camsync_startup_normal_cam_second_rev);
  tcase_add_test(
    decoder_tests,
    check_missing_tooth_camsync_startup_normal_cam_two_revs_in_row);
  tcase_add_test(decoder_tests,
                 check_missing_tooth_camsync_startup_normal_cam_stops_working);

  tcase_add_test(decoder_tests, check_bulk_decoder_updates);

  tcase_add_test(decoder_tests, check_update_rpm_single_point);
  tcase_add_test(decoder_tests, check_update_rpm_sufficient_points);
  tcase_add_test(decoder_tests, check_update_rpm_window_larger);
  tcase_add_test(decoder_tests, check_update_rpm_window_smaller);
  return decoder_tests;
}
#endif
