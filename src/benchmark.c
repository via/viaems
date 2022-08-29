#define BENCHMARK
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "calculations.h"
#include "config.h"
#include "decoder.h"
#include "platform.h"
#include "scheduler.h"
#include "sensors.h"
#include "table.h"
#include "tasks.h"
#include "util.h"

static void do_fuel_calculation_bench() {
  platform_load_config();

  uint64_t start = cycle_count();
  calculate_fueling();
  uint64_t end = cycle_count();
  printf("calculate_fueling: %llu ns\r\n",
         (unsigned long long)cycles_to_ns(end - start));
}

static void do_schedule_ignition_event_bench() {
  platform_load_config();

  config.events[0].start.state = SCHED_UNSCHEDULED;
  config.events[0].stop.state = SCHED_UNSCHEDULED;
  config.events[0].angle = 180;

  config.decoder.valid = 1;
  calculated_values.timing_advance = 20.0f;
  calculated_values.dwell_us = 2000;

  uint64_t start = cycle_count();
  schedule_event(&config.events[0]);
  uint64_t end = cycle_count();
  assert(config.events[0].start.state == SCHED_SCHEDULED);

  printf("fresh ignition schedule_event: %llu ns\r\n",
         (unsigned long long)cycles_to_ns(end - start));

  calculated_values.timing_advance = 25.0f;
  start = cycle_count();
  schedule_event(&config.events[0]);
  end = cycle_count();

  printf("backward schedule_event: %llu ns\r\n",
         (unsigned long long)cycles_to_ns(end - start));

  calculated_values.timing_advance = 15.0f;
  start = cycle_count();
  schedule_event(&config.events[0]);
  end = cycle_count();

  printf("forward schedule_event: %llu ns\r\n",
         (unsigned long long)cycles_to_ns(end - start));
}

static void do_sensor_adc_calcs() {
  platform_load_config();
  for (int i = 0; i < NUM_SENSORS; i++) {
    config.sensors[i].raw_value = 2048;
  }
  uint64_t start = cycle_count();
  sensors_process(SENSOR_ADC);
  uint64_t end = cycle_count();

  printf("process_sensor(adc): %llu ns\r\n",
         (unsigned long long)cycles_to_ns(end - start));
}

static void do_sensor_single_therm() {
  platform_load_config();
  for (int i = 0; i < NUM_SENSORS; i++) {
    config.sensors[i].source = SENSOR_NONE;
  }
  config.sensors[0] = (struct sensor_input){
    .source = SENSOR_ADC,
    .method = METHOD_THERM,
    .therm = {
      .bias=2490,
      .a=0.00146167419060305,
      .b=0.00022887572003919,
      .c=1.64484831669638E-07,
    },
    .fault_config = {
      .min = 0,
      .max = 4095,
    },
    .raw_value = 10,
  };

  uint64_t start = cycle_count();
  sensors_process(SENSOR_ADC);
  uint64_t end = cycle_count();

  printf("process_sensor(single-therm): %llu ns\r\n",
         (unsigned long long)cycles_to_ns(end - start));
}

int main() {
  initialize_scheduler();

  /* Preparations for all benchmarks */
  config.sensors[SENSOR_IAT].processed_value = 15.0f;
  config.sensors[SENSOR_BRV].processed_value = 14.8f;
  config.sensors[SENSOR_MAP].processed_value = 100.0f;
  config.sensors[SENSOR_CLT].processed_value = 90.0f;
  config.decoder.rpm = 3000;

  do {
    do_fuel_calculation_bench();
    do_schedule_ignition_event_bench();
    do_sensor_adc_calcs();
    do_sensor_single_therm();
  } while (1);
  return 0;
}
