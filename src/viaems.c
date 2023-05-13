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

  struct sdcard s = sdcard_init();

  sensors_process(SENSOR_CONST);
  while (1) {
    console_process();
  }

  return 0;
}
