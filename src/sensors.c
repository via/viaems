#include <math.h>

#include "config.h"
#include "platform.h"
#include "sensors.h"
#include "stats.h"

static float sensor_convert_linear(struct sensor_input *in, float raw) {
  float partial = raw / 4096.0f;
  return in->params.range.min +
         partial * (in->params.range.max - in->params.range.min);
}

static float sensor_convert_freq(float raw) {
  if (!raw) {
    return 0.0; /* Prevent div by zero */
  }
  return (float)TICKRATE / raw;
}

float sensor_convert_thermistor(struct thermistor_config *tc, float raw) {
  stats_start_timing(STATS_SENSOR_THERM_TIME);
  float r = tc->bias / ((4096.0f / raw) - 1);
  float t = 1 / (tc->a + tc->b * logf(r) + tc->c * powf(logf(r), 3));
  stats_finish_timing(STATS_SENSOR_THERM_TIME);

  return t - 273.15f;
}

static void sensor_convert(struct sensor_input *in) {
  /* Handle conn and range fault conditions */
  if ((in->fault == FAULT_NONE) && (in->fault_config.max != 0)) {
    if ((in->fault_config.min > in->raw_value) ||
        (in->fault_config.max < in->raw_value)) {
      in->fault = FAULT_RANGE;
    }
  }
  if (in->fault != FAULT_NONE) {
    in->processed_value = in->fault_config.fault_value;
    return;
  }

  float raw;
  switch (in->source) {
  case SENSOR_ADC:
    raw = in->raw_value;
    break;
  case SENSOR_FREQ:
    raw = sensor_convert_freq(in->raw_value);
    break;
  case SENSOR_CONST:
    in->processed_value = in->params.fixed_value;
    return;
  default:
    raw = 0.0;
    break;
  }

  float old_value = in->processed_value;
  switch (in->method) {
  case METHOD_LINEAR:
    in->processed_value = sensor_convert_linear(in, raw);
    break;
  case METHOD_TABLE:
    in->processed_value = interpolate_table_oneaxis(in->params.table, raw);
    break;
  case METHOD_THERM:
    in->processed_value = sensor_convert_thermistor(&in->params.therm, raw);
    break;
  }

  /* Do lag filtering */
  in->processed_value =
    ((old_value * in->lag) + (in->processed_value * (100.0 - in->lag))) / 100.0;

  /* Process derivative */
  timeval_t process_time = current_time();
  if (process_time != in->process_time) {
    in->derivative = TICKRATE * (in->processed_value - old_value) /
                     (process_time - in->process_time);
  }
  in->process_time = process_time;
}

void sensors_process(sensor_source source) {
  for (int i = 0; i < NUM_SENSORS; ++i) {
    if (config.sensors[i].source != source) {
      continue;
    }
    sensor_convert(&config.sensors[i]);
  }
}
uint32_t sensor_fault_status() {
  uint32_t faults = 0;
  for (int i = 0; i < NUM_SENSORS; ++i) {
    if (config.sensors[i].fault != FAULT_NONE) {
      faults |= (1 << i);
    }
  }
  return faults;
}

#ifdef UNITTEST
#include <check.h>

START_TEST(check_sensor_convert_linear) {
  struct sensor_input si = {
    .params = {
      .range = { .min=-10.0, .max=10.0},
    },
  };

  si.raw_value = 0;
  ck_assert(sensor_convert_linear(&si, si.raw_value) == -10.0);

  si.raw_value = 4096.0;
  ck_assert(sensor_convert_linear(&si, si.raw_value) == 10.0);

  si.raw_value = 2048.0;
  ck_assert(sensor_convert_linear(&si, si.raw_value) == 0.0);
}
END_TEST

START_TEST(check_sensor_convert_freq) {
  ck_assert_float_eq_tol(sensor_convert_freq(100.0), 40000, .1);

  ck_assert_float_eq_tol(sensor_convert_freq(1000.0), 4000, .1);

  ck_assert_float_eq_tol(sensor_convert_freq(0.0), 0.0, 0.01);
}
END_TEST

START_TEST(check_sensor_convert_therm) {
  // test parameters for my CHT sensor
  struct thermistor_config tc = {
    .bias = 2490.0,
    .a = 0.00131586818223649,
    .b = 0.000256187001401003,
    .c = 1.84741994569279E-07,
  };

  ck_assert_float_eq_tol(sensor_convert_thermistor(&tc, 2048), 20.31, 0.2);

  ck_assert_float_eq_tol(sensor_convert_thermistor(&tc, 4092), -97.43, 0.2);
}
END_TEST

TCase *setup_sensor_tests() {
  TCase *sensor_tests = tcase_create("sensors");
  tcase_add_test(sensor_tests, check_sensor_convert_linear);
  tcase_add_test(sensor_tests, check_sensor_convert_freq);
  tcase_add_test(sensor_tests, check_sensor_convert_therm);
  return sensor_tests;
}

#endif
