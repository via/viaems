#include "calculations.h"
#include "config.h"
#include "decoder.h"
#include "platform.h"
#include "scheduler.h"
#include "sensors.h"
#include "table.h"
#include "controllers.h"
#include "util.h"
#include <assert.h>
#include <stdio.h>

int main() {
  platform_load_config();
  decoder_init(&config.decoder);

  assert(config_valid());

  start_controllers();

  return 0;
}
