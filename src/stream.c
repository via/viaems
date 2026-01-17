#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef PLATFORM_HAS_NATIVE_MESSAGING

#include "console.h"
#include "util.h"
#include "stream.h"
#include "crc.h"

/* This file implements the platform message API for platforms that do not have a native messaging API.
 * It does so by using the platform streaming reads and writes.  Messages are encoded with COBS and framed with a starting
 * length and a trailing CRC and NUL delimiter:
 *
 * ---------------------------------
 * | [ LEN | MESSAGE | CRC ] | NUL | 
 * ---------------------------------
 *
 *  [ LEN | MESSAGE | CRC] is COBS encoded, and thus can contain no NUL bytes:
 *    - LEN is a 16 bit little endian integer for the length of MESSAGE.
 *    - MESSAGE is the PDU
 *    - CRC is a CRC-32/HDLC, the same used for ethernet. It is computed across the bytes in MESSAGE
 *  NUL is a single NUL
 */

/* Encode in_size bytes from in_buffer with COBS, using write and arg as the
 * underlying stream. 
 * Returns true if succeeded.
 */

typedef bool (*stream_read_fn)(size_t n, uint8_t data[n], void *arg);
typedef bool (*stream_write_fn)(size_t n, const uint8_t data[n], void *arg);

struct cobs_encoder {
  uint8_t scratchpad[512];
  size_t next_write_pos;
  size_t last_ovh_pos;
};

struct cobs_decoder {
  size_t n_bytes;          /* Number of bytes remaining in current block */
  bool zero_current_block; /* Whether this block should be null-terminated */
};

struct stream_message_writer {
  struct cobs_encoder cobs;
  uint16_t length;
  struct crc32 crc;
  uint16_t consumed;
};

struct stream_message_reader {
  struct cobs_decoder cobs;
  uint16_t length;
  uint16_t bytes_read;
  struct crc32 crc;
  bool eof;
};

struct blocking_reader {
  timeval_t fail_after;
};

static struct stream_message_writer msg_writer;
static struct stream_message_reader msg_reader;
static bool read_stream_synchronized = true;
static struct blocking_reader blocking_read_arg;

static bool blocking_read_timeout(size_t n, uint8_t data[n], void *arg) {
  struct blocking_reader *br = (struct blocking_reader *)arg;
  uint8_t *ptr = data;
  size_t remaining = n;
  while (remaining > 0) {
    if (current_time() > br->fail_after) {
      return false;
    }
    size_t amt = platform_read(ptr, remaining);
    remaining -= amt;
    ptr += amt;
  }
  return true;
}

static bool nonblocking_read(size_t n, uint8_t data[n], void *arg) {
  (void)arg;
  return platform_read(data, n) == n;
}

static bool blocking_platform_write(size_t n, const uint8_t data[n], void *arg) {
  (void)arg;

  size_t remaining = n;
  const uint8_t *ptr = data;
  while (remaining > 0) {
    size_t written = platform_write(ptr, remaining);
    ptr += written;
    remaining -= written;
  }
  return true;
}


static void cobs_init(struct cobs_encoder *enc) {
  enc->last_ovh_pos = 0;
  enc->next_write_pos = 1;
}

static bool cobs_encode(struct cobs_encoder *enc,
                        size_t in_size,
                        const uint8_t in_buffer[in_size],
                        stream_write_fn write,
                        void *arg) {

  for (size_t i = 0; i < in_size; i++) {
    size_t n_bytes = enc->next_write_pos - enc->last_ovh_pos - 1;
    if (n_bytes == 254) {
      enc->scratchpad[enc->last_ovh_pos] = n_bytes + 1;
      if (!write(enc->next_write_pos, enc->scratchpad, arg)) {
        return false;
      }
      enc->next_write_pos = 1;
      enc->last_ovh_pos = 0;
      n_bytes = 0;
    }

    if (in_buffer[i] == 0) {
      enc->scratchpad[enc->last_ovh_pos] = n_bytes + 1;
      if (enc->last_ovh_pos > 254) {
        /* Immediately write the whole buffer*/
        if (!write(enc->next_write_pos, enc->scratchpad, arg)) {
          return false;
        }
        enc->next_write_pos = 1;
        enc->last_ovh_pos = 0;
      } else {
        enc->last_ovh_pos = enc->next_write_pos;
        enc->next_write_pos += 1;
      }
    } else {
      enc->scratchpad[enc->next_write_pos] = in_buffer[i];
      enc->next_write_pos += 1;
    }
  }
  return true;
}

static bool cobs_flush(struct cobs_encoder *enc, stream_write_fn write, void *arg) {
  if (enc->next_write_pos == 1) {
    enc->scratchpad[0] = '\0';
    if (!write(1, enc->scratchpad, arg)) {
      return false;
    }
  } else {
    size_t n_bytes = enc->next_write_pos - enc->last_ovh_pos - 1;
    enc->scratchpad[enc->last_ovh_pos] = n_bytes + 1;
    enc->scratchpad[enc->next_write_pos] = '\0';
    if (!write(enc->next_write_pos + 1, enc->scratchpad, arg)) {
      return false;
    }
  }
  return true;
}


/* Decode in_size bytes into in_buffer using read and arg as the underlying
 * stream. Returns true if succeeded.
 */
