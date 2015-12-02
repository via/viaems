#include "config.h"
#include "sensors.h"

struct table timing_vs_rpm_and_map = {
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

struct config config = {
  .num_events = 10,
  .events = {
    /* {.type=IGNITION_EVENT, .angle=0, .output_id=14}, */
    {.type=IGNITION_EVENT, .angle=0, .output_id=0, .inverted=1},
    {.type=IGNITION_EVENT, .angle=90, .output_id=0, .inverted=1},
    {.type=IGNITION_EVENT, .angle=180, .output_id=0, .inverted=1},
    {.type=IGNITION_EVENT, .angle=270, .output_id=0, .inverted=1},
    {.type=ADC_EVENT, .angle=290},
/*    {.type=IGNITION_EVENT, .angle=360, .output_id=14}, */
    {.type=IGNITION_EVENT, .angle=360, .output_id=0, .inverted=1},
    {.type=IGNITION_EVENT, .angle=450, .output_id=0, .inverted=1},
    {.type=IGNITION_EVENT, .angle=540, .output_id=0, .inverted=1},
    {.type=IGNITION_EVENT, .angle=630, .output_id=0, .inverted=1},
    {.type=ADC_EVENT, .angle=650},
  },
  .trigger = FORD_TFI,
  .decoder = {
    .offset = 45,
    .trigger_max_rpm_change = 0.55, /*Startup sucks with only 90* trigger */
    .trigger_min_rpm = 80,
    .t0_pin = 3,
    .t1_pin = 4,
  },
  .sensors = {
    [SENSOR_MAP] = {.pin=1, .method=SENSOR_ADC, .process=sensor_process_linear, 
      .params={.range={.min=0, .max=100}}},
    [SENSOR_IAT] = {.pin=2, .method=SENSOR_ADC, .process=sensor_process_linear, 
      .params={.range={.min=-30.0, .max=120.0}}},
  },
  .timing = &timing_vs_rpm_and_map,
  .rpm_stop = 4000,
  .rpm_start = 3800,
  .console = {
    .baud = 115200,
    .stop_bits = 1,
    .data_bits = 8,
    .parity = 'N',
  },
};

