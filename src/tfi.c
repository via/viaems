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
#include "tasks.h"


int main() {
  platform_load_config();
  decoder_init(&config.decoder);
  console_init();
  platform_init(0, NULL);
  initialize_scheduler();

  enable_test_trigger(FORD_TFI, 2000);

  while (1) {                   
    stats_increment_counter(STATS_MAINLOOP_RATE);

    console_process();
    handle_fuel_pump();
    handle_boost_control();
    handle_idle_control();
  }

  return 0;
}

