#include "config.h"
#include "sensors.h"
#include "tasks.h"

struct table_2d enrich_vs_temp_and_map __attribute__((section(".configdata"))) = {
  .title = "temp_enrich",
  .cols = { 
    .name = "TEMP", .num = 6,
    .values = {-20, 0, 20, 40, 102, 120 },
  },
  .rows = { 
    .name = "MAP", .num = 4,
    .values = {20, 60, 80, 100},
  },
  .data = {
    {2.5, 1.8, 1.5, 1.0, 1.0, 1.2},
    {2.0, 1.8, 1.3, 1.0, 1.0, 1.2},
    {1.5, 1.5, 1.2, 1.0, 1.0, 1.2},
    {1.2, 1.3, 1.1, 1.0, 1.0, 1.2},
  },
};

/* Thousandths of a cm^3 */
struct table_2d tipin_vs_tpsrate_and_tps __attribute__((section(".configdata"))) = {
  .title = "tipin_enrich_amount",
  .cols = { 
    .name = "TPSRATE", .num = 5,
    .values = {-1000, 0, 100, 500, 1000},
  },
  .rows = {
    .name = "TPS", .num = 5,
    .values = {0, 10, 20, 50, 80},
  },
  .data = {
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 20, 30},
    {0, 0, 0, 15, 20},
    {0, 0, 0, 10, 10},
  },
};

struct table_1d tipin_duration_vs_rpm __attribute__((section(".configdata"))) = {
  .title = "tipin_enrich_duration",
  .cols = { 
    .name = "RPM", .num = 4,
    .values = {0, 1000, 2000, 4000},
  },
  .data = {
    300.0, 150.0, 80.0, 40.0
  },
};

struct table_1d dwell_ms_vs_brv __attribute__((section(".configdata"))) = {
  .title = "dwell",
  .cols = { 
    .name = "BRV", .num = 4,
    .values = {5, 10, 14, 18},
  },
  .data = {
    6.0, 4.0, 2.5, 1.8
  },
};

struct table_2d ve_vs_rpm_and_map __attribute__((section(".configdata"))) = {
  .title = "ve",
  .cols = { 
    .name = "RPM", .num = 16,
    .values = {250, 500, 900, 1200, 1600, 2000, 2400, 3000, 3600, 4000, 4400, 5200, 5800, 6400, 6800, 7200},
  },
  .rows = {
    .name = "MAP", .num = 16,
    .values = {20, 30, 40, 50, 60, 70, 80, 90, 100, 120, 140, 160, 180, 200, 220, 240},
  },
  .data = {
   /*  250,   500,   900,  1200,  1600,   2000,  2400,  3000, 3600,  4000,   4400,  5200,  5800,  6400,  6800,  7200}, */
      {65.0,  30.0,  2.0,   2.0,   10.0,  10.0,  8.0,   8.0,   8.0,   8.0,   12.0,  12.0,  12.0,  12.0,  16.0,  16.0}, /* 20 */
      {65.0,  45.0,  15.0,  12.0,   8.0,   9.0,  8.0,   8.0,  23.0,   9.0,   11.0,  12.0,  12.0,  12.0,  26.0,  26.0}, /* 30 */
      {65.0,  45.0,  25.0,  25.0,  25.0,  18.0,  16.0,  24.0,  26.0,  40.0,  40.0,  43.0,  45.0,  45.0,  45.0,  45.0}, /* 40 */
      {65.0,  45.0,  50.0,  42.0,  40.0,  46.0,  46.0,  43.0,  36.0,  42.0,  48.0,  50.0,  52.0,  52.0,  52.0,  52.0}, /* 50 */
      {65.0,  45.0,  55.0,  53.0,  50.0,  53.0,  49.0,  57.0,  58.0,  61.0,  56.0,  52.0,  52.0,  52.0,  52.0,  52.0}, /* 60 */
      {65.0,  45.0,  63.0,  59.0,  62.0,  60.0,  63.0,  61.0,  67.0,  67.0,  66.0,  66.0,  67.0,  67.0,  67.0,  67.0}, /* 70 */
      {65.0,  45.0,  72.0,  65.0,  71.0,  65.0,  65.0,  66.0,  71.0,  70.0,  70.0,  74.0,  70.0,  68.0,  68.0,  68.0}, /* 80 */
      {65.0,  45.0,  72.0,  72.0,  75.0,  80.0,  78.0,  75.0,  80.0,  83.0,  80.0,  80.0,  70.0,  70.0,  70.0,  72.0}, /* 90 */
      {65.0,  45.0,  72.0,  72.0,  82.0,  79.0,  76.0,  76.0,  80.0,  80.0,  80.0,  80.0,  77.0,  77.0,  78.0,  78.0}, /* 100 */
      {65.0,  45.0,  72.0,  72.0,  83.0,  83.0,  83.0,  84.0,  81.0,  82.0,  82.0,  82.0,  80.0,  80.0,  80.0,  80.0}, /* 120 */
      {65.0,  45.0,  72.0,  72.0,  83.0,  83.0,  85.0,  83.0,  88.0,  89.0,  90.0,  90.0,  90.0,  90.0,  90.0,  90.0}, /* 140 */
      {65.0,  45.0,  72.0,  72.0,  83.0,  83.0,  90.0,  90.0,  90.0,  90.0,  90.0,  90.0,  90.0,  90.0,  90.0,  90.0}, /* 160 */
      {65.0,  45.0,  72.0,  72.0,  83.0,  83.0,  90.0,  90.0,  90.0,  90.0,  90.0,  90.0,  90.0,  90.0,  90.0,  90.0}, /* 180 */
      {65.0,  45.0,  72.0,  72.0,  83.0,  83.0,  90.0,  90.0,  90.0,  90.0,  90.0,  90.0,  90.0,  90.0,  90.0,  90.0}, /* 200 */
      {65.0,  45.0,  72.0,  72.0,  83.0,  83.0,  90.0,  90.0,  90.0,  90.0,  90.0,  90.0,  90.0,  90.0,  90.0,  90.0}, /* 220 */
      {65.0,  45.0,  72.0,  72.0,  83.0,  83.0,  90.0,  90.0,  90.0,  90.0,  90.0,  90.0,  90.0,  90.0,  90.0,  90.0}, /* 240 */
  },
};