static bool cobs_decode(struct cobs_decoder *dec,
                        size_t in_size,
                        uint8_t in_buffer[in_size],
                        stream_read_fn read,
                        void *arg) {

  size_t remaining = in_size;
  uint8_t *ptr = in_buffer;
  while (remaining > 0) {
    /* We are at the start of a block */
    if (dec->n_bytes == 0) {
      /* Write a null if needed. This happens at the end of any block except for
       * the first one, or where the block was 255 bytes long */
      if (dec->zero_current_block) {
        ptr[0] = '\0';
        ptr++;
        remaining--;
      }

      /* Read the block first byte */
      uint8_t first_byte;
      if (!read(1, &first_byte, arg)) {
        return false;
      }
      if (first_byte == 0) {
        return false;
      }
      dec->n_bytes = first_byte - 1;
      dec->zero_current_block = (first_byte != 255);
    }

    /* Read the smallest of either out target buffer or the remaining block
     * length */
    uint8_t amt_to_read = dec->n_bytes < remaining ? dec->n_bytes : remaining;
    if (!read(amt_to_read, ptr, arg)) {
      return false;
    }

    remaining -= amt_to_read;
    ptr += amt_to_read;
    dec->n_bytes -= amt_to_read;
  }
  return true;
}

/* Initialize a stream_message_writer and write the frame header to the
 * underlying stream.
 * write and arg are used as the underlying stream_write_fn.
 * length and crc are used to populate the header.
 * Returns true if succeeded.
 */
static bool stream_message_writer_new(struct stream_message_writer *msg,
                                      stream_write_fn write,
                                      void *arg,
                                      uint16_t length) {
                               
  cobs_init(&msg->cobs);
  msg->length = length;
  msg->consumed = 0;
  crc32_init(&msg->crc);

  /* First encode a length as a 16 bit little endian integer */
  uint8_t header[2];
  header[0] = msg->length & 0xff;
  header[1] = (msg->length >> 8) & 0xff;

  return cobs_encode(&msg->cobs, sizeof(header), header, write, arg);
}

/* Serialize size bytes from buffer into the underlying write stream.
 * Returns true if succeeded.
 */
static bool stream_message_write(struct stream_message_writer *msg,
                                 stream_write_fn write,
                                 void *arg,
                                 size_t size,
                                 const uint8_t buffer[size]) {

  crc32_add_bytes(&msg->crc, size, buffer);

  if (!cobs_encode(&msg->cobs, size, buffer, write, arg)) {
    return false;
  }
  msg->consumed += size;
  if (msg->consumed == msg->length) {
    /* finish by frame with the crc and flushing the cobs decoder, which produces a NUL delimiter */
    uint32_t crc = crc32_finish(&msg->crc);

    uint8_t crc_bytes[] = {
      crc & 0xff,
      (crc >> 8) & 0xff,
      (crc >> 16) & 0xff,
      (crc >> 24) & 0xff,
    }; 

    if (!cobs_encode(&msg->cobs, sizeof(crc_bytes), crc_bytes, write, arg)) {
      return false;
    }
    if (!cobs_flush(&msg->cobs, write, arg)) {
      return false;
    }
  }

  return true;
}

/* Initialize a stream_message_reader and attempt to deserialize the frame
 * header from the underlying stream. The length and crc from the frame header
 * are populated into *msg.
 * read and arg are used as the underlying stream.
 * Returns true if succeeded.
 */
static bool stream_message_reader_new(struct stream_message_reader *msg,
                                      stream_read_fn read,
                                      void *arg) {
  msg->cobs = (struct cobs_decoder){ 0 };
  msg->bytes_read = 0;
  msg->eof = false;
  crc32_init(&msg->crc);

  /* Read length */
  uint8_t received_length[2];
  if (!cobs_decode(&msg->cobs,
                   sizeof(received_length),
                   received_length,
                   read,
                   arg)) {
    return false;
  }
  msg->length = (uint16_t)received_length[0] |
                ((uint16_t)received_length[1] << 8);

  return true;
}

/* Attempt to deserialize size bytes from the underlying stream into buffer.
 * Returns true if succeeded.
 */
static bool stream_message_read(struct stream_message_reader *msg,
                                stream_read_fn read,
                                void *arg,
                                size_t size,
                                uint8_t buffer[size]) {

  if (msg->eof) {
    return false;
  }
  
  if (size + msg->bytes_read > msg->length) {
    return false;
  }

  if (!cobs_decode(&msg->cobs, size, buffer, read, arg)) {
    return false;
  }
	crc32_add_bytes(&msg->crc, size, buffer);

  msg->bytes_read += size;
  if (msg->bytes_read == msg->length) {
    /* Read CRC */
    uint8_t crc_bytes[4];
    if (!cobs_decode(
          &msg->cobs, sizeof(crc_bytes), crc_bytes, read, arg)) {
      return false;
    }
    uint32_t received_crc = (uint32_t)crc_bytes[0] | 
                            ((uint32_t)crc_bytes[1] << 8) |
                            ((uint32_t)crc_bytes[2] << 16) |
                            ((uint32_t)crc_bytes[3] << 24);
    uint32_t computed_crc = crc32_finish(&msg->crc);
    if (received_crc != computed_crc) {
      return false;
    }

    uint8_t delimiter;
    if (!read(1, &delimiter, arg) || (delimiter != '\0')) {
      return false;
    }
    msg->eof = true;
  }
  return true;
}


