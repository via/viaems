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

uint32_t before, after;
int main() {
  platform_load_config();
  decoder_init(&config.decoder);
  platform_init(0, NULL);

  assert(config_valid());

  struct flash f = flash_init();
  uint8_t buf1[256];
  uint8_t buf2[256];

  flash_read(&f, buf1, 0, 256);
  flash_erase_sector(&f, 0);
  flash_erase_sector(&f, 0x1000);
  flash_erase_sector(&f, 0x2000);
  flash_erase_sector(&f, 0x3000);
  for (int i = 0; i < 256; i++) {
    buf1[i] -= 1;
  }
  before = cycle_count();
  for (int p = 0; p < 256; p++) {
    flash_write(&f, buf1, p * 256, 256);
  }
  after = cycle_count();

  flash_read(&f, buf2, 0x2000, 256);

  sensors_process(SENSOR_CONST);
  while (1) {
    console_process();
  }

  return 0;
}
