#include "calculations.h"
#include "config.h"
#include "decoder.h"
#include "platform.h"
#include "flash.h"
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


  BYTE fmt_work_buf[FF_MAX_SS];
  FATFS fs;
  FIL fil;
  FRESULT res = f_mkfs("1:", 0, fmt_work_buf, sizeof(fmt_work_buf));
  f_mount(&fs, "1:", 0);
  res = f_open(&fil, "1:/hello.txt", FA_CREATE_NEW | FA_WRITE);

  sensors_process(SENSOR_CONST);
  while (1) {
//    console_process();
    size_t console_feed_line(uint8_t *dest, size_t bsize);
    uint8_t buf[1024];
    size_t len = console_feed_line(buf, 1024);
    UINT bw;
    f_write(&fil, buf, len, &bw);
    f_sync(&fil);
  }

  return 0;
}
