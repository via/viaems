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
      {15, 18, 20, 22, 24, 26, 28, 32},
      {10, 15, 20, 22, 24, 26, 28, 32},
      {5,   8, 12, 15, 18, 23, 25, 28},
    },
  },
};

struct config config = {
  .num_events = 10,
  .events = {
    {IGNITION_EVENT, 0, 14, {}, {}},
    {IGNITION_EVENT, 90, 13, {}, {}},
    {IGNITION_EVENT, 180, 12, {}, {}},
    {IGNITION_EVENT, 270, 11, {}, {}},
    {ADC_EVENT, 290, 0, {}, {}},
    {IGNITION_EVENT, 360, 14, {}, {}},
    {IGNITION_EVENT, 450, 13, {}, {}},
    {IGNITION_EVENT, 540, 12, {}, {}},
    {IGNITION_EVENT, 630, 11, {}, {}},
    {ADC_EVENT, 650, 0, {}, {}},
  },
  .trigger = FORD_TFI,
  .decoder = {
    .offset = 45,
    .trigger_max_rpm_change = 0.25,
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

