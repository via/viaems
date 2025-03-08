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

#include "stream.h"

int main(int argc, char *argv[]) {
  platform_load_config();
  decoder_init(&config.decoder);
  platform_init(argc, argv);

  assert(config_valid());
  uint16_t bleh = CRC16_INIT;
  crc16_add_byte(&bleh, 1);
  crc16_add_byte(&bleh, 2);
  crc16_add_byte(&bleh, 3);
  crc16_add_byte(&bleh, 4);
  set_test_trigger_rpm(bleh);

  while (1) {
    console_process();
  }

  return 0;
}
