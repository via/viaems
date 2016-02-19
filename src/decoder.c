#include "decoder.h"
#include "platform.h"
#include "util.h"
#include "scheduler.h"
#include "config.h"

#include <stdlib.h>

static struct sched_entry expire_event;

static void handle_decoder_invalidate_event(void *_d) {
  struct decoder *d = (struct decoder *)_d;
  d->valid = 0;
  d->state = DECODER_NOSYNC;
  d->current_triggers_rpm = 0;
  invalidate_scheduled_events(config.events, config.num_events);
}

static void set_expire_event(timeval_t t) {
  disable_interrupts();
  expire_event.time = t;
  schedule_insert(current_time(), &expire_event);
  enable_interrupts();
}

static void push_time(struct decoder *d, timeval_t t) {
  for (int i = d->required_triggers_rpm - 1; i > 0; --i) {
    d->times[i] = d->times[i - 1];
  }
  d->times[0] = t;
}

void tfi_pip_decoder(struct decoder *d) {
  timeval_t t0;

  disable_interrupts();
  t0 = d->last_t0;
  d->needs_decoding = 0;
  enable_interrupts();

  push_time(d, t0);
  d->t0_count++;

  timeval_t diff = d->times[0] - d->times[1];
  unsigned int slicerpm = rpm_from_time_diff(diff, d->degrees_per_trigger);
  d->last_trigger_angle += d->degrees_per_trigger;
  if (d->last_trigger_angle == 720) {
    d->last_trigger_angle = 0;
  }

  switch (d->state) {
    case DECODER_NOSYNC:
      if (d->current_triggers_rpm >= d->required_triggers_rpm) {
        d->state = DECODER_RPM;
      } else { 
        d->current_triggers_rpm++;  
      }
      break;
    case DECODER_RPM:
    case DECODER_SYNC:
      d->rpm = rpm_from_time_diff(d->times[0] - d->times[2], 
        d->degrees_per_trigger * 2);
      if (d->rpm) {
        d->trigger_cur_rpm_change = abs(d->rpm - slicerpm) / (float)d->rpm;
      }
      if ((slicerpm <= d->trigger_min_rpm) ||
           (slicerpm > d->rpm + (d->rpm * d->trigger_max_rpm_change)) ||
           (slicerpm < d->rpm - (d->rpm * d->trigger_max_rpm_change))) {
        handle_decoder_invalidate_event(d);  
      } else {
        d->valid = 1;
        d->last_trigger_time = t0;
        d->expiration = t0 + diff * 1.5;
        set_expire_event(d->expiration);
      }
      break;
  }
}

void decoder_init(struct decoder *d) {
  d->last_t0 = 0;
  d->last_t1 = 0;
  d->needs_decoding = 0;
  switch (config.trigger) {
    case FORD_TFI:
      d->decode = tfi_pip_decoder;
      d->required_triggers_rpm = 3;
      d->degrees_per_trigger = 90;
    default:
      break;
  }
  d->current_triggers_rpm = 0;
  d->valid = 0;
  d->rpm = 0;
  d->state = DECODER_NOSYNC;
  d->last_trigger_time = 0;
  d->last_trigger_angle = 0;
  d->expiration = 0;

  expire_event.callback = handle_decoder_invalidate_event;
  expire_event.ptr = d;
}

