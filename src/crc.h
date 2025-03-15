#ifndef CRC_H
#define CRC_H

#include <stdint.h>

static const uint16_t CRC16_INIT = 0x0;
void crc16_add_byte(uint16_t *crc, uint8_t byte);
void crc16_finish(uint16_t *crc);

static const uint32_t CRC32_INIT = 0xffffffff;
void crc32_add_byte(uint32_t *crc, uint8_t byte);
void crc32_finish(uint32_t *crc);

#ifdef UNITTEST
#include <check.h>
TCase *setup_crc_tests(void);
#endif

#endif
