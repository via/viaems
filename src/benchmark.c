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

#define BENCH_RUNS 1000000

static void do_fuel_calculation_bench() {
  uint64_t start = current_realtime_ns();
  for (int i = 0; i < BENCH_RUNS; i++) {
    calculate_fueling();
  }
  uint64_t end = current_realtime_ns();

  uint64_t res = (end - start) / BENCH_RUNS;
  printf("calculate_fueling: %lu ns\r\n", res);
}

static void do_schedule_ignition_event_bench() {
  uint64_t start = current_realtime_ns();
  for (int i = 0; i < BENCH_RUNS; i++) {
    config.events[0].start.scheduled = 0;
    config.events[0].start.fired = 0;
    config.events[0].stop.scheduled = 0;
    config.events[0].stop.fired = 0;
    schedule_event(&config.events[0]);
  }
  uint64_t end = current_realtime_ns();

  uint64_t res = (end - start) / BENCH_RUNS;
  printf("schedule_event: %lu ns\r\n", res);
}

static void do_schedule_backward_ignition_event_bench() {
  uint64_t start = current_realtime_ns();
  for (int i = 0; i < BENCH_RUNS; i++) {
    schedule_event(&config.events[0]);
  }
  uint64_t end = current_realtime_ns();

  uint64_t res = (end - start) / BENCH_RUNS;
  printf("backward schedule_event: %lu ns\r\n", res);
}

static void do_sensor_adc_calcs() {
  for (int i = 0; i < NUM_SENSORS; i++) {
    config.sensors[i].raw_value = 2048;
  }
  uint64_t start = current_realtime_ns();
  for (int i = 0; i < BENCH_RUNS; i++) {
    sensors_process(SENSOR_ADC);
  }
  uint64_t end = current_realtime_ns();

  uint64_t res = (end - start) / BENCH_RUNS;
  printf("process_sensor(adc): %lu ns\r\n", res);
}

static void do_buffer_swap_bench() {
  uint64_t start = current_realtime_ns();
  for (int i = 0; i < BENCH_RUNS; i++) {
    scheduler_buffer_swap();
  }
  uint64_t end = current_realtime_ns();

  uint64_t res = (end - start) / BENCH_RUNS;
  printf("buffer_swap: %lu ns\r\n", res);
}

int main() {
  platform_load_config();

  /* Preparations for all benchmarks */
  config.sensors[SENSOR_IAT].processed_value = 15.0f;
  config.sensors[SENSOR_BRV].processed_value = 14.8f;
  config.sensors[SENSOR_MAP].processed_value = 100.0f;
  config.sensors[SENSOR_CLT].processed_value = 90.0f;
  config.decoder.rpm = 3000;
  config.decoder.valid = 1;
  calculated_values.timing_advance = 20.0f;
  calculated_values.dwell_us = 2000;

  do_fuel_calculation_bench();
  do_schedule_ignition_event_bench();
  do_schedule_backward_ignition_event_bench();
  do_sensor_adc_calcs();
  do_buffer_swap_bench();
  return 0;
}
