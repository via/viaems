#include "calculations.h"
#include "config.h"
#include "decoder.h"
#include "platform.h"
#include "scheduler.h"
#include "sensors.h"
#include "table.h"
#include "tasks.h"
#include "util.h"
#include <assert.h>
#include <stdio.h>

int main() {
  platform_load_config();
  decoder_init(&config.decoder);
  platform_init(0, NULL);

  assert(config_valid());
  set_test_trigger_rpm(1000);

  sensors_process(SENSOR_CONST);
  while (1) {
    console_process();
  }

  return 0;
}
