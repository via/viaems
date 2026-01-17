#ifndef CRC_H
#define CRC_H

#include <stddef.h>

struct crc32 {
  uint32_t value;
};

void crc32_init(struct crc32 *);
void crc32_add_byte(struct crc32 *crc, uint8_t byte);
void crc32_add_bytes(struct crc32 *crc, size_t n_bytes, const uint8_t bytes[n_bytes]);
uint32_t crc32_finish(struct crc32 *crc);

#ifdef UNITTEST
#include <check.h>
TCase *setup_crc_tests(void);
#endif

#endif
