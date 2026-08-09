#include "s32k1xx.h"
#include "crc.h"

void crc32_init(struct crc32 *crc) {
  *CRC_CTRL = CRC_CTRL_TOT(2) | // Swap bits, but also swap bytes because we're writing little endian values
              CRC_CTRL_TOTR(2) | // Swap the final read
              CRC_CTRL_FXOR | // XOR the result
              CRC_CTRL_WAS |  // Nexts write will be a seed
              CRC_CTRL_TCRC; // 32 bit crc

  *CRC_GPOLY = 0x04C11DB7;
  *CRC_DATA = 0xFFFFFFFF;
  *CRC_CTRL &= ~CRC_CTRL_WAS; // Start writing data
}

void crc32_add_byte(struct crc32 *crc, uint8_t byte) {
  *CRC_DATA8 = byte;
}

void crc32_add_bytes(struct crc32 *crc, size_t len, const uint8_t bytes[len]) {

  const uint8_t *src = bytes;
  size_t remaining = len;
  while ((remaining > 0) && ((uint32_t)src & 0x3) != 0) {
    crc32_add_byte(crc, *src);
    src++;
    remaining--;
  }

  // src is now 32 bit aligned
  while (remaining >= 4) {
    const uint32_t *src32 = (const uint32_t *)src;
    uint32_t val = *src32;
    *CRC_DATA = val;

    remaining -= 4;
    src += 4;
  }

  while (remaining > 0) {
    *CRC_DATA8 = *src;
    src++;
    remaining--;
  }
}

uint32_t crc32_finish(struct crc32 *crc) {
  return *CRC_DATA;
}

