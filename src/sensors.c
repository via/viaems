#include <math.h>

#include "config.h"
#include "decoder.h"
#include "platform.h"
#include "sensors.h"

static float sensor_convert_linear(const struct sensor_input *in) {
  if (in->raw_max == in->raw_min) {
    return 0.0f;
  }
  float partial = (in->raw_value - in->raw_min) / (in->raw_max - in->raw_min);
  return in->range.min + partial * (in->range.max - in->range.min);
}

static int current_angle_in_window(const struct sensor_input *in,
                                   degrees_t angle) {
  degrees_t cur_angle = clamp_angle(angle - in->window.offset, 720);
  for (uint32_t i = 0; i < 720; i += in->window.total_width) {
    if ((cur_angle >= i) && (cur_angle < i + in->window.capture_width)) {
      return 1;
    }
  }
  return 0;
}

static float sensor_convert_linear_windowed(struct sensor_input *in,
                                            degrees_t angle) {
  if (!config.decoder.rpm) {
    /* If engine not turning, don't window */
    return sensor_convert_linear(in);
  }

  float result = in->value;

  /* If not in a window, or we have exceeded a window size */
  if (!current_angle_in_window(in, angle) ||
      clamp_angle(angle - in->window.collection_start_angle, 720) >=
        in->window.capture_width) {

    if (in->window.collecting && in->window.samples) {
      in->window.collecting = 0;
      result = in->window.accumulator / in->window.samples;
    }
  }

  /* Currently in a window. If we just started collecting, initialize */
  if (current_angle_in_window(in, angle)) {
    if (!in->window.collecting) {
      /* TODO: change this to current window start, not current angle */
      in->window.collection_start_angle = angle;
      in->window.accumulator = 0;
      in->window.samples = 0;
    }
    /* Accumulate */
    in->window.collecting = 1;
    in->window.accumulator += sensor_convert_linear(in);
    in->window.samples += 1;
  }

  /* Return previous processed value */
  return result;
}

float sensor_convert_thermistor(const struct sensor_input *in) {
  if (in->raw_value == 0.0f) {
    return 0.0f;
  }

  const struct thermistor_config *tc = &in->therm;
  float r = tc->bias / ((in->raw_max / in->raw_value) - 1);
  float logf_r = logf(r);
  float t = 1 / (tc->a + tc->b * logf_r + tc->c * logf_r * logf_r * logf_r);

  return t - 273.15f;
}

static sensor_fault detect_faults(const struct sensor_input *in) {
  if (in->fault != FAULT_NONE) {
    /* Some faults come from platform code before this point, pass it back */
    return in->fault;
  }
  /* Handle conn and range fault conditions */
  if ((in->fault_config.max != 0)) {
    if ((in->fault_config.min > in->raw_value) ||
        (in->fault_config.max < in->raw_value)) {
      return FAULT_RANGE;
    }
  }
  return FAULT_NONE;
}

static float process_derivative(const struct sensor_input *in) {
  return TICKRATE * (in->value - in->previous_value) /
         time_from_us(1000000 / platform_adc_samplerate());
}

static float process_lag_filter(const struct sensor_input *in,
                                float new_value) {
  if (in->lag == 0.0f) {
    return new_value;
  }
  return ((in->previous_value * in->lag) + (new_value * (100.0f - in->lag))) /
         100.0f;
}

static void sensor_convert(struct sensor_input *in, float raw, timeval_t time) {
  in->raw_value = raw;
  in->time = time;

  float new_value = 0.0f;
  float new_derivative = 0.0f;

  if (detect_faults(in) != FAULT_NONE) {
    new_value = in->fault_config.fault_value;
  } else {

    switch (in->method) {
    case METHOD_LINEAR:
      new_value = sensor_convert_linear(in);
      break;
    case METHOD_LINEAR_WINDOWED:
      new_value = sensor_convert_linear_windowed(in, current_angle());
      break;
    case METHOD_THERM:
      new_value = sensor_convert_thermistor(in);
      break;
    }

    new_value = process_lag_filter(in, new_value);
    new_derivative = process_derivative(in);
    in->previous_value = new_value;
  }

  /* in->value and in->derivative are read by decode/calculation in interrupts,
   * so these race */
  disable_interrupts();
  in->value = new_value;
  in->derivative = new_derivative;
  enable_interrupts();
}

