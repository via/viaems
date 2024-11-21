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

#include "interface/types.pb.h"

int main(int argc, char *argv[]) {
  platform_load_config();
  decoder_init(&config.decoder);
  platform_init(argc, argv);

  assert(config_valid());
  set_test_trigger_rpm(3000);

  while (1) {
    console_process();
  }

  return 0;
}
