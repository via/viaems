#include "decoder.h"
#include "platform.h"
#include "util.h"
#include "scheduler.h"
#include "config.h"
#include "stats.h"

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

  if (!current_triggers) {
    return 0;
  }

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
  unsigned int rpm_window_size = current_rpm_window_size(d->current_triggers_rpm,
      d->rpm_window_size);

  if (rpm_window_size > 1) {
    /* We have at least two data points to draw an rpm from */
    d->rpm = rpm_from_time_diff(d->times[0] - d->times[rpm_window_size - 1], 
      d->degrees_per_trigger * (rpm_window_size - 1));
    if (d->rpm) {
      d->trigger_cur_rpm_change = abs(d->rpm - slicerpm) / (float)d->rpm;
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
  d->expiration = d->times[0] + diff * 1.5;
  set_expire_event(d->expiration);
}


static void trigger_update(struct decoder *d, timeval_t t) {
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
  }

  if (d->state == DECODER_RPM || d->state == DECODER_SYNC) {
    trigger_update_rpm(d);
  }

}

static void sync_update(struct decoder *d) {
  d->t1_count++;
  if (d->state == DECODER_RPM) {
    d->state = DECODER_SYNC;
    d->last_trigger_angle = 0;
  } else if (d->state == DECODER_SYNC) {
    if (d->triggers_since_last_sync == d->num_triggers) {
      d->state = DECODER_SYNC;
      d->last_trigger_angle = 0;
    } else {
      d->state = DECODER_NOSYNC;
      d->loss = DECODER_TRIGGERCOUNT_LOW;
    }
  }
  d->triggers_since_last_sync = 0;
}

void cam_nplusone_decoder(struct decoder *d) {
  timeval_t t0 = d->last_t0;
  decoder_state oldstate = d->state;

  if (d->needs_decoding_t0) {
    trigger_update(d, t0);
    d->needs_decoding_t0 = 0;
  } 
  if (d->needs_decoding_t1) {
    sync_update(d);
    d->needs_decoding_t1 = 0;
  }

  if (d->state == DECODER_SYNC) {
    d->valid = 1;
    d->last_trigger_time = t0;
  } else {
    if (oldstate == DECODER_SYNC) {
      /* We lost sync */
      invalidate_decoder();
    }
  }
}

void tfi_pip_decoder(struct decoder *d) {
  stats_start_timing(STATS_DECODE_TIME);
  timeval_t t0;
  decoder_state oldstate = d->state;

  t0 = d->last_t0;
  d->needs_decoding_t0 = 0;

  trigger_update(d, t0);
  if (d->state == DECODER_RPM || d->state == DECODER_SYNC) {
    d->state = DECODER_SYNC;
    d->valid = 1;
    d->last_trigger_time = t0;
    d->triggers_since_last_sync = 0; /* There is no sync */;
  } else {
    if (oldstate == DECODER_SYNC) {
      /* We lost sync */
      invalidate_decoder();
    }
  }
  stats_finish_timing(STATS_DECODE_TIME);
}

