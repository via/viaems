#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "stream.h"

typedef bool (*write_fn)(size_t n, uint8_t data[n], void *arg);

struct cobs_encoder {
  uint8_t scratchpad[256];
  size_t n_bytes;
};

static bool cobs_encode(
    struct cobs_encoder *enc,
    size_t in_size,
    uint8_t in_buffer[in_size],
    write_fn write,
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

struct stream_message {
  struct cobs_encoder cobs;
  uint32_t length;
  uint32_t crc;
  timeval_t timeout;
};

bool stream_message_new(
    struct stream_message *msg,
    timeval_t timeout,
    uint32_t length,
    uint32_t crc) {
  msg->cobs.n_bytes = 0;
  msg->length = length;
  msg->crc = crc;
  msg->timeout = timeout;
  
}

bool stream_message_write(
    struct stream_message *msg,
    size_t size,
    uint8_t buffer[size]) {

  return false;
}



/* Decodes and validates a fragment from a stream oriented console.
 * The fragment is expected to end with the COBS null byte. After COBS
 * decoding, a 4 byte header is expected at the beginning of the fragment. The
 * payload is validated against the header, and the header is then stripped.
 * The decoded fragment size is set into *size and true is returned if
 * validation succeeded.
 */
stream_result_t decode_cobs_fragment_in_place(
    uint16_t *size, 
    uint8_t fragment[MAX_ENCODED_FRAGMENT_SIZE]) {

  uint16_t in_size = *size;
  *size = 0;

  if (in_size > MAX_ENCODED_FRAGMENT_SIZE) {
    return STREAM_BAD_SIZE;
  }
  if (in_size < STREAM_HEADER_SIZE + 1) {
    return STREAM_BAD_SIZE;
  }

  /* Decode COBS buffer that is under 255 bytes long with special handling to
   * move the stream header out, as well as crc calculation */
  uint16_t crc = CRC16_INIT;

  uint8_t stream_header[STREAM_HEADER_SIZE];
  uint8_t cobs_byte = fragment[0];
  for (int i = 1; i < in_size; i++) {
    uint8_t byte_to_write = 0;
    if (i == cobs_byte) {
      cobs_byte = fragment[i] + i;
    } else {
      byte_to_write = fragment[i];
    }

    /* Special handling for the first four bytes, move them into a different
     * buffer so that we can have the fragment buffer be just the underlying
     * fragment with only a single pass */
    if (i < STREAM_HEADER_SIZE + 1) {
      stream_header[i - 1] = byte_to_write;
    } else {
      fragment[i - STREAM_HEADER_SIZE - 1] = byte_to_write;
      crc16_add_byte(&crc, byte_to_write);
    }
  }

  uint16_t header_frag_length = (uint16_t)stream_header[0] | 
                                ((uint16_t)stream_header[1] << 8);

  uint16_t header_frag_crc = (uint16_t)stream_header[2] | 
                             ((uint16_t)stream_header[3] << 8);

  if (header_frag_length != in_size - STREAM_HEADER_SIZE - 1) {
    return STREAM_BAD_SIZE;
  }

  if (header_frag_crc != crc) {
    return STREAM_BAD_CRC;
  }

  *size = header_frag_length;
  return STREAM_SUCCESS;
}

/* Encodes a fragment in a way suitable for an unreliably byte oriented
 * transport. A stream header is prepended that contains a length and CRC, and
 * the result is COBS-encoded to contain no zeros. This payload is suitable for
 * sending with nulls as a frame delimiter.
 */
stream_result_t encode_cobs_fragment(
    uint16_t in_size, 
    const uint8_t fragment[in_size],
    uint16_t *out_size,
    uint8_t encoded_fragment[MAX_ENCODED_FRAGMENT_SIZE]) {

  /* Is there sufficient remaining space for a  */
  if (in_size > CONSOLE_MAX_FRAGMENT_SIZE) {
    return STREAM_BAD_SIZE;
  }

  *out_size = in_size + STREAM_HEADER_SIZE + 1;

  /* Copy the payload into place while computing the CRC */
  uint16_t pdu_crc = CRC16_INIT;
  for (int src_idx = 0; src_idx < in_size; src_idx++) {
    const int dst_idx = src_idx + STREAM_HEADER_SIZE + 1;
    const uint8_t byte = fragment[src_idx];
    encoded_fragment[dst_idx] = byte;
    crc16_add_byte(&pdu_crc, byte);
  }

  /* Encode PDU CRC */
  encoded_fragment[3] = pdu_crc & 0xFF;
  encoded_fragment[4] = (pdu_crc >> 8) & 0xFF;

  /* Encode PDU Length */
  encoded_fragment[1] = in_size & 0xFF;
  encoded_fragment[2] = (in_size >> 8) & 0xFF;

  /* Encode COBS in place starting at the end */
  uint8_t cobs_pos = *out_size;
  for (int i = cobs_pos - 1; i > 0; i--) {
    if (encoded_fragment[i] == 0) {
      /* Replace this zero with the distance from here to the cobs position */
      encoded_fragment[i] = cobs_pos - i;
      cobs_pos = i;
    } 
  }
  encoded_fragment[0] = cobs_pos;
  return STREAM_SUCCESS;
}

#ifdef UNITTEST
START_TEST(test_decode_cobs_fragment_in_place_failure_checking) {
  {
    /* test an empty input */
    uint8_t empty[MAX_ENCODED_FRAGMENT_SIZE] = {};
    uint16_t size = 0;
    ck_assert_int_eq(decode_cobs_fragment_in_place(&size, empty), STREAM_BAD_SIZE);
    ck_assert_int_eq(size, 0);
  }

  {
    /* test a 4-byte input */
    uint8_t four[MAX_ENCODED_FRAGMENT_SIZE] = {1, 2, 3, 4};
    uint16_t size = 4;
    ck_assert_int_eq(decode_cobs_fragment_in_place(&size, four), STREAM_BAD_SIZE);
    ck_assert_int_eq(size, 0);
  }

  {
    /* test a 5-byte input, implying an empty PDU, but the embedded size is
     * definitely wrong*/
    uint8_t five[MAX_ENCODED_FRAGMENT_SIZE] = {0x10, 0x11, 0x12, 0x13, 0x14};
    uint16_t size = 5;
    ck_assert_int_eq(decode_cobs_fragment_in_place(&size, five), STREAM_BAD_SIZE);
    ck_assert_int_eq(size, 0);
  }

  {
    /* test an 8 byte input with the encoded size being 2, such that the
     * fragment size is wrong definitely wrong*/
    uint8_t eight[MAX_ENCODED_FRAGMENT_SIZE] = {
      0xFF, 0x02, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00
    };
    uint16_t size = 8;
    ck_assert_int_eq(decode_cobs_fragment_in_place(&size, eight), STREAM_BAD_SIZE);
    ck_assert_int_eq(size, 0);
  }

  {
    /* test with 4-byte PDU with bad crc */
    uint8_t fragment[MAX_ENCODED_FRAGMENT_SIZE] = {
      0x2, 0x04, 0x01, 0x01, 0x05,
      0x10, 0x11, 0x12, 0x13
    };
    uint16_t size = 9;
    ck_assert_int_eq(decode_cobs_fragment_in_place(&size, fragment), STREAM_BAD_CRC);
    ck_assert_int_eq(size, 0);
  }
} END_TEST

START_TEST(test_decode_cobs_fragment_in_place) {
  {
    /* test with 4-byte PDU with no zeros */
    uint8_t fragment[MAX_ENCODED_FRAGMENT_SIZE] = {
      0x2, 0x04, 0x07, 0xD3, 0x98,
      0x10, 0x11, 0x12, 0x13
    };
    uint16_t size = 9;
    ck_assert_int_eq(decode_cobs_fragment_in_place(&size, fragment), STREAM_SUCCESS);
    ck_assert_int_eq(size, 4);
    const uint8_t expected[] = {0x10, 0x11, 0x12, 0x13};
    ck_assert_mem_eq(fragment, expected, 4);
  }

  {
    /* test with 4-byte PDU with no zeros */
    const uint8_t fragment_before_cobs[] __attribute__((unused)) = {
      0x4, 0x0, 0x7B, 0x2F,
      0x10, 0x11, 0x0, 0x12
    };

    uint8_t fragment[MAX_ENCODED_FRAGMENT_SIZE] = {
      0x2, 0x04, 0x05, 0x7B, 0x2F,
      0x10, 0x11, 0x02, 0x12
    };

    uint16_t size = 9;
    ck_assert_int_eq(decode_cobs_fragment_in_place(&size, fragment), STREAM_SUCCESS);
    ck_assert_int_eq(size, 4);
    const uint8_t expected[] = {0x10, 0x11, 0x0, 0x12};
    ck_assert_mem_eq(fragment, expected, 4);
  }

} END_TEST

START_TEST(test_encode_decode) {

  const uint8_t payload[] = { 0x05, 0x06, 0x00, 0x08,
                            0xA1, 0xA2, 0xA3, 0xA4,
                            0x00, 0x01, 0x02, 0x03 };

  uint8_t encoded[MAX_ENCODED_FRAGMENT_SIZE];
  uint16_t encoded_size;
  ck_assert_int_eq(encode_cobs_fragment(sizeof(payload),
                                        payload,
                                        &encoded_size,
                                        encoded), STREAM_SUCCESS);

  ck_assert_int_eq(encoded_size, sizeof(payload) + 5);

  ck_assert_int_eq(decode_cobs_fragment_in_place(&encoded_size,
                                                 encoded), STREAM_SUCCESS);

  ck_assert_int_eq(encoded_size, sizeof(payload));
  ck_assert_mem_eq(encoded, payload, encoded_size);
} END_TEST

static uint8_t outbuffer[1024];
static size_t outbuffer_size = 0;
static inline bool test_write_fn(size_t n, uint8_t buffer[n], void *arg) {
  memcpy(outbuffer + outbuffer_size, buffer, n);
  outbuffer_size += n;
  return true;
}


START_TEST(test_cobs_encode) {

  {
    uint8_t nozeros[] = {1, 2, 3, 4, 5, 0};
    struct cobs_encoder enc = { 0 };

    outbuffer_size = 0;
    cobs_encode(&enc, sizeof(nozeros), nozeros, test_write_fn, NULL);
    const uint8_t expected[] = {6, 1, 2, 3, 4, 5};
    ck_assert_int_eq(outbuffer_size, 6);
    ck_assert_mem_eq(expected, outbuffer, outbuffer_size);

  }

  {
    uint8_t somezeros[] = {0, 1, 2, 0, 3, 4, 0, 0};
    struct cobs_encoder enc = { 0 };

    outbuffer_size = 0;
    cobs_encode(&enc, sizeof(somezeros), somezeros, test_write_fn, NULL);
    const uint8_t expected[] = {1, 3, 1, 2, 3, 3, 4, 1};
    ck_assert_int_eq(outbuffer_size, 8);
    ck_assert_mem_eq(expected, outbuffer, outbuffer_size);

  }

  {
    uint8_t zero1[] = {0, 1, 2, 0};
    uint8_t zero2[] = {3, 4, 0, 0};
    struct cobs_encoder enc = { 0 };

    outbuffer_size = 0;
    cobs_encode(&enc, sizeof(zero1), zero1, test_write_fn, NULL);
    cobs_encode(&enc, sizeof(zero2), zero2, test_write_fn, NULL);
    const uint8_t expected[] = {1, 3, 1, 2, 3, 3, 4, 1};
    ck_assert_int_eq(outbuffer_size, 8);
    ck_assert_mem_eq(expected, outbuffer, outbuffer_size);

  }
  {
    uint8_t large[260]; // 259 1s and then a null terminator
    for (int i = 0; i < sizeof(large) - 1; i++) {
      large[i] = 1;
    }
    large[259] = 0;

    struct cobs_encoder enc = { 0 };
    outbuffer_size = 0;
    cobs_encode(&enc, sizeof(large), large, test_write_fn, NULL);
    ck_assert_int_eq(outbuffer_size, 261);

  }

} END_TEST



TCase *setup_stream_tests() {
  TCase *stream_tests = tcase_create("stream");
  tcase_add_test(stream_tests, test_decode_cobs_fragment_in_place_failure_checking);
  tcase_add_test(stream_tests, test_decode_cobs_fragment_in_place);
  tcase_add_test(stream_tests, test_encode_decode);

  tcase_add_test(stream_tests, test_cobs_encode);

  return stream_tests;
}
#endif
