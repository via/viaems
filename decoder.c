#include "decoder.h"
#include "platform.h"
#include "util.h"

/* This function assumes it will be called at least once every timer
 * wrap-around.  That should be a valid assumption seeing as this should
 * be called in the main loop repeatedly 
 */

#define MAX_RPM_DEVIATION 100
#define SAVE_VALS 4 

int decoder_valid(struct decoder *d) {
  if (!d->valid)
    return 0;

  timeval_t curtime = current_time();
  if (d->expiration > d->last_trigger_time) { 
    /* No timer-wrap */
    if ((curtime >= d->last_trigger_time) && (curtime < d->expiration)) {
      return 1;
    } else {
      d->valid = 0;
      return 0;
    }
  } else {
    if ((curtime >= d->expiration) && (curtime < d->last_trigger_time)) {
      d->valid = 0;
      return 0;
    } else {
      return 1;
    }
  }
}

void tfi_pip_decoder(struct decoder *d) {
  timeval_t t0, prev_t0;
  timeval_t acc = 0;
  int i;
  static timeval_t last_times[SAVE_VALS];
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
  if (valid_time_count >= SAVE_VALS - 1) {
    timeval_t diff = time_diff(t0, prev_t0);
    int slicerpm = rpm_from_time_diff(diff, 45);
    if ((slicerpm <= MAX_RPM_DEVIATION) ||
        (slicerpm > d->rpm + MAX_RPM_DEVIATION) ||
        (slicerpm < d->rpm - MAX_RPM_DEVIATION)) {
      /* RPM changed too much, or is too low */
      valid_time_count = 0;
      d->valid = 0;
      return;
    } else {
      valid_time_count = SAVE_VALS - 1;
      d->valid = 1;
      d->last_trigger_time = t0;
      d->expiration = t0 + diff + time_from_rpm_diff(MAX_RPM_DEVIATION, 45);
      d->rpm = rpm_from_time_diff(last_times[cur_index], last_times[(cur_index + 3) % SAVE_VALS]);
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

