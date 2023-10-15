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
  platform_init(0, NULL);

  assert(config_valid());

  while (1);
  start_controllers();

  return 0;
}
