#include "calculations.h"
#include "config.h"
#include "decoder.h"
#include "platform.h"
#include "scheduler.h"
#include "sensors.h"
#include "stats.h"
#include "table.h"
#include "tasks.h"
#include "util.h"
#include <assert.h>
#include <stdio.h>

int main() {
  platform_load_config();
  decoder_init(&config.decoder);
  platform_init(0, NULL);
  initialize_scheduler();
  configure_knock(&config.knock[0], &goertzel_filters[0]);
  configure_knock(&config.knock[1], &goertzel_filters[1]);

  assert(config_valid());

  sensors_process(SENSOR_CONST);
  while (1) {
    stats_increment_counter(STATS_MAINLOOP_RATE);
    console_process();
  }

  return 0;
}
