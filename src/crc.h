#ifndef CRC_H
#define CRC_H

struct crc32 {
  uint32_t value;
};

void crc32_init(struct crc32 *);
void crc32_add_byte(struct crc32 *crc, uint8_t byte);
void crc32_add_word(struct crc32 *crc, uint32_t word);
uint32_t crc32_finish(struct crc32 *crc);

#ifdef UNITTEST
#include <check.h>
TCase *setup_crc_tests(void);
#endif

#endif
