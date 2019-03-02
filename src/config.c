#include "config.h"
#include "sensors.h"

struct table timing_vs_rpm_and_map __attribute__((section(".configdata"))) = {
  .title = "Timing",
  .num_axis = 2,
  .axis = {
    { 
      .name = "RPM", 
      .num = 5,
      .values = {200, 400, 700, 1000, 2400, 6000}
    },
    {
      .name = "MAP",
      .num = 3,
      .values = {25, 75, 100}
    },
  },
  .data = {
    .two = {
      {15, 3, 10, 14, 40, 45},
      {15, 3, 10, 10, 25, 45},
      {15, 3, 10, 10, 20, 25},
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
  .num_events = 18,
  .events = {
    {.type=IGNITION_EVENT, .angle=0, .output_id=12, .inverted=1},
    {.type=IGNITION_EVENT, .angle=90, .output_id=12, .inverted=1},
    {.type=IGNITION_EVENT, .angle=180, .output_id=12, .inverted=1},
    {.type=IGNITION_EVENT, .angle=270, .output_id=12, .inverted=1},
    {.type=ADC_EVENT, .angle=270},

    {.type=IGNITION_EVENT, .angle=360, .output_id=12, .inverted=1},
    {.type=IGNITION_EVENT, .angle=450, .output_id=12, .inverted=1},
    {.type=IGNITION_EVENT, .angle=540, .output_id=12, .inverted=1},
    {.type=IGNITION_EVENT, .angle=630, .output_id=12, .inverted=1},
    {.type=ADC_EVENT, .angle=630},

    {.type=FUEL_EVENT, .angle=0, .output_id=0},
    {.type=FUEL_EVENT, .angle=90,   .output_id=1},
    {.type=FUEL_EVENT, .angle=180,  .output_id=2},
    {.type=FUEL_EVENT, .angle=270, .output_id=3},
    {.type=FUEL_EVENT, .angle=360, .output_id=0},
    {.type=FUEL_EVENT, .angle=450, .output_id=1},
    {.type=FUEL_EVENT, .angle=540, .output_id=2},
    {.type=FUEL_EVENT, .angle=630, .output_id=3},
  },
  .decoder = {
    .type = FORD_TFI,
    .offset = 55,
    .trigger_max_rpm_change = 0.55, /*Startup sucks with only 90* trigger */
    .trigger_min_rpm = 80,
  },
  .sensors = {
    [SENSOR_BRV] = {.pin=0, .source=SENSOR_ADC, .method=METHOD_LINEAR,
      .params={.range={.min=0, .max=24.5}},
      .fault_config={.min = 100, .max = 4000, .fault_value = 13.8}},
    [SENSOR_IAT] = {.pin=1, .source=SENSOR_ADC, .method=METHOD_LINEAR,
      .params={.range={.min=0, .max=100}}},
    [SENSOR_CLT] = {.pin=2, .source=SENSOR_ADC, .method=METHOD_THERM,
      .params={
        .therm={
          .bias=2490,
          .a=0.00131586818223649,
          .b=0.00025618700140100302,
          .c=0.00000018474199456928
        }}},
    [SENSOR_TPS] = {.pin=3, .source=SENSOR_ADC, .method=METHOD_LINEAR,
      .params={.range={.min=0, .max=100}},
      .fault_config={.min = 100, .max = 4000, .fault_value = 25.0},
      .lag = 10.0},
    [SENSOR_EGO] = {.pin=4, .source=SENSOR_ADC, .method=METHOD_LINEAR,
      .params={.range={.min=0, .max=100}}},
    [SENSOR_MAP] = {.pin=3, .source=SENSOR_FREQ, .method=METHOD_LINEAR,
      .params={.range={.min=-50, .max=1929}}, /* Ford MAP sensor*/
      .fault_config={.min = 90, .max = 350, .fault_value = 80.0}},
    [SENSOR_AAP] = {.source=SENSOR_CONST, .params={.fixed_value = 102.0}},
    [SENSOR_FRT] = {.source=SENSOR_CONST, .params={.fixed_value = 0.0}},
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
    .fuel_pump_pin = 0,
    .density_of_fuel = 0.755, /* g/cm^3 at 15C */
    .density_of_air_stp = 1.2754e-3, /* g/cm^3 */
  },
  .ignition = {
    .dwell = DWELL_FIXED_DUTY,
    .dwell_duty = 0.5,
    .dwell_us = 1000,
    .min_fire_time_us = 500,
  },
};

