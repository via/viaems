#undef NDEBUG // Enable benchmark checks
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "calculations.h"
#include "config.h"
#include "platform.h"
#include "scheduler.h"
#include "table.h"
#include "tasks.h"
#include "viaems.h"

#define FAR_FUTURE (TICKRATE * 60)

struct benchmark_results {
  int n_runs;
  uint32_t min;
  uint32_t avg;
  uint32_t max;
};

static void report_benchmark(const char *name,
                             const struct benchmark_results results) {

  printf("%-40s%-10u%-10u%-10u\r\n",
         name,
         (unsigned int)cycles_to_ns(results.min),
         (unsigned int)cycles_to_ns(results.avg),
         (unsigned int)cycles_to_ns(results.max));
}

typedef uint32_t (*benchmark_fn)(void);
static struct benchmark_results run_benchmark(benchmark_fn fn, int count) {
  struct benchmark_results results = { .min = -1 };
  uint64_t total = 0;
  for (int i = 0; i < count; i++) {
    uint32_t cycles = fn();
    if (cycles < results.min) {
      results.min = cycles;
    }
    if (cycles > results.max) {
      results.max = cycles;
    }
    total += cycles;
  }
  results.avg = total / count;
  results.n_runs = count;

  return results;
}

static uint32_t do_calculation_bench() {

  struct engine_update update = (struct engine_update){
    .position = {
      .rpm = 200,
      .tooth_rpm = 200, /* Force crank enrich */
      .valid_until = -1,
      .has_position = true,
    },
    .sensors = {
      .MAP = { .value = 80.0f },
      .IAT = { .value = 25.0f },
      .CLT = { .value = 15.0f }, /* Force crank enrich */
      .TPS = { .value = 0.0f, .derivative = 100.0f },
      .BRV = { .value = 13.8f },
      .FRT = { .value = 25.0f },
    },
    .current_time = 0,
  };

  struct calculations calcs = { 0 };
  uint64_t start = cycle_count();
  calculate_ignition_and_fuel(&default_config, &update, &calcs);
  uint64_t end = cycle_count();

  return end - start;
}

static const struct engine_update schedule_bench_update = {
  .position = {
    .rpm = 1000,
    .tooth_rpm = 1000,
    .valid_until = FAR_FUTURE,
    .has_position = true,
  },
  .sensors = {
    .MAP = { .value = 80.0f },
    .IAT = { .value = 25.0f },
    .CLT = { .value = 95.0f },
    .TPS = { .value = 0.0f, .derivative = 0.0f },
    .BRV = { .value = 13.8f },
    .FRT = { .value = 25.0f },
  },
};

static const struct calculated_values bench_calcs = {
  .timing_advance = 20.0f,
  .dwell_us = 2000,
  .fueling_us = 2000,
};

static uint32_t do_schedule_ignition_from_unscheduled() {

  struct output_event_config ev_conf = {
    .angle = 180,
    .type = IGNITION_EVENT,
  };
  struct output_event_schedule_state ev = { .config = &ev_conf };

  uint64_t start = cycle_count();
  schedule_events(
    &default_config, &bench_calcs, &schedule_bench_update.position, &ev, 1, 0);
  uint64_t end = cycle_count();

  assert(ev.start.state == SCHED_SCHEDULED);

  return end - start;
}

static uint32_t do_schedule_ignition_earlier() {

  struct output_event_config ev_conf = {
    .angle = 180,
    .type = IGNITION_EVENT,
  };
  struct output_event_schedule_state ev = { .config = &ev_conf };

  schedule_events(
    &default_config, &bench_calcs, &schedule_bench_update.position, &ev, 1, 0);

  struct calculated_values newcalcs = bench_calcs;
  newcalcs.timing_advance += 20.0f;

  uint64_t start = cycle_count();
  schedule_events(
    &default_config, &newcalcs, &schedule_bench_update.position, &ev, 1, 0);
  uint64_t end = cycle_count();
  assert(ev.start.state == SCHED_SCHEDULED);

  return end - start;
}

static uint32_t do_schedule_ignition_later() {
  struct output_event_config ev_conf = {
    .angle = 180,
    .type = IGNITION_EVENT,
  };
  struct output_event_schedule_state ev = { .config = &ev_conf };

  schedule_events(
    &default_config, &bench_calcs, &schedule_bench_update.position, &ev, 1, 0);

  struct calculated_values newcalcs = bench_calcs;
  newcalcs.timing_advance -= 20.0f;

  uint64_t start = cycle_count();
  schedule_events(
    &default_config, &newcalcs, &schedule_bench_update.position, &ev, 1, 0);
  uint64_t end = cycle_count();
  assert(ev.start.state == SCHED_SCHEDULED);

  return end - start;
}