bool platform_message_writer_new(struct console_tx_message *msg, size_t length) {
  (void)msg;
  if (!stream_message_writer_new(&msg_writer, blocking_platform_write, NULL, length)) {
    return false;
  }

  return true;
}

bool platform_message_writer_write(struct console_tx_message *msg, const uint8_t *data, size_t length) {
  (void)msg;
  if (!stream_message_write(&msg_writer, blocking_platform_write, NULL, length, data)) {
    return false;
  }

  return true;
}

void platform_message_writer_abort(struct console_tx_message *msg) {
  (void)msg;
}

static bool read_until_null_byte(void) {
  uint8_t byte;
  /* Scan the bytes immediately available for a null */
  while (platform_read(&byte, 1) != 0) {
    if (byte == 0) {
      return true;
    }
  }
  return false;
}

bool platform_message_reader_new(struct console_rx_message *msg) {
  if (!read_stream_synchronized && !read_until_null_byte()) {
    return false;
  }

  /* Use a nonblocking read to see if we have length bytes available. 
   * This means the 2 length bytes must both be readable similtaneously.
   */
  if (!stream_message_reader_new(&msg_reader, nonblocking_read, NULL)) {
    return false;
  }
  msg->length = msg_reader.length;
  /* Bound the total reception time to 100 ms to avoid blocking transmits
   * forever */
  blocking_read_arg = (struct blocking_reader){ .fail_after = current_time() + time_from_us(100000) };
  return true;
}


bool platform_message_reader_read(struct console_rx_message *msg, uint8_t *data, size_t length) {
  (void)msg;

  if (!stream_message_read(&msg_reader, blocking_read_timeout, &blocking_read_arg, length, data)) {
    if (!msg_reader.eof) {
      read_stream_synchronized = false;
    }
    return false;
  }

  msg->eof = msg_reader.eof;
  return true;
}

#ifdef UNITTEST
#include <check.h>

struct test_write_ctx {
  uint8_t buffer[1024];
  size_t size;
};

struct test_read_ctx {
  const uint8_t *buffer;
  size_t size;
  size_t position;
};

static inline bool test_write_fn(size_t n, const uint8_t buffer[n], void *arg) {
  struct test_write_ctx *ctx = (struct test_write_ctx *)arg;
  memcpy(ctx->buffer + ctx->size, buffer, n);
  ctx->size += n;
  return true;
}

static inline bool test_read_fn(size_t n, uint8_t buffer[n], void *arg) {
  struct test_read_ctx *ctx = (struct test_read_ctx *)arg;
  memcpy(buffer, ctx->buffer + ctx->position, n);
  ctx->position += n;
  return true;
}

