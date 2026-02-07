#ifndef _DECODER_H
#define _DECODER_H

#include "platform.h"
#include "sensors.h"

#define MAX_TRIGGERS 60

typedef enum {
  /* Trigger wheel is N even teeth that add to 720 degrees.  This decoder is
   * only useful for low-resolution wheels, such as a Ford TFI */
  TRIGGER_EVEN_NOSYNC,

  /* Cam (if adds to 720) or crank (if adds to 360) wheel with N even teeth
   * and a second wheel on a cam.  Cam pulse indicates that next tooth angle
   * is 0 degrees */
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
  DECODER_EXPIRED,
  DECODER_OVERFLOW,
} decoder_loss_reason;

struct decoder_config {
  decoder_type type;
  degrees_t offset;

  float trigger_max_rpm_change;
  uint32_t trigger_min_rpm;
  uint32_t required_triggers_rpm;

  uint32_t num_triggers;
  degrees_t degrees_per_trigger;
  uint32_t rpm_window_size;
};

struct engine_position {
  timeval_t time;
  timeval_t valid_until;

  bool has_rpm;
  uint32_t rpm;
  uint32_t tooth_rpm;

  bool has_position;
  degrees_t last_trigger_angle;
};

typedef enum {
  DECODER_NOSYNC,
  DECODER_RPM,
  DECODER_SYNC,
} decoder_state;

struct decoder {
  const struct decoder_config *config;
  decoder_state state;

  uint32_t current_triggers_rpm;
  uint32_t triggers_since_last_sync;
  float trigger_cur_rpm_change;
  timeval_t times[MAX_TRIGGERS + 1];

  struct engine_position output;

  /* Debug */
  uint32_t t0_count;
  uint32_t t1_count;
  decoder_loss_reason loss;
};

struct trigger_event {
  timeval_t time;
  trigger_type type;
};

void decoder_init(const struct decoder_config *conf, struct decoder *state);
void decoder_desync(struct decoder *state, decoder_loss_reason reason);

void decoder_update(struct decoder *state, struct trigger_event *trigger);

struct engine_position decoder_get_engine_position(const struct decoder *);

degrees_t engine_current_angle(const struct engine_position *d,
                               timeval_t at_time);
bool engine_position_is_synced(const struct engine_position *d,
                               timeval_t at_time);

#ifdef UNITTEST
#include <check.h>
TCase *setup_decoder_tests(void);
#endif
#endif
