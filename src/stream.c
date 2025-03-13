#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "stream.h"
#include "util.h"

static bool cobs_encode(
    struct cobs_encoder *enc,
    size_t in_size,
    const uint8_t in_buffer[in_size],
    stream_write_fn write,
    void *arg) {

  for (int i = 0; i < in_size; i++) {
    if (enc->n_bytes == 254) {
      const size_t wr_size = enc->n_bytes + 1;
      enc->scratchpad[0] = wr_size;
      if (!write(wr_size, enc->scratchpad, arg)) {
        return false;
      }
      enc->n_bytes = 0;
    }

    if (in_buffer[i] == 0) {
      const size_t wr_size = enc->n_bytes + 1;
      enc->scratchpad[0] = wr_size;
      if (!write(wr_size, enc->scratchpad, arg)) {
        return false;
      }
      enc->n_bytes = 0;
    } else {
      enc->n_bytes += 1;
      enc->scratchpad[enc->n_bytes] = in_buffer[i];
    }
  }
  return true;
}

struct cobs_decoder {
  size_t n_bytes;
  bool zero_current_block;
};

static bool cobs_decode(
    struct cobs_decoder *dec,
    size_t in_size,
    uint8_t in_buffer[in_size],
    stream_read_fn read,
    void *arg) {
  
  size_t remaining = in_size;
  uint8_t *ptr = in_buffer;
  while (remaining > 0) {

    if (dec->n_bytes == 0) {
      if (dec->zero_current_block) {
        ptr[0] = '\0';
        ptr++;
        remaining--;
      }

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


static bool platform_stream_write_timeout(size_t n, const uint8_t data[n], void *arg) {

  timeval_t continue_before_time = ((struct stream_message *)arg)->timeout;
  const uint8_t *ptr = data;
  size_t remaining = n;

  while (remaining > 0) {
    bool timed_out = time_before(continue_before_time, current_time());
    if (timed_out) {
      return false;
    }
    size_t written = platform_stream_write(remaining, ptr);
    ptr += written;
    remaining -= written;
  }

  return true;
}

static bool platform_stream_read_timeout(size_t n, const uint8_t data[n], void *arg) {

  timeval_t continue_before_time = ((struct stream_message *)arg)->timeout;
  const uint8_t *ptr = data;
  size_t remaining = n;

  while (remaining > 0) {
    bool timed_out = time_before(continue_before_time, current_time());
    if (timed_out) {
      return false;
    }
    size_t amt_read = platform_stream_read(remaining, ptr);
    ptr += amt_read;
    remaining -= amt_read;
  }

  return true;
}

bool stream_message_new(
    struct stream_message *msg,
    stream_write_fn write,
    void *arg,
    uint32_t length,
    uint32_t crc) {
  msg->cobs.n_bytes = 0;
  msg->length = length;
  msg->consumed = 0;
  msg->crc = crc;
  msg->write = write;
  msg->arg = arg;

  /* First encode a length and a crc as two little-endian 32 bit words */
  uint8_t header[8];
  header[0] = msg->length & 0xff;
  header[1] = (msg->length >> 8) & 0xff;
  header[2] = (msg->length >> 16) & 0xff;
  header[3] = (msg->length >> 24) & 0xff;
  header[4] = msg->crc & 0xff;
  header[5] = (msg->crc >> 8) & 0xff;
  header[6] = (msg->crc >> 16) & 0xff;
  header[7] = (msg->crc >> 24) & 0xff;

  return cobs_encode(&msg->cobs, sizeof(header), header, msg->write , msg->arg);
}

bool stream_message_write(
    struct stream_message *msg,
    size_t size,
    const uint8_t buffer[size]) {

  if (!cobs_encode(&msg->cobs, size, buffer, msg->write, msg->arg)) {
    return false;
  }
  msg->consumed += size;
  if (msg->consumed == msg->length) {
    uint8_t nullbyte = '\0';
    /* finish by encoding with a null byte to flush the encoder, 
     * then write a null separator */
    if (!cobs_encode(&msg->cobs, 1, &nullbyte, msg->write, msg->arg)) {
      return false;
    }
    return msg->write(1, &nullbyte, msg->arg);
  }

  return true;
}


#define UNITTEST
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

START_TEST(test_cobs_encode) {

  {
    uint8_t nozeros[] = {1, 2, 3, 4, 5, 0};
    struct cobs_encoder enc = { 0 };
    struct test_write_ctx ctx = { 0 };

    cobs_encode(&enc, sizeof(nozeros), nozeros, test_write_fn, &ctx);
    const uint8_t expected[] = {6, 1, 2, 3, 4, 5};
    ck_assert_int_eq(ctx.size, 6);
    ck_assert_mem_eq(expected, ctx.buffer, ctx.size);

  }

  {
    uint8_t somezeros[] = {0, 1, 2, 0, 3, 4, 0, 0};
    struct cobs_encoder enc = { 0 };
    struct test_write_ctx ctx = { 0 };

    cobs_encode(&enc, sizeof(somezeros), somezeros, test_write_fn, &ctx);
    const uint8_t expected[] = {1, 3, 1, 2, 3, 3, 4, 1};
    ck_assert_int_eq(ctx.size, 8);
    ck_assert_mem_eq(expected, ctx.buffer, ctx.size);

  }

  {
    uint8_t zero1[] = {0, 1, 2, 0};
    uint8_t zero2[] = {3, 4, 0, 0};
    struct cobs_encoder enc = { 0 };
    struct test_write_ctx ctx = { 0 };

    cobs_encode(&enc, sizeof(zero1), zero1, test_write_fn, &ctx);
    cobs_encode(&enc, sizeof(zero2), zero2, test_write_fn, &ctx);
    const uint8_t expected[] = {1, 3, 1, 2, 3, 3, 4, 1};
    ck_assert_int_eq(ctx.size, 8);
    ck_assert_mem_eq(expected, ctx.buffer, ctx.size);

  }
  {
    struct test_write_ctx ctx = { 0 };
    uint8_t large[260]; // 259 1s and then a null terminator
    for (int i = 0; i < sizeof(large) - 1; i++) {
      large[i] = 1;
    }
    large[259] = 0;

    struct cobs_encoder enc = { 0 };
    cobs_encode(&enc, sizeof(large), large, test_write_fn, &ctx);
    ck_assert_int_eq(ctx.size, 261);

  }

} END_TEST

START_TEST(test_stream_message_end_to_end_small) {

  const uint8_t payload[] = { 0x05, 0x06, 0x00, 0x08,
                            0xA1, 0xA2, 0xA3, 0xA4,
                            0x00, 0x01, 0x02, 0x03 };

  ck_assert(false);
} END_TEST

START_TEST(test_stream_message_write) {
  struct test_write_ctx ctx = {0};
  const uint8_t msg_text[] = "Hello, World!\n";

  uint32_t size = sizeof(msg_text);
  uint32_t crc = 0x5A5AFFFF;

  struct stream_message msg;
  bool result = stream_message_new(&msg, test_write_fn, &ctx, sizeof(msg_text), crc);
  ck_assert(result);

  result = stream_message_write(&msg, sizeof(msg_text), msg_text);
  ck_assert(result);

  const uint8_t expected[] = {
    2,                 /* Distance to first zero */
    15, 1, 1, 19,       /* Encoded 15, little endian with zeroes encoded */
    0xFF, 0xFF, 0x5A, 0x5A, /* CRC little endian, encoded */
    'H', 'e', 'l', 'l', 'o', ',', ' ', /* Hello, */
    'W', 'o', 'r', 'l', 'd', '!',      /* World! */
    '\n', 1,                          /* Newline and encoded terminator */
    '\0' /* Frame delimiter */
  };

  ck_assert_int_eq(ctx.size, sizeof(expected));
  ck_assert_mem_eq(ctx.buffer, expected, sizeof(expected));

} END_TEST;

START_TEST(test_cobs_decode) {

  const uint8_t expected[] = {
    2,                 /* Distance to first zero */
    15, 1, 1, 19,       /* Encoded 15, little endian with zeroes encoded */
    0xFF, 0xFF, 0x5A, 0x5A, /* CRC little endian, encoded */
    'H', 'e', 'l', 'l', 'o', ',', ' ', /* Hello, */
    'W', 'o', 'r', 'l', 'd', '!',      /* World! */
    '\n', 1,                          /* Newline and encoded terminator */
    '\0' /* Frame delimiter */
  };

  struct cobs_decoder dec = { 0 };
  struct test_read_ctx ctx = { .buffer = expected, .size = sizeof(expected) };

  uint8_t buffer[3];
  bool res = cobs_decode(&dec, sizeof(buffer), buffer, test_read_fn, &ctx);
  ck_assert(res);
  ck_assert_mem_eq(buffer, &((uint8_t[]){15, 0, 0}), 3);

  res = cobs_decode(&dec, sizeof(buffer), buffer, test_read_fn, &ctx);
  ck_assert(res);
  ck_assert_mem_eq(buffer, &((uint8_t[]){0, 0xff, 0xff}), 3);

  res = cobs_decode(&dec, sizeof(buffer), buffer, test_read_fn, &ctx);
  ck_assert(res);
  ck_assert_mem_eq(buffer, &((uint8_t[]){0x5a, 0x5a, 'H'}), 3);

  uint8_t rest_of_text[] = "ello, World!\n";
  uint8_t biggerbuffer[sizeof(rest_of_text)];
  res = cobs_decode(&dec, sizeof(biggerbuffer), biggerbuffer, test_read_fn, &ctx);
  ck_assert(res);
  ck_assert_mem_eq(biggerbuffer, rest_of_text, sizeof(rest_of_text));

  res = cobs_decode(&dec, 1, buffer, test_read_fn, &ctx);
  ck_assert(!res);
}

TCase *setup_stream_tests() {
  TCase *stream_tests = tcase_create("stream");

  tcase_add_test(stream_tests, test_cobs_encode);
  tcase_add_test(stream_tests, test_cobs_decode);
//  tcase_add_test(stream_tests, test_stream_message_end_to_end_small);
  tcase_add_test(stream_tests, test_stream_message_write);

  return stream_tests;
}
#endif
