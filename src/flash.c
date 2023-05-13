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

static uint8_t *find_response(uint8_t *b, int32_t len) {
  int32_t i = 0;
  for (; i < len; i++) {
    if (b[i] != 0xff) {
      return &b[i];
    }
  }
  return NULL;
}

static uint8_t sdcard_command(uint8_t cmd, uint32_t arg, uint8_t crc) {
  uint8_t arg1 = arg >> 24;
  uint8_t arg2 = (arg & 0xff0000) >> 16;
  uint8_t arg3 = (arg & 0xff00) >> 8;
  uint8_t arg4 = (arg & 0xff);
  uint8_t tx[16] = {0x40 | cmd, arg1, arg2, arg3, arg4, crc};
  uint8_t rx[16];
  for (uint32_t i = 6; i < sizeof(tx); i++) {
    tx[i] = 0xff;
  }
  sdcard_spi_chipselect(true);
  sdcard_spi_transaction(tx, rx, sizeof(rx));
  sdcard_spi_chipselect(false);
  uint8_t *r1 = find_response(rx + 6, sizeof(rx) - 6);
  if (r1) {
    return *r1;
  } else {
    return 0xff;
  }
}

uint8_t sdcard_scratchpad[1024];
static bool sdcard_data_read_command(uint8_t cmd, uint32_t arg, uint8_t *dest, size_t len) {

  /* Optimistically assume minimal delay between command and data response, 32
   * bytes at most */
  size_t readlen = len + 32;
  memset(sdcard_scratchpad, 0xff, readlen);

  uint8_t arg1 = arg >> 24;
  uint8_t arg2 = (arg & 0xff0000) >> 16;
  uint8_t arg3 = (arg & 0xff00) >> 8;
  uint8_t arg4 = (arg & 0xff);
  uint8_t cmdseq[6] = {0x40 | cmd, arg1, arg2, arg3, arg4, 0x0};
  memcpy(sdcard_scratchpad, cmdseq, sizeof(cmdseq));

  sdcard_spi_chipselect(true);
  sdcard_spi_transaction(sdcard_scratchpad, sdcard_scratchpad, readlen);
  uint8_t *r1 = find_response(sdcard_scratchpad + 6, readlen - 6);
  if (!r1 || (*r1 != 0)) {
    sdcard_spi_chipselect(false);
    return false;
  }

  int32_t remaining_scratch = readlen - (r1 + 1 - sdcard_scratchpad);
  uint8_t *token = find_response(r1 + 1, remaining_scratch);
  if (!token) {
    /* TODO: Do another transaction, loop until token found */
  }
  if ((*token & 0xe0) == 0x00) { /* Error token */
    sdcard_spi_chipselect(false);
    return false;
  }
  if (*token != 0xfe) { /* Not a data token */
    sdcard_spi_chipselect(false);
    return false;
  }
  remaining_scratch = readlen - (token + 1 - sdcard_scratchpad);

  if (remaining_scratch > 0) {
    size_t amt_to_copy = ((size_t)remaining_scratch > len) ? len : (size_t)remaining_scratch;  
    memcpy(dest, token + 1, amt_to_copy);
    len -= amt_to_copy;
    dest += amt_to_copy;
  }

  if (len > 0) { /* Fetch more */
    readlen = len + 16;
    memset(sdcard_scratchpad, 0xff, readlen);
    sdcard_spi_transaction(sdcard_scratchpad, sdcard_scratchpad, readlen);
    memcpy(dest, sdcard_scratchpad, len);
  }
  sdcard_spi_chipselect(false);
  return true;
}


struct sdcard sdcard_init() {
  struct sdcard sdcard = {0};

  /* Go to idle */
  uint8_t resp = sdcard_command(0, 0, 0x95);
  if (resp != 0x1) {
    return sdcard;
  }

  bool initialized = false;
  while (!initialized) {
    /* Initialize with CMD8 */
    resp = sdcard_command(8, 0x1aa, 0x87);
    if (resp != 0x1) {
      return sdcard;
    }

    /* Prefix CMD55 */
    resp = sdcard_command(55, 0, 0x65);
    if (resp & 0x4) { /* TODO: Invalid command, handle with CMD1 */
      return sdcard;
    }

    resp = sdcard_command(41, 0x40000000, 0x77);
    if (resp == 0) {
      break;
    }
  }

  uint8_t csp[16];
  bool success = sdcard_data_read_command(9, 0, csp, 16);

  return (struct sdcard){.valid = success, .num_sectors=resp};
}