/* 590 random bytes for large cbor validation */
static const uint8_t large_array[] = {
  0x8a, 0xb6, 0xcd, 0x3f, 0xa4, 0xec, 0xa1, 0xcd, 0x05, 0x19, 0x5e, 0x2a, 0x57,
  0xc5, 0xf6, 0x6d, 0x24, 0x01, 0xd1, 0x23, 0x6d, 0xcc, 0xed, 0x69, 0x52, 0x05,
  0xb5, 0xf1, 0xcf, 0xac, 0x7f, 0x3b, 0x6e, 0xa8, 0x10, 0x9f, 0x7d, 0xdb, 0xa7,
  0xfd, 0x46, 0xb1, 0xe1, 0x1e, 0xcf, 0x67, 0xb1, 0x05, 0x64, 0x15, 0x43, 0xa4,
  0x0f, 0xd0, 0x1c, 0xea, 0x48, 0x79, 0xff, 0xc7, 0xef, 0x9f, 0xe2, 0x72, 0xc0,
  0x8c, 0x1e, 0x0b, 0x18, 0xfc, 0xd0, 0xf8, 0x63, 0x4d, 0x59, 0xd1, 0x20, 0xff,
  0xb0, 0x6a, 0x05, 0xbb, 0x1b, 0x5b, 0xca, 0x1d, 0xb9, 0x6a, 0x2f, 0x99, 0x40,
  0x67, 0xfa, 0x19, 0x3c, 0x7a, 0x00, 0x31, 0x08, 0x62, 0x25, 0xbe, 0x4d, 0xcc,
  0x0b, 0x64, 0xb4, 0x24, 0x9f, 0x09, 0x80, 0x04, 0xa1, 0x70, 0xa3, 0x4e, 0x6f,
  0x5a, 0x64, 0x5b, 0x77, 0xc1, 0xc2, 0xc1, 0x30, 0x38, 0x32, 0x86, 0x6e, 0xc2,
  0xa5, 0xc1, 0x62, 0x26, 0xd4, 0x49, 0xf8, 0x4f, 0x95, 0xb8, 0x3f, 0x10, 0x63,
  0xc5, 0xa1, 0x3d, 0xe9, 0xf9, 0xd2, 0xa1, 0xd3, 0x13, 0x6a, 0xc8, 0x74, 0x7f,
  0xc1, 0x68, 0xa6, 0x4e, 0x80, 0x12, 0xaa, 0x2b, 0xb2, 0xfd, 0xda, 0x75, 0x18,
  0xb5, 0x76, 0x1e, 0xfd, 0x57, 0x37, 0xc6, 0x6c, 0xd6, 0x64, 0x8c, 0x09, 0x9a,
  0x7a, 0x2c, 0x17, 0x13, 0x6c, 0xd0, 0x2f, 0x2d, 0x1c, 0x31, 0x98, 0x65, 0xad,
  0xef, 0x66, 0xea, 0x9b, 0x80, 0x7f, 0xb9, 0x0a, 0x8d, 0x54, 0x04, 0x11, 0x13,
  0x7b, 0x52, 0x02, 0x2d, 0x87, 0x03, 0xe5, 0x39, 0x9b, 0x73, 0x45, 0x0e, 0x06,
  0x34, 0x11, 0xbe, 0x1f, 0x9b, 0x2a, 0x78, 0x89, 0x05, 0xb5, 0x25, 0xd0, 0xcf,
  0xa2, 0xee, 0xc5, 0x6d, 0x44, 0x5c, 0x7f, 0x52, 0x57, 0xc8, 0x90, 0xca, 0x7d,
  0x3a, 0x79, 0x33, 0x63, 0x81, 0x76, 0x0e, 0xce, 0x71, 0xf8, 0x01, 0x34, 0x6d,
  0xab, 0x59, 0x02, 0x57, 0xac, 0xbc, 0xf4, 0x8c, 0x1f, 0xff, 0xec, 0x35, 0xb5,
  0x1e, 0x23, 0x1c, 0xda, 0xaf, 0xb8, 0x9d, 0x01, 0xff, 0xbe, 0xdb, 0x03, 0x0b,
  0x5d, 0x0f, 0x4c, 0xe4, 0x8c, 0xf6, 0xc6, 0x95, 0x7c, 0x26, 0x94, 0x80, 0x08,
  0x81, 0x75, 0x42, 0x21, 0x18, 0x49, 0xd9, 0xde, 0x41, 0xa7, 0xc6, 0x76, 0x6d,
  0x6f, 0x85, 0xee, 0xbc, 0xae, 0x04, 0x70, 0x06, 0xcf, 0x4c, 0xcc, 0x16, 0xb8,
  0x85, 0x95, 0xe7, 0x72, 0x93, 0x4c, 0x26, 0x60, 0x5f, 0x09, 0xb4, 0x35, 0x39,
  0xbe, 0xe0, 0x74, 0x8a, 0x3a, 0x2d, 0xbf, 0x5e, 0x4a, 0xba, 0x55, 0x66, 0x7e,
  0x91, 0x71, 0xd4, 0x68, 0x09, 0x0a, 0xc2, 0xe3, 0xed, 0x98, 0x3d, 0xdc, 0x3f,
  0xb4, 0xe9, 0xe5, 0x6a, 0xa5, 0x7e, 0xf4, 0x6a, 0x7b, 0x80, 0x6b, 0xbd, 0x97,
  0x51, 0xe3, 0xf0, 0x52, 0x2a, 0x95, 0xaa, 0x27, 0x70, 0xa2, 0x5e, 0x12, 0xf6,
  0x59, 0x3b, 0xf3, 0x55, 0x07, 0x6d, 0x74, 0xd7, 0xb1, 0x05, 0x29, 0xba, 0x33,
  0x08, 0xf0, 0x71, 0x5e, 0x33, 0xe5, 0x3f, 0xd2, 0x7b, 0x8a, 0xf7, 0x5f, 0xb7,
  0x98, 0xac, 0x5d, 0x0e, 0xba, 0x22, 0xcf, 0x6b, 0xab, 0x72, 0xff, 0x86, 0x9d,
  0x49, 0x01, 0x93, 0x4a, 0x5e, 0xf8, 0x78, 0xed, 0x84, 0x41, 0xab, 0x4e, 0x2b,
  0xec, 0x53, 0xda, 0x29, 0x1f, 0x41, 0xf6, 0x93, 0x44, 0xfd, 0xdd, 0xeb, 0x61,
  0xad, 0x32, 0xb9, 0x11, 0x3a, 0x67, 0x16, 0x00, 0x3f, 0xdf, 0x43, 0x10, 0x14,
  0x02, 0x5b, 0xb3, 0xe1, 0xcc, 0xd8, 0xac, 0xa4, 0xeb, 0xa1, 0xf4, 0x21, 0x80,
  0xd3, 0xe4, 0x64, 0xa8, 0xe7, 0xc5, 0x20, 0x36, 0x66, 0x26, 0xa6, 0xe2, 0x18,
  0x77, 0x10, 0x00, 0x3d, 0x7c, 0x38, 0x16, 0x97, 0x0e, 0x92, 0xf5, 0xfa, 0x96,
  0x67, 0x87, 0x71, 0x67, 0x49, 0xbf, 0xfb, 0x78, 0x35, 0xc3, 0x41, 0xbc, 0x6d,
  0x29, 0x8a, 0x31, 0xd3, 0x7f, 0x79, 0x18, 0x0f, 0x1b, 0x3d, 0x73, 0xa4, 0x44,
  0x35, 0xf0, 0x9d, 0xbc, 0x81, 0x0f, 0x31, 0x27, 0x52, 0x02, 0x61, 0x38, 0x07,
  0x13, 0x8a, 0xa8, 0x58, 0x12, 0x43, 0x6e, 0x1a, 0x10, 0x9b, 0x21, 0x1c, 0x77,
  0xe8, 0xab, 0xe0, 0xfe, 0x04, 0x0d, 0xb2, 0x70, 0xe4, 0x58, 0x00, 0x2a, 0x74,
  0xb6, 0x8d, 0x8f, 0x76, 0x94, 0xf3, 0x1f, 0xaa, 0xd7, 0x79, 0x72, 0xb9, 0x75,
  0x35, 0xd8, 0x50, 0xfc, 0x02
};

