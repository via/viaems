#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "platform.h"
#include "flash.h"

static uint8_t spi_tx_buffer[520];
static uint8_t spi_rx_buffer[520];

struct flash flash_init() {
  
  const uint8_t query_cmd[4] = {0x9f, 0x00, 0x00, 0x00};
  uint8_t query_resp[4];
  flash_spi_transaction(query_cmd, query_resp, sizeof(query_cmd));

  const uint8_t devid = query_resp[1];
  const uint8_t memid = query_resp[2];
  const uint8_t capacity = query_resp[3];
  if (devid == 0xef && memid == 0x40 &&
    capacity > 0x13 && capacity < 0x20) {
    /* Winbond SPI NOR flash */

    uint32_t capacity_bytes = 1 << capacity;
    return (struct flash){
      .valid = true,
      .sector_size = 4096,
      .page_size = 256,
      .capacity_bytes = capacity_bytes,
      .config_a_pgs = 16,
      .config_b_pgs = 16,
    };
  }
  return (struct flash){.valid = false};
  
}


bool flash_read(struct flash *f,  uint8_t *dest, uint32_t address, size_t length) {
  if (!f->valid) {
    return false;
  }
  if (length > 512) {
    return false;
  }
  if (address + length >= f->capacity_bytes) {
    return false;
  }

  uint8_t cmd[] = {0x03, 
                   (address & 0xff0000) >> 16,
                   (address & 0xff00) >> 8,
                   (address & 0xff),
                   };
  memset(spi_tx_buffer, 0, sizeof(cmd) + length);
  memcpy(spi_tx_buffer, cmd, sizeof(cmd));
  flash_spi_transaction(spi_tx_buffer, spi_rx_buffer, sizeof(cmd) + length);
  memcpy(dest, spi_rx_buffer + 4, length);
  return true;
}

bool flash_is_busy(struct flash *f) {
  const uint8_t read_sr1_cmd[] = {0x5, 0x00};
  uint8_t read_sr1_resp[2];

  flash_spi_transaction(read_sr1_cmd, read_sr1_resp, sizeof(read_sr1_cmd));
  return (read_sr1_resp[1] & 0x1) != 0; 
}

bool flash_enable_write(struct flash *f) {
  const uint8_t write_enable_cmd[] = {0x6};
  uint8_t write_enable_resp[1];

  flash_spi_transaction(write_enable_cmd, write_enable_resp, sizeof(write_enable_cmd));
  while (flash_is_busy(f));
  return true;
}

bool flash_erase_sector(struct flash *f, uint32_t address) {
  if (f->valid) { 
    return false;
  }
  if ((address & (f->sector_size - 1)) != 0) {
    /* Address must be on page boundary */
    return false;
  }
  if (address >= f->capacity_bytes) {
    return false;
  }

  flash_enable_write(f);

  uint8_t cmd[] = {0x20, 
                   (address & 0xff0000) >> 16,
                   (address & 0xff00) >> 8,
                   (address & 0xff),
                   };

  uint8_t resp[4];

  flash_spi_transaction(cmd, resp, sizeof(cmd));

  while(flash_is_busy(f));
  return true;
}

bool flash_write(struct flash *f, const uint8_t *src, uint32_t address, size_t length) {
  if (!f->valid) { 
    return false;
  }
  uint32_t start_page = address / f->page_size;
  uint32_t end_page = (address + length - 1) / f->page_size;

  if (start_page != end_page) {
    /* Full written range must be in single page */
    return false;
  }
  if ((address + length) >= f->capacity_bytes) {
    return false;
  }

  flash_enable_write(f);

  uint8_t cmd[] = {0x02, 
                   (address & 0xff0000) >> 16,
                   (address & 0xff00) >> 8,
                   (address & 0xff),
                   };
  memcpy(spi_tx_buffer, cmd, sizeof(cmd));
  memcpy(spi_tx_buffer + sizeof(cmd), src, length);
  flash_spi_transaction(spi_tx_buffer, spi_rx_buffer, sizeof(cmd) + length);

  while (flash_is_busy(f));
  return true;

}
