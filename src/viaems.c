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
#include "config_builtin.h"

int main() {
  restore_config_from_binary(cbor_configuration, sizeof(cbor_configuration));

  decoder_init(&config.decoder);
  platform_init(0, NULL);
  initialize_scheduler();

  assert(config_valid());

  sensors_process(SENSOR_CONST);
  while (1) {
    console_process();
  }

  return 0;
}