void sensor_update_freq(const struct freq_update *u) {
  for (int i = 0; i < NUM_SENSORS; ++i) {
    struct sensor_input *in = &config.sensors[i];
    if (in->pin != u->pin) {
      continue;
    }
    switch (in->source) {
    case SENSOR_FREQ:
      in->fault = u->valid ? FAULT_NONE : FAULT_CONN;
      sensor_convert(in, u->frequency, u->time);
      break;
    case SENSOR_PULSEWIDTH:
      in->fault = u->valid ? FAULT_NONE : FAULT_CONN;
      sensor_convert(in, u->pulsewidth, u->time);
      break;
    default:
      continue;
    }
  }
}

void sensor_update_adc(const struct adc_update *update) {
  for (int i = 0; i < NUM_SENSORS; ++i) {
    struct sensor_input *in = &config.sensors[i];
    if (in->source != SENSOR_ADC) {
      continue;
    }
    if (in->pin >= MAX_ADC_PINS) {
      continue;
    }
    in->fault = update->valid ? FAULT_NONE : FAULT_CONN;
    sensor_convert(in, update->values[in->pin], update->time);
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

void knock_configure(struct knock_input *knock) {
  uint32_t samplerate = platform_knock_samplerate();
  float bucketsize = samplerate / 64.0f;
  uint32_t bucket = (knock->frequency + bucketsize) / bucketsize;

  knock->state.width = 64;
  knock->state.freq = bucket;
  knock->state.w =
    2.0f * 3.14159f * (float)knock->state.freq / (float)knock->state.width;
  knock->state.cr = cosf(knock->state.w);

  knock->state.n_samples = 0;
  knock->state.sprev = 0.0f;
  knock->state.sprev2 = 0.0f;
};

static void knock_add_sample(struct knock_input *knock, float sample) {

  knock->state.n_samples += 1;
  float s =
    sample + knock->state.cr * 2 * knock->state.sprev - knock->state.sprev2;
  knock->state.sprev2 = knock->state.sprev;
  knock->state.sprev = s;

  if (knock->state.n_samples == 64) {
    knock->value =
      knock->state.sprev * knock->state.sprev +
      knock->state.sprev2 * knock->state.sprev2 -
      knock->state.sprev * knock->state.sprev2 * knock->state.cr * 2;

    knock->state.n_samples = 0;
    knock->state.sprev = 0;
    knock->state.sprev2 = 0;
  }
}

void sensor_update_knock(const struct knock_update *update) {
  struct knock_input *in =
    (update->pin == 0) ? &config.knock_inputs[0] : &config.knock_inputs[1];

  for (int i = 0; i < update->n_samples; i++) {
    knock_add_sample(in, update->samples[i]);
  }
}

#ifdef UNITTEST
#include <check.h>

START_TEST(check_sensor_convert_linear) {
  struct sensor_input si = {
    .range = { .min = -10.0, .max = 10.0 },
    .raw_max = 4096.0f,
  };

  si.raw_value = 0;
  ck_assert(sensor_convert_linear(&si) == -10.0);

  si.raw_value = 4096.0;
  ck_assert(sensor_convert_linear(&si) == 10.0);

  si.raw_value = 2048.0;
  ck_assert(sensor_convert_linear(&si) == 0.0);
}
END_TEST

START_TEST(check_sensor_convert_linear_windowed) {
  struct sensor_input si = {
    .value = 12.0,
    .range = { .min=0, .max=4096.0},
    .raw_max = 4096.0,
    .window = {
      .total_width = 90,
      .capture_width = 45,
    },
  };

  config.decoder.rpm = 1000;

  si.raw_value = 2048.0f;
  ck_assert_float_eq_tol(sensor_convert_linear_windowed(&si, 0), 12.0, 0.01);

  si.raw_value = 2500.0f;
  ck_assert_float_eq_tol(sensor_convert_linear_windowed(&si, 10), 12.0, 0.01);

  si.raw_value = 2600.0f;
  ck_assert_float_eq_tol(sensor_convert_linear_windowed(&si, 40), 12.0, 0.01);

  /* End of window, should average first three results together */
  si.raw_value = 2700.0f;
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&si, 50), 2382.666, 0.1);

  /* All values in this non-capture window should be ignored */
  si.raw_value = 2800.0f;
  ck_assert_float_eq_tol(sensor_convert_linear_windowed(&si, 60), 12, 0.1);
  si.raw_value = 2800.0f;
  ck_assert_float_eq_tol(sensor_convert_linear_windowed(&si, 80), 12, 0.1);
}
END_TEST

