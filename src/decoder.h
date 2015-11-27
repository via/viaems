#ifndef _DECODER_H
#define _DECODER_H

#include "platform.h"

struct decoder;
typedef void (*decoder_func)(struct decoder *);

typedef enum {
  FORD_TFI,
  TOYOTA_24_1_CAS,
} trigger_type;

struct decoder {
  /* Unsafe interrupt-written vars */
  volatile timeval_t last_t0;
  volatile timeval_t last_t1;
  volatile char needs_decoding;

  /* Safe, only handled in main loop */
  decoder_func decode;
  unsigned char valid;
  unsigned int rpm;
  timeval_t last_trigger_time;
  degrees_t last_trigger_angle;
  timeval_t expiration;

  /* Configuration */
  degrees_t offset;
  float trigger_max_rpm_change;
  unsigned int trigger_min_rpm;
  unsigned int t0_pin;
  unsigned int t1_pin;

  /* Debug */
  unsigned int t0_count;
  unsigned int t1_count;
};

int decoder_valid(struct decoder *);
void decoder_init(struct decoder *);
void enable_test_trigger(trigger_type t, unsigned int rpm);

#endif

