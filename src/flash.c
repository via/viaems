#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "flash.h"
#include "platform.h"

#include "ff.h"
#include "diskio.h"

static uint8_t spi_tx_buffer[520];
static uint8_t spi_rx_buffer[520];

struct flash flash_init() {

  const uint8_t query_cmd[4] = { 0x9f, 0x00, 0x00, 0x00 };
  uint8_t query_resp[4];
  flash_spi_transaction(query_cmd, query_resp, sizeof(query_cmd));

  const uint8_t devid = query_resp[1];
  const uint8_t memid = query_resp[2];
  const uint8_t capacity = query_resp[3];
  if (devid == 0xef && memid == 0x40 && capacity > 0x13 && capacity < 0x20) {
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
  return (struct flash){ .valid = false };
}

bool flash_read(struct flash *f,
                uint8_t *dest,
                uint32_t address,
                size_t length) {
  if (!f->valid) {
    return false;
  }
  if (length > 512) {
    return false;
  }
  if (address + length >= f->capacity_bytes) {
    return false;
  }

  uint8_t cmd[] = {
    0x03,
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
  const uint8_t read_sr1_cmd[] = { 0x5, 0x00 };
  uint8_t read_sr1_resp[2];

  flash_spi_transaction(read_sr1_cmd, read_sr1_resp, sizeof(read_sr1_cmd));
  return (read_sr1_resp[1] & 0x1) != 0;
}

bool flash_enable_write(struct flash *f) {
  const uint8_t write_enable_cmd[] = { 0x6 };
  uint8_t write_enable_resp[1];

  flash_spi_transaction(
    write_enable_cmd, write_enable_resp, sizeof(write_enable_cmd));
  while (flash_is_busy(f))
    ;
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

  uint8_t cmd[] = {
    0x20,
    (address & 0xff0000) >> 16,
    (address & 0xff00) >> 8,
    (address & 0xff),
  };

  uint8_t resp[4];

  flash_spi_transaction(cmd, resp, sizeof(cmd));

  while (flash_is_busy(f))
    ;
  return true;
}

bool flash_write(struct flash *f,
                 const uint8_t *src,
                 uint32_t address,
                 size_t length) {
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

  uint8_t cmd[] = {
    0x02,
    (address & 0xff0000) >> 16,
    (address & 0xff00) >> 8,
    (address & 0xff),
  };
  memcpy(spi_tx_buffer, cmd, sizeof(cmd));
  memcpy(spi_tx_buffer + sizeof(cmd), src, length);
  flash_spi_transaction(spi_tx_buffer, spi_rx_buffer, sizeof(cmd) + length);

  while (flash_is_busy(f))
    ;
  return true;
}

static uint8_t sdcard_command(uint8_t cmd, uint32_t arg, uint8_t crc) {
  uint8_t arg1 = arg >> 24;
  uint8_t arg2 = (arg & 0xff0000) >> 16;
  uint8_t arg3 = (arg & 0xff00) >> 8;
  uint8_t arg4 = (arg & 0xff);
  uint8_t tx[16] = { 0x40 | cmd, arg1, arg2, arg3, arg4, crc };
  uint8_t rx[16];
  for (uint32_t i = 6; i < sizeof(tx); i++) {
    tx[i] = 0xff;
  }
  sdcard_spi_chipselect(true);
  sdcard_spi_transaction(tx, rx, sizeof(rx));
  sdcard_spi_chipselect(false);
  for (int i = 6; i < sizeof(rx); i++) {
    if ((rx[i] & 0x80) == 0) {
      return rx[i];
    }
  }
  return 0xff;
}

struct slice {
  uint8_t *rxstorageptr;
  uint8_t *txstorageptr;
  size_t storagelen;
  uint8_t *ptr;
  size_t len;
};

static void slice_repopulate(struct slice *s, size_t chunksize) {
    s->len = chunksize;
    s->ptr = s->rxstorageptr;
    memset(s->txstorageptr, 0xff, chunksize);
    sdcard_spi_transaction(s->txstorageptr, s->rxstorageptr, chunksize);
}

static uint8_t slice_next_byte(struct slice *s, size_t chunksize) {
  if (s->len == 0) {
    slice_repopulate(s, chunksize);
  }
  uint8_t result = *s->ptr;
  s->len--;
  s->ptr++;
  return result;
}

static void slice_range(struct slice *s, uint8_t *dest, size_t len, size_t chunksize) {
  do {
    if (s->len == 0) {
      slice_repopulate(s, chunksize);
    }
    size_t amt_to_copy = (len > s->len) ? s->len : len;
    memcpy(dest, s->ptr, amt_to_copy);
    s->len -= amt_to_copy;
    s->ptr += amt_to_copy;
    dest += amt_to_copy;
    len -= amt_to_copy;
  } while (len > 0);
}

static void slice_skip(struct slice *s, size_t n_skip, size_t chunksize) {
  while (n_skip > 0) {
    slice_next_byte(s, chunksize);
    n_skip--;
  }
}

uint8_t sdcard_rxbuffer[1024];
uint8_t sdcard_txbuffer[1024];
static bool sdcard_data_read_command(uint8_t cmd,
                                     uint32_t arg,
                                     uint8_t *dest,
                                     size_t len) {

  /* Optimistically assume minimal delay between command and data response, 32
   * bytes at most */
  size_t readlen = len + 32;
  memset(sdcard_rxbuffer, 0xff, readlen);

  uint8_t arg1 = arg >> 24;
  uint8_t arg2 = (arg & 0xff0000) >> 16;
  uint8_t arg3 = (arg & 0xff00) >> 8;
  uint8_t arg4 = (arg & 0xff);
  uint8_t cmdseq[6] = { 0x40 | cmd, arg1, arg2, arg3, arg4, 0x0 };
  memcpy(sdcard_rxbuffer, cmdseq, sizeof(cmdseq));

  sdcard_spi_chipselect(true);
  sdcard_spi_transaction(sdcard_rxbuffer, sdcard_rxbuffer, readlen);

  struct slice buf = {
    .rxstorageptr = sdcard_rxbuffer,
    .txstorageptr = sdcard_txbuffer,
    .storagelen = sizeof(sdcard_txbuffer),
    .len = readlen,
    .ptr = sdcard_rxbuffer,
  };
  slice_skip(&buf, 6, 32); /* Start search after command */

  int r1_retries = 1024;
  uint8_t byte;
  for (; r1_retries > 0; r1_retries--) {
    byte = slice_next_byte(&buf, 64);
    if ((byte & 0x80) == 0) {
      break;
    }
  }
  if (r1_retries == 0) { /* Exhausted attempts */
    sdcard_spi_chipselect(false);
    itm_debug("read: failed to find r1!\n");
    return false;
  }
  if (byte != 0) {
    sdcard_spi_chipselect(false);
    itm_debug("read: r1 response indicates error\n");
    while(1);
    return false;
  }

  int token_retries = 2048;
  for (; token_retries > 0; token_retries--) {
    byte = slice_next_byte(&buf, 512);
    if (byte != 0xff) {
        break;
    }
  }
  if (token_retries == 0) { /* Exhausted attempts */
    sdcard_spi_chipselect(false);
    itm_debug("read: failed to find data token!\n");
    return false;
  }

  if ((byte & 0xe0) == 0x00) { /* Error token */
    itm_debug("read: received error token\n");
    sdcard_spi_chipselect(false);
    return false;
  }
  if (byte != 0xfe) { /* Not a data token */
    itm_debug("read: received invalid token\n");
    sdcard_spi_chipselect(false);
    return false;
  }

  slice_range(&buf, dest, len, 512); /* Primary read */
  slice_skip(&buf, 2, 16); /* CRC bytes */

  sdcard_spi_chipselect(false);
  itm_debug("read: success\n");
  return true;
}

static bool sdcard_data_write_command(uint8_t cmd,
                                      uint32_t arg,
                                      uint8_t *src,
                                      size_t len) {

  /* Optimistically assume minimal delay between command and data response, 32
   * bytes at most */
  memset(sdcard_txbuffer, 0xff, sizeof(sdcard_txbuffer));

  uint8_t arg1 = arg >> 24;
  uint8_t arg2 = (arg & 0xff0000) >> 16;
  uint8_t arg3 = (arg & 0xff00) >> 8;
  uint8_t arg4 = (arg & 0xff);
  uint8_t cmdseq[6] = { 0x40 | cmd, arg1, arg2, arg3, arg4, 0x0 };
  memcpy(sdcard_txbuffer, cmdseq, sizeof(cmdseq));

  /* Command response must be within first 16 bytes, so start data packet at
   * byte 24 */
  sdcard_txbuffer[24] = 0xfe; /* Data token */
  memcpy(&sdcard_txbuffer[25], src, len);

  sdcard_spi_chipselect(true);
  sdcard_spi_transaction(
    sdcard_txbuffer, sdcard_rxbuffer, sizeof(sdcard_rxbuffer));

  struct slice buf = {
    .rxstorageptr = sdcard_rxbuffer,
    .txstorageptr = sdcard_txbuffer,
    .storagelen = sizeof(sdcard_txbuffer),
    .len = sizeof(sdcard_txbuffer),
    .ptr = sdcard_rxbuffer,
  };
  slice_skip(&buf, 6, 512); /* Start search after command */

  int r1_retries = 1024;
  uint8_t byte;
  for (; r1_retries > 0; r1_retries--) {
    byte = slice_next_byte(&buf, 64);
    if ((byte & 0x80) == 0) {
      break;
    }
  }
  if (r1_retries == 0) { /* Exhausted attempts */
    sdcard_spi_chipselect(false);
    itm_debug("write: failed to find r1!\n");
    return false;
  }
  if (byte != 0) {
    sdcard_spi_chipselect(false);
    itm_debug("write: r1 response indicates error\n");
    while(1);
    return false;
  }

  int response_retries = 16384;
  for (; response_retries > 0; response_retries--) {
    byte = slice_next_byte(&buf, 512);
    if ((byte & 0xe0) == 0) { /* Error token */
      sdcard_spi_chipselect(false);
      itm_debug("write: failed to find response token!\n");
      return false;
    }
    if ((byte & 0x11) == 1) {
      if ((byte & 0x1f) != 0x05) { /* Not accepted */
        itm_debug("write: received invalid token\n");
        sdcard_spi_chipselect(false);
        while (1);
        return false;
      }
      break;
    }
  }
  if (response_retries == 0) { /* Exhausted attempts */
    itm_debug("write: timed out\n");
    sdcard_spi_chipselect(false);
    return false;
  }

  int busywait_retries = 500000;
  for (; busywait_retries > 0; busywait_retries--) {
    byte = slice_next_byte(&buf, 512);
    if (byte != 0) {
      break;
    }
  }
  if (busywait_retries == 0) { /* Exhausted attempts */
    itm_debug("write: busy timed out\n");
    sdcard_spi_chipselect(false);
    return false;
  }

  itm_debug("write: success\n");
  sdcard_spi_chipselect(false);
  return true;
}

static bool sdcard_data_write_multiple(uint32_t arg,
                                      uint8_t *src,
                                      size_t n_sectors) {

  const uint8_t cmd = 25;
  /* Optimistically assume minimal delay between command and data response, 32
   * bytes at most */
  memset(sdcard_txbuffer, 0xff, sizeof(sdcard_txbuffer));

  uint8_t arg1 = arg >> 24;
  uint8_t arg2 = (arg & 0xff0000) >> 16;
  uint8_t arg3 = (arg & 0xff00) >> 8;
  uint8_t arg4 = (arg & 0xff);
  uint8_t cmdseq[6] = { 0x40 | cmd, arg1, arg2, arg3, arg4, 0x0 };
  memcpy(sdcard_txbuffer, cmdseq, sizeof(cmdseq));

  /* Send write multiple command */
  sdcard_spi_chipselect(true);
  sdcard_spi_transaction(
    sdcard_txbuffer, sdcard_rxbuffer, 16);

  struct slice buf = {
    .rxstorageptr = sdcard_rxbuffer,
    .txstorageptr = sdcard_txbuffer,
    .storagelen = sizeof(sdcard_txbuffer),
    .len = 16,
    .ptr = sdcard_rxbuffer,
  };
  slice_skip(&buf, sizeof(cmdseq), 16); /* Start search after command */

  int r1_retries = 1024;
  uint8_t byte;
  for (; r1_retries > 0; r1_retries--) {
    byte = slice_next_byte(&buf, 32);
    if ((byte & 0x80) == 0) {
      break;
    }
  }
  if (r1_retries == 0) { /* Exhausted attempts */
    sdcard_spi_chipselect(false);
    itm_debug("mwrite: failed to find r1!\n");
    return false;
  }
  if (byte != 0) {
    sdcard_spi_chipselect(false);
    itm_debug("mwrite: r1 response indicates error\n");
    while(1);
    return false;
  }

  while (n_sectors > 0) {
    itm_debug("mwrite: new sector\n");
    /* Write a sector */
    sdcard_txbuffer[0] = 0xfc; /* multiple-write */
    memcpy(&sdcard_txbuffer[1], src, 512);
    sdcard_txbuffer[513] = 0x0;
    sdcard_txbuffer[514] = 0x0;
    sdcard_spi_transaction(sdcard_txbuffer, sdcard_rxbuffer, 515); /* Full data packet */

    buf.len = 0;

    int response_retries = 2048;
    for (; response_retries > 0; response_retries--) {
      byte = slice_next_byte(&buf, 16);
      if ((byte & 0xe0) == 0) { /* Error token */
        sdcard_spi_chipselect(false);
        itm_debug("write: failed to find response token!\n");
        while(1);
        return false;
      }
      if ((byte & 0x11) == 1) {
        if ((byte & 0x1f) != 0x05) { /* Not accepted */
          itm_debug("write: received invalid token\n");
          sdcard_spi_chipselect(false);
          while(1);
          return false;
        }
        break;
      }
    }
    if (response_retries == 0) { /* Exhausted attempts */
      itm_debug("write: timed out\n");
      sdcard_spi_chipselect(false);
      return false;
    }

    int busywait_retries = 500000;
    for (; busywait_retries > 0; busywait_retries--) {
      byte = slice_next_byte(&buf, 128);
      if (byte != 0) {
        break;
      }
    }
    if (busywait_retries == 0) { /* Exhausted attempts */
      itm_debug("write: busy timed out\n");
      sdcard_spi_chipselect(false);
      return false;
    }

    src += 512;
    n_sectors -= 1;
  }

  /* send stop command */
  sdcard_txbuffer[0] = 0xfd;
  sdcard_txbuffer[1] = 0xff;
  sdcard_txbuffer[2] = 0xff;
  sdcard_txbuffer[3] = 0xff;
  sdcard_spi_transaction(sdcard_txbuffer, sdcard_rxbuffer, 4);
  buf.len = 0;

  int busywait_retries = 500000;
  for (; busywait_retries > 0; busywait_retries--) {
    byte = slice_next_byte(&buf, 128);
    if (byte != 0) {
      break;
    }
  }
  if (busywait_retries == 0) { /* Exhausted attempts */
    itm_debug("mwrite: final busy timed out\n");
    sdcard_spi_chipselect(false);
    return false;
  }


  itm_debug("write: success\n");
  sdcard_spi_chipselect(false);
  return true;
}

struct sdcard sdcard_init() {
  struct sdcard sdcard = { 0 };
  sdcard_spi_highspeed(false);

  /* 74+ cycles with CS deasserted */
  memset(sdcard_rxbuffer, 0xff, 32);
  sdcard_spi_transaction(sdcard_rxbuffer, sdcard_rxbuffer, 32);

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

  sdcard_spi_highspeed(true);
  return (struct sdcard){ .valid = true, .num_sectors = 0 };
}

static struct sdcard sd = { 0 };

DSTATUS disk_status(
    BYTE pdrv /* Physical drive nmuber to identify the drive */
    ) {
  DSTATUS stat;
  int result;

  if (pdrv != 1) {
    return STA_NOINIT;
  }

  if (!sd.valid) {
    return STA_NOINIT;
  }
  return 0;
}

DSTATUS disk_initialize(
    BYTE pdrv /* Physical drive nmuber to identify the drive */
    ) {
  if (pdrv != 1) {
    return STA_NOINIT;
  }
  if (!sd.valid) {
    sd = sdcard_init();
  }
  return disk_status(pdrv);
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(BYTE pdrv, /* Physical drive nmuber to identify the drive */
    BYTE * buff,  /* Data buffer to store read data */
    LBA_t sector, /* Start sector in LBA */
    UINT count    /* Number of sectors to read */
    ) {
  if (pdrv != 1) {
    return RES_NOTRDY;
  }

  while (count > 0) {
    if (!sdcard_data_read_command(17, sector, buff, 512)) {
      return RES_ERROR;
    }
    buff += 512;
    sector += 1;
    count -= 1;
  }
  return RES_OK;
}

static int max_sector_write = 0;
DRESULT disk_write(
    BYTE pdrv,        /* Physical drive nmuber to identify the drive */
    const BYTE *buff, /* Data to be written */
    LBA_t sector,     /* Start sector in LBA */
    UINT count        /* Number of sectors to write */
    ) {
  DRESULT res;
  int result;

  if (pdrv != 1) {
    return RES_PARERR;
  }
  if (!sdcard_data_write_multiple(sector, buff, count)) {
    return RES_ERROR;
  }
  return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, /* Physical drive nmuber (0..) */
    BYTE cmd,  /* Control code */
    void *buff /* Buffer to send/receive control data */
    ) {
  DRESULT res;
  int result;

  if (pdrv != 1) {
    return RES_PARERR;
  }

  switch (cmd) {
    case CTRL_SYNC:
      return RES_OK;
    case GET_SECTOR_SIZE:
      *(WORD *)buff = 512;
      return RES_OK;
    case GET_SECTOR_COUNT:
      *(LBA_t *)buff = 10000000;
      return RES_OK;
    case GET_BLOCK_SIZE:
      *(DWORD *)buff = 1;
      return RES_OK;
    case CTRL_TRIM:
      return RES_OK;
  }

  return RES_PARERR;
}

DWORD get_fattime(void) {
  return 1;
}
