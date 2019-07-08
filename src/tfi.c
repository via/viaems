#include <stdio.h>
#include <assert.h>
#include "platform.h"
#include "util.h"
#include "decoder.h"
#include "scheduler.h"
#include "config.h"
#include "table.h"
#include "sensors.h"
#include "calculations.h"
#include "stats.h"
#include "tasks.h"


int main() {
  platform_load_config();
  decoder_init(&config.decoder);
  console_init();
  platform_init(0, NULL);
  initialize_scheduler();

  assert(config_valid());

  enable_test_trigger(FORD_TFI, 2000);

  sensors_process(SENSOR_CONST);
  set_pwm(1, 0.55);
  while (1) {                   
    stats_increment_counter(STATS_MAINLOOP_RATE);

    console_process();
    handle_fuel_pump();
    handle_boost_control();
    handle_idle_control();

    adc_gather();
  }

  return 0;
}

