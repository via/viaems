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

void tfi_pip_decoder(struct decoder *d) {
  timeval_t t0, prev_t0;
  static timeval_t last_times[SAVE_VALS] = {0, 0, 0, 0};
  static int cur_index = 0;
  static int valid_time_count = 0;

  disable_interrupts();
  t0 = d->last_t0;
  d->needs_decoding = 0;
  enable_interrupts();

  prev_t0 = last_times[cur_index];
  cur_index = (cur_index + 1) % SAVE_VALS;
  last_times[cur_index] = t0;
  valid_time_count++;
  if (valid_time_count >= SAVE_VALS) {
    timeval_t diff = time_diff(t0, prev_t0);
    int slicerpm = rpm_from_time_diff(diff, 45);
    d->rpm = rpm_from_time_diff(time_diff(last_times[cur_index], 
          last_times[(cur_index + 1) % SAVE_VALS]), 135);
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
      d->expiration = t0 + time_from_rpm_diff(d->rpm + MAX_RPM_DEVIATION, 45);
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

