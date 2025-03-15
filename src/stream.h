
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

struct cobs_decoder {
  size_t n_bytes;           /* Number of bytes remaining in current block */
  bool zero_current_block;  /* Whether this block should be null-terminated */
};

struct stream_message_writer {
  struct cobs_encoder cobs;
  stream_write_fn write;
  void *arg;
  uint32_t length;
  uint32_t crc;
  uint32_t consumed;
};

bool stream_message_writer_new(
    struct stream_message_writer *msg,
    stream_write_fn write,
    void *arg,
    uint32_t length,
    uint32_t crc);

bool stream_message_write(
    struct stream_message_writer *msg,
    size_t size,
    const uint8_t buffer[size]);

struct stream_message_reader {
  struct cobs_decoder cobs;
  stream_read_fn read;
  void *arg;
  uint32_t length;
  uint32_t crc;
};

bool stream_message_reader_new(
    struct stream_message_reader *msg,
    stream_read_fn read,
    void *arg);

bool stream_message_read(
    struct stream_message_reader *msg,
    size_t size,
    uint8_t buffer[size]);

#ifdef UNITTEST
#include <check.h>
TCase *setup_stream_tests(void);
#endif

#endif
