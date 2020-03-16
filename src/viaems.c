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
  console_init();
  platform_init(0, NULL);
  initialize_scheduler();

  assert(config_valid());
  set_test_trigger_rpm(6000);

  sensors_process(SENSOR_CONST);
  while (1) {
    stats_increment_counter(STATS_MAINLOOP_RATE);

    console_process();
    handle_fuel_pump();
    handle_boost_control();
    handle_idle_control();
    handle_check_engine_light();
  }

  return 0;
}
