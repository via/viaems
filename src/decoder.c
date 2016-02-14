#include "decoder.h"
#include "platform.h"
#include "util.h"
#include "scheduler.h"
#include "config.h"

/* This function assumes it will be called at least once every timer
 * wrap-around.  That should be a valid assumption seeing as this should
 * be called in the main loop repeatedly 
 */

#define SAVE_VALS 4 

static struct sched_entry expire_event;

static void handle_decoder_invalidate_event(void *_d) {
  struct decoder *d = (struct decoder *)_d;
  d->valid = 0;
  /* Disable all not-yet-fired scheduled events */
  /* But for this moment, just disable events */
  invalidate_scheduled_events();
}

static void decoder_invalidate(struct decoder *d) {
  disable_interrupts();
  handle_decoder_invalidate_event(d);
  enable_interrupts();
}

int decoder_valid(struct decoder *d) {
  return d->valid;
}

/* Assumes idx is less than `max` greater than the bounds of the array */
static inline unsigned char constrain(unsigned char idx, 
                                      unsigned char max) {
  if (idx >= max) {
    return idx - max;
  } else {
    return idx;
  }
}

static void set_expire_event(timeval_t t) {
  disable_interrupts();
  expire_event.time = t;
  schedule_insert(current_time(), &expire_event);
  enable_interrupts();
}

void tfi_pip_decoder(struct decoder *d) {
  timeval_t t0, prev_t0;
  static timeval_t last_times[SAVE_VALS] = {0, 0, 0, 0};
  static unsigned char cur_index = 0;
  static unsigned char valid_time_count = 0;

  disable_interrupts();
  t0 = d->last_t0;
  d->needs_decoding = 0;
  enable_interrupts();

  d->t0_count++;
  prev_t0 = last_times[cur_index];
  cur_index = constrain((cur_index + 1), SAVE_VALS);
  last_times[cur_index] = t0;
  valid_time_count++;
  if (valid_time_count >= SAVE_VALS) {
    timeval_t diff = time_diff(t0, prev_t0);
    unsigned int slicerpm = rpm_from_time_diff(diff, 90);
    d->rpm = rpm_from_time_diff(time_diff(last_times[cur_index], 
          last_times[constrain(cur_index + 2, SAVE_VALS)]), 180);
    valid_time_count = SAVE_VALS;
    if ((slicerpm <= d->trigger_min_rpm) ||
        (slicerpm > d->rpm + (d->rpm * d->trigger_max_rpm_change)) ||
        (slicerpm < d->rpm - (d->rpm * d->trigger_max_rpm_change))) {
      /* RPM changed too much, or is too low */
      decoder_invalidate(d);
      return;
    } else {
      d->valid = 1;
      d->last_trigger_time = t0;
      d->last_trigger_angle += 90;
      if (d->last_trigger_angle == 720) {
        d->last_trigger_angle = 0;
      }
      d->expiration = t0 + diff + (diff >> 1); /* 1.5x length of previous slice */
      set_expire_event(d->expiration);
    }
  } else {
    decoder_invalidate(d);
  }
}

void decoder_init(struct decoder *d) {
  d->last_t0 = 0;
  d->last_t1 = 0;
  d->needs_decoding = 0;
  if (config.trigger == FORD_TFI) {
    d->decode = tfi_pip_decoder;
  }
  d->valid = 0;
  d->rpm = 0;
  d->last_trigger_time = 0;
  d->last_trigger_angle = 0;
  d->expiration = 0;

  expire_event.callback = handle_decoder_invalidate_event;
  expire_event.ptr = d;
}

