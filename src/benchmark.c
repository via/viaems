#define BENCHMARK
#include <stdio.h>
#include <string.h>
#include <assert.h>

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

static void do_fuel_calculation_bench() {
  platform_load_config();

  uint64_t start = cycle_count();
  calculate_fueling();
  uint64_t end = cycle_count();
  printf("calculate_fueling: %llu ns\r\n", (unsigned long long)cycles_to_ns(end - start));
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

  printf("backward schedule_event: %llu ns\r\n", (unsigned long long)cycles_to_ns(end - start));

  calculated_values.timing_advance = 15.0f;
  start = cycle_count();
  schedule_event(&config.events[0]);
  end = cycle_count();

  printf("forward schedule_event: %llu ns\r\n", (unsigned long long)cycles_to_ns(end - start));
}

static void do_sensor_adc_calcs() {
  platform_load_config();
  for (int i = 0; i < NUM_SENSORS; i++) {
    config.sensors[i].raw_value = 2048;
  }
  uint64_t start = cycle_count();
  sensors_process(SENSOR_ADC);
  uint64_t end = cycle_count();

  printf("process_sensor(adc): %llu ns\r\n", (unsigned long long)cycles_to_ns(end - start));
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

  printf("process_sensor(single-therm): %llu ns\r\n", (unsigned long long)cycles_to_ns(end - start));
}

void do_output_buffer_bench_all_ready() {
  platform_load_config();

  struct output_buffer *ob = current_output_buffer();
  for (int i = 0; i < MAX_EVENTS; i++) {
    config.events[i].start.state = SCHED_SCHEDULED;
    config.events[i].start.time = ob->first_time;
    config.events[i].stop.state = SCHED_SCHEDULED;
    config.events[i].stop.time = ob->first_time;
  }


  uint64_t start = cycle_count();
  scheduler_output_buffer_ready(ob);
  uint64_t end = cycle_count();

  printf("output_buffer_ready: %llu ns\r\n", (unsigned long long)cycles_to_ns(end - start));
}

void do_output_buffer_bench_all_fired() {
  platform_load_config();

  struct output_buffer *ob = current_output_buffer();
  for (int i = 0; i < MAX_EVENTS; i++) {
    config.events[i].start.state = SCHED_SUBMITTED;
    config.events[i].start.time = ob->first_time;
    config.events[i].stop.state = SCHED_SUBMITTED;
    config.events[i].stop.time = ob->first_time;
  }

  uint64_t start = cycle_count();
  scheduler_output_buffer_fired(ob);
  uint64_t end = cycle_count();

  printf("output_buffer_fired: %llu ns\r\n", (unsigned long long)cycles_to_ns(end - start));
}

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
    do_fuel_calculation_bench();
    do_schedule_ignition_event_bench();
    do_sensor_adc_calcs();
    do_sensor_single_therm();
    do_output_buffer_bench_all_fired();
    do_output_buffer_bench_all_ready();
  } while (1);
  return 0;
}
