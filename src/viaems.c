#include "calculations.h"
#include "config.h"
#include "decoder.h"
#include "flash.h"
#include "platform.h"
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
  sensors_process(SENSOR_CONST);
  bool open_file = logstorage_init();

  while (true) { 
    char buf[512];
    size_t len = console_feed_line(buf, 512);
    if (open_file) {
      open_file = logstorage_log_write(buf, len);
    }
  }

  return 0;
}
