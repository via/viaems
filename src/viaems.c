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
#include "configuration.h"

int main() {
  //restore_config_from_binary(obj_stm32f4_configuration_cbor, sizeof(obj_stm32f4_configuration_cbor));
  restore_config_from_binary(obj_hosted_configuration_cbor, sizeof(obj_hosted_configuration_cbor));

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