static uint32_t do_schedule_fuel_from_unscheduled() {
  struct output_event_config ev_conf = {
    .type = FUEL_EVENT,
    .angle = 180,
  };
  struct output_event_schedule_state ev = { .config = &ev_conf };

  uint64_t start = cycle_count();
  schedule_events(
    &default_config, &bench_calcs, &schedule_bench_update.position, &ev, 1, 0);
  uint64_t end = cycle_count();

  assert(ev.start.state == SCHED_SCHEDULED);

  return end - start;
}

static uint32_t do_schedule_fuel_earlier() {
  struct output_event_config ev_conf = {
    .type = FUEL_EVENT,
    .angle = 180,
  };
  struct output_event_schedule_state ev = { .config = &ev_conf };

  schedule_events(
    &default_config, &bench_calcs, &schedule_bench_update.position, &ev, 1, 0);

  ev_conf.angle -= 60;
  uint64_t start = cycle_count();
  schedule_events(
    &default_config, &bench_calcs, &schedule_bench_update.position, &ev, 1, 0);
  uint64_t end = cycle_count();

  assert(ev.start.state == SCHED_SCHEDULED);

  return end - start;
}

static uint32_t do_schedule_fuel_later() {
  struct output_event_config ev_conf = {
    .type = FUEL_EVENT,
    .angle = 180,
  };
  struct output_event_schedule_state ev = { .config = &ev_conf };

  schedule_events(
    &default_config, &bench_calcs, &schedule_bench_update.position, &ev, 1, 0);

  ev_conf.angle += 60;
  uint64_t start = cycle_count();
  schedule_events(
    &default_config, &bench_calcs, &schedule_bench_update.position, &ev, 1, 0);
  uint64_t end = cycle_count();

  assert(ev.start.state == SCHED_SCHEDULED);

  return end - start;
}

static uint32_t do_schedule_deschedule() {

  struct output_event_config ev_conf = {
    .angle = 180,
    .type = IGNITION_EVENT,
  };
  struct output_event_schedule_state ev = { .config = &ev_conf };

  schedule_events(
    &default_config, &bench_calcs, &schedule_bench_update.position, &ev, 1, 0);

  assert(ev.start.state == SCHED_SCHEDULED);

  uint64_t start = cycle_count();
  invalidate_scheduled_events(&ev, 1);
  uint64_t end = cycle_count();

  assert(ev.start.state == SCHED_UNSCHEDULED);
  assert(ev.stop.state == SCHED_UNSCHEDULED);

  return end - start;
}

