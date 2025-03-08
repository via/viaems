#include <stdbool.h>
#include <stdint.h>

#include "stream.h"

/* Compute CRC-16-CCITT/KERMIT incrementally from bytes */
uint16_t crc16_add_byte(uint16_t *crc, uint8_t byte) {
  const uint16_t crc16_poly_rev = 0x8408;

  uint16_t current = *crc;

  current = current ^ (uint16_t)byte;
  for (int i = 0; i < 8; i++) {
    if (current & 1) {
      current = (current >> 1) ^ crc16_poly_rev;
    } else {
      current = current >> 1;
    }
  }
  *crc = current;
  return current;
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

START_TEST(test_crc16_add_byte) {
  uint16_t crc = CRC16_INIT;
  crc16_add_byte(&crc, 1);
  crc16_add_byte(&crc, 2);
  crc16_add_byte(&crc, 3);
  crc16_add_byte(&crc, 4);

  ck_assert_uint_eq(crc, 0xC54F);


} END_TEST

TCase *setup_stream_tests() {
  TCase *stream_tests = tcase_create("stream");
  tcase_add_test(stream_tests, test_decode_cobs_fragment_in_place_failure_checking);
  tcase_add_test(stream_tests, test_decode_cobs_fragment_in_place);
  tcase_add_test(stream_tests, test_crc16_add_byte);
  return stream_tests;
}
#endif
