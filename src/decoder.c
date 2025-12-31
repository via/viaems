#include "decoder.h"
#include "config.h"
#include "console.h"
#include "platform.h"
#include "scheduler.h"
#include "sensors.h"
#include "util.h"
#include "fault.h"

#include <assert.h>
#include <stdlib.h>

static void invalidate_decoder(struct decoder *s) {
  s->state = DECODER_NOSYNC;
  s->current_triggers_rpm = 0;
  s->triggers_since_last_sync = 0;
  s->output.has_position = false;
  s->output.has_rpm = false;
}

void decoder_desync(struct decoder *state, decoder_loss_reason reason) {
  state->loss = reason;
  invalidate_decoder(state);
}

static void push_time(struct decoder *d, timeval_t t) {
  // TODO make this a circular buffer
  size_t len = sizeof(d->times) / sizeof(d->times[0]);
  for (int i = len - 1; i > 0; --i) {
    d->times[i] = d->times[i - 1];
  }
  d->times[0] = t;
}

static bool decoder_config_is_valid(const struct decoder_config *config) {
  bool valid = true;
  if ((int)config->type >= DECODER_TYPE_END) {
    valid = false;
  }

  if ((config->offset < 0.0f) || (config->offset >= 720.0f)) {
    valid = false;
  }

  if ((config->trigger_max_rpm_change < 0.0f) || (config->trigger_max_rpm_change > 1.0f)) {
    valid = false;
  }

  if (config->required_triggers_rpm > 36) {
    valid = false;
  }

  if (config->num_triggers > MAX_TRIGGERS) {
    valid = false;
  }

  if ((config->degrees_per_trigger < 1.0f) || (config->degrees_per_trigger > 90.0f)) {
    valid = false;
  }

  if (config->rpm_window_size > MAX_TRIGGERS) {
    valid = false;
  }

  return valid;
}