static uint32_t do_sensor_all_adc_calcs() {
  const struct sensor_configs conf = {
    .BRV = {.pin=2, .source=SENSOR_ADC, .method=METHOD_LINEAR,
      .raw_min=0, .raw_max=5,
      .range={.min=0, .max=24.5}, .lag=80,
      .fault_config={.min = 0.1f, .max = 4.9f, .fault_value = 13.8}},
    .IAT = {.pin=4, .source=SENSOR_ADC, .method=METHOD_THERM,
      .raw_min=0, .raw_max=5,
      .fault_config={.min = 0.05f, .max = 4.95f, .fault_value = 10.0},
      .therm={
        .bias=2490,
        .a=0.00146167419060305,
        .b=0.00022887572003919,
        .c=1.64484831669638E-07,
      }},
    .CLT = { .pin=5, .source=SENSOR_ADC, .method=METHOD_THERM,
      .raw_min=0, .raw_max=5,
      .fault_config={.min = 0.05f, .max = 4.95f, .fault_value = 50.0},
      .therm={
        .bias=2490,
        .a=0.00131586818223649,
        .b=0.00025618700140100302,
        .c=0.00000018474199456928,
      }},
    .EGO = {.pin=7, .source=SENSOR_ADC, .method=METHOD_LINEAR,
      .raw_min=0, .raw_max=5,
      .range={.min=0.499, .max=1.309}},
    .MAP = {.pin=3, .source=SENSOR_ADC, .method=METHOD_LINEAR_WINDOWED,
      .raw_min=0, .raw_max=5,
      .range={.min=12, .max=420}, /* AEM 3.5 bar MAP sensor*/
      .fault_config={.min = 0.05f, .max = 4.95f, .fault_value = 50.0},
      .window={.total_width=120, .capture_width = 120}},
    .AAP = {.pin=5, .source=SENSOR_CONST, .const_value=102.0f},
    .TPS = {.pin=6, .source=SENSOR_ADC, .method=METHOD_LINEAR,
      .raw_min=0, .raw_max=5,
      .range={.min=-15.74, .max=145.47},
      .fault_config={.min = 0.25f, .max = 4.5f, .fault_value = 25.0},
      .lag = 10.0},
    .FRT = {.pin=3, .source=SENSOR_PULSEWIDTH,
      .raw_min=0.001f, .raw_max=0.005f, .range={.min=-40, .max=125}},
    .FRP = {.source=SENSOR_CONST, .const_value = 100},
    .ETH = {.pin=3, .source=SENSOR_FREQ, .method=METHOD_LINEAR,
      .raw_min=50, .raw_max=150, .range={.min=0, .max=100}},
  };

  struct sensors state;
  sensors_init(&conf, &state);

  struct adc_update u = {
    .time = current_time(),
    .valid = true,
  };
  for (int i = 0; i < 16; i++) {
    u.values[i] = 2.5f;
  }

  struct engine_position dout = {
    .rpm = 1000,
    .tooth_rpm = 1000,
    .valid_until = FAR_FUTURE,
    .has_position = true,
    .has_rpm = true,
  };

  uint64_t start = cycle_count();
  sensor_update_adc(&state, &dout, &u);
  uint64_t end = cycle_count();

  return end - start;
}

static uint32_t do_sensor_single_therm() {
  struct sensor_config conf = {
    .pin = 0,
    .source = SENSOR_ADC,
    .method = METHOD_THERM,
    .therm = {
      .bias=2490,
      .a=0.00146167419060305,
      .b=0.00022887572003919,
      .c=1.64484831669638E-07,
    },
    .fault_config = {
      .min = 0.0f,
      .max = 5.0f,
    },
  };

  uint64_t start = cycle_count();
  sensor_convert_thermistor(&conf, 2.5f);
  uint64_t end = cycle_count();

  return end - start;
}

static uint32_t do_sensor_single_linear() {
  struct sensor_config conf = {
    .pin = 0,
    .source = SENSOR_ADC,
    .method = METHOD_LINEAR,
    .range = {
      .min = 100.0f,
      .max = 200.0f,
    },
    .raw_min = 0.0f,
    .raw_max = 5.0f,
    .fault_config = {
      .min = 0.0f,
      .max = 5.0f,
    },
  };

  uint64_t start = cycle_count();
  sensor_convert_linear(&conf, 2.5f);
  uint64_t end = cycle_count();

  return end - start;
}

static uint32_t do_table1d_lookups(void) {

  struct table_1d t = {
    .title = "Example 1d",
    .cols = {
      .name = "Example axis",
      .num = 24,
      .values = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
    },
    .data = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
  };

  uint64_t start = cycle_count();
  interpolate_table_oneaxis(&t, 14.0);
  uint64_t end = cycle_count();

  return end - start;
}

static uint32_t do_table2d_lookups(void) {
  struct table_2d t = {
    .title = "Example 2d",
    .rows = {
      .name = "Example rows",
      .num = 24,
      .values = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
    },
    .cols = {
      .name = "Example cols",
      .num = 24,
      .values = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
    },
    .data = {
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
    },
  };

  uint64_t start = cycle_count();
  interpolate_table_twoaxis(&t, 15.0, 10.0);
  uint64_t end = cycle_count();

  return end - start;
}
static struct benchmark_results do_missing_tooth_sequence(uint32_t count) {

  /* Issue a sequence of trigger updates for the decoder to become synchronized
   */

  const struct decoder_config config = {
    .type = TRIGGER_MISSING_CAMSYNC,
    .offset = 0,
    .trigger_max_rpm_change = 0.5f,
    .required_triggers_rpm = 8,
    .num_triggers = 36,
    .degrees_per_trigger = 10,
    .rpm_window_size = 4,
  };
  struct decoder decoder;
  decoder_init(&config, &decoder);
  struct benchmark_results results = { .min = -1 };
  uint64_t accumulator = 0;

