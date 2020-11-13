#ifndef _DECODER_H
#define _DECODER_H

#include "platform.h"
#define MAX_TRIGGERS 36

typedef enum {
  TRIGGER_N_EVEN,
  TRIGGER_N_MINUS_1,
} trigger_type;

typedef enum {
  NO_SYNC,
  ONE_SYNC,
} cam_sync_type;

typedef enum {
  DECODER_NOSYNC,
  DECODER_RPM,
  DECODER_CRANK_SYNC,
  DECODER_CAM_SYNC,
} decoder_state;


typedef enum {
  DECODER_NO_LOSS,
  DECODER_VARIATION,
  DECODER_TRIGGERCOUNT_HIGH,
  DECODER_TRIGGERCOUNT_LOW,
  DECODER_EXPIRED,
} decoder_loss_reason;

struct decoder {
  /* Unsafe interrupt-written vars */
  timeval_t last_t0;
  timeval_t last_t1;
  uint32_t needs_decoding_t0;
  uint32_t needs_decoding_t1;

  /* Safe, only handled in main loop */
  uint32_t valid;
  uint32_t tooth_rpm;
  uint32_t rpm;
  timeval_t last_trigger_time;
  degrees_t last_trigger_angle;
  timeval_t expiration;

  /* Configuration */
  trigger_type trigger;
  cam_sync_type cam_sync;
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
  int valid;
  decoder_loss_reason reason;
  struct decoder_event *next;
#endif
};

void decoder_init(struct decoder *);
void decoder_update_scheduling(struct decoder_event *, unsigned int count);

degrees_t current_angle();

#ifdef UNITTEST
#include <check.h>
TCase *setup_decoder_tests();
#endif
#endif