struct table_2d lambda_vs_rpm_and_map __attribute__((section(".configdata"))) = {
  .title = "lambda",
  .cols = { 
    .name = "RPM", .num = 16,
    .values = {250, 500, 900, 1200, 1600, 2000, 2400, 3000, 3600, 4000, 4400, 5200, 5800, 6400, 6800, 7200},
  },
  .rows = { 
    .name = "MAP", .num = 16,
    .values = {20, 30, 40, 50, 60, 70, 80, 90, 100, 120, 140, 160, 180, 200, 220, 240},
  },
  .data = {
      {1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00},
      {1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00},
      {1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00},
      {1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00},
      {1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00},
      {1.00, 1.00, 0.92, 0.95, 0.90, 0.90, 0.90, 0.90, 0.90, 0.90, 0.90, 0.90, 0.90, 0.90, 0.90, 0.90},
      {0.90, 0.90, 0.90, 0.90, 0.90, 0.90, 0.90, 0.90, 0.90, 0.90, 0.90, 0.90, 0.90, 0.90, 0.90, 0.90},
      {0.85, 0.85, 0.85, 0.85, 0.85, 0.85, 0.85, 0.85, 0.85, 0.85, 0.85, 0.85, 0.85, 0.85, 0.85, 0.85},
      {0.85, 0.85, 0.85, 0.85, 0.85, 0.85, 0.85, 0.85, 0.85, 0.85, 0.85, 0.85, 0.85, 0.85, 0.85, 0.85},
      {0.84, 0.84, 0.84, 0.84, 0.84, 0.84, 0.84, 0.84, 0.84, 0.84, 0.84, 0.84, 0.84, 0.84, 0.84, 0.84},
      {0.83, 0.83, 0.83, 0.83, 0.83, 0.82, 0.82, 0.82, 0.82, 0.82, 0.82, 0.82, 0.82, 0.82, 0.82, 0.82},
      {0.83, 0.83, 0.83, 0.83, 0.83, 0.82, 0.82, 0.82, 0.82, 0.82, 0.82, 0.82, 0.82, 0.82, 0.82, 0.82},
      {0.81, 0.81, 0.81, 0.81, 0.81, 0.76, 0.76, 0.76, 0.76, 0.76, 0.76, 0.76, 0.76, 0.76, 0.76, 0.76},
      {0.73, 0.73, 0.73, 0.81, 0.81, 0.73, 0.73, 0.73, 0.73, 0.73, 0.73, 0.73, 0.73, 0.73, 0.73, 0.73},
      {0.75, 0.75, 0.75, 0.73, 0.73, 0.73, 0.73, 0.73, 0.73, 0.73, 0.73, 0.73, 0.73, 0.73, 0.73, 0.73},
      {0.75, 0.75, 0.75, 0.73, 0.73, 0.73, 0.73, 0.73, 0.73, 0.73, 0.73, 0.73, 0.73, 0.73, 0.73, 0.73},
  },
};

