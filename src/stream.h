
#ifndef STREAM_H
#define STREAM_H

#include <stdbool.h>
#include <stdint.h>

#include "platform.h"

typedef bool (*stream_read_fn)(size_t n, uint8_t data[n], void *arg);
typedef bool (*stream_write_fn)(size_t n, const uint8_t data[n], void *arg);

struct cobs_encoder {
  uint8_t scratchpad[256];
  size_t n_bytes;
};

struct cobs_decoder {
  size_t n_bytes;          /* Number of bytes remaining in current block */
  bool zero_current_block; /* Whether this block should be null-terminated */
};

struct stream_message_writer {
  struct cobs_encoder cobs;
  stream_write_fn write;
  void *arg;
  uint32_t length;
  uint32_t crc;
  uint32_t consumed;
};

/* Initialize a stream_message_writer and write the frame header to the
 * underlying stream.
 * write and arg are used as the underlying stream_write_fn.
 * length and crc are used to populate the header.
 * Returns true if succeeded.
 */
bool stream_message_writer_new(struct stream_message_writer *msg,
                               stream_write_fn write,
                               void *arg,
                               uint32_t length,
                               uint32_t crc);

/* Serialize size bytes from buffer into the underlying write stream.
 * Returns true if succeeded.
 */
bool stream_message_write(struct stream_message_writer *msg,
                          size_t size,
                          const uint8_t buffer[size]);

struct stream_message_reader {
  struct cobs_decoder cobs;
  stream_read_fn read;
  void *arg;
  uint32_t length;
  uint32_t crc;
};

/* Initialize a stream_message_reader and attempt to deserialize the frame
 * header from the underlying stream. The length and crc from the frame header
 * are populated into *msg.
 * read and arg are used as the underlying stream.
 * Returns true if succeeded.
 */
bool stream_message_reader_new(struct stream_message_reader *msg,
                               stream_read_fn read,
                               void *arg);

/* Attempt to deserialize size bytes from the underlying stream into buffer.
 * Returns true if succeeded.
 */
bool stream_message_read(struct stream_message_reader *msg,
                         size_t size,
                         uint8_t buffer[size]);

#ifdef UNITTEST
#include <check.h>
TCase *setup_stream_tests(void);
#endif

#endif
