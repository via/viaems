#include "config.h"
#include "adc.h"

struct table timing_vs_rpm = {
  .title = "Timing",
  .num_axis = 2,
  .axis = {
    { 
      .name = "RPM", 
      .num = 8,
      .values = {400, 700, 1000, 1500, 2000, 2500, 3000, 6000}
    },
    {
      .name = "MAP",
      .num = 3,
      .values = {25, 75, 100}
    },
  },
  .data = {
    .two = {
      {0, 3, 5, 8, 12, 14, 16, 20},
      /* Map isnt physically hooked up yet */
    },
  },
};

struct config config = {
  .num_events = 10,
  .events = {
    {.type=IGNITION_EVENT, .angle=0, .output_id=14},
    {.type=IGNITION_EVENT, .angle=0, .output_id=0, .inverted=1},
    {.type=IGNITION_EVENT, .angle=90, .output_id=0, .inverted=1},
    {.type=IGNITION_EVENT, .angle=180, .output_id=0, .inverted=1},
    {.type=IGNITION_EVENT, .angle=270, .output_id=0, .inverted=1},
    {.type=ADC_EVENT, .angle=290},
    {.type=IGNITION_EVENT, .angle=360, .output_id=14},
    {.type=IGNITION_EVENT, .angle=360, .output_id=0, .inverted=1},
    {.type=IGNITION_EVENT, .angle=450, .output_id=0, .inverted=1},
    {.type=IGNITION_EVENT, .angle=540, .output_id=0, .inverted=1},
    {.type=IGNITION_EVENT, .angle=630, .output_id=0, .inverted=1},
    {.type=IGNITION_EVENT, .angle=450, .output_id=0, .inverted=1},
    {.type=ADC_EVENT, .angle=650},
  },
  .trigger = FORD_TFI,
  .decoder = {
    .offset = 45,
    .trigger_max_rpm_change = 0.55, /*Startup sucks with only 90* trigger */
    .trigger_min_rpm = 80,
    .t0_pin = 0,
    .t1_pin = 1,
  },
  .adc = {
    [ADC_MAP] = {1, adc_process_linear, 0.0, 100.0, 0, 0},
    [ADC_IAT] = {2, adc_process_linear, -30.0, 120.0, 0, 0},
  },
  .timing = &timing_vs_rpm,
  .rpm_stop = 4000,
  .rpm_start = 3800,
  .console = {
    .baud = 115200,
    .stop_bits = 1,
    .data_bits = 8,
    .parity = 'N',
  },
};