struct table_2d timing_vs_rpm_and_map __attribute__((section(".configdata"))) = {
  .title = "Timing",
  .cols = {
    .name = "RPM", .num = 16,
    .values = {250, 500, 900, 1200, 1600, 2000, 2400, 3000, 3600, 4000, 4400, 5200, 5800, 6400, 6800, 7200},
  },
  .rows = {
    .name = "MAP", .num = 16,
    .values = {20, 30, 40, 50, 60, 70, 80, 90, 100, 120, 140, 160, 180, 200, 220, 240},
  },
  .data = {
      {12.0, 15.0, 18.0, 28.0, 32.0, 35.0, 42.0, 42.0, 42.0, 42.0, 42.0, 42.0, 42.0, 42.0, 42.0, 42.0},
      {12.0, 15.0, 18.0, 24.0, 28.0, 32.0, 36.0, 36.0, 38.0, 38.0, 40.0, 40.0, 40.0, 40.0, 40.0, 40.0},
      {12.0, 15.0, 18.0, 24.0, 25.0, 30.0, 32.0, 35.0, 36.0, 36.0, 38.0, 40.0, 40.0, 40.0, 40.0, 40.0},
      {10.0, 15.0, 15.0, 24.0, 24.0, 28.0, 30.0, 32.0, 34.0, 35.0, 35.0, 35.0, 35.0, 35.0, 35.0, 35.0},
      {10.0, 15.0, 15.0, 20.0, 22.0, 24.0, 28.0, 30.0, 34.0, 34.0, 34.0, 34.0, 34.0, 34.0, 34.0, 34.0},
      {10.0, 15.0, 15.0, 17.0, 17.0, 22.0, 26.0, 28.0, 32.0, 32.0, 32.0, 32.0, 32.0, 32.0, 32.0, 32.0},
      {10.0, 10.0, 10.0, 15.0, 18.0, 18.0, 24.0, 26.0, 31.0, 31.0, 31.0, 31.0, 31.0, 31.0, 31.0, 31.0},
      {8.0, 10.0, 10.0, 12.0, 15.0, 15.0, 22.0, 28.0, 30.0, 32.0, 32.0, 32.0, 32.0, 32.0, 32.0, 32.0},

      {8.0, 8.0, 8.0, 8.0, 10.0, 18.0, 24.0, 28.0, 28.0, 30.0, 30.0, 30.0, 30.0, 30.0, 30.0, 30.0},

      {8.0, 10.0, 10.0, 12.0, 13.0, 15.0, 22.0, 25.0, 26.0, 28.0, 28.0, 28.0, 28.0, 28.0, 28.0, 28.0},
      {8.0, 10.0, 10.0, 12.0, 13.0, 15.0, 18.0, 21.0, 23.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0},
      {8.0, 10.0, 10.0, 12.0, 13.0, 13.0, 15.0, 17.0, 20.0, 20.0, 21.0, 21.0, 21.0, 21.0, 21.0, 21.0},
      {8.0, 10.0, 10.0, 12.0, 13.0, 13.0, 15.0, 16.0, 16.0, 18.0, 18.0, 18.0, 18.0, 18.0, 18.0, 18.0},
      {8.0, 10.0, 10.0, 12.0, 12.0, 13.0, 13.0, 13.0, 15.0, 15.0, 15.0, 15.0, 15.0, 15.0, 15.0, 15.0},
      {8.0, 10.0, 10.0, 10.0, 10.0, 11.0, 13.0, 13.0, 13.0, 13.0, 13.0, 13.0, 13.0, 13.0, 13.0, 13.0},
      {8.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0,  8.0,  8.0,  8.0},
  },
};

