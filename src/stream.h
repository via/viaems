
#ifndef STREAM_H
#define STREAM_H

#include <stdint.h>
#include <stdbool.h>

#include "crc.h"

#define CONSOLE_MAX_FRAGMENT_SIZE 244

#define STREAM_HEADER_SIZE 4
#define MAX_ENCODED_FRAGMENT_SIZE (CONSOLE_MAX_FRAGMENT_SIZE + STREAM_HEADER_SIZE + 1)

typedef enum {
  STREAM_SUCCESS = 0,
  STREAM_BAD_SIZE = 1,
  STREAM_BAD_CRC = 2,
} stream_result_t;

stream_result_t decode_cobs_fragment_in_place(
    uint16_t *size, 
    uint8_t fragment[MAX_ENCODED_FRAGMENT_SIZE]);

stream_result_t encode_cobs_fragment(
    uint16_t in_size, 
    const uint8_t fragment[in_size],
    uint16_t *out_size, 
    uint8_t encoded_fragment[MAX_ENCODED_FRAGMENT_SIZE]);

#ifdef UNITTEST
#include <check.h>
TCase *setup_stream_tests(void);
#endif

#endif