static const uint8_t large_array_encoded[] = {
  0x61, 0x8a, 0xb6, 0xcd, 0x3f, 0xa4, 0xec, 0xa1, 0xcd, 0x05, 0x19, 0x5e, 0x2a,
  0x57, 0xc5, 0xf6, 0x6d, 0x24, 0x01, 0xd1, 0x23, 0x6d, 0xcc, 0xed, 0x69, 0x52,
  0x05, 0xb5, 0xf1, 0xcf, 0xac, 0x7f, 0x3b, 0x6e, 0xa8, 0x10, 0x9f, 0x7d, 0xdb,
  0xa7, 0xfd, 0x46, 0xb1, 0xe1, 0x1e, 0xcf, 0x67, 0xb1, 0x05, 0x64, 0x15, 0x43,
  0xa4, 0x0f, 0xd0, 0x1c, 0xea, 0x48, 0x79, 0xff, 0xc7, 0xef, 0x9f, 0xe2, 0x72,
  0xc0, 0x8c, 0x1e, 0x0b, 0x18, 0xfc, 0xd0, 0xf8, 0x63, 0x4d, 0x59, 0xd1, 0x20,
  0xff, 0xb0, 0x6a, 0x05, 0xbb, 0x1b, 0x5b, 0xca, 0x1d, 0xb9, 0x6a, 0x2f, 0x99,
  0x40, 0x67, 0xfa, 0x19, 0x3c, 0x7a, 0xff, 0x31, 0x08, 0x62, 0x25, 0xbe, 0x4d,
  0xcc, 0x0b, 0x64, 0xb4, 0x24, 0x9f, 0x09, 0x80, 0x04, 0xa1, 0x70, 0xa3, 0x4e,
  0x6f, 0x5a, 0x64, 0x5b, 0x77, 0xc1, 0xc2, 0xc1, 0x30, 0x38, 0x32, 0x86, 0x6e,
  0xc2, 0xa5, 0xc1, 0x62, 0x26, 0xd4, 0x49, 0xf8, 0x4f, 0x95, 0xb8, 0x3f, 0x10,
  0x63, 0xc5, 0xa1, 0x3d, 0xe9, 0xf9, 0xd2, 0xa1, 0xd3, 0x13, 0x6a, 0xc8, 0x74,
  0x7f, 0xc1, 0x68, 0xa6, 0x4e, 0x80, 0x12, 0xaa, 0x2b, 0xb2, 0xfd, 0xda, 0x75,
  0x18, 0xb5, 0x76, 0x1e, 0xfd, 0x57, 0x37, 0xc6, 0x6c, 0xd6, 0x64, 0x8c, 0x09,
  0x9a, 0x7a, 0x2c, 0x17, 0x13, 0x6c, 0xd0, 0x2f, 0x2d, 0x1c, 0x31, 0x98, 0x65,
  0xad, 0xef, 0x66, 0xea, 0x9b, 0x80, 0x7f, 0xb9, 0x0a, 0x8d, 0x54, 0x04, 0x11,
  0x13, 0x7b, 0x52, 0x02, 0x2d, 0x87, 0x03, 0xe5, 0x39, 0x9b, 0x73, 0x45, 0x0e,
  0x06, 0x34, 0x11, 0xbe, 0x1f, 0x9b, 0x2a, 0x78, 0x89, 0x05, 0xb5, 0x25, 0xd0,
  0xcf, 0xa2, 0xee, 0xc5, 0x6d, 0x44, 0x5c, 0x7f, 0x52, 0x57, 0xc8, 0x90, 0xca,
  0x7d, 0x3a, 0x79, 0x33, 0x63, 0x81, 0x76, 0x0e, 0xce, 0x71, 0xf8, 0x01, 0x34,
  0x6d, 0xab, 0x59, 0x02, 0x57, 0xac, 0xbc, 0xf4, 0x8c, 0x1f, 0xff, 0xec, 0x35,
  0xb5, 0x1e, 0x23, 0x1c, 0xda, 0xaf, 0xb8, 0x9d, 0x01, 0xff, 0xbe, 0xdb, 0x03,
  0x0b, 0x5d, 0x0f, 0x4c, 0xe4, 0x8c, 0xf6, 0xc6, 0x95, 0x7c, 0x26, 0x94, 0x80,
  0x08, 0x81, 0x75, 0x42, 0x21, 0x18, 0x49, 0xd9, 0xde, 0x41, 0xa7, 0xc6, 0x76,
  0x6d, 0x6f, 0x85, 0xee, 0xbc, 0xae, 0x04, 0x70, 0x06, 0xcf, 0x4c, 0xcc, 0x16,
  0xb8, 0x85, 0x95, 0xe7, 0x72, 0x93, 0x4c, 0x26, 0x60, 0x5f, 0x09, 0xb4, 0x35,
  0x39, 0xbe, 0xe0, 0x74, 0x8a, 0x3a, 0x2d, 0xbf, 0x5e, 0x4a, 0xba, 0x55, 0x66,
  0x7e, 0x70, 0x91, 0x71, 0xd4, 0x68, 0x09, 0x0a, 0xc2, 0xe3, 0xed, 0x98, 0x3d,
  0xdc, 0x3f, 0xb4, 0xe9, 0xe5, 0x6a, 0xa5, 0x7e, 0xf4, 0x6a, 0x7b, 0x80, 0x6b,
  0xbd, 0x97, 0x51, 0xe3, 0xf0, 0x52, 0x2a, 0x95, 0xaa, 0x27, 0x70, 0xa2, 0x5e,
  0x12, 0xf6, 0x59, 0x3b, 0xf3, 0x55, 0x07, 0x6d, 0x74, 0xd7, 0xb1, 0x05, 0x29,
  0xba, 0x33, 0x08, 0xf0, 0x71, 0x5e, 0x33, 0xe5, 0x3f, 0xd2, 0x7b, 0x8a, 0xf7,
  0x5f, 0xb7, 0x98, 0xac, 0x5d, 0x0e, 0xba, 0x22, 0xcf, 0x6b, 0xab, 0x72, 0xff,
  0x86, 0x9d, 0x49, 0x01, 0x93, 0x4a, 0x5e, 0xf8, 0x78, 0xed, 0x84, 0x41, 0xab,
  0x4e, 0x2b, 0xec, 0x53, 0xda, 0x29, 0x1f, 0x41, 0xf6, 0x93, 0x44, 0xfd, 0xdd,
  0xeb, 0x61, 0xad, 0x32, 0xb9, 0x11, 0x3a, 0x67, 0x16, 0x22, 0x3f, 0xdf, 0x43,
  0x10, 0x14, 0x02, 0x5b, 0xb3, 0xe1, 0xcc, 0xd8, 0xac, 0xa4, 0xeb, 0xa1, 0xf4,
  0x21, 0x80, 0xd3, 0xe4, 0x64, 0xa8, 0xe7, 0xc5, 0x20, 0x36, 0x66, 0x26, 0xa6,
  0xe2, 0x18, 0x77, 0x10, 0x49, 0x3d, 0x7c, 0x38, 0x16, 0x97, 0x0e, 0x92, 0xf5,
  0xfa, 0x96, 0x67, 0x87, 0x71, 0x67, 0x49, 0xbf, 0xfb, 0x78, 0x35, 0xc3, 0x41,
  0xbc, 0x6d, 0x29, 0x8a, 0x31, 0xd3, 0x7f, 0x79, 0x18, 0x0f, 0x1b, 0x3d, 0x73,
  0xa4, 0x44, 0x35, 0xf0, 0x9d, 0xbc, 0x81, 0x0f, 0x31, 0x27, 0x52, 0x02, 0x61,
  0x38, 0x07, 0x13, 0x8a, 0xa8, 0x58, 0x12, 0x43, 0x6e, 0x1a, 0x10, 0x9b, 0x21,
  0x1c, 0x77, 0xe8, 0xab, 0xe0, 0xfe, 0x04, 0x0d, 0xb2, 0x70, 0xe4, 0x58, 0x15,
  0x2a, 0x74, 0xb6, 0x8d, 0x8f, 0x76, 0x94, 0xf3, 0x1f, 0xaa, 0xd7, 0x79, 0x72,
  0xb9, 0x75, 0x35, 0xd8, 0x50, 0xfc, 0x02, 0x00
};

