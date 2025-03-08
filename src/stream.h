
#ifndef STREAM_H
#define STREAM_H

#include <stdint.h>
#include <stdbool.h>

#define CONSOLE_MAX_FRAGMENT_SIZE 244

#define STREAM_HEADER_SIZE 4
#define MAX_ENCODED_FRAGMENT_SIZE (CONSOLE_MAX_FRAGMENT_SIZE + STREAM_HEADER_SIZE + 1)

static const uint16_t CRC16_INIT = 0x0;
uint16_t crc16_add_byte(uint16_t *crc, uint8_t byte);

typedef enum {
  STREAM_SUCCESS = 0,
  STREAM_BAD_SIZE = 1,
  STREAM_BAD_CRC = 2,
} stream_result_t;

stream_result_t decode_cobs_fragment_in_place(
    uint16_t *size, 
    uint8_t fragment[MAX_ENCODED_FRAGMENT_SIZE]);

#ifdef UNITTEST
#include <check.h>
TCase *setup_stream_tests(void);
#endif

#endif
