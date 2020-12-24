#ifndef _DECODER_H
#define _DECODER_H

#include "platform.h"

typedef enum {
  TRIGGER_EVEN_NOSYNC,
  TRIGGER_EVEN_CAMSYNC,

  /* Trigger wheel is N teeth with a single missing tooth. The first tooth after
   * the gap is 0 degrees.  This decoder can provide sequential scheduling
   * configured to add to 720 degrees */
  TRIGGER_MISSING_NOSYNC,

  /* Trigger wheel identical to `TRIGGER_MISSING_NOSYNC`, but is expected to
   * always be a crank wheel adding to 360 degrees, with a second single tooth
   * wheel indicating the cam phase.  The crank cycle (measured from tooth 1 to
   * tooth 1) with the cam sync is the first crank cycle, 0-360. The first
   * tooth 1 *after* a cam sync is angle 360. */
  TRIGGER_MISSING_CAMSYNC,
} decoder_type;


typedef enum {
  DECODER_NO_LOSS,
  DECODER_VARIATION,
  DECODER_TRIGGERCOUNT_HIGH,
  DECODER_TRIGGERCOUNT_LOW,
  DECODER_MISSING_SYNC,
  DECODER_EXTRA_SYNC,
  DECODER_EXPIRED,
} decoder_loss_reason;

struct decoder_config {
  decoder_type type;
  degrees_t offset;
  float max_variance;
  uint32_t trigger_min_rpm;
  uint32_t required_triggers_rpm;
  uint32_t num_triggers;
  degrees_t degrees_per_trigger;
};

struct decoder_status {
  uint32_t valid;

  uint32_t rpm;
  uint32_t tooth_rpm;

  timeval_t last_trigger_time;
  degrees_t last_trigger_angle;
  timeval_t expiration;

  float cur_variance;

  uint32_t t0_count;
  uint32_t t1_count;
  decoder_loss_reason loss;
};

struct decoder_event {
  unsigned int trigger;
  timeval_t time;
};

extern struct decoder_status decoder_status;

void decoder_init();
void decoder_update_scheduling(struct decoder_event *, unsigned int count);
degrees_t current_angle();
timeval_t next_time_of_angle(degrees_t angle);

#ifdef UNITTEST
#include <check.h>
TCase *setup_decoder_tests();
#endif
#endif
