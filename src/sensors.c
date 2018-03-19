#include "sensors.h"
#include "config.h"
#include "platform.h"

static volatile int adc_data_ready;
static volatile int freq_data_ready;

static float sensor_convert_linear(struct sensor_input *in) {
  float partial = in->raw_value / 4096.0f;
  return in->params.range.min + partial * 
    (in->params.range.max - in->params.range.min);
}

static float sensor_convert_freq(struct sensor_input *in) {
  float tickrate = TICKRATE;
  if (in->raw_value) {
    return 1.0/((in->raw_value*SENSOR_FREQ_DIVIDER)/tickrate);
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

  float old_value = in->processed_value;
  switch (in->method) {
    case METHOD_LINEAR:
      in->processed_value = sensor_convert_linear(in);
      break;
    case METHOD_TABLE:
      in->processed_value = interpolate_table_oneaxis(in->params.table, in->raw_value);
      break;
  }


  /* Do lag filtering */
  in->processed_value = ((old_value * in->lag) +
                        (in->processed_value * (100.0 - in->lag))) / 100.0;

}
  
void
sensors_process() {
  if (adc_data_ready) {
    adc_data_ready = 0;
    for (int i = 0; i < NUM_SENSORS; ++i) {
      if (config.sensors[i].source == SENSOR_ADC) {
        sensor_convert(&config.sensors[i]);
      }
    }
  }
  if (freq_data_ready) {
    freq_data_ready = 0;
    for (int i = 0; i < NUM_SENSORS; ++i) {
      if (config.sensors[i].source == SENSOR_FREQ) {
        /* TODO: fix freq */
        sensor_convert(&config.sensors[i]);
      }
    }
  }
  for (int i = 0; i < NUM_SENSORS; ++i) {
    if (config.sensors[i].source == SENSOR_CONST) {
      config.sensors[i].processed_value = config.sensors[i].params.fixed_value;
    }
  }

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
  ck_assert(sensor_convert_linear(&si) == -10.0);

  si.raw_value = 4096.0;
  ck_assert(sensor_convert_linear(&si) == 10.0);

  si.raw_value = 2048.0;
  ck_assert(sensor_convert_linear(&si) == 0.0);

} END_TEST

START_TEST(check_sensor_convert_freq) {
  struct sensor_input si;

  si.raw_value = 100.0;
  ck_assert_float_eq_tol(sensor_convert_freq(&si), 9.765625, .1);

  si.raw_value = 1000.0;
  ck_assert_float_eq_tol(sensor_convert_freq(&si), 0.976562, .1);

  si.raw_value = 0.0;
  ck_assert_float_eq_tol(sensor_convert_freq(&si), 0.0, 0.01);

} END_TEST

TCase *setup_sensor_tests() {
  TCase *sensor_tests = tcase_create("sensors");
  tcase_add_test(sensor_tests, check_sensor_convert_linear);
  tcase_add_test(sensor_tests, check_sensor_convert_freq);
  return sensor_tests;
}

#endif