struct table_1d injector_dead_time __attribute__((section(".configdata"))) = {
  .title = "dead_time",
  .cols = { .name = "BRV", .num = 16,
    .values = {8.5, 9, 9.5, 10, 10.5, 11, 11.5, 12, 12.5, 13, 13.5, 14, 14.5, 15, 15.5, 16},
  },
  .data = {
    2.97, 2.66, 2.39, 2.155, 1.96, 1.82, 1.70, 1.59, 1.48, 1.4, 1.32, 1.25, 1.19, 1.11, 1.05, 1,
  },
};

struct table_1d injector_pw_correction __attribute__((section(".configdata"))) = {
  .title = "pulse_width_correction",
  .cols = { .name = "PW(ms)", .num = 6,
    .values = {0, 0.333, 0.521, 0.816, 1.278, 2.0},
  },
  .data = {0, 0.090, 0.086, 0.038, 0.023, 0},
};

struct table_1d boost_control_pwm __attribute__((section(".configdata"))) = {
  .title = "boost_control",
  .cols = { .name = "RPM", .num = 6,
    .values = {1000.0, 2000.0, 3000.0, 4000.0, 5000.0, 6000.0},
  },
  .data = {
    50, 50, 50, 50, 50, 50,
  },
};

struct config config __attribute__((section(".configdata"))) = {
  .events = {
    {.type=IGNITION_EVENT, .angle=0, .pin=0},
    {.type=IGNITION_EVENT, .angle=120, .pin=1},
    {.type=IGNITION_EVENT, .angle=240, .pin=2},
    {.type=IGNITION_EVENT, .angle=360, .pin=0},
    {.type=IGNITION_EVENT, .angle=480, .pin=1},
    {.type=IGNITION_EVENT, .angle=600, .pin=2},

    {.type=FUEL_EVENT, .angle=700, .pin=8},
    {.type=FUEL_EVENT, .angle=460, .pin=9},
    {.type=FUEL_EVENT, .angle=220, .pin=10},
  },
  .decoder = {
    .type = TRIGGER_MISSING_CAMSYNC,
    .degrees_per_trigger = 10,
    .required_triggers_rpm = 4,
    .num_triggers = 36,
    .offset = 0,
    .trigger_min_rpm = 80,
    .trigger_max_rpm_change = 0.35f,
  },
  .knock_inputs = {
    [0] = { .frequency = 7000 },
    [1] = { .frequency = 7000 },
  },
  .freq_inputs = {
    [0] = {.edge = RISING_EDGE, .type = TRIGGER},
    [1] = {.edge = RISING_EDGE, .type = TRIGGER},
    [2] = {.edge = RISING_EDGE, .type = FREQ},
    [3] = {.edge = FALLING_EDGE, .type = FREQ},
  },
  .sensors = {
    [SENSOR_BRV] = {.pin=2, .source=SENSOR_ADC, .method=METHOD_LINEAR,
      .raw_min=0, .raw_max=5,
      .range={.min=0, .max=24.5}, .lag=80,
      .fault_config={.min = 0.1f, .max = 4.9f, .fault_value = 13.8}},
    [SENSOR_IAT] = {.pin=4, .source=SENSOR_ADC, .method=METHOD_THERM,
      .raw_min=0, .raw_max=5,
      .fault_config={.min = 0.05f, .max = 4.95f, .fault_value = 10.0},
      .therm={
        .bias=2490,
        .a=0.00146167419060305,
        .b=0.00022887572003919,
        .c=1.64484831669638E-07,
      }},
    [SENSOR_CLT] = {.pin=5, .source=SENSOR_ADC, .method=METHOD_THERM,
      .raw_min=0, .raw_max=5,
      .fault_config={.min = 0.05f, .max = 4.95f, .fault_value = 50.0},
      .therm={
        .bias=2490,
        .a=0.00131586818223649,
        .b=0.00025618700140100302,
        .c=0.00000018474199456928,
      }},
    [SENSOR_EGO] = {.pin=7, .source=SENSOR_ADC, .method=METHOD_LINEAR,
      .raw_min=0, .raw_max=5,
      .range={.min=0.499, .max=1.309}},
    [SENSOR_MAP] = {.pin=3, .source=SENSOR_ADC, .method=METHOD_LINEAR_WINDOWED,
      .raw_min=0, .raw_max=5,
      .range={.min=12, .max=420}, /* AEM 3.5 bar MAP sensor*/
      .fault_config={.min = 0.05f, .max = 4.95f, .fault_value = 50.0},
      .window={.total_width=120, .capture_width = 120}},
    [SENSOR_AAP] = {.pin=5, .source=SENSOR_CONST, .value=102.0f},
    [SENSOR_TPS] = {.pin=6, .source=SENSOR_ADC, .method=METHOD_LINEAR,
      .raw_min=0, .raw_max=5,
      .range={.min=-15.74, .max=145.47},
      .fault_config={.min = 0.25f, .max = 4.5f, .fault_value = 25.0},
      .lag = 10.0},
    [SENSOR_FRT] = {.pin=3, .source=SENSOR_PULSEWIDTH,
      .raw_min=0.001f, .raw_max=0.005f, .range={.min=-40, .max=125}},
    [SENSOR_FRP] = {.source=SENSOR_CONST, .value = 100},
    [SENSOR_ETH] = {.pin=3, .source=SENSOR_FREQ, .method=METHOD_LINEAR,
      .raw_min=50, .raw_max=150, .range={.min=0, .max=100}},
  },
  .timing = &timing_vs_rpm_and_map,
  .injector_deadtime_offset = &injector_dead_time,
  .injector_pw_correction = &injector_pw_correction,
  .ve = &ve_vs_rpm_and_map,
  .commanded_lambda = &lambda_vs_rpm_and_map,
  .engine_temp_enrich = &enrich_vs_temp_and_map,
  .tipin_enrich_amount = &tipin_vs_tpsrate_and_tps,
  .tipin_enrich_duration = &tipin_duration_vs_rpm,
  .dwell = &dwell_ms_vs_brv,
  .rpm_stop = 6700,
  .rpm_start = 6200,
  .fueling = {
    .injector_cc_per_minute = 1015,
    .cylinder_cc = 500,
    .fuel_stoich_ratio = 14.7,
    .injections_per_cycle = 1, /* All batched */
    .fuel_pump_pin = 0,
    .density_of_fuel = 0.755, /* g/cm^3 at 15C */
    .density_of_air_stp = 1.2922e-3, /* g/cm^3 at 0C */
    .crank_enrich_config = {
      .crank_rpm = 400,
      .cutoff_temperature = 20.0,
      .enrich_amt = 2.5,
    },
  },
  .ignition = {
    .dwell = DWELL_BRV,
    .min_fire_time_us = 500,
  },
  .boost_control = {
    .pwm_duty_vs_rpm = &boost_control_pwm,
    .enable_threshold_kpa = 90.0,
    .control_threshold_kpa = 190.0,
    .control_threshold_tps = 105.0,
    .pin = 1,
    .overboost = 260.0,
  },
  .cel = {
    .pin = 2,
    .lean_boost_kpa = 140.0,
    .lean_boost_ego = .91,
  },
  .idle_control = {
    .method = IDLE_METHOD_FIXED,
    .interface_type = IDLE_INTERFACE_UNIPOLAR_STEPPER,
    .fixed_value = 25,
    .stepper_steps = 120,
    .pin_phase_a = 4,
    .pin_phase_b = 5,
    .pin_phase_c = 6,
    .pin_phase_d = 7,
  },
};

int config_valid() {
  if (config.ve && !table_valid_twoaxis(config.ve)) {
    return 0;
  }

  if (config.timing && !table_valid_twoaxis(config.timing)) {
    return 0;
  }

  if (config.injector_deadtime_offset &&
      !table_valid_oneaxis(config.injector_deadtime_offset)) {
    return 0;
  }

  if (config.injector_pw_correction &&
      !table_valid_oneaxis(config.injector_pw_correction)) {
    return 0;
  }

  if (config.commanded_lambda &&
      !table_valid_twoaxis(config.commanded_lambda)) {
    return 0;
  }

  if (config.tipin_enrich_amount &&
      !table_valid_twoaxis(config.tipin_enrich_amount)) {
    return 0;
  }

  if (config.tipin_enrich_duration &&
      !table_valid_oneaxis(config.tipin_enrich_duration)) {
    return 0;
  }

  return 1;
}