START_TEST(test_cobs_encode) {

  {
    uint8_t nozeros[] = { 1, 2, 3, 4, 5 };
    struct cobs_encoder enc = { 0 };
    struct test_write_ctx ctx = { 0 };

    cobs_init(&enc);
    cobs_encode(&enc, sizeof(nozeros), nozeros, test_write_fn, &ctx);
    cobs_flush(&enc, test_write_fn, &ctx);
    const uint8_t expected[] = { 6, 1, 2, 3, 4, 5, 0};
    ck_assert_int_eq(ctx.size, sizeof(expected));
    ck_assert_mem_eq(expected, ctx.buffer, ctx.size);
  }

  {
    uint8_t somezeros[] = { 0, 1, 2, 0, 3, 4, 0 };
    struct cobs_encoder enc = { 0 };
    struct test_write_ctx ctx = { 0 };

    cobs_init(&enc);
    cobs_encode(&enc, sizeof(somezeros), somezeros, test_write_fn, &ctx);
    cobs_flush(&enc, test_write_fn, &ctx);
    const uint8_t expected[] = { 1, 3, 1, 2, 3, 3, 4, 1, 0};
    ck_assert_int_eq(ctx.size, sizeof(expected));
    ck_assert_mem_eq(expected, ctx.buffer, ctx.size);
  }

  {
    uint8_t zero1[] = { 0, 1, 2, 0 };
    uint8_t zero2[] = { 3, 4, 0 };
    struct cobs_encoder enc = { 0 };
    struct test_write_ctx ctx = { 0 };

    cobs_init(&enc);
    cobs_encode(&enc, sizeof(zero1), zero1, test_write_fn, &ctx);
    cobs_encode(&enc, sizeof(zero2), zero2, test_write_fn, &ctx);
    cobs_flush(&enc, test_write_fn, &ctx);
    const uint8_t expected[] = { 1, 3, 1, 2, 3, 3, 4, 1, 0 };
    ck_assert_int_eq(ctx.size, sizeof(expected));
    ck_assert_mem_eq(expected, ctx.buffer, ctx.size);
  }
  {
    struct test_write_ctx ctx = { 0 };
    uint8_t large[259]; // 259 1s and then a null terminator
    for (size_t i = 0; i < sizeof(large) - 1; i++) {
      large[i] = 1;
    }

    struct cobs_encoder enc = { 0 };
    cobs_init(&enc);
    cobs_encode(&enc, sizeof(large), large, test_write_fn, &ctx);
    cobs_flush(&enc, test_write_fn, &ctx);
    ck_assert_int_eq(ctx.size, 262);
  }
  {
    /* Test zero at +127 then run of 255 with no zeros and then flush */
    uint8_t large[128+255]; // 259 1s and then a null terminator
    for (size_t i = 0; i < sizeof(large); i++) {
      large[i] = 1;
    }
    large[127] = '\0';
    
    struct test_write_ctx ctx = { 0 };
    struct cobs_encoder enc = { 0 };
    cobs_init(&enc);
    cobs_encode(&enc, sizeof(large), large, test_write_fn, &ctx);
    cobs_flush(&enc, test_write_fn, &ctx);

    uint8_t expected[128+255+2+1];
    for (size_t i = 0; i < sizeof(expected); i++) {
      expected[i] = 1;
    }
    expected[0] = 128;
    expected[128] = 0xff;
    expected[128+255] = 2;
    expected[128+255+2] = 0;
    ck_assert_int_eq(ctx.size, sizeof(expected));
    for (unsigned i = 0; i < ctx.size; i++) {
      if (expected[i] != ctx.buffer[i]) {
        ck_abort_msg("expected[%d] == %d, but ctx[%d] == %d", i, expected[i], i, ctx.buffer[i]);
      }
    }
  }
  {
    /* Some thing as previously, but only 254 with no zeros */
    uint8_t large[128+254]; // 259 1s and then a null terminator
    for (size_t i = 0; i < sizeof(large); i++) {
      large[i] = 1;
    }
    large[127] = '\0';
    
    struct test_write_ctx ctx = { 0 };
    struct cobs_encoder enc = { 0 };
    cobs_init(&enc);
    cobs_encode(&enc, sizeof(large), large, test_write_fn, &ctx);
    cobs_flush(&enc, test_write_fn, &ctx);

    uint8_t expected[128+254+1+1];
    for (size_t i = 0; i < sizeof(expected); i++) {
      expected[i] = 1;
    }
    expected[0] = 128;
    expected[128] = 0xff;
    expected[128+254+1] = 0;
    ck_assert_int_eq(ctx.size, sizeof(expected));
    for (unsigned i = 0; i < ctx.size; i++) {
      if (expected[i] != ctx.buffer[i]) {
        ck_abort_msg("expected[%d] == %d, but ctx[%d] == %d", i, expected[i], i, ctx.buffer[i]);
      }
    }
  }
}
END_TEST

