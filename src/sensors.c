#include "sensors.h"
#include "config.h"
#include "platform.h"
#include "stats.h"

static volatile int adc_data_ready;
static volatile int freq_data_ready;

static float sensor_convert_linear(struct sensor_input *in, float raw) {
  float partial = raw / 4096.0f;
  return in->params.range.min + partial * 
    (in->params.range.max - in->params.range.min);
}

/* Returns 0 - 4095 for 0 Hz - 2 kHz */
static float sensor_convert_freq(float raw) {
  float tickrate = TICKRATE;
  if (raw) {
    return 40.96f * 1.0 / ((raw * SENSOR_FREQ_DIVIDER )/ tickrate);
  } else {
    return 0.0; /* Prevent Div by Zero */
  }

}

static void sensor_convert(struct sensor_input *in) {
  /* Handle conn and range fault conditions */
  if ((in->fault == FAULT_NONE) &&
      (in->fault_config.max != 0)) {
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
  }


  /* Do lag filtering */
  in->processed_value = ((old_value * in->lag) +
                        (in->processed_value * (100.0 - in->lag))) / 100.0;

}
  
void
sensors_process() {
  stats_start_timing(STATS_SENSORS_TIME);
  for (int i = 0; i < NUM_SENSORS; ++i) {
    switch(config.sensors[i].source) {
      case SENSOR_ADC:
        if (adc_data_ready) {
          sensor_convert(&config.sensors[i]);
        }
        break;
      case SENSOR_FREQ:
        if (freq_data_ready) {
          sensor_convert(&config.sensors[i]);
        }
        break;
      case SENSOR_DIGITAL:
      case SENSOR_PWM:
      case SENSOR_CONST:
        sensor_convert(&config.sensors[i]);
        break;
      default:
        break;
    }
  }
  for (int i = 0; i < NUM_SENSORS; ++i) {
    if (config.sensors[i].source == SENSOR_CONST) {
      config.sensors[i].processed_value = config.sensors[i].params.fixed_value;
    }
  }

  stats_finish_timing(STATS_SENSORS_TIME);
}

void sensor_adc_new_data() {
  adc_data_ready = 1;
}

void sensor_freq_new_data() {
  freq_data_ready = 1;
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

} END_TEST

START_TEST(check_sensor_convert_freq) {
  ck_assert_float_eq_tol(sensor_convert_freq(100.0), 400, .1);

  ck_assert_float_eq_tol(sensor_convert_freq(1000.0), 40, .1);

  ck_assert_float_eq_tol(sensor_convert_freq(0.0), 0.0, 0.01);

} END_TEST

TCase *setup_sensor_tests() {
  TCase *sensor_tests = tcase_create("sensors");
  tcase_add_test(sensor_tests, check_sensor_convert_linear);
  tcase_add_test(sensor_tests, check_sensor_convert_freq);
  return sensor_tests;
}

#endif