static unsigned int even_tooth_rpm_window_size(
  unsigned int current_triggers,
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
static void even_tooth_trigger_update_rpm(struct decoder *state) {
  timeval_t diff = state->times[0] - state->times[1];
  const struct decoder_config *conf = state->config;
  struct engine_position *out = &state->output;
  out->tooth_rpm = rpm_from_time_diff(diff, conf->degrees_per_trigger);

  unsigned int rpm_window_size = even_tooth_rpm_window_size(
    state->current_triggers_rpm, conf->rpm_window_size);

  if (rpm_window_size > 1) {
    /* We have at least two data points to draw an rpm from */
    out->rpm =
      rpm_from_time_diff(state->times[0] - state->times[rpm_window_size],
                         conf->degrees_per_trigger * rpm_window_size);

    if (out->rpm > 0) {
      unsigned int delta = (out->rpm > out->tooth_rpm)
                             ? out->rpm - out->tooth_rpm
                             : out->tooth_rpm - out->rpm;
      state->trigger_cur_rpm_change = delta / (float)out->rpm;
    }
  } else {
    out->rpm = 0;
  }

  /* Check for excessive per-tooth variation */
  if ((out->tooth_rpm <= conf->trigger_min_rpm) ||
      (out->tooth_rpm > out->rpm + (out->rpm * conf->trigger_max_rpm_change)) ||
      (out->tooth_rpm < out->rpm - (out->rpm * conf->trigger_max_rpm_change))) {
    state->state = DECODER_NOSYNC;
    state->loss = DECODER_VARIATION;
  }

  /* Check for too many triggers between syncs */
  if (state->triggers_since_last_sync > conf->num_triggers) {
    state->state = DECODER_NOSYNC;
    state->loss = DECODER_TRIGGERCOUNT_HIGH;
  }

  /* If we pass 150% of a inter-tooth delay, lose sync */
  out->valid_until = state->times[0] + (timeval_t)(diff * 1.5f);
}

static void even_tooth_trigger_update(struct decoder *state, timeval_t t) {
  /* Bookkeeping */
  push_time(state, t);
  state->triggers_since_last_sync++;

  /* Count triggers up until a full wheel */
  if (state->current_triggers_rpm < MAX_TRIGGERS + 1) {
    state->current_triggers_rpm++;
  }

  /* If we get enough triggers, we now have RPM */
  if ((state->state == DECODER_NOSYNC) &&
      (state->current_triggers_rpm >= state->config->required_triggers_rpm)) {
    state->state = DECODER_RPM;
  }

  if (state->state == DECODER_SYNC) {
    state->output.last_trigger_angle += state->config->degrees_per_trigger;
    if (state->output.last_trigger_angle >= 720) {
      state->output.last_trigger_angle -= 720;
    }
    state->output.time = t;
  }

  if (state->state == DECODER_RPM || state->state == DECODER_SYNC) {
    even_tooth_trigger_update_rpm(state);
  }
}

static void even_tooth_sync_update(struct decoder *state) {
  if (state->state == DECODER_RPM) {
    state->state = DECODER_SYNC;
    state->loss = DECODER_NO_LOSS;
    state->output.last_trigger_angle = 0;
  } else if (state->state == DECODER_SYNC) {
    if (state->triggers_since_last_sync == state->config->num_triggers) {
      state->state = DECODER_SYNC;
      state->loss = DECODER_NO_LOSS;
      state->output.last_trigger_angle = 0;
    } else {
      state->state = DECODER_NOSYNC;
      state->loss = DECODER_TRIGGERCOUNT_LOW;
    }
  }
  state->triggers_since_last_sync = 0;
}

static void decode_even_with_camsync(struct decoder *state,
                                     struct trigger_event *ev) {
  decoder_state oldstate = state->state;

  if (ev->type == TRIGGER) {
    even_tooth_trigger_update(state, ev->time);
  } else if (ev->type == SYNC) {
    even_tooth_sync_update(state);
  }

  if (state->state == DECODER_SYNC) {
    state->loss = DECODER_NO_LOSS;
    state->output.has_position = true;
  } else {
    if (oldstate == DECODER_SYNC) {
      /* We lost sync */
      invalidate_decoder(state);
    }
  }
}

static void decode_even_no_sync(struct decoder *state,
                                struct trigger_event *ev) {
  assert(ev->type == TRIGGER);

  decoder_state oldstate = state->state;
  even_tooth_trigger_update(state, ev->time);
  if (state->state == DECODER_RPM || state->state == DECODER_SYNC) {
    state->state = DECODER_SYNC;
    state->loss = DECODER_NO_LOSS;
    state->output.time = ev->time;
    state->output.has_position = true;
    state->triggers_since_last_sync = 0; /* There is no sync */
  } else {
    if (oldstate == DECODER_SYNC) {
      /* We lost sync */
      invalidate_decoder(state);
    }
  }
}

static uint32_t missing_tooth_rpm(struct decoder *state) {
  bool last_tooth_missing =
    (state->state == DECODER_SYNC) && (state->triggers_since_last_sync == 0);
  timeval_t last_tooth_diff = state->times[0] - state->times[1];
  degrees_t rpm_degrees =
    state->config->degrees_per_trigger * (last_tooth_missing ? 2 : 1);
  return rpm_from_time_diff(last_tooth_diff, rpm_degrees);
}

static void missing_tooth_trigger_update(struct decoder *state, timeval_t t) {
  push_time(state, t);

  const struct decoder_config *conf = state->config;
  /* Count triggers up until a full wheel */
  if (state->current_triggers_rpm < MAX_TRIGGERS) {
    state->current_triggers_rpm++;
  }

  if (state->current_triggers_rpm < conf->required_triggers_rpm) {
    return;
  }

  /* If we get enough triggers, we now have RPM */
  if (state->state == DECODER_NOSYNC) {
    state->state = DECODER_RPM;
  }

  timeval_t last_tooth_diff = state->times[0] - state->times[1];

  /* Calculate the last N average. If we are synced, and the missing tooth was
   * in the last N teeth, don't forget to include it */
  timeval_t rpm_window_tooth_diff =
    state->times[1] - state->times[conf->required_triggers_rpm];
  uint32_t rpm_window_tooth_count = conf->required_triggers_rpm - 1;
  if ((state->state == DECODER_SYNC) &&
      (state->triggers_since_last_sync < rpm_window_tooth_count)) {
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
  float max_variance = conf->trigger_max_rpm_change;
  bool is_acceptable_missing_tooth = (tooth_ratio >= 2.0f - max_variance) &&
                                     (tooth_ratio <= 2.0f + max_variance);
  bool is_acceptable_normal_tooth = (tooth_ratio >= 1.0f - max_variance) &&
                                    (tooth_ratio <= 1.0f + max_variance);

  /* Preserve meaning from old even tooth code */
  state->trigger_cur_rpm_change =
    tooth_ratio < 1.0f ? 1.0f - tooth_ratio : tooth_ratio - 1.0f;

  if (state->state == DECODER_RPM) {
    if (is_acceptable_missing_tooth) {
      state->state = DECODER_SYNC;
      state->triggers_since_last_sync = 0;
      state->output.last_trigger_angle = 0;
    } else if (!is_acceptable_normal_tooth) {
      state->state = DECODER_NOSYNC;
      state->loss = DECODER_VARIATION;
    }
  } else if (state->state == DECODER_SYNC) {
    state->triggers_since_last_sync += 1;
    /* Compare against num_triggers - 1, since we're missing a tooth */
    if (state->triggers_since_last_sync == conf->num_triggers - 1) {
      if (is_acceptable_missing_tooth) {
        state->triggers_since_last_sync = 0;
      } else {
        state->state = DECODER_NOSYNC;
        state->loss = DECODER_TRIGGERCOUNT_HIGH;
      }
    } else if (!is_acceptable_normal_tooth) {
      state->state = DECODER_NOSYNC;
      state->loss = DECODER_TRIGGERCOUNT_LOW;
    }

    degrees_t rpm_degrees =
      conf->degrees_per_trigger * (is_acceptable_missing_tooth ? 2 : 1);
    state->output.time = t;
    state->output.last_trigger_angle += rpm_degrees;
    if (state->output.last_trigger_angle >= 720) {
      state->output.last_trigger_angle -= 720;
    }

    degrees_t expected_gap =
      conf->degrees_per_trigger *
      ((state->triggers_since_last_sync == conf->num_triggers - 2) ? 2 : 1);
    timeval_t expected_time =
      time_from_rpm_diff(state->output.rpm, expected_gap);
    state->output.valid_until =
      state->times[0] +
      (timeval_t)(expected_time * (1.0f + conf->trigger_max_rpm_change));
  }
}

static uint32_t missing_tooth_average_rpm(const struct decoder *state) {
  const struct decoder_config *conf = state->config;
  if (state->current_triggers_rpm >= conf->num_triggers) {
    if (state->triggers_since_last_sync == 0) {
      return rpm_from_time_diff(state->times[0] -
                                  state->times[conf->num_triggers - 1],
                                conf->num_triggers * conf->degrees_per_trigger);
    } else {
      return state->output.rpm;
    }
  }
  return state->output.tooth_rpm;
}

static void decode_missing_no_sync(struct decoder *state,
                                   struct trigger_event *ev) {

  decoder_state oldstate = state->state;

  if (ev->type == TRIGGER) {
    missing_tooth_trigger_update(state, ev->time);
  }

  if (state->state != DECODER_NOSYNC) {
    state->output.tooth_rpm = missing_tooth_rpm(state);
  }

  state->output.rpm = missing_tooth_average_rpm(state);

  if (state->state == DECODER_SYNC) {
    state->output.has_position = true;
    state->loss = DECODER_NO_LOSS;
  } else {
    if (oldstate == DECODER_SYNC) {
      /* We lost sync */
      invalidate_decoder(state);
    }
  }
}

static void decode_missing_with_camsync(struct decoder *state,
                                        struct trigger_event *ev) {
  bool was_valid = state->output.has_position;
  static bool camsync_seen_this_rotation = false;
  static bool camsync_seen_last_rotation = false;

  if (ev->type == TRIGGER) {
    missing_tooth_trigger_update(state, ev->time);
    if ((state->state == DECODER_SYNC) &&
        (state->triggers_since_last_sync == 0)) {
      if (!camsync_seen_this_rotation && !camsync_seen_last_rotation) {
        /* We've gone two cycles without a camsync, desync */
        state->output.has_position = false;
      }
      camsync_seen_last_rotation = camsync_seen_this_rotation;
      camsync_seen_this_rotation = false;
    }
  } else if (ev->type == SYNC) {
    if (state->state == DECODER_SYNC) {
      if (camsync_seen_this_rotation || camsync_seen_last_rotation) {
        state->output.has_position = false;
        camsync_seen_this_rotation = false;
      } else {
        camsync_seen_this_rotation = true;
      }
    }
  }

  if (state->state != DECODER_NOSYNC) {
    state->output.tooth_rpm = missing_tooth_rpm(state);
  }

  state->output.rpm = missing_tooth_average_rpm(state);

  bool has_seen_camsync =
    camsync_seen_this_rotation || camsync_seen_last_rotation;
  if (state->state != DECODER_SYNC) {
    state->output.has_position = false;
  } else if (!was_valid && (state->state == DECODER_SYNC) && has_seen_camsync) {
    /* We gained sync */
    state->output.has_position = true;
    state->loss = DECODER_NO_LOSS;
    state->output.last_trigger_angle =
      state->config->degrees_per_trigger * state->triggers_since_last_sync +
      (camsync_seen_this_rotation ? 0 : 360);
  }
  if (was_valid && !state->output.has_position) {
    /* We lost sync */
    invalidate_decoder(state);
  }
}

void decoder_init(const struct decoder_config *conf, struct decoder *state) {
  state->config = conf;
  state->current_triggers_rpm = 0;
  state->output = (struct engine_position){
    .has_position = false,
    .valid_until = 0,
    .rpm = 0,
    .tooth_rpm = 0,
    .time = 0,
    .last_trigger_angle = 0,
  };
  state->state = DECODER_NOSYNC;
  state->triggers_since_last_sync = 0;
  state->t0_count = 0;
  state->t1_count = 0;
}

/* When decoder has new information, reschedule everything */
void decoder_update(struct decoder *state, struct trigger_event *ev) {

  console_record_event((struct logged_event){
    .type = EVENT_TRIGGER,
    .value = ev->type == TRIGGER ? 0 : 1,
    .time = ev->time,
  });

  if (ev->type == TRIGGER) {
    state->t0_count++;
  } else if (ev->type == SYNC) {
    state->t1_count++;
  }

  if (!decoder_config_is_valid(state->config)) {
    decoder_desync(state, DECODER_BADCONFIG);
    return;
  }

  switch (state->config->type) {
  case TRIGGER_EVEN_NOSYNC:
    decode_even_no_sync(state, ev);
    break;
  case TRIGGER_EVEN_CAMSYNC:
    decode_even_with_camsync(state, ev);
    break;
  case TRIGGER_MISSING_NOSYNC:
    decode_missing_no_sync(state, ev);
    break;
  case TRIGGER_MISSING_CAMSYNC:
    decode_missing_with_camsync(state, ev);
    break;
  default:
    break;
  }
}

struct engine_position decoder_get_engine_position(
  const struct decoder *state) {
  struct engine_position result = state->output;
  result.has_rpm =
    (state->state == DECODER_RPM) || (state->state == DECODER_SYNC);
  return result;
}

bool engine_position_is_synced(const struct engine_position *d,
                               timeval_t at_time) {
  return d->has_position && time_before(at_time, d->valid_until + 1600);
}

degrees_t engine_current_angle(const struct engine_position *d,
                               timeval_t at_time) {
  if (!d->has_position || !d->has_rpm) {
    return d->last_trigger_angle;
  }

  timeval_t last_time = d->time;
  degrees_t last_angle = d->last_trigger_angle;

  degrees_t angle_since_last_tooth =
    degrees_from_time_diff(at_time - last_time, d->rpm);

  if ((angle_since_last_tooth < 0) || (angle_since_last_tooth > 720)) {
    /* This should never happen unless something is wrong with the input time,
     * but let's protect the inputs to clamp_angle regardless */
    angle_since_last_tooth = last_angle;
  }

  return clamp_angle(last_angle + angle_since_last_tooth, 720);
}

#ifdef UNITTEST
#include "config.h"
#include "decoder.h"
#include "platform.h"

#include <check.h>

struct decoder_event {
  unsigned int trigger;
  timeval_t time;
  decoder_state state;
  bool valid;
  decoder_loss_reason reason;
  struct decoder_event *next;
};

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

static void add_trigger_event_transition(struct decoder_event **entries,
                                         timeval_t duration,
                                         unsigned int trigger,
                                         bool validity,
                                         decoder_state state,
                                         decoder_loss_reason reason) {
  ck_assert(*entries);
  struct decoder_event *last = find_last_trigger_event(entries);
  struct decoder_event new = (struct decoder_event){
    .trigger = trigger,
    .time = last->time + duration,
    .state = state,
    .valid = validity,
    .reason = reason,
  };
  append_trigger_event(last, new);
}

static void validate_decoder_sequence(struct decoder *state,
                                      struct decoder_event *ev) {
  for (; ev; ev = ev->next) {
    struct trigger_event e = { .time = ev->time,
                               .type = ev->trigger == 0 ? TRIGGER : SYNC };
    decoder_update(state, &e);
    ck_assert_msg(
      (state->state == ev->state) && (state->output.has_position == ev->valid),
      "expected event at %d: (state=%d, valid=%d). Got (state=%d, valid=%d)",
      ev->time,
      ev->state,
      (int)ev->valid,
      state->state,
      state->output.has_position);

    if (!ev->valid) {
      ck_assert_msg(state->loss == ev->reason,
                    "reason mismatch at %d: %d. Got %d",
                    ev->time,
                    ev->reason,
                    state->loss);
    }
  }
}

static void prepare_ford_tfi_decoder(struct decoder *dstate) {
  static const struct decoder_config dconfig = {
    .type = TRIGGER_EVEN_NOSYNC,
    .required_triggers_rpm = 4,
    .degrees_per_trigger = 90,
    .rpm_window_size = 3,
    .num_triggers = 8,
  };
  decoder_init(&dconfig, dstate);
}

static void prepare_7mgte_cps_decoder(struct decoder *dstate) {
  static const struct decoder_config dconfig = {
    .type = TRIGGER_EVEN_CAMSYNC,
    .required_triggers_rpm = 8,
    .degrees_per_trigger = 30,
    .rpm_window_size = 8,
    .num_triggers = 24,
  };
  decoder_init(&dconfig, dstate);
}

static void prepare_36minus1_cam_wheel_decoder(struct decoder *dstate) {
  static const struct decoder_config dconfig = {
    .type = TRIGGER_MISSING_NOSYNC,
    .required_triggers_rpm = 4,
    .degrees_per_trigger = 20,
    .num_triggers = 36,
  };
  decoder_init(&dconfig, dstate);
}

static void prepare_36minus1_crank_wheel_plus_cam_decoder(
  struct decoder *dstate) {
  static const struct decoder_config dconfig = {
    .type = TRIGGER_MISSING_CAMSYNC,
    .required_triggers_rpm = 4,
    .degrees_per_trigger = 10,
    .num_triggers = 36,
  };
  decoder_init(&dconfig, dstate);
}

START_TEST(check_tfi_decoder_startup_normal) {
  struct decoder_event *entries = NULL;
  struct decoder dstate;
  prepare_ford_tfi_decoder(&dstate);

  /* Triggers to get RPM */
  for (unsigned int i = 0; i < dstate.config->required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }
  add_trigger_event_transition(
    &entries, 25000, 0, 1, DECODER_SYNC, DECODER_NO_LOSS);
  add_trigger_event(&entries, 25000, 0);

  validate_decoder_sequence(&dstate, entries);

  struct engine_position out = decoder_get_engine_position(&dstate);
  ck_assert_int_eq(out.last_trigger_angle, 90);
  ck_assert(out.rpm != 0);
  ck_assert(out.tooth_rpm != 0);

  free_trigger_list(entries);
}
END_TEST

START_TEST(check_tfi_decoder_syncloss_variation) {
  struct decoder_event *entries = NULL;
  struct decoder dstate;
  prepare_ford_tfi_decoder(&dstate);

  /* Triggers to get RPM */
  for (unsigned int i = 0; i < dstate.config->required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }
  add_trigger_event_transition(
    &entries, 25000, 0, 1, DECODER_SYNC, DECODER_NO_LOSS);
  add_trigger_event(&entries, 25000, 0);
  /* Trigger far too soon */
  add_trigger_event_transition(
    &entries, 10000, 0, 0, DECODER_NOSYNC, DECODER_VARIATION);

  validate_decoder_sequence(&dstate, entries);

  ck_assert_int_eq(dstate.current_triggers_rpm, 0);

  free_trigger_list(entries);
}
END_TEST

START_TEST(check_tfi_decoder_syncloss_expire) {
  struct decoder_event *entries = NULL;
  struct decoder dstate;
  prepare_ford_tfi_decoder(&dstate);

  /* Triggers to get RPM */
  for (unsigned int i = 0; i < dstate.config->required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }
  add_trigger_event_transition(
    &entries, 25000, 0, 1, DECODER_SYNC, DECODER_NO_LOSS);
  add_trigger_event(&entries, 25000, 0);

  validate_decoder_sequence(&dstate, entries);

  /* Currently expiration is not dependent on variation setting, fixed 1.5x
   * trigger time */
  timeval_t expected_expiration =
    find_last_trigger_event(&entries)->time + (1.5 * 25000);
  ck_assert_int_eq(dstate.output.valid_until, expected_expiration);
  free_trigger_list(entries);
}
END_TEST

/* Gets decoder up to sync, plus an additional trigger */
static void cam_nplusone_normal_startup_to_sync(
  const struct decoder_config *config,
  struct decoder_event **entries) {

  /* Triggers to get RPM */
  for (unsigned int i = 0; i < config->required_triggers_rpm - 1; ++i) {
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
  struct decoder_event *entries = NULL;
  struct decoder dstate;
  prepare_7mgte_cps_decoder(&dstate);

  cam_nplusone_normal_startup_to_sync(dstate.config, &entries);

  /* Two additional triggers, three total after sync pulse */
  add_trigger_event(&entries, 25000, 0);
  add_trigger_event(&entries, 25000, 0);

  validate_decoder_sequence(&dstate, entries);

  struct engine_position pos = decoder_get_engine_position(&dstate);
  ck_assert_int_eq(pos.last_trigger_angle,
                   3 * dstate.config->degrees_per_trigger);

  ck_assert(pos.rpm != 0);
  ck_assert(pos.tooth_rpm != 0);

  free_trigger_list(entries);
}
END_TEST

START_TEST(check_cam_nplusone_startup_normal_then_early_sync) {
  struct decoder_event *entries = NULL;
  struct decoder dstate;
  prepare_7mgte_cps_decoder(&dstate);

  cam_nplusone_normal_startup_to_sync(dstate.config, &entries);

  /* Two additional triggers, three total after sync pulse */
  add_trigger_event(&entries, 25000, 0);
  add_trigger_event(&entries, 25000, 0);

  /* Spurious early sync */
  add_trigger_event_transition(
    &entries, 500, 1, 0, DECODER_NOSYNC, DECODER_TRIGGERCOUNT_LOW);

  validate_decoder_sequence(&dstate, entries);

  ck_assert(!dstate.output.has_position);

  free_trigger_list(entries);
}
END_TEST

START_TEST(check_cam_nplusone_startup_normal_sustained) {
  struct decoder_event *entries = NULL;
  struct decoder dstate;
  prepare_7mgte_cps_decoder(&dstate);

  cam_nplusone_normal_startup_to_sync(dstate.config, &entries);

  /* continued wheel of additional triggers */
  for (unsigned int i = 0; i < dstate.config->num_triggers - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }
  /* Plus another sync and trigger */
  add_trigger_event(&entries, 500, 1);
  add_trigger_event(&entries, 24500, 0);

  validate_decoder_sequence(&dstate, entries);

  ck_assert_int_eq(dstate.output.last_trigger_angle,
                   1 * dstate.config->degrees_per_trigger);

  free_trigger_list(entries);
}
END_TEST

START_TEST(check_cam_nplusone_startup_normal_no_second_trigger) {
  struct decoder_event *entries = NULL;
  struct decoder dstate;
  prepare_7mgte_cps_decoder(&dstate);

  cam_nplusone_normal_startup_to_sync(dstate.config, &entries);

  /* continued wheel of additional triggers */
  for (unsigned int i = 0; i < dstate.config->num_triggers - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }
  /* Another trigger, no sync when there should be one */
  add_trigger_event_transition(
    &entries, 25500, 0, 0, DECODER_NOSYNC, DECODER_TRIGGERCOUNT_HIGH);

  validate_decoder_sequence(&dstate, entries);
  ck_assert(!dstate.output.has_position);
  free_trigger_list(entries);
}
END_TEST

START_TEST(check_nplusone_decoder_syncloss_expire) {
  struct decoder_event *entries = NULL;
  struct decoder dstate;
  prepare_7mgte_cps_decoder(&dstate);

  cam_nplusone_normal_startup_to_sync(dstate.config, &entries);

  /* Three extra triggers */
  add_trigger_event(&entries, 25000, 0);
  add_trigger_event(&entries, 25000, 0);
  add_trigger_event(&entries, 25000, 0);

  validate_decoder_sequence(&dstate, entries);

  /* Currently expiration is not dependent on variation setting, fixed 1.5x
   * trigger time */
  timeval_t expected_expiration =
    find_last_trigger_event(&entries)->time + (1.5 * 25000);
  ck_assert_int_eq(dstate.output.valid_until, expected_expiration);
}
END_TEST

START_TEST(check_missing_tooth_wrong_wheel) {
  struct decoder_event *entries = NULL;
  struct decoder dstate;
  prepare_36minus1_cam_wheel_decoder(&dstate);

  for (unsigned int i = 0; i < dstate.config->required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };
  add_trigger_event_transition(
    &entries, 25000, 0, 0, DECODER_RPM, DECODER_NO_LOSS);

  /* Create logs of triggers, we should never get sync */
  for (unsigned int i = 0; i < dstate.config->num_triggers * 4; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }
  validate_decoder_sequence(&dstate, entries);

  ck_assert(!dstate.output.has_position);
  free_trigger_list(entries);
}
END_TEST

START_TEST(check_missing_tooth_startup_normal) {
  struct decoder_event *entries = NULL;
  struct decoder dstate;
  prepare_36minus1_cam_wheel_decoder(&dstate);

  for (unsigned int i = 0; i < dstate.config->required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };
  add_trigger_event_transition(
    &entries, 25000, 0, 0, DECODER_RPM, DECODER_NO_LOSS);

  for (unsigned int i = 0; i < dstate.config->required_triggers_rpm; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }

  /* Missing tooth */
  add_trigger_event_transition(
    &entries, 50000, 0, 1, DECODER_SYNC, DECODER_NO_LOSS);

  add_trigger_event(&entries, 25000, 0);
  validate_decoder_sequence(&dstate, entries);

  struct engine_position pos = decoder_get_engine_position(&dstate);

  ck_assert(pos.has_position);
  ck_assert_int_eq(pos.last_trigger_angle, dstate.config->degrees_per_trigger);
  ck_assert(pos.rpm != 0);
  ck_assert(pos.tooth_rpm != 0);
  free_trigger_list(entries);
}
END_TEST

START_TEST(check_missing_tooth_rpm_after_gap) {
  struct decoder_event *entries = NULL;
  struct decoder dstate;
  prepare_36minus1_cam_wheel_decoder(&dstate);

  for (unsigned int i = 0; i < dstate.config->required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };
  add_trigger_event_transition(
    &entries, 25000, 0, 0, DECODER_RPM, DECODER_NO_LOSS);

  for (unsigned int i = 0; i < dstate.config->required_triggers_rpm; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }

  /* Missing tooth */
  add_trigger_event_transition(
    &entries, 50000, 0, 1, DECODER_SYNC, DECODER_NO_LOSS);
  validate_decoder_sequence(&dstate, entries);

  struct engine_position pos = decoder_get_engine_position(&dstate);

  ck_assert(pos.has_position);
  ck_assert_int_eq(pos.last_trigger_angle, 0);
  ck_assert(dstate.state == DECODER_SYNC);
  ck_assert_int_eq(
    pos.rpm, rpm_from_time_diff(25000, dstate.config->degrees_per_trigger));
  free_trigger_list(entries);
}
END_TEST

START_TEST(check_missing_tooth_startup_then_early) {
  struct decoder_event *entries = NULL;
  struct decoder dstate;
  prepare_36minus1_cam_wheel_decoder(&dstate);

  for (unsigned int i = 0; i < dstate.config->required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };
  add_trigger_event_transition(
    &entries, 25000, 0, 0, DECODER_RPM, DECODER_NO_LOSS);

  for (unsigned int i = 0; i < dstate.config->required_triggers_rpm; ++i) {
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

  validate_decoder_sequence(&dstate, entries);

  ck_assert(!dstate.output.has_position);
  free_trigger_list(entries);
}
END_TEST

START_TEST(check_missing_tooth_startup_then_late) {
  struct decoder_event *entries = NULL;
  struct decoder dstate;
  prepare_36minus1_cam_wheel_decoder(&dstate);

  for (unsigned int i = 0; i < dstate.config->required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };
  add_trigger_event_transition(
    &entries, 25000, 0, 0, DECODER_RPM, DECODER_NO_LOSS);

  for (unsigned int i = 0; i < dstate.config->required_triggers_rpm; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }

  /* Missing tooth */
  add_trigger_event_transition(
    &entries, 50000, 0, 1, DECODER_SYNC, DECODER_NO_LOSS);

  for (unsigned int i = 0; i < dstate.config->num_triggers - 2; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }

  /* Add a normal tooth when we should have a missing tooth */
  add_trigger_event_transition(
    &entries, 25000, 0, 0, DECODER_NOSYNC, DECODER_TRIGGERCOUNT_HIGH);

  /* Now a late missing tooth, we shouldn't immediately regain sync */
  add_trigger_event(&entries, 50000, 0);

  validate_decoder_sequence(&dstate, entries);

  ck_assert(!dstate.output.has_position);
  free_trigger_list(entries);
}
END_TEST

START_TEST(check_missing_tooth_false_starts) {
  struct decoder_event *entries = NULL;
  struct decoder dstate;
  prepare_36minus1_cam_wheel_decoder(&dstate);

  add_trigger_event(&entries, 25000, 0);
  add_trigger_event(&entries, 50000, 0);
  add_trigger_event(&entries, 25000, 0);
  add_trigger_event_transition(
    &entries, 50000, 0, 0, DECODER_NOSYNC, DECODER_VARIATION);
  add_trigger_event(&entries, 25000, 0);
  add_trigger_event(&entries, 50000, 0);

  validate_decoder_sequence(&dstate, entries);

  ck_assert(!dstate.output.has_position);
  free_trigger_list(entries);
}
END_TEST

START_TEST(check_missing_tooth_camsync_startup_no_cam_long_time) {
  struct decoder_event *entries = NULL;
  struct decoder dstate;
  prepare_36minus1_crank_wheel_plus_cam_decoder(&dstate);

  for (unsigned int i = 0; i < dstate.config->required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };
  add_trigger_event_transition(
    &entries, 25000, 0, 0, DECODER_RPM, DECODER_NO_LOSS);

  for (unsigned int i = 0; i < dstate.config->required_triggers_rpm; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }

  /* Missing tooth, remaining invalid */
  add_trigger_event_transition(
    &entries, 50000, 0, 0, DECODER_SYNC, DECODER_NO_LOSS);

  for (unsigned int i = 0; i < dstate.config->num_triggers - 2; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }

  add_trigger_event(&entries, 50000, 0);

  validate_decoder_sequence(&dstate, entries);
  free_trigger_list(entries);
}
END_TEST

START_TEST(check_missing_tooth_camsync_startup_normal_cam_first_rev) {
  struct decoder_event *entries = NULL;
  struct decoder dstate;
  prepare_36minus1_crank_wheel_plus_cam_decoder(&dstate);

  for (unsigned int i = 0; i < dstate.config->required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };
  add_trigger_event_transition(
    &entries, 25000, 0, 0, DECODER_RPM, DECODER_NO_LOSS);

  for (unsigned int i = 0; i < dstate.config->required_triggers_rpm; ++i) {
    add_trigger_event(&entries, 25000, 0);
  }

  /* Missing tooth, but remainings not valid due to lack of sync pulse */
  add_trigger_event_transition(
    &entries, 50000, 0, 0, DECODER_SYNC, DECODER_NO_LOSS);
  add_trigger_event(&entries, 25000, 0);

  /* Sync tooth indicating 0-360* (first) revolution, becomes valid */
  add_trigger_event_transition(
    &entries, 5000, 1, 1, DECODER_SYNC, DECODER_NO_LOSS);

  validate_decoder_sequence(&dstate, entries);

  ck_assert_int_eq(dstate.output.last_trigger_angle,
                   dstate.config->degrees_per_trigger);
  free_trigger_list(entries);
}
END_TEST

START_TEST(check_missing_tooth_camsync_startup_normal_cam_second_rev) {
  struct decoder_event *entries = NULL;
  struct decoder dstate;
  prepare_36minus1_crank_wheel_plus_cam_decoder(&dstate);

  for (unsigned int i = 0; i < dstate.config->required_triggers_rpm - 1; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };
  add_trigger_event_transition(
    &entries, 25000, 0, 0, DECODER_RPM, DECODER_NO_LOSS);

  /* Missing tooth */
  add_trigger_event_transition(
    &entries, 50000, 0, 0, DECODER_SYNC, DECODER_NO_LOSS);

  for (unsigned int i = 0; i < dstate.config->num_triggers - 2; ++i) {
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
  add_trigger_event(&entries, 20000, 0);
  add_trigger_event(&entries, 25000, 0);
  add_trigger_event(&entries, 25000, 0);

  validate_decoder_sequence(&dstate, entries);

  ck_assert_int_eq(dstate.output.last_trigger_angle,
                   dstate.config->degrees_per_trigger * 6);
  free_trigger_list(entries);
}
END_TEST

START_TEST(check_missing_tooth_camsync_startup_normal_cam_two_revs_in_row) {
  struct decoder_event *entries = NULL;
  struct decoder dstate;
  prepare_36minus1_crank_wheel_plus_cam_decoder(&dstate);

  for (unsigned int i = 0; i < dstate.config->required_triggers_rpm - 1; ++i) {
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

  add_trigger_event(&entries, 20000, 0);
  for (unsigned int i = 0; i < dstate.config->num_triggers - 6; ++i) {
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
    &entries, 50000, 1, 0, DECODER_NOSYNC, DECODER_NO_LOSS);

  validate_decoder_sequence(&dstate, entries);

  free_trigger_list(entries);
}
END_TEST

START_TEST(check_missing_tooth_camsync_startup_normal_cam_stops_working) {
  struct decoder_event *entries = NULL;
  struct decoder dstate;
  prepare_36minus1_crank_wheel_plus_cam_decoder(&dstate);

  for (unsigned int i = 0; i < dstate.config->required_triggers_rpm - 1; ++i) {
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

  add_trigger_event(&entries, 20000, 0);
  for (unsigned int i = 0; i < dstate.config->num_triggers - 6; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };

  /* Missing tooth */
  add_trigger_event(&entries, 50000, 0);

  for (unsigned int i = 0; i < dstate.config->num_triggers - 2; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };

  /* Missing tooth */
  add_trigger_event(&entries, 50000, 0);

  for (unsigned int i = 0; i < dstate.config->num_triggers - 2; ++i) {
    add_trigger_event(&entries, 25000, 0);
  };

  /* Missing tooth: now we've not seen a cam sync in two cycles, lose sync */
  add_trigger_event_transition(
    &entries, 50000, 0, 0, DECODER_NOSYNC, DECODER_NO_LOSS);

  validate_decoder_sequence(&dstate, entries);

  free_trigger_list(entries);
}
END_TEST

START_TEST(check_update_rpm_single_point) {
  struct decoder_config conf = {
    .degrees_per_trigger = 30,
    .rpm_window_size = 4,
    .trigger_max_rpm_change = 0.5,
  };
  struct decoder d = {
    .config = &conf,
    .current_triggers_rpm = 1,
    .times = { 100 },
  };

  even_tooth_trigger_update_rpm(&d);
  ck_assert_int_eq(d.output.rpm, 0);
  ck_assert_int_eq(d.state, DECODER_NOSYNC);
}
END_TEST

START_TEST(check_update_rpm_sufficient_points) {
  struct decoder_config config = {
    .degrees_per_trigger = 30,
    .rpm_window_size = 4,
    .num_triggers = 24,
    .trigger_max_rpm_change = 0.5,
  };
  struct decoder d = {
    .config = &config,
    .times = { 400, 300, 200, 100 },
    .current_triggers_rpm = 4,
  };

  even_tooth_trigger_update_rpm(&d);
  ck_assert_int_eq(d.output.rpm,
                   rpm_from_time_diff(100, config.degrees_per_trigger));
}
END_TEST

START_TEST(check_update_rpm_window_larger) {
  struct decoder_config config = {
    .degrees_per_trigger = 30,
    .rpm_window_size = 8,
    .num_triggers = 24,
    .trigger_max_rpm_change = 0.5,
  };
  struct decoder d = {
    .config = &config,
    .times = { 800, 700, 700, 500, 400, 300, 200, 100 },
    .current_triggers_rpm = 4,
  };

  even_tooth_trigger_update_rpm(&d);
  ck_assert_int_eq(d.output.rpm,
                   rpm_from_time_diff(100, config.degrees_per_trigger));
}
END_TEST

START_TEST(check_update_rpm_window_smaller) {
  struct decoder_config config = {
    .degrees_per_trigger = 30,
    .rpm_window_size = 4,
    .num_triggers = 24,
    .trigger_max_rpm_change = 0.5,
  };
  struct decoder d = {
    .config = &config,
    .times = { 800, 700, 700, 500, 400, 300, 200, 100 },
    .current_triggers_rpm = 8,
  };

  even_tooth_trigger_update_rpm(&d);
  ck_assert_int_eq(d.output.rpm,
                   rpm_from_time_diff(100, config.degrees_per_trigger));
}
END_TEST

START_TEST(check_missing_tooth_average_rpm) {
  struct decoder_config config = {
    .degrees_per_trigger = 45,
    .num_triggers = 8,
    .trigger_max_rpm_change = 0.5,
  };
  struct decoder d = {
    /* Missing tooth should result in full revolution being 9500 - 2000 */
    .config = &config,
    .times = { 9500, 8400, 7300, 6200, 5100, 4000, 3000, 2000, 1000 },
    .current_triggers_rpm = 8,
    .triggers_since_last_sync = 0,
  };

  uint32_t rpm = missing_tooth_average_rpm(&d);
  ck_assert_int_eq(rpm, 32000);
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

  tcase_add_test(decoder_tests, check_update_rpm_single_point);
  tcase_add_test(decoder_tests, check_update_rpm_sufficient_points);
  tcase_add_test(decoder_tests, check_update_rpm_window_larger);
  tcase_add_test(decoder_tests, check_update_rpm_window_smaller);
  tcase_add_test(decoder_tests, check_missing_tooth_average_rpm);
  return decoder_tests;
}
#endif
