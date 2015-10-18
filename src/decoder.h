#ifndef _DECODER_H
#define _DECODER_H

#include "platform.h"

struct decoder;
typedef void (*decoder_func)(struct decoder *);

struct decoder {
  /* Unsafe interrupt-written vars */
  timeval_t last_t0;
  timeval_t last_t1;
  char needs_decoding;

  /* Safe, only handled in main loop */
  decoder_func decode;
  int valid;
  int rpm;
  timeval_t last_trigger_time;
  int last_trigger_rpm;
  timeval_t expiration;
};

int decoder_valid(struct decoder *);
void decoder_init(struct decoder *);

#endif