void decoder_init(struct decoder *d) {
  d->last_t0 = 0;
  d->last_t1 = 0;
  d->needs_decoding_t0 = 0;
  d->needs_decoding_t1 = 0;
  switch (d->type) {
    case FORD_TFI:
      d->decode = tfi_pip_decoder;
      d->required_triggers_rpm = 4;
      d->degrees_per_trigger = 90;
      d->rpm_window_size = 3;
      d->num_triggers = 8;
      d->t0_edge = FALLING_EDGE;
      break;
    case TOYOTA_24_1_CAS:
      d->decode = cam_nplusone_decoder;
      d->required_triggers_rpm = 8;
      d->degrees_per_trigger = 30;
      d->rpm_window_size = 8;
      d->num_triggers = 24;
      d->t0_edge = RISING_EDGE;
      d->t1_edge = RISING_EDGE;
      break;
    default:
      break;
  }
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

/* When decoder has new information, reschedule everything */
void decoder_update_scheduling(int trigger, timeval_t time) {
  stats_start_timing(STATS_SCHEDULE_LATENCY);

  switch (trigger) {
  case 0:
    config.decoder.last_t0 = time;
    config.decoder.needs_decoding_t0 = 1;
    break;
  case 1:
    config.decoder.last_t1 = time;
    config.decoder.needs_decoding_t1 = 1;
    break;
  }
  console_record_event((struct logged_event){
    .type = trigger == 0 ? EVENT_TRIGGER0 : EVENT_TRIGGER1,
    .time = time,
  });
  config.decoder.decode(&config.decoder);

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

#ifdef UNITTEST
#include "platform.h"
#include "config.h"
#include "decoder.h"

#include <check.h>

struct decoder_event {
  unsigned int t0 : 1;
  unsigned int t1 : 1;
  timeval_t time;
  decoder_state state;
  int valid;
  decoder_loss_reason reason;
  struct decoder_event *next;
}; 

static struct decoder_event *find_last_trigger_event(struct decoder_event **entries) {
  struct decoder_event *entry;
  for (entry = *entries; entry->next; entry = entry->next);
  return entry;
}

static void append_trigger_event(struct decoder_event *last, struct decoder_event new) {
  last->next = malloc(sizeof(struct decoder_event));
  *(last->next) = new;
}

static void add_trigger_event(struct decoder_event **entries, timeval_t duration, unsigned int primary, unsigned int secondary) {

  if (!*entries) {
    *entries = malloc(sizeof(struct decoder_event));
    **entries = (struct decoder_event){
      .t0 = primary, .t1 = secondary,
      .time = duration, .state = DECODER_NOSYNC,
    };
    return;
  }

  struct decoder_event *last = find_last_trigger_event(entries);
  struct decoder_event new = (struct decoder_event){
    .t0 = primary, .t1 = secondary,
      .time = last->time + duration, .state = last->state,
      .valid = last->valid, .reason = last->reason,
  };
  append_trigger_event(last, new);
}

static void add_trigger_event_transition_rpm(struct decoder_event **entries, timeval_t duration, unsigned int primary, unsigned int secondary) {
  ck_assert(*entries);

  struct decoder_event *last = find_last_trigger_event(entries);
  struct decoder_event new = (struct decoder_event){
    .t0 = primary, .t1 = secondary,
      .time = last->time + duration, .state = DECODER_RPM,
      .valid = last->valid, .reason = last->reason,
  };
  append_trigger_event(last, new);

}

static void add_trigger_event_transition_sync(struct decoder_event **entries, timeval_t duration, unsigned int primary, unsigned int secondary) {
  ck_assert(*entries);

  struct decoder_event *last = find_last_trigger_event(entries);
  struct decoder_event new = (struct decoder_event){
    .t0 = primary, .t1 = secondary,
      .time = last->time + duration, .state = DECODER_SYNC,
      .valid = 1, .reason = 0,
  };
  append_trigger_event(last, new);
}

static void add_trigger_event_transition_loss(struct decoder_event **entries, timeval_t duration, unsigned int primary, unsigned int secondary, decoder_loss_reason reason) {
  ck_assert(*entries);

  struct decoder_event *last = find_last_trigger_event(entries);
  struct decoder_event new = (struct decoder_event){
    .t0 = primary, .t1 = secondary,
      .time = last->time + duration, .state = DECODER_NOSYNC,
      .valid = 0, .reason = reason,
  };
  append_trigger_event(last, new);
}

static void validate_decoder_sequence(struct decoder_event *ev) {
  for (; ev; ev = ev->next) {
    if (ev->t0) {
      config.decoder.last_t0 = ev->time;
      config.decoder.needs_decoding_t0 = 1;
    }
    if (ev->t1) {
      config.decoder.last_t1 = ev->time;
      config.decoder.needs_decoding_t1 = 1;
    }

    set_current_time(ev->time);
    config.decoder.decode(&config.decoder);
    ck_assert_msg(config.decoder.state == ev->state, 
        "expected event at %d: (state=%d, valid=%d). Got (state=%d, valid=%d)",
        ev->time, ev->state, ev->valid, 
        config.decoder.state, config.decoder.valid);
    if (!ev->valid) {
      ck_assert_msg(config.decoder.loss == ev->reason,
      "reason mismatch at %d: %d. Got %d",
      ev->time, ev->reason, config.decoder.loss);
    }
  }
}

static void prepare_decoder(trigger_type type) {
  config.decoder.type = type;
  decoder_init(&config.decoder);
}


#if 0
START_TEST(check_tfi_decoder_startup_normal) {

  prepare_decoder(FORD_TFI);
  validate_decoder_sequence(tfi_startup_events, 6);
  ck_assert_int_eq(config.decoder.last_trigger_angle, 180);

} END_TEST

START_TEST(check_tfi_decoder_syncloss_variation) {

  prepare_decoder(FORD_TFI);
  validate_decoder_sequence(tfi_startup_events, 6);

  struct decoder_event ev[] = {
    {1, 0, 150000, DECODER_SYNC, 1, 0},
    {1, 0, 155000, DECODER_NOSYNC, 0, DECODER_VARIATION},
  };
  validate_decoder_sequence(ev, 2);
  ck_assert_int_eq(0, config.decoder.current_triggers_rpm);
} END_TEST

START_TEST(check_tfi_decoder_syncloss_expire) {
  prepare_decoder(FORD_TFI);
  validate_decoder_sequence(tfi_startup_events, 6);
  ck_assert_int_eq(config.decoder.expiration, 162500);

  set_current_time(170000);
  handle_decoder_expire(&config.decoder);
  ck_assert_int_eq(0, config.decoder.current_triggers_rpm);
  ck_assert_int_eq(0, config.decoder.valid);
  ck_assert_int_eq(DECODER_EXPIRED, config.decoder.loss);
} END_TEST


START_TEST(check_cam_nplusone_startup_normal) {
  prepare_decoder(TOYOTA_24_1_CAS);
  config.decoder.required_triggers_rpm = 9;
  validate_decoder_sequence(cam_nplusone_startup_events, 11);
  ck_assert_int_eq(config.decoder.last_trigger_angle, 30);

} END_TEST

START_TEST(check_cam_nplusone_startup_normal_then_die) {
  prepare_decoder(TOYOTA_24_1_CAS);
  config.decoder.required_triggers_rpm = 9;
  validate_decoder_sequence(cam_nplusone_startup_events, 11);

  struct decoder_event cam_nplusone_death_events[] = {
    {0, 1, 250000, DECODER_NOSYNC, 0, DECODER_TRIGGERCOUNT_LOW},
  };
  validate_decoder_sequence(cam_nplusone_death_events, 1);

} END_TEST
#endif


/* Gets decoder up to sync, plus an additional trigger */
static void cam_nplusone_normal_startup_to_sync(struct decoder_event **entries) {
  /* Triggers to get RPM */
  for (int i = 0; i < config.decoder.required_triggers_rpm - 1; ++i) {
    add_trigger_event(entries, 25000, 1, 0);
  }
  add_trigger_event_transition_rpm(entries, 25000, 1, 0);

  /* Wait a few triggers before sync */
  add_trigger_event(entries, 25000, 1, 0);
  add_trigger_event(entries, 25000, 1, 0);

  /* Get a sync pulse and then continue to maintain sync for 3 triggers */
  add_trigger_event_transition_sync(entries, 500, 0, 1);
  add_trigger_event(entries, 24500, 1, 0);
}
  
START_TEST(check_cam_nplusone_startup_normal_sustained) {

  struct decoder_event *entries = NULL;

  cam_nplusone_normal_startup_to_sync(&entries);
  
  /* Two additional triggers, three total after sync pulse */
  add_trigger_event(&entries, 25000, 1, 0);
  add_trigger_event(&entries, 25000, 1, 0);

  prepare_decoder(TOYOTA_24_1_CAS);
  validate_decoder_sequence(entries);

  ck_assert_int_eq(config.decoder.last_trigger_angle, 
      3 * config.decoder.degrees_per_trigger);

} END_TEST

#if 0
START_TEST(check_cam_nplusone_startup_normal_no_second_trigger) {
//  prepare_decoder(TOYOTA_24_1_CAS);
//  config.decoder.required_triggers_rpm = 9;
//  validate_decoder_sequence(cam_nplusone_startup_events, 11);
//
//  struct decoder_event events[27] = {0};
//  for (int i = 0; i < 24; ++i) {
//    add_decoder_test_entry(events, 25000, 1, 0, 1);
//  }
//  add_decoder_test_entry(events, 25000, 1, 0, 1);
//  add_decoder_test_entry(events, 25000, 1, 0, 0);
//
//  validate_decoder_sequence(events, 27);
//
//  ck_assert(config.decoder.loss == DECODER_TRIGGERCOUNT_HIGH);

} END_TEST

START_TEST(check_nplusone_decoder_syncloss_expire) {
  struct decoder_event cam_nplusone_startup_events[] = {
    {1, 0, 18000, DECODER_NOSYNC, 0, 0},
    {1, 0, 25000, DECODER_NOSYNC, 0, 0},
    {1, 0, 50000, DECODER_NOSYNC, 0, 0},
    {1, 0, 75000, DECODER_NOSYNC, 0, 0},
    {1, 0, 100000, DECODER_NOSYNC, 0, 0},
    {1, 0, 125000, DECODER_NOSYNC, 0, 0},
    {1, 0, 150000, DECODER_NOSYNC, 0, 0},
    {1, 0, 175000, DECODER_NOSYNC, 0, 0},
    {1, 0, 200000, DECODER_RPM, 0, 0},
    {0, 1, 200500, DECODER_SYNC, 1, 0},
    {1, 0, 225000, DECODER_SYNC, 1, 0},
  };
  prepare_decoder(TOYOTA_24_1_CAS);
  config.decoder.required_triggers_rpm = 9;
  validate_decoder_sequence(cam_nplusone_startup_events, 11);

  ck_assert_int_eq(config.decoder.expiration, 262500);

} END_TEST
#endif

TCase *setup_decoder_tests() {
  TCase *decoder_tests = tcase_create("decoder");
//  tcase_add_test(decoder_tests, check_tfi_decoder_startup_normal);
//  tcase_add_test(decoder_tests, check_tfi_decoder_syncloss_variation);
//  tcase_add_test(decoder_tests, check_tfi_decoder_syncloss_expire);
//  tcase_add_test(decoder_tests, check_cam_nplusone_startup_normal);
//  tcase_add_test(decoder_tests, check_cam_nplusone_startup_normal_then_die);
  tcase_add_test(decoder_tests, check_cam_nplusone_startup_normal_sustained);
//  tcase_add_test(decoder_tests, check_cam_nplusone_startup_normal_no_second_trigger);
//  tcase_add_test(decoder_tests, check_nplusone_decoder_syncloss_expire);
  return decoder_tests;
}
#endif

