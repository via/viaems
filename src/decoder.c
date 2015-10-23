#include "decoder.h"
#include "platform.h"
#include "util.h"

/* This function assumes it will be called at least once every timer
 * wrap-around.  That should be a valid assumption seeing as this should
 * be called in the main loop repeatedly 
 */

#define MAX_RPM_DEVIATION 200
#define SAVE_VALS 4 

int decoder_valid(struct decoder *d) {
  if (!d->valid)
    return 0;

  timeval_t curtime = current_time();
  if (time_in_range(curtime, d->last_trigger_time, d->expiration)) {
    return 1;
  } else {
    d->valid = 0;
    return 0;
  }
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

void tfi_pip_decoder(struct decoder *d) {
  timeval_t t0, prev_t0;
  static timeval_t last_times[SAVE_VALS] = {0, 0, 0, 0};
  static unsigned char cur_index = 0;
  static unsigned char valid_time_count = 0;

  disable_interrupts();
  t0 = d->last_t0;
  d->needs_decoding = 0;
  enable_interrupts();

  prev_t0 = last_times[cur_index];
  cur_index = constrain((cur_index + 1), SAVE_VALS);
  last_times[cur_index] = t0;
  valid_time_count++;
  if (valid_time_count >= SAVE_VALS) {
    timeval_t diff = time_diff(t0, prev_t0);
    unsigned int slicerpm = rpm_from_time_diff(diff, 90);
    d->rpm = rpm_from_time_diff(time_diff(last_times[cur_index], 
          last_times[constrain(cur_index + 1, SAVE_VALS)]), 270);
    valid_time_count = SAVE_VALS;
    if ((slicerpm <= MAX_RPM_DEVIATION) ||
        (slicerpm > d->rpm + MAX_RPM_DEVIATION) ||
        (slicerpm < d->rpm - MAX_RPM_DEVIATION)) {
      /* RPM changed too much, or is too low */
      d->valid = 0;
      return;
    } else {
      d->valid = 1;
      d->last_trigger_time = t0;
      d->expiration = t0 + diff + (diff >> 1); /* 1.5x length of previous slice */
    }
  } else {
    d->valid = 0;
  }
}


void decoder_init(struct decoder *d) {
  d->last_t0 = 0;
  d->last_t1 = 0;
  d->needs_decoding = 0;
  d->decode = tfi_pip_decoder;
  d->valid = 0;
  d->rpm = 0;
  d->last_trigger_time = 0;
  d->last_trigger_rpm = 0;
  d->expiration = 0;
}

