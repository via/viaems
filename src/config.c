#include "config.h"
#include "sensors.h"

struct table timing_vs_rpm_and_map __attribute__((section(".configdata"))) = {
  .title = "Timing",
  .num_axis = 2,
  .axis = {
    { 
      .name = "RPM", 
      .num = 5,
      .values = {400, 700, 1000, 2400, 6000}
    },
    {
      .name = "MAP",
      .num = 3,
      .values = {25, 75, 100}
    },
  },
  .data = {
    .two = {
      {0, 4, 8, 35, 40},
      {0, 4, 8, 35, 40},
      {0, 4, 8, 35, 40},
      /* Map isnt physically hooked up yet */
    },
  },
};

struct table injector_dead_time __attribute__((section(".configdata"))) = {
  .title = "Dead Time",
  .num_axis = 1,
  .axis = {
    { 
      .name = "BRV", 
      .num = 5,
      .values = {7, 10, 12, 13.8, 18}
    },
  },
  .data = {
    .one = {
      2000, 1500, 1200, 1000, 700,
    },
  },
};

struct config config __attribute__((section(".configdata"))) = {
  .num_events = 9,
  .events = {
    {.type=IGNITION_EVENT, .angle=0, .output_id=12, .inverted=1},
    {.type=IGNITION_EVENT, .angle=90, .output_id=12, .inverted=1},
    {.type=IGNITION_EVENT, .angle=180, .output_id=12, .inverted=1},
    {.type=IGNITION_EVENT, .angle=270, .output_id=12, .inverted=1},
 //   {.type=ADC_EVENT, .angle=270},

    {.type=IGNITION_EVENT, .angle=360, .output_id=12, .inverted=1},
    {.type=IGNITION_EVENT, .angle=450, .output_id=12, .inverted=1},
    {.type=IGNITION_EVENT, .angle=540, .output_id=12, .inverted=1},
    {.type=IGNITION_EVENT, .angle=630, .output_id=12, .inverted=1},
//    {.type=ADC_EVENT, .angle=630},

    {.type=FUEL_EVENT, .angle=90, .output_id=13},
//    {.type=FUEL_EVENT, .angle=90,   .output_id=6},
//    {.type=FUEL_EVENT, .angle=90,  .output_id=7},
//    {.type=FUEL_EVENT, .angle=90, .output_id=8},
//    {.type=FUEL_EVENT, .angle=90, .output_id=9},
//    {.type=FUEL_EVENT, .angle=90, .output_id=10},
//    {.type=FUEL_EVENT, .angle=90, .output_id=11},
//    {.type=FUEL_EVENT, .angle=90, .output_id=12},
  },
  .trigger = FORD_TFI,
  .decoder = {
    .offset = 38,
    .trigger_max_rpm_change = 0.55, /*Startup sucks with only 90* trigger */
    .trigger_min_rpm = 80,
  },
  .sensors = {
#if 0
    [SENSOR_MAP] = {.pin=1, .method=SENSOR_FREQ, .process=sensor_process_freq, 
      .params={.range={.min=0, .max=100}}},
    [SENSOR_IAT] = {.pin=2, .method=SENSOR_ADC, .process=sensor_process_linear, 
      .params={.range={.min=-30.0, .max=120.0}}},
#else
    [SENSOR_MAP] = {.pin=7, .method=SENSOR_ADC, .process=sensor_process_linear,
      .params={.range={.min=0, .max=100}}},
    [SENSOR_AAP] = {.method=SENSOR_CONST, .params={.fixed_value = 102.0}},
    [SENSOR_IAT] = {.method=SENSOR_CONST, .params={.fixed_value = 29.0}},
    [SENSOR_FRT] = {.method=SENSOR_CONST, .params={.fixed_value = 0.0}},
    [SENSOR_CLT] = {.method=SENSOR_CONST, .params={.fixed_value = 15.0}},
    [SENSOR_BRV] = {.method=SENSOR_CONST, .params={.fixed_value = 13.8}},
    [SENSOR_TPS] = {.method=SENSOR_CONST, .params={.fixed_value = 100.0}},
#endif
  },
  .timing = &timing_vs_rpm_and_map,
  .injector_pw_compensation = &injector_dead_time,
  .rpm_stop = 7000,
  .rpm_start = 6800,
  .fueling = {
    .injector_cc_per_minute = 550,
    .cylinder_cc = 500,
    .fuel_stoich_ratio = 14.7,
    .injections_per_cycle = 2, /* All batched */
    .density_of_fuel = 0.755,
    .density_of_air_stp = 1.2754e-3,
  },
  .ignition = {
    .dwell = DWELL_FIXED_DUTY,
    .dwell_duty = 0.5,
    .min_fire_time_us = 100,
  },
  .console = {
    .baud = 115200,
    .stop_bits = 1,
    .data_bits = 8,
    .parity = 'N',
  },
};

