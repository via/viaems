
#ifndef STREAM_H
#define STREAM_H

#include <stdint.h>
#include <stdbool.h>

#include "platform.h"

typedef bool (*stream_read_fn)(size_t n, uint8_t data[n], void *arg);
typedef bool (*stream_write_fn)(size_t n, const uint8_t data[n], void *arg);

struct cobs_encoder {
  uint8_t scratchpad[256];
  size_t n_bytes;
};

struct stream_message {
  struct cobs_encoder cobs;
  stream_write_fn write;
  void *arg;
  uint32_t length;
  uint32_t consumed;
  uint32_t crc;
  timeval_t timeout;
};

bool stream_message_new(
    struct stream_message *msg,
    stream_write_fn write,
    void *arg,
    uint32_t length,
    uint32_t crc);

bool stream_message_write(
    struct stream_message *msg,
    size_t size,
    const uint8_t buffer[size]);

#ifdef UNITTEST
#include <check.h>
TCase *setup_stream_tests(void);
#endif

#endif
