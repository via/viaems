#include "calculations.h"
#include "config.h"
#include "decoder.h"
#include "flash.h"
#include "platform.h"
#include "scheduler.h"
#include "sensors.h"
#include "table.h"
#include "tasks.h"
#include "util.h"
#include <assert.h>
#include <stdio.h>

#include "ff.h"

uint32_t before, after;
int main() {
  platform_load_config();
  decoder_init(&config.decoder);
  platform_init(0, NULL);

  assert(config_valid());

  BYTE fmt_work_buf[512];
  FATFS fs;
  FIL fil;
  FRESULT res = f_mkfs("1:", 0, fmt_work_buf, sizeof(fmt_work_buf));
  f_mount(&fs, "1:", 0);
  res = f_open(&fil, "1:/hello.txt", FA_CREATE_NEW | FA_WRITE);

  sensors_process(SENSOR_CONST);
  int count = 0;
  uint8_t writebuf[16384];
  size_t writebuflen = 0;
  while (1) {
    //    console_process();
    size_t console_feed_line(uint8_t * dest, size_t bsize);
    writebuflen += console_feed_line(writebuf + writebuflen, 1024);

    if (writebuflen > 16000) {
      UINT bw;
      f_write(&fil, writebuf, writebuflen, &bw);
      writebuflen = 0;
    }
    if (count % 3000 == 0) {
      f_sync(&fil);
    }
    count += 1;
  }

  return 0;
}
