#include "crc.h"

/* Compute CRC-16-CCITT/KERMIT incrementally from bytes */
void crc16_add_byte(uint16_t *crc, uint8_t byte) {
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
}

void crc16_finish(uint16_t *crc) {
  (void)crc;
  /* Nothing to do */
}

/* Compute CRC-32/HDLC:
 */
void crc32_add_byte(uint32_t *crc, uint8_t byte) {
  const uint32_t crc32_poly_rev = 0xedb88320;

  uint32_t current = *crc;

  current = current ^ (uint32_t)byte;
  for (int i = 0; i < 8; i++) {
    if (current & 1) {
      current = (current >> 1) ^ crc32_poly_rev;
    } else {
      current = current >> 1;
    }
  }
  *crc = current;
  return current;
}

void crc32_finish(uint32_t *crc) {
  *crc ^= 0xffffffff;
}


#ifdef UNITTEST
START_TEST(test_crc16_add_byte) {
  uint16_t crc = CRC16_INIT;
  crc16_add_byte(&crc, 1);
  crc16_add_byte(&crc, 2);
  crc16_add_byte(&crc, 3);
  crc16_add_byte(&crc, 4);
  crc16_finish(&crc);

  ck_assert_uint_eq(crc, 0xC54F);
} END_TEST

START_TEST(test_crc32_add_byte) {
  {
    uint32_t crc = CRC32_INIT;
    crc32_add_byte(&crc, 1);
    crc32_add_byte(&crc, 2);
    crc32_add_byte(&crc, 3);
    crc32_add_byte(&crc, 4);
    crc32_finish(&crc);
    ck_assert_uint_eq(crc, 0xb63cfbcd);
  }

  {
    uint32_t crc = CRC32_INIT;
    crc32_add_byte(&crc, 8);
    crc32_add_byte(&crc, 9);
    crc32_add_byte(&crc, 10);
    crc32_add_byte(&crc, 11);
    crc32_add_byte(&crc, 12);
    crc32_finish(&crc);
    ck_assert_uint_eq(crc, 0x6c8fe360);
  }
} END_TEST

TCase *setup_crc_tests() {
  TCase *crc_tests = tcase_create("crc");
  tcase_add_test(crc_tests, test_crc16_add_byte);
  tcase_add_test(crc_tests, test_crc32_add_byte);

  return crc_tests;
}

#endif
