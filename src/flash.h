#ifndef _FLASH_H
#define _FLASH_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

struct flash {
  bool valid;
  size_t sector_size;  /* Erase size */
  size_t page_size;  /* Max size of written block */
  size_t capacity_bytes;

  size_t config_a_pgs;
  size_t config_b_pgs;
};

struct flash flash_init(void);
bool flash_read(struct flash *,  uint8_t *dest, uint32_t address, size_t length);
bool flash_erase_sector(struct flash *, uint32_t address);
bool flash_write(struct flash *, const uint8_t *src, uint32_t address, size_t length);


#endif