  timeval_t time = 0;
  for (size_t tooth = 0, i = 0; i < count;) {
    bool missing = (tooth % 36 == 10);
    bool cam_sync = (tooth % 72 == 20);

    if (cam_sync) {
      uint32_t start = cycle_count();
      decoder_update(&decoder,
                     &(struct trigger_event){ .time = time, .type = SYNC });
      uint32_t cycles = cycle_count() - start;
      accumulator += cycles;
      if (cycles < results.min) {
        results.min = cycles;
      }
      if (cycles > results.max) {
        results.max = cycles;
      }
      results.n_runs += 1;
      i += 1;
    }

    if (missing) {
      time += 10000;
      tooth += 1;
    } else {
      uint32_t start = cycle_count();
      decoder_update(&decoder,
                     &(struct trigger_event){ .time = time, .type = TRIGGER });
      uint32_t cycles = cycle_count() - start;
      accumulator += cycles;
      if (cycles < results.min) {
        results.min = cycles;
      }
      if (cycles > results.max) {
        results.max = cycles;
      }
      results.n_runs += 1;

      time += 10000;
      i += 1;
      tooth += 1;
    }
  }
  assert(decoder.state == DECODER_SYNC);
  results.avg = (results.n_runs != 0) ? (accumulator / results.n_runs) : 0;
  return results;
}

static uint32_t do_viaems_reschedule_normal() {

  struct engine_update update = {
    .position = {
      .rpm = 200,
      .tooth_rpm = 200, /* Force crank enrich */
      .valid_until = FAR_FUTURE,
      .has_position = true,
      .has_rpm = true,
    },
    .sensors = {
      .MAP = { .value = 80.0f },
      .IAT = { .value = 25.0f },
      .CLT = { .value = 15.0f }, /* Force crank enrich */
      .TPS = { .value = 0.0f, .derivative = 100.0f },
      .BRV = { .value = 13.8f },
      .FRT = { .value = 25.0f },
    },
    .current_time = 0,
  };

  struct viaems v;
  viaems_init(&v, &default_config);

  struct platform_plan plan = {
    .schedulable_start = 0,
    .schedulable_end = FAR_FUTURE,
  };

  uint64_t start = cycle_count();
  viaems_reschedule(&v, &update, &plan);
  uint64_t end = cycle_count();

  return end - start;
}

int start_benchmarks() {
  for (volatile int i = 0; i < 10000000; i++)
    ;

  puts("ViaEMS Benchmark Suite\r\n\r\n");

  puts("Name                                    Min       Average   Max\r\n");
  puts(
    "==============================          =======   =======   =======\r\n");

  report_benchmark("Fuel Calculations",
                   run_benchmark(do_calculation_bench, 1000));
  report_benchmark("Sensors - Thermistor",
                   run_benchmark(do_sensor_single_therm, 1000));
  report_benchmark("Sensors - Linear",
                   run_benchmark(do_sensor_single_linear, 1000));
  report_benchmark("Sensors - Process All ADC",
                   run_benchmark(do_sensor_all_adc_calcs, 1000));
  report_benchmark("Tables - 1D", run_benchmark(do_table1d_lookups, 1000));
  report_benchmark("Tables - 2D", run_benchmark(do_table2d_lookups, 1000));
  report_benchmark("Scheduler - Ign schedule",
                   run_benchmark(do_schedule_ignition_from_unscheduled, 1000));
  report_benchmark("Scheduler - Ign (earlier)",
                   run_benchmark(do_schedule_ignition_earlier, 1000));
  report_benchmark("Scheduler - Ign (later)",
                   run_benchmark(do_schedule_ignition_later, 1000));

  report_benchmark("Scheduler - Fuel schedule",
                   run_benchmark(do_schedule_fuel_from_unscheduled, 1000));
  report_benchmark("Scheduler - Fuel (later)",
                   run_benchmark(do_schedule_fuel_later, 1000));
  report_benchmark("Scheduler - Fuel (earlier)",
                   run_benchmark(do_schedule_fuel_earlier, 1000));
  report_benchmark("Scheduler - Deschedule",
                   run_benchmark(do_schedule_deschedule, 1000));
  report_benchmark("Engine Loop - Happy path",
                   run_benchmark(do_viaems_reschedule_normal, 1000));

  report_benchmark("Decoder - missing+camsync",
                   do_missing_tooth_sequence(1000));
  puts("\r\nDone!\r\n");

  return 0;
}