START_TEST(check_sensor_convert_linear_windowed_skipped) {
  struct sensor_input si = {
    .value = 12.0,
    .range = { .min=0, .max=4096.0},
    .raw_max = 4096.0,
    .window = {
      .total_width = 90,
      .capture_width = 45,
    },
  };

  config.decoder.rpm = 1000;

  si.raw_value = 2048;
  ck_assert_float_eq_tol(sensor_convert_linear_windowed(&si, 0), 12.0, 0.01);

  si.raw_value = 2500;
  ck_assert_float_eq_tol(sensor_convert_linear_windowed(&si, 10), 12.0, 0.01);

  si.raw_value = 2600;
  ck_assert_float_eq_tol(sensor_convert_linear_windowed(&si, 40), 12.0, 0.01);

  /* Move into next capture window, it should still produce average */
  si.raw_value = 2700;
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&si, 100), 2382.666, 0.1);

  /* Remaining samples in this window should add to average */
  si.raw_value = 1000;
  ck_assert_float_eq_tol(sensor_convert_linear_windowed(&si, 120), 12, 0.1);

  si.raw_value = 1500;
  ck_assert_float_eq_tol(sensor_convert_linear_windowed(&si, 130), 12, 0.1);

  /* Reaches end of window, should average two prior samples */
  si.raw_value = 2000;
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&si, 140), 1733.33, 0.1);
}
END_TEST

START_TEST(check_sensor_convert_linear_windowed_wide) {
  struct sensor_input si = {
    .value = 12.0,
    .range = { .min=0, .max=4096.0},
    .raw_max = 4096.0,
    .window = {
      .total_width = 90,
      .capture_width = 90,
    },
  };

  config.decoder.rpm = 1000;

  si.raw_value = 2048;
  ck_assert_float_eq_tol(sensor_convert_linear_windowed(&si, 0), 12.0, 0.01);

  si.raw_value = 2500;
  ck_assert_float_eq_tol(sensor_convert_linear_windowed(&si, 45), 12.0, 0.01);

  si.raw_value = 2600;
  ck_assert_float_eq_tol(sensor_convert_linear_windowed(&si, 89), 12.0, 0.01);

  /* Move into next capture window, it should still produce average of all
   * three*/
  si.raw_value = 2700;
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&si, 100), 2382.666, 0.1);

  /* Remaining samples in this window should add to average at end */
  si.raw_value = 1000;
  ck_assert_float_eq_tol(sensor_convert_linear_windowed(&si, 120), 12, 0.1);

  si.raw_value = 1500;
  ck_assert_float_eq_tol(sensor_convert_linear_windowed(&si, 130), 12, 0.1);

  /* TODO: right now this needs to be capture_width degrees past the first
   * sample in the last window, which is not ideal. We should just treat all
   * samples in the window as acceptable */
  /* Reaches end of window, should average all three prior samples */
  si.raw_value = 2000;
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&si, 210), 1733.33, 0.1);
}
END_TEST

