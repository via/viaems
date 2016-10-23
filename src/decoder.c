#include "decoder.h"
#include "platform.h"
#include "util.h"
#include "scheduler.h"
#include "config.h"

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
  for (int i = d->required_triggers_rpm - 1; i > 0; --i) {
    d->times[i] = d->times[i - 1];
  }
  d->times[0] = t;
}

static void trigger_update(struct decoder *d, timeval_t t) {
  push_time(d, t);
  d->t0_count++;
  d->triggers_since_last_sync++;
  timeval_t diff = d->times[0] - d->times[1];
  unsigned int slicerpm = rpm_from_time_diff(diff, d->degrees_per_trigger);

  if (d->state == DECODER_SYNC) {
    d->last_trigger_angle += d->degrees_per_trigger;
    if (d->last_trigger_angle >= 720) {
      d->last_trigger_angle -= 720;
    }
  }

  if (d->state == DECODER_RPM || d->state == DECODER_SYNC) {
    d->rpm = rpm_from_time_diff(d->times[0] - d->times[d->rpm_window_size], 
      d->degrees_per_trigger * d->rpm_window_size);
    if (d->rpm) {
      d->trigger_cur_rpm_change = abs(d->rpm - slicerpm) / (float)d->rpm;
    }
    if ((slicerpm <= d->trigger_min_rpm) ||
         (slicerpm > d->rpm + (d->rpm * d->trigger_max_rpm_change)) ||
         (slicerpm < d->rpm - (d->rpm * d->trigger_max_rpm_change))) {
      d->state = DECODER_NOSYNC;
      d->loss = DECODER_VARIATION;
    }
    if (d->triggers_since_last_sync > d->num_triggers) {
      d->state = DECODER_NOSYNC;
      d->loss = DECODER_TRIGGERCOUNT_HIGH;
    }
    d->expiration = t + diff * 1.5;
    set_expire_event(d->expiration);
  }

}

static void sync_update(struct decoder *d) {
  d->t1_count++;
  if (d->state == DECODER_RPM || d->state == DECODER_SYNC) {
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
  timeval_t t0, t1;
  int sync, trigger;
  decoder_state oldstate = d->state;

  disable_interrupts();
  t0 = d->last_t0;
  t1 = d->last_t1;

  trigger = d->needs_decoding_t0;
  d->needs_decoding_t0 = 0;

  sync = d->needs_decoding_t1;
  d->needs_decoding_t1 = 0;
  enable_interrupts();

  if (d->state == DECODER_NOSYNC && trigger) {
    if (d->current_triggers_rpm >= d->required_triggers_rpm) {
      d->state = DECODER_RPM;
    } else {
      d->current_triggers_rpm++;
    }
  }

  if (trigger && sync) {
    /* Which came first? */
    if (time_in_range(t1, t0, current_time())) {
      trigger_update(d, t0);
      sync_update(d);
    } else {
      sync_update(d);
      trigger_update(d, t0);
    }
  } else if (trigger) {
    trigger_update(d, t0);
  } else {
    sync_update(d);
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
  timeval_t t0;
  decoder_state oldstate = d->state;

  disable_interrupts();
  t0 = d->last_t0;
  d->needs_decoding_t0 = 0;
  enable_interrupts();

  if (d->state == DECODER_NOSYNC) {
    if (d->current_triggers_rpm >= d->required_triggers_rpm) {
      d->state = DECODER_RPM;
    } else {
      d->current_triggers_rpm++;
    }
  }

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
}

void decoder_init(struct decoder *d) {
  d->last_t0 = 0;
  d->last_t1 = 0;
  d->needs_decoding_t0 = 0;
  d->needs_decoding_t1 = 0;
  switch (config.trigger) {
    case FORD_TFI:
      d->decode = tfi_pip_decoder;
      d->required_triggers_rpm = 3;
      d->degrees_per_trigger = 90;
      d->rpm_window_size = 2;
      d->num_triggers = 8;
      break;
    case TOYOTA_24_1_CAS:
      d->decode = cam_nplusone_decoder;
      d->required_triggers_rpm = 4;
      d->degrees_per_trigger = 30;
      d->rpm_window_size = 3;
      d->num_triggers = 24;
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

#ifdef UNITTEST
void check_handle_decoder_expire(void *_d) {
  handle_decoder_expire(_d);
}
#endif

