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

void decoder_updated() {
  if (!config.decoder.valid) {
    invalidate_scheduled_events();
  } else {
    calculate_ignition();
    calculate_fueling();
    schedule_events();
  }


int main() {
  platform_load_config();
  decoder_init(&config.decoder);
  platform_init(0, NULL);
  initialize_scheduler();

  assert(config_valid());

  set_test_trigger_rpm(1000);

  sensors_process(SENSOR_CONST);
  while (1) {
    stats_increment_counter(STATS_MAINLOOP_RATE);
    console_process();
  }

  return 0;
}