START_TEST(check_sensor_convert_linear_windowed_offset) {
  struct sensor_input si = {
    .value = 12.0,
    .range = { .min=0, .max=4096.0},
    .raw_max = 4096.0,
    .window = {
      .total_width = 90,
      .capture_width = 45,
      .offset = 45,
    },
  };

  config.decoder.rpm = 1000;

  /* All values should be ignored */
  si.raw_value = 2048;
  ck_assert_float_eq_tol(sensor_convert_linear_windowed(&si, 0), 12.0, 0.01);

  si.raw_value = 2500;
  ck_assert_float_eq_tol(sensor_convert_linear_windowed(&si, 10), 12.0, 0.01);

  si.raw_value = 2600;
  ck_assert_float_eq_tol(sensor_convert_linear_windowed(&si, 40), 12.0, 0.01);

  /* Start of real window */
  si.raw_value = 2700;
  ck_assert_float_eq_tol(sensor_convert_linear_windowed(&si, 50), 12, 0.1);

  si.raw_value = 2400;
  ck_assert_float_eq_tol(sensor_convert_linear_windowed(&si, 80), 12, 0.1);

  /* End of window, should produce average of two prior samples */
  si.raw_value = 2000;
  ck_assert_float_eq_tol(sensor_convert_linear_windowed(&si, 90), 2550, 0.1);
}
END_TEST

START_TEST(check_sensor_convert_therm) {
  // test parameters for my CHT sensor
  struct sensor_input in = {
    .raw_max = 4096.0f,
    .therm = {
      .bias = 2490.0,
      .a = 0.00131586818223649,
      .b = 0.000256187001401003,
      .c = 1.84741994569279E-07,
    },
  };

  in.raw_value = 2048;
  ck_assert_float_eq_tol(sensor_convert_thermistor(&in), 20.31, 0.2);

  in.raw_value = 4092;
  ck_assert_float_eq_tol(sensor_convert_thermistor(&in), -97.43, 0.2);
}
END_TEST

START_TEST(check_current_angle_in_window) {
  struct sensor_input in = {
    .window = {
      .offset = 0,
      .total_width=100,
      .capture_width=60,
    },
  };
  ck_assert(current_angle_in_window(&in, 0));
  ck_assert(current_angle_in_window(&in, 50));
  ck_assert(!current_angle_in_window(&in, 70));

  ck_assert(current_angle_in_window(&in, 100));
  ck_assert(current_angle_in_window(&in, 120));
  ck_assert(!current_angle_in_window(&in, 170));

  ck_assert(!current_angle_in_window(&in, 690));
  ck_assert(current_angle_in_window(&in, 710));
}
END_TEST

START_TEST(check_knock_configure) {
  struct knock_input ki = {
    .frequency = 7000,
  };

  knock_configure(&ki);
  ck_assert_int_eq(ki.state.width, 64);
  ck_assert_int_eq(ki.state.freq, 9);
}
END_TEST

TCase *setup_sensor_tests() {
  TCase *sensor_tests = tcase_create("sensors");
  tcase_add_test(sensor_tests, check_sensor_convert_linear);
  tcase_add_test(sensor_tests, check_sensor_convert_linear_windowed);
  tcase_add_test(sensor_tests, check_sensor_convert_linear_windowed_skipped);
  tcase_add_test(sensor_tests, check_sensor_convert_linear_windowed_wide);
  tcase_add_test(sensor_tests, check_sensor_convert_linear_windowed_offset);
  tcase_add_test(sensor_tests, check_sensor_convert_therm);

  tcase_add_test(sensor_tests, check_current_angle_in_window);

  tcase_add_test(sensor_tests, check_knock_configure);
  return sensor_tests;
}

#endif