START_TEST(test_cobs_decode) {

  const uint8_t expected[] = {
    2,                      /* Distance to first zero */
    15,   1,    1,    19,   /* Encoded 15, little endian with zeroes encoded */
    0xFF, 0xFF, 0x5A, 0x5A, /* CRC little endian, encoded */
    'H',  'e',  'l',  'l',  'o', ',', ' ', /* Hello, */
    'W',  'o',  'r',  'l',  'd', '!',      /* World! */
    '\n', 1,                               /* Newline and encoded terminator */
    '\0'                                   /* Frame delimiter */
  };

  struct cobs_decoder dec = { 0 };
  struct test_read_ctx ctx = { .buffer = expected, .size = sizeof(expected) };

  uint8_t buffer[3];
  bool res = cobs_decode(&dec, sizeof(buffer), buffer, test_read_fn, &ctx);
  ck_assert(res);
  ck_assert_mem_eq(buffer, &((uint8_t[]){ 15, 0, 0 }), 3);

  res = cobs_decode(&dec, sizeof(buffer), buffer, test_read_fn, &ctx);
  ck_assert(res);
  ck_assert_mem_eq(buffer, &((uint8_t[]){ 0, 0xff, 0xff }), 3);

  res = cobs_decode(&dec, sizeof(buffer), buffer, test_read_fn, &ctx);
  ck_assert(res);
  ck_assert_mem_eq(buffer, &((uint8_t[]){ 0x5a, 0x5a, 'H' }), 3);

  uint8_t rest_of_text[] = "ello, World!\n";
  uint8_t biggerbuffer[sizeof(rest_of_text)];
  res =
    cobs_decode(&dec, sizeof(biggerbuffer), biggerbuffer, test_read_fn, &ctx);
  ck_assert(res);
  ck_assert_mem_eq(biggerbuffer, rest_of_text, sizeof(rest_of_text));

  res = cobs_decode(&dec, 1, buffer, test_read_fn, &ctx);
  ck_assert(!res);
}

START_TEST(test_cobs_decode_large) {
  struct cobs_decoder dec = { 0 };
  struct test_read_ctx ctx = { .buffer = large_array_encoded,
                               .size = sizeof(large_array_encoded) };

  uint8_t decoded[sizeof(large_array)];

  bool res = cobs_decode(&dec, sizeof(decoded), decoded, test_read_fn, &ctx);
  ck_assert(res);
  ck_assert_mem_eq(decoded, large_array, sizeof(decoded));
}

