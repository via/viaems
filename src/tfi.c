#include <stdio.h>
#include "platform.h"
#include "util.h"
#include "decoder.h"
#include "scheduler.h"
#include "config.h"
#include "table.h"
#include "sensors.h"
#include "calculations.h"
#include "stats.h"


static void handle_fuel_pump() {
  static timeval_t last_valid = 0;
  /* If engine is turning, keep pump on */
  if (config.decoder.state != DECODER_NOSYNC) {
    last_valid = current_time();
    set_gpio(config.fueling.fuel_pump_pin, 1);
    return;
  }

  /* Allow 4 seconds of fueling */
  if (time_diff(current_time(), last_valid) > time_from_us(4000000)) {
    set_gpio(config.fueling.fuel_pump_pin, 0);
    /* Keep last valid 5 seconds behind to prevent rollover bugs */
    last_valid = time_diff(current_time(), time_from_us(5000000));
  } else {
    set_gpio(config.fueling.fuel_pump_pin, 1);
  }
}

static void handle_boost_control() {
}

static void handle_idle_control() {
}

static void low_priority_cycle() {
    console_process();
    handle_fuel_pump();
    handle_boost_control();
    handle_idle_control();
}

int main() {
  platform_load_config();
  decoder_init(&config.decoder);
  console_init();
  platform_init(0, NULL);
  initialize_scheduler();

  enable_test_trigger(FORD_TFI, 2000);

  while (1) {                   
    stats_increment_counter(STATS_MAINLOOP_RATE);
    low_priority_cycle();
  }

  return 0;
}

