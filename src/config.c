#include "config.h"
#include "calculations.h"
#include "sensors.h"

#include "console.pb.h"
#include "table.h"

struct config default_config __attribute__((section(".configdata"))) = {
  .outputs = {
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
  .trigger_inputs = {
    [0] = {.edge = RISING_EDGE, .type = TRIGGER},
    [1] = {.edge = RISING_EDGE, .type = SYNC},
    [2] = {.edge = RISING_EDGE, .type = FREQ},
    [3] = {.edge = FALLING_EDGE, .type = FREQ},
  },
  .sensors = {
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
    .KNK1 = { .frequency = 7000 },
    .KNK2 = { .frequency = 7000 },
  },
  .timing = {
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
  },
  .injector_deadtime_offset = {
    .title = "dead_time",
    .cols = { .name = "BRV", .num = 16,
      .values = {8.5, 9, 9.5, 10, 10.5, 11, 11.5, 12, 12.5, 13, 13.5, 14, 14.5, 15, 15.5, 16},
    },
    .data = {
      2.97, 2.66, 2.39, 2.155, 1.96, 1.82, 1.70, 1.59, 1.48, 1.4, 1.32, 1.25, 1.19, 1.11, 1.05, 1,
    },
  },
  .injector_pw_correction = {
    .title = "pulse_width_correction",
    .cols = { .name = "PW(ms)", .num = 6,
      .values = {0, 0.333, 0.521, 0.816, 1.278, 2.0},
    },
    .data = {0, 0.090, 0.086, 0.038, 0.023, 0},
  },
  .ve = {
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
},
  .commanded_lambda = {
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
  },
  .engine_temp_enrich = {
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
  },
  .tipin_enrich_amount = {
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
  },
  .tipin_enrich_duration = {
    .title = "tipin_enrich_duration",
    .cols = { 
      .name = "RPM", .num = 4,
      .values = {0, 1000, 2000, 4000},
    },
    .data = {
      300.0, 150.0, 80.0, 40.0
    },
  },
  .dwell = {
    .title = "dwell",
    .cols = { 
      .name = "BRV", .num = 4,
      .values = {5, 10, 14, 18},
    },
    .data = {
      6.0, 4.0, 2.5, 1.8
    },
  },
  .rpm_stop = 6700,
  .rpm_start = 6200,
  .fueling = {
    .injector_cc_per_minute = 1015,
    .cylinder_cc = 500,
    .fuel_stoich_ratio = 14.7,
    .injections_per_cycle = 1, /* All batched */
    .max_duty_cycle = 95.0,
    .fuel_pump_pin = 0,
    .density_of_fuel = 0.755, /* g/cm^3 at 15C */
    .crank_enrich_config = {
      .crank_rpm = 400,
      .cutoff_temperature = 20.0,
      .enrich_amt = 2.5,
    },
  },
  .ignition = {
    .dwell = DWELL_BRV,
    .min_coil_cooldown_us = 500,
    .min_dwell_us = 1000,
    .ignitions_per_cycle = 2, /* Wasted spark */
  },
  .boost_control = {
    .pwm_duty_vs_rpm = {
      .title = "boost_control",
      .cols = { .name = "RPM", .num = 6,
        .values = {1000.0, 2000.0, 3000.0, 4000.0, 5000.0, 6000.0},
      },
      .data = {
        50, 50, 50, 50, 50, 50,
      },
    },
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
};


static void store_output_config(const struct output_event_config *ev, struct viaems_console_Configuration_Output *ev_msg) {
  switch (ev->type) {
    case DISABLED_EVENT: 
			ev_msg->type = viaems_console_Configuration_Output_OutputType_OutputDisabled;
      break;
    case FUEL_EVENT: 
			ev_msg->type = viaems_console_Configuration_Output_OutputType_Fuel;
      break;
    case IGNITION_EVENT: 
			ev_msg->type = viaems_console_Configuration_Output_OutputType_Ignition;
      break;
  }
  ev_msg->pin = ev->pin;
  ev_msg->inverted = ev->inverted;
  ev_msg->angle = ev-> angle;

  ev_msg->has_type = true;
  ev_msg->has_pin = true;
  ev_msg->has_inverted = true;
  ev_msg->has_angle = true;
}

static void store_trigger_config(const struct trigger_input *t, struct viaems_console_Configuration_TriggerInput *t_msg) {
  switch (t->edge) {
    case RISING_EDGE:
      t_msg->edge = viaems_console_Configuration_InputEdge_Rising;
      break;
    case FALLING_EDGE:
      t_msg->edge = viaems_console_Configuration_InputEdge_Falling;
      break;
    case BOTH_EDGES:
      t_msg->edge = viaems_console_Configuration_InputEdge_Both;
      break;
  }

  switch (t->type) {
    case NONE:
      t_msg->type = viaems_console_Configuration_InputType_InputDisabled;
      break;
    case FREQ:
      t_msg->type = viaems_console_Configuration_InputType_Freq;
      break;
    case TRIGGER:
      t_msg->type = viaems_console_Configuration_InputType_Trigger;
      break;
    case SYNC:
      t_msg->type = viaems_console_Configuration_InputType_Sync;
      break;
  }

  t_msg->has_edge = true;
  t_msg->has_type = true;
}

static void store_sensor_config(const struct sensor_config *config, struct viaems_console_Configuration_Sensor *msg) {
  msg->has_source = true;
  switch (config->source) {
    case SENSOR_NONE:
      msg->source = viaems_console_Configuration_SensorSource_None;
      break;
    case SENSOR_ADC:
      msg->source = viaems_console_Configuration_SensorSource_Adc;
      break;
    case SENSOR_FREQ:
      msg->source = viaems_console_Configuration_SensorSource_Frequency;
      break;
    case SENSOR_PULSEWIDTH:
      msg->source = viaems_console_Configuration_SensorSource_Pulsewidth;
      break;
    case SENSOR_CONST:
      msg->source = viaems_console_Configuration_SensorSource_Const;
      break;
  }
  msg->has_method = true;
  switch (config->method) {
    case METHOD_LINEAR:
      msg->method = viaems_console_Configuration_SensorMethod_Linear;
      break;
    case METHOD_LINEAR_WINDOWED:
      msg->method = viaems_console_Configuration_SensorMethod_LinearWindowed;
      break;
    case METHOD_THERM:
      msg->method = viaems_console_Configuration_SensorMethod_Thermistor;
      break;
  }
  msg->has_pin = true;
  msg->pin = config->pin;
  msg->has_lag = true;
  msg->lag = config->lag;

  if (config->source == SENSOR_CONST) {
    msg->has_const_config = true;
    msg->const_config.fixed_value = config->const_value;
  } else if ((config->method == METHOD_LINEAR) || (config->method == METHOD_LINEAR_WINDOWED)) {
    msg->has_linear_config = true;
    msg->linear_config.output_min = config->range.min;
    msg->linear_config.output_max = config->range.max;
    msg->linear_config.input_min = config->raw_min;
    msg->linear_config.input_max = config->raw_max;
    if (config->method == METHOD_LINEAR_WINDOWED) {
      msg->has_window_config = true;
      msg->window_config.capture_width = config->window.capture_width;
      msg->window_config.total_width = config->window.total_width;
      msg->window_config.offset = config->window.offset;
    }
  } else if (config->method == METHOD_THERM) {
    msg->has_thermistor_config = true;
    msg->thermistor_config.A = config->therm.a;
    msg->thermistor_config.B = config->therm.b;
    msg->thermistor_config.C = config->therm.c;
    msg->thermistor_config.bias = config->therm.bias;
  }

  msg->has_fault_config = true;
  msg->fault_config.min = config->fault_config.min;
  msg->fault_config.max = config->fault_config.max;
  msg->fault_config.value = config->fault_config.fault_value;
}

static void store_knock_config(const struct knock_sensor_config *config, struct viaems_console_Configuration_KnockSensor *msg) {
  msg->has_enabled = true;
  msg->enabled = true;
  msg->has_frequency = true;
  msg->frequency = config->frequency;
  msg->has_threshold = true;
  msg->threshold = config->threshold;
}

static void store_table_axis(const struct table_axis *axis, struct viaems_console_Configuration_TableAxis *msg) {
  size_t name_len = strlen(axis->name);
  msg->has_name = true;
  msg->name.len = name_len;
  memcpy(msg->name.str, axis->name, name_len);
  msg->values_count = axis->num;
  memcpy(msg->values, axis->values, sizeof(float) * axis->num);
}

static void store_table_row(const float *row, size_t count, struct viaems_console_Configuration_TableRow *msg) {
  msg->values_count = count;
  memcpy(msg->values, row, sizeof(float) * count);
}

static void store_table1d(const struct table_1d *table, struct viaems_console_Configuration_Table1d *msg) {
  size_t name_len = strlen(table->title);
  msg->has_name = true;
  msg->name.len = name_len;
  memcpy(msg->name.str, table->title, name_len);
  
  msg->has_cols = true;
  store_table_axis(&table->cols, &msg->cols);

  msg->has_data = true;
  store_table_row(table->data, table->cols.num, &msg->data);

}

static void store_table2d(const struct table_2d *table, struct viaems_console_Configuration_Table2d *msg) {
  size_t name_len = strlen(table->title);
  msg->has_name = true;
  msg->name.len = name_len;
  memcpy(msg->name.str, table->title, name_len);
  
  msg->has_cols = true;
  store_table_axis(&table->cols, &msg->cols);

  msg->has_rows = true;
  store_table_axis(&table->rows, &msg->rows);

  msg->data_count = table->rows.num;
  for (size_t row = 0; row < table->rows.num; row++) {
    store_table_row(table->data[row], table->cols.num, &msg->data[row]);
  }

}

void config_store_to_console_pbtype(const struct config *config, struct viaems_console_Configuration *msg) {

  msg->outputs_count = MAX_EVENTS;
  for (int i = 0; i < MAX_EVENTS; i++) {
    store_output_config(&config->outputs[i], &msg->outputs[i]);
	}


  msg->triggers_count = 4;
  for (int i = 0; i < 4; i++) {
    store_trigger_config(&config->trigger_inputs[i], &msg->triggers[i]);
	}

  msg->has_sensors = true;
  msg->sensors.has_AmbientAirPressure = true;
  store_sensor_config(&config->sensors.AAP, &msg->sensors.AmbientAirPressure);
  msg->sensors.has_BatteryVoltage = true;
  store_sensor_config(&config->sensors.BRV, &msg->sensors.BatteryVoltage);
  msg->sensors.has_CoolantTemperature = true;
  store_sensor_config(&config->sensors.CLT, &msg->sensors.CoolantTemperature);
  msg->sensors.has_ExhaustGasOxygen = true;
  store_sensor_config(&config->sensors.EGO, &msg->sensors.ExhaustGasOxygen);
  msg->sensors.has_FuelRailTemperature = true;
  store_sensor_config(&config->sensors.FRT, &msg->sensors.FuelRailTemperature);
  msg->sensors.has_IntakeAirTemperature = true;
  store_sensor_config(&config->sensors.IAT, &msg->sensors.IntakeAirTemperature);
  msg->sensors.has_ManifoldPressure = true;
  store_sensor_config(&config->sensors.MAP, &msg->sensors.ManifoldPressure);
  msg->sensors.has_ThrottlePosition = true;
  store_sensor_config(&config->sensors.TPS, &msg->sensors.ThrottlePosition);
  msg->sensors.has_FuelRailPressure = true;
  store_sensor_config(&config->sensors.FRP, &msg->sensors.FuelRailPressure);
  msg->sensors.has_EthanolContent = true;
  store_sensor_config(&config->sensors.ETH, &msg->sensors.EthanolContent);
  msg->sensors.has_knock1 = true;
  store_knock_config(&config->sensors.KNK1, &msg->sensors.knock1);
  msg->sensors.has_knock2 = true;
  store_knock_config(&config->sensors.KNK2, &msg->sensors.knock2);

  msg->has_ignition = true;
  msg->ignition.has_type = true;
  switch (config->ignition.dwell) {
    case DWELL_FIXED_DUTY:
      msg->ignition.type = viaems_console_Configuration_Ignition_DwellType_FixedDuty;
      break;
    case DWELL_FIXED_TIME:
      msg->ignition.type = viaems_console_Configuration_Ignition_DwellType_FixedTime;
      break;
    case DWELL_BRV:
      msg->ignition.type = viaems_console_Configuration_Ignition_DwellType_BatteryVoltage;
      break;
  }
  msg->ignition.has_fixed_dwell = true;
  msg->ignition.fixed_dwell = config->ignition.dwell_us;

  msg->ignition.has_dwell = true;
  store_table1d(&config->dwell, &msg->ignition.dwell);

  msg->ignition.has_timing = true;
  store_table2d(&config->timing, &msg->ignition.timing);

  msg->has_fueling = true;
  msg->fueling.has_fuel_pump_pin = true;
  msg->fueling.fuel_pump_pin = config->fueling.fuel_pump_pin;
  msg->fueling.has_cylinder_cc = true;
  msg->fueling.cylinder_cc = config->fueling.cylinder_cc;
  msg->fueling.has_fuel_density = true;
  msg->fueling.fuel_density = config->fueling.density_of_fuel;
  msg->fueling.has_fuel_stoich_ratio = true;
  msg->fueling.fuel_stoich_ratio = config->fueling.fuel_stoich_ratio;
  msg->fueling.has_injections_per_cycle = true;
  msg->fueling.injections_per_cycle = config->fueling.injections_per_cycle;
  msg->fueling.has_injector_cc = true;
  msg->fueling.injector_cc = config->fueling.injector_cc_per_minute;
  msg->fueling.has_max_duty_cycle = true;
  msg->fueling.max_duty_cycle = config->fueling.max_duty_cycle;
  msg->fueling.has_crank_enrich = true;
  msg->fueling.crank_enrich.has_cranking_rpm = true;
  msg->fueling.crank_enrich.cranking_rpm = config->fueling.crank_enrich_config.crank_rpm;
  msg->fueling.crank_enrich.has_cranking_temp = true;
  msg->fueling.crank_enrich.cranking_temp = config->fueling.crank_enrich_config.cutoff_temperature;
  msg->fueling.crank_enrich.has_enrich_amt = true;
  msg->fueling.crank_enrich.enrich_amt = config->fueling.crank_enrich_config.enrich_amt;

  msg->fueling.has_PulseWidthCompensation = true;
  store_table1d(&config->injector_pw_correction, &msg->fueling.PulseWidthCompensation);
  msg->fueling.has_InjectorDeadTime = true;
  store_table1d(&config->injector_deadtime_offset, &msg->fueling.InjectorDeadTime);
  msg->fueling.has_EngineTempEnrichment = true;
  store_table2d(&config->engine_temp_enrich, &msg->fueling.EngineTempEnrichment);
  msg->fueling.has_commanded_lambda = true;
  store_table2d(&config->commanded_lambda, &msg->fueling.commanded_lambda);
  msg->fueling.has_ve = true;
  store_table2d(&config->ve, &msg->fueling.ve);
  msg->fueling.has_tipin_enrich_amount = true;
  store_table2d(&config->tipin_enrich_amount, &msg->fueling.tipin_enrich_amount);
  msg->fueling.has_tipin_enrich_duration = true;
  store_table1d(&config->tipin_enrich_duration, &msg->fueling.tipin_enrich_duration);

}
