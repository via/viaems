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
#include <stdio.h>
#include <string.h>

static void do_fuel_calculation_bench() {
  uint64_t start = cycle_count();
  calculate_fueling();
  uint64_t end = cycle_count();
  printf("calculate_fueling: %llu ns\r\n", cycles_to_ns(end - start));
}

static void do_schedule_ignition_event_bench() {
  config.events[0].start.scheduled = 0;
  config.events[0].start.fired = 0;
  config.events[0].stop.scheduled = 0;
  config.events[0].stop.fired = 0;

  config.decoder.valid = 1;
  calculated_values.timing_advance = 20.0f;
  calculated_values.dwell_us = 2000;

  uint64_t start = cycle_count();
  schedule_event(&config.events[0]);
  uint64_t end = cycle_count();
  assert(config.events[0].start.scheduled);

  printf("fresh ignition schedule_event: %llu ns\r\n",
         cycles_to_ns(end - start));

  calculated_values.timing_advance = 25.0f;
  start = cycle_count();
  schedule_event(&config.events[0]);
  end = cycle_count();

  printf("backward schedule_event: %llu ns\r\n", cycles_to_ns(end - start));

  calculated_values.timing_advance = 15.0f;
  start = cycle_count();
  schedule_event(&config.events[0]);
  end = cycle_count();

  printf("forward schedule_event: %llu ns\r\n", cycles_to_ns(end - start));
}

static void do_sensor_adc_calcs() {
  for (int i = 0; i < NUM_SENSORS; i++) {
    config.sensors[i].raw_value = 2048;
  }
  uint64_t start = cycle_count();
  sensors_process(SENSOR_ADC);
  uint64_t end = cycle_count();

  printf("process_sensor(adc): %llu ns\r\n", cycles_to_ns(end - start));
}

void do_buffer_swap_bench();

int main() {
  platform_benchmark_init();
  initialize_scheduler();

  /* Preparations for all benchmarks */
  config.sensors[SENSOR_IAT].processed_value = 15.0f;
  config.sensors[SENSOR_BRV].processed_value = 14.8f;
  config.sensors[SENSOR_MAP].processed_value = 100.0f;
  config.sensors[SENSOR_CLT].processed_value = 90.0f;
  config.decoder.rpm = 3000;

  do {
    platform_load_config();
    do_fuel_calculation_bench();

    platform_load_config();
    do_schedule_ignition_event_bench();

    platform_load_config();
    do_sensor_adc_calcs();

    platform_load_config();
    do_buffer_swap_bench();
  } while (1);
  return 0;
}
