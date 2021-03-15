#ifndef _DECODER_H
#define _DECODER_H

#include "platform.h"
#define MAX_TRIGGERS 36

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
  DECODER_NOSYNC,
  DECODER_RPM,
  DECODER_SYNC,
} decoder_state;

typedef enum {
  DECODER_NO_LOSS,
  DECODER_VARIATION,
  DECODER_TRIGGERCOUNT_HIGH,
  DECODER_TRIGGERCOUNT_LOW,
  DECODER_EXPIRED,
  DECODER_OVERFLOW,
} decoder_loss_reason;

struct decoder {
  uint32_t valid;
  uint32_t rpm;
  uint32_t tooth_rpm;
  timeval_t last_trigger_time;
  degrees_t last_trigger_angle;
  timeval_t expiration;

  /* Configuration */
  decoder_type type;
  degrees_t offset;

  float trigger_max_rpm_change;
  float trigger_cur_rpm_change;
  uint32_t trigger_min_rpm;
  uint32_t required_triggers_rpm;

  uint32_t num_triggers;
  degrees_t degrees_per_trigger;
  uint32_t rpm_window_size;

  /* Debug */
  uint32_t t0_count;
  uint32_t t1_count;
  decoder_loss_reason loss;

  /* Internal state */
  decoder_state state;
  uint32_t current_triggers_rpm;
  uint32_t triggers_since_last_sync;
  timeval_t times[MAX_TRIGGERS];
};

struct decoder_event {
  unsigned int trigger;
  timeval_t time;
#ifdef UNITTEST
  decoder_state state;
  uint32_t valid;
  decoder_loss_reason reason;
  struct decoder_event *next;
#endif
};

void decoder_init(struct decoder *);
void decoder_update_scheduling(struct decoder_event *, unsigned int count);
void decoder_desync(decoder_loss_reason);
degrees_t current_angle();

#ifdef UNITTEST
#include <check.h>
TCase *setup_decoder_tests();
#endif
#endif