START_TEST(test_cobs_encode_large) {
  struct cobs_encoder enc = { 0 };

  struct test_write_ctx ctx = { 0 };
  cobs_init(&enc);
  bool res =
    cobs_encode(&enc, sizeof(large_array), large_array, test_write_fn, &ctx);
  ck_assert(res);

  cobs_flush(&enc, test_write_fn, &ctx);

  ck_assert_int_eq(ctx.size, sizeof(large_array_encoded));
  ck_assert_mem_eq(
    ctx.buffer, large_array_encoded, sizeof(large_array_encoded));
}

START_TEST(test_stream_message_write) {
  struct test_write_ctx ctx = { 0 };
  const uint8_t msg_text[] = "Hello, World!\n";

  struct stream_message_writer msg;
  bool result =
    stream_message_writer_new(&msg, test_write_fn, &ctx, sizeof(msg_text));
  ck_assert(result);

  result = stream_message_write(&msg, test_write_fn, &ctx, sizeof(msg_text), msg_text);
  ck_assert(result);

  const uint8_t expected[] = {
    2,            /* Distance to first zero */
    15,     15,   /* Encoded 15, little endian with zeroes encoded */
    'H',  'e',  'l',  'l',  'o', ',', ' ',  /* Hello, */
    'W',  'o',  'r',  'l',  'd', '!', '\n', /* World! */
    5,                      /* Null terminated string */
    0x2a, 0x40, 0x63, 0x38, /* CRC little endian, encoded */
    '\0'                    /* Frame delimiter */
  };

  ck_assert_int_eq(ctx.size, sizeof(expected));
  ck_assert_mem_eq(ctx.buffer, expected, sizeof(expected));
}
END_TEST;

START_TEST(test_stream_message_read) {
  const uint8_t msg_text[] = "Hello, World!\n";

  uint32_t size = sizeof(msg_text);
  uint32_t crc = 0x3863402a;

  const uint8_t encoded[] = {
    2,            /* Distance to first zero */
    15,     15,   /* Encoded 15, little endian with zeroes encoded */
    'H',  'e',  'l',  'l',  'o', ',', ' ',  /* Hello, */
    'W',  'o',  'r',  'l',  'd', '!', '\n', /* World! */
    5,                      /* Null terminated string */
    0x2a, 0x40, 0x63, 0x38, /* CRC little endian, encoded */
    '\0'                    /* Frame delimiter */
  };

  struct test_read_ctx ctx = { .buffer = encoded, .size = sizeof(encoded) };

  struct stream_message_reader msg;
  bool result = stream_message_reader_new(&msg, test_read_fn, &ctx);
  ck_assert(result);

  ck_assert_int_eq(size, msg.length);

  uint8_t dest[sizeof(msg_text)];
  result = stream_message_read(&msg, test_read_fn, &ctx, sizeof(dest), dest);
  ck_assert(result);
  ck_assert(msg.eof);
  ck_assert_int_eq(crc, msg.crc.value);
  ck_assert_mem_eq(dest, msg_text, size);

  uint8_t more;
  result = stream_message_read(&msg, test_read_fn, &ctx, 1, &more);
  ck_assert(!result);
}
END_TEST;

START_TEST(test_stream_message_read_badcrc) {
  const uint8_t msg_text[] = "Hello, World!\n";

  uint32_t size = sizeof(msg_text);

  const uint8_t encoded[] = {
    2,            /* Distance to first zero */
    15,     15,   /* Encoded 15, little endian with zeroes encoded */
    'H',  'e',  'l',  'l',  'o', ',', ' ',  /* Hello, */
    'W',  'o',  'r',  'l',  'd', '!', '\n', /* World! */
    5,                      /* Null terminated string */
    0x2a, 0x41, 0x63, 0x38, /* CRC little endian, encoded */
    '\0'                    /* Frame delimiter */
  };

  struct test_read_ctx ctx = { .buffer = encoded, .size = sizeof(encoded) };

  struct stream_message_reader msg;
  bool result = stream_message_reader_new(&msg, test_read_fn, &ctx);
  ck_assert(result);

  ck_assert_int_eq(size, msg.length);

  uint8_t dest[sizeof(msg_text)];
  result = stream_message_read(&msg, test_read_fn, &ctx, sizeof(dest), dest);
  ck_assert(!result);
  ck_assert(!msg.eof);

  uint8_t more;
  result = stream_message_read(&msg, test_read_fn, &ctx, 1, &more);
  ck_assert(!result);
}
END_TEST;

TCase *setup_stream_tests() {
  TCase *stream_tests = tcase_create("stream");

  tcase_add_test(stream_tests, test_cobs_encode);
  tcase_add_test(stream_tests, test_cobs_encode_large);

  tcase_add_test(stream_tests, test_cobs_decode);
  tcase_add_test(stream_tests, test_cobs_decode_large);

  tcase_add_test(stream_tests, test_stream_message_write);
  tcase_add_test(stream_tests, test_stream_message_read);
  tcase_add_test(stream_tests, test_stream_message_read_badcrc);

  return stream_tests;
}
#endif
#endif
