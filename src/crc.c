#include <stdint.h>
#include "crc.h"

#ifndef PLATFORM_HAS_CRC32

/* Compute CRC-32/HDLC:
 */
void crc32_init(struct crc32 *c) {
  c->value = 0xffffffff;
}

void crc32_add_byte(struct crc32 *crc, uint8_t byte) {
  const uint32_t crc32_poly_rev = 0xedb88320;

  uint32_t current = crc->value;

  current = current ^ (uint32_t)byte;
  for (int i = 0; i < 8; i++) {
    if (current & 1) {
      current = (current >> 1) ^ crc32_poly_rev;
    } else {
      current = current >> 1;
    }
  }
  crc->value = current;
}

uint32_t crc32_finish(struct crc32 *crc) {
  crc->value ^= 0xffffffff;
  return crc->value;
}

#ifdef UNITTEST

START_TEST(test_crc32_add_byte) {
  {
    struct crc32 crc;
    crc32_init(&crc);
    crc32_add_byte(&crc, 1);
    crc32_add_byte(&crc, 2);
    crc32_add_byte(&crc, 3);
    crc32_add_byte(&crc, 4);
    uint32_t result = crc32_finish(&crc);
    ck_assert_uint_eq(result, 0xb63cfbcd);
  }

  {
    struct crc32 crc;
    crc32_init(&crc);
    crc32_add_byte(&crc, 8);
    crc32_add_byte(&crc, 9);
    crc32_add_byte(&crc, 10);
    crc32_add_byte(&crc, 11);
    crc32_add_byte(&crc, 12);
    uint32_t result = crc32_finish(&crc);
    ck_assert_uint_eq(result, 0x6c8fe360);
  }
}
END_TEST

TCase *setup_crc_tests() {
  TCase *crc_tests = tcase_create("crc");
  tcase_add_test(crc_tests, test_crc32_add_byte);

  return crc_tests;
}

#endif // UNITTEST

#endif // PLATFORM_HAS_CRC32
