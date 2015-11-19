#ifndef _DECODER_H
#define _DECODER_H

#include "platform.h"

struct decoder;
typedef void (*decoder_func)(struct decoder *);

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
  degrees_t offset;
  timeval_t expiration;
};

int decoder_valid(struct decoder *);
void decoder_init(struct decoder *);

#endif

