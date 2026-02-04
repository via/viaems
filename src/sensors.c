#include <math.h>

#include "config.h"
#include "decoder.h"
#include "platform.h"
#include "sensors.h"
#include "util.h"

float sensor_convert_linear(const struct sensor_config *conf, const float raw) {
  if (conf->raw_max == conf->raw_min) {
    return 0.0f;
  }
  float partial = (raw - conf->raw_min) / (conf->raw_max - conf->raw_min);
  return conf->range.min + partial * (conf->range.max - conf->range.min);
}

static bool current_window_for_angle(const struct window_config *conf,
                                     degrees_t *window_start,
                                     degrees_t angle) {

  degrees_t window_size = 720.0 / conf->windows_per_cycle;
  int window_num = clamp_angle(angle - conf->window_offset, 720) / window_size;
  *window_start = window_num * window_size + conf->window_offset;
  degrees_t angle_in_window = clamp_angle(angle - *window_start, 720);
  if (angle_in_window < conf->window_opening) {
    return true;
  }
  return false;
}

static float sensor_convert_linear_windowed(struct sensor_state *state,
                                            degrees_t angle,
                                            float raw) {
  float result = state->output.value;

  degrees_t window_start;
  bool in_window = current_window_for_angle(&state->config->window, &window_start, angle);

  /* If not in a window, or we are not in the same window that we started accumulating with */
  if (!in_window || (window_start != state->window_start)) {
    if (state->window_collecting && state->window_sample_count > 0) {
      state->window_collecting = false;
      result = state->window_accumulator / state->window_sample_count;
    }
  }

  if (in_window) {
    if (!state->window_collecting) {
      state->window_start = window_start;
      state->window_accumulator = 0;
      state->window_sample_count = 0;
    }
    /* Accumulate */
    state->window_collecting = true;
    state->window_accumulator += sensor_convert_linear(state->config, raw);
    state->window_sample_count += 1;
  }

  /* Return previous processed value */
  return result;
}

float sensor_convert_thermistor(const struct sensor_config *conf,
                                const float raw) {
  if (raw == 0.0f) {
    return 0.0f;
  }

  const struct thermistor_config *tc = &conf->therm;
  float r = tc->bias / ((conf->raw_max / raw) - 1);
  float logf_r = logf(r);
  float t = 1 / (tc->a + tc->b * logf_r + tc->c * logf_r * logf_r * logf_r);

  return t - 273.15f;
}

static sensor_fault detect_faults(const struct sensor_state *s, float raw) {
  if (s->output.fault != FAULT_NONE) {
    /* Some faults come from platform code before this point, pass it back */
    return s->output.fault;
  }
  /* Handle conn and range fault conditions */
  if ((s->config->fault_config.max != 0)) {
    if ((s->config->fault_config.min > raw) ||
        (s->config->fault_config.max < raw)) {
      return FAULT_RANGE;
    }
  }
  return FAULT_NONE;
}

static float process_derivative(float previous_value, float new_value) {
  return TICKRATE * (new_value - previous_value) /
         time_from_us(1000000 / platform_adc_samplerate());
}

static float process_lag_filter(float lag, float old_value, float new_value) {
  if (lag == 0.0f) {
    return new_value;
  }
  return ((old_value * lag) + (new_value * (100.0f - lag))) / 100.0f;
}

static void sensor_update_raw(struct sensor_state *s,
                              const struct engine_position *d,
                              timeval_t time,
                              float raw) {
  const struct sensor_config *conf = s->config;
  struct sensor_value out;

  if ((out.fault = detect_faults(s, raw)) != FAULT_NONE) {
    out.value = conf->fault_config.fault_value;
    out.derivative = 0;
  } else {
    float new_value = 0.0f;
    switch (conf->method) {
    case METHOD_LINEAR:
      new_value = sensor_convert_linear(conf, raw);
      break;
    case METHOD_LINEAR_WINDOWED:
      if (d->has_position && (d->rpm > 0)) {
        new_value =
          sensor_convert_linear_windowed(s, engine_current_angle(d, time), raw);
      } else {
        new_value = sensor_convert_linear(conf, raw);
      }
      break;
    case METHOD_THERM:
      new_value = sensor_convert_thermistor(conf, raw);
      break;
    }

    out.value = process_lag_filter(conf->lag, s->output.value, new_value);
    out.derivative = process_derivative(s->output.value, out.value);
  }

  s->output = out;
}

static void update_single_freq_sensor(struct sensor_state *s,
                                      const struct engine_position *p,
                                      const struct freq_update *u) {
  if (s->config->pin != u->pin) {
    return;
  }

  if (s->config->source == SENSOR_FREQ) {
    sensor_update_raw(s, p, u->time, u->frequency);
    s->output.fault = u->valid ? FAULT_NONE : FAULT_CONN;
  } else if (s->config->source == SENSOR_PULSEWIDTH) {
    sensor_update_raw(s, p, u->time, u->pulsewidth);
    s->output.fault = u->valid ? FAULT_NONE : FAULT_CONN;
  }
}

void sensor_update_freq(struct sensors *s,
                        const struct engine_position *p,
                        const struct freq_update *u) {
  update_single_freq_sensor(&s->MAP, p, u);
  update_single_freq_sensor(&s->BRV, p, u);
  update_single_freq_sensor(&s->IAT, p, u);
  update_single_freq_sensor(&s->CLT, p, u);
  update_single_freq_sensor(&s->EGO, p, u);
  update_single_freq_sensor(&s->AAP, p, u);
  update_single_freq_sensor(&s->TPS, p, u);
  update_single_freq_sensor(&s->FRT, p, u);
  update_single_freq_sensor(&s->FRP, p, u);
  update_single_freq_sensor(&s->ETH, p, u);
}

static void update_single_adc_sensor(struct sensor_state *s,
                                     const struct engine_position *p,
                                     const struct adc_update *u) {
  if (s->config->source != SENSOR_ADC) {
    return;
  }
  if (s->config->pin >= MAX_ADC_PINS) {
    return;
  }
  s->output.fault = u->valid ? FAULT_NONE : FAULT_CONN;
  sensor_update_raw(s, p, u->time, u->values[s->config->pin]);
}

void sensor_update_adc(struct sensors *s,
                       const struct engine_position *p,
                       const struct adc_update *u) {
  update_single_adc_sensor(&s->MAP, p, u);
  update_single_adc_sensor(&s->BRV, p, u);
  update_single_adc_sensor(&s->IAT, p, u);
  update_single_adc_sensor(&s->CLT, p, u);
  update_single_adc_sensor(&s->EGO, p, u);
  update_single_adc_sensor(&s->AAP, p, u);
  update_single_adc_sensor(&s->TPS, p, u);
  update_single_adc_sensor(&s->FRT, p, u);
  update_single_adc_sensor(&s->FRP, p, u);
  update_single_adc_sensor(&s->ETH, p, u);
}

static void update_single_const_sensor(struct sensor_state *s) {
  if (s->config->source != SENSOR_CONST) {
    return;
  }
  s->output.fault = FAULT_NONE;
  s->output.value = s->config->const_value;
}

bool sensor_has_faults(const struct sensor_values *s) {
  /* For now, only report faults for the sensors we use */
  return (s->MAP.fault != FAULT_NONE) || (s->BRV.fault != FAULT_NONE) ||
         (s->IAT.fault != FAULT_NONE) || (s->CLT.fault != FAULT_NONE) ||
         (s->TPS.fault != FAULT_NONE) || (s->FRT.fault != FAULT_NONE);
}

void knock_configure(struct knock_sensor *knock) {
  uint32_t samplerate = platform_knock_samplerate();
  float bucketsize = samplerate / 64.0f;
  uint32_t bucket = (knock->config->frequency + bucketsize) / bucketsize;

  knock->width = 64;
  knock->freq = bucket;
  knock->w = 2.0f * 3.14159f * (float)knock->freq / (float)knock->width;
  knock->cr = cosf(knock->w);

  knock->n_samples = 0;
  knock->sprev = 0.0f;
  knock->sprev2 = 0.0f;
};

static void knock_add_sample(struct knock_sensor *knock, float sample) {

  knock->n_samples += 1;
  float s = sample + knock->cr * 2 * knock->sprev - knock->sprev2;
  knock->sprev2 = knock->sprev;
  knock->sprev = s;

  if (knock->n_samples == 64) {
    knock->value = knock->sprev * knock->sprev + knock->sprev2 * knock->sprev2 -
                   knock->sprev * knock->sprev2 * knock->cr * 2;

    knock->n_samples = 0;
    knock->sprev = 0;
    knock->sprev2 = 0;
  }
}

void sensor_update_knock(struct sensors *s, const struct knock_update *update) {
  struct knock_sensor *knk = (update->pin == 0) ? &s->KNK1 : &s->KNK2;

  for (int i = 0; i < update->n_samples; i++) {
    knock_add_sample(knk, update->samples[i]);
  }
}

struct sensor_values sensors_get_values(const struct sensors *s) {
  return (struct sensor_values){
    .MAP = s->MAP.output,
    .BRV = s->BRV.output,
    .IAT = s->IAT.output,
    .CLT = s->CLT.output,
    .EGO = s->EGO.output,
    .AAP = s->AAP.output,
    .TPS = s->TPS.output,
    .FRT = s->FRT.output,
    .FRP = s->FRP.output,
    .ETH = s->ETH.output,
    .KNK1 = s->KNK1.value,
    .KNK2 = s->KNK2.value,
  };
}

void sensors_init(const struct sensor_configs *configs, struct sensors *s) {
  *s = (struct sensors){
    .MAP = { .config = &configs->MAP },
    .BRV = { .config = &configs->BRV },
    .IAT = { .config = &configs->IAT },
    .CLT = { .config = &configs->CLT },
    .EGO = { .config = &configs->EGO },
    .AAP = { .config = &configs->AAP },
    .TPS = { .config = &configs->TPS },
    .FRT = { .config = &configs->FRT },
    .FRP = { .config = &configs->FRP },
    .ETH = { .config = &configs->ETH },
    .KNK1 = { .config = &configs->KNK1 },
    .KNK2 = { .config = &configs->KNK2 },
  };

  /* Initialize constant values */
  update_single_const_sensor(&s->MAP);
  update_single_const_sensor(&s->BRV);
  update_single_const_sensor(&s->IAT);
  update_single_const_sensor(&s->CLT);
  update_single_const_sensor(&s->EGO);
  update_single_const_sensor(&s->AAP);
  update_single_const_sensor(&s->TPS);
  update_single_const_sensor(&s->FRT);
  update_single_const_sensor(&s->FRP);
  update_single_const_sensor(&s->ETH);
}

#ifdef UNITTEST
#include <check.h>

START_TEST(check_sensor_convert_linear) {
  struct sensor_config conf = {
    .range = { .min = -10.0, .max = 10.0 },
    .raw_max = 4096.0f,
  };

  ck_assert(sensor_convert_linear(&conf, 0.0f) == -10.0);
  ck_assert(sensor_convert_linear(&conf, 4096.0f) == 10.0);
  ck_assert(sensor_convert_linear(&conf, 2048.0f) == 0.0);
}
END_TEST

START_TEST(check_sensor_convert_linear_windowed) {
  struct sensor_config conf = {
    .range = { .min=0, .max=4096.0},
    .raw_max = 4096.0,
    .window = {
      .windows_per_cycle = 8,
      .window_opening = 45,
    },
  };

  struct sensor_state state = {
    .config = &conf,
    .output = {
      .value = 12.0f, /* "Last" value */
    },
  };

  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&state, 0, 2048.0f), 12.0, 0.01);

  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&state, 10, 2500.0f), 12.0, 0.01);

  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&state, 40, 2600.0f), 12.0, 0.01);

  /* End of window, should average first three results together */
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&state, 50, 2700.0f), 2382.666, 0.1);

  /* All values in this non-capture window should be ignored */
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&state, 60, 2800.0f), 12, 0.1);
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&state, 80, 2800.0f), 12, 0.1);
}
END_TEST

START_TEST(check_sensor_convert_linear_windowed_skipped) {
  struct sensor_config conf = {
    .range = { .min=0, .max=4096.0},
    .raw_max = 4096.0,
    .window = {
      .windows_per_cycle = 8,
      .window_opening = 45,
    },
  };
  struct sensor_state state = {
    .config = &conf,
    .output = {
      .value = 12.0f,
    },
  };

  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&state, 0, 2048.0f), 12.0, 0.01);
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&state, 10, 2500.0f), 12.0, 0.01);
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&state, 40, 2600.0f), 12.0, 0.01);

  /* Move into next capture window, it should still produce average */
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&state, 100, 2700.0f), 2382.666, 0.1);

  /* Remaining samples in this window should add to average */
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&state, 120, 1000.0f), 12, 0.1);

  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&state, 130, 1500.0f), 12, 0.1);

  /* Reaches end of window, should average two prior samples */
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&state, 140, 2000.0f), 1733.33, 0.1);
}
END_TEST

START_TEST(check_sensor_convert_linear_windowed_wide) {
  struct sensor_config conf = {
    .range = { .min=0, .max=4096.0},
    .raw_max = 4096.0,
    .window = {
      .windows_per_cycle = 8,
      .window_opening = 90,
    },
  };

  struct sensor_state state = {
    .config = &conf,
    .output = {
      .value = 12.0f,
    },
  };

  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&state, 0, 2048), 12.0, 0.01);

  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&state, 45, 2500), 12.0, 0.01);

  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&state, 89, 2600), 12.0, 0.01);

  /* Move into next capture window, it should still produce average of all
   * three*/
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&state, 100, 2700), 2382.666, 0.1);

  /* Remaining samples in this window should add to average at end */
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&state, 120, 1000), 12, 0.1);

  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&state, 130, 1500), 12, 0.1);


  /* Reaches end of window, should average all three prior samples */
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&state, 180, 2000), 1733.33, 0.1);
}
END_TEST

START_TEST(check_sensor_convert_linear_windowed_offset) {
  struct sensor_config conf = {
    .range = { .min=0, .max=4096.0},
    .raw_max = 4096.0,
    .window = {
      .windows_per_cycle = 8,
      .window_opening = 45,
      .window_offset = 45,
    },
  };
  struct sensor_state state = {
    .config = &conf,
    .output = {
      .value = 12.0f,
    },
  };

  /* All values should be ignored */
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&state, 0, 2048), 12.0, 0.01);

  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&state, 10, 2500), 12.0, 0.01);

  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&state, 40, 2600), 12.0, 0.01);

  /* Start of real window */
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&state, 50, 2700), 12, 0.1);

  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&state, 80, 2400), 12, 0.1);

  /* End of window, should produce average of two prior samples */
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&state, 90, 2000), 2550, 0.1);
}
END_TEST

START_TEST(check_sensor_convert_therm) {
  // test parameters for my CHT sensor
  struct sensor_config conf = {
    .raw_max = 4096.0f,
    .therm = {
      .bias = 2490.0,
      .a = 0.00131586818223649,
      .b = 0.000256187001401003,
      .c = 1.84741994569279E-07,
    },
  };

  ck_assert_float_eq_tol(sensor_convert_thermistor(&conf, 2048), 20.31, 0.2);

  ck_assert_float_eq_tol(sensor_convert_thermistor(&conf, 4092), -97.43, 0.2);
}
END_TEST

START_TEST(check_current_angle_in_window) {
  struct sensor_config conf = {
    .window = {
      .windows_per_cycle = 8,
      .window_opening = 60,
      .window_offset = 60 ,
    },
  };
  /* This config should produce 8 windows:
   * - [60, 120)
   * - [150, 210)
   * - [240, 300)
   * - [330, 390)
   * - [420, 480)
   * - [510, 570)
   * - [600, 660)
   * - [690, 30)
   */

  degrees_t window;
  ck_assert(current_window_for_angle(&conf.window, &window, 0));
  ck_assert(window == 690);
  ck_assert(!current_window_for_angle(&conf.window, &window, 50));

  ck_assert(current_window_for_angle(&conf.window, &window, 90));
  ck_assert(window == 60);
  ck_assert(!current_window_for_angle(&conf.window, &window, 140));

  ck_assert(current_window_for_angle(&conf.window, &window, 180));
  ck_assert(window == 150);
  ck_assert(!current_window_for_angle(&conf.window, &window, 230));

  ck_assert(current_window_for_angle(&conf.window, &window, 270));
  ck_assert(window == 240);
  ck_assert(!current_window_for_angle(&conf.window, &window, 320));

  ck_assert(current_window_for_angle(&conf.window, &window, 360));
  ck_assert(window == 330);
  ck_assert(!current_window_for_angle(&conf.window, &window, 410));

  ck_assert(current_window_for_angle(&conf.window, &window, 450));
  ck_assert(window == 420);
  ck_assert(!current_window_for_angle(&conf.window, &window, 500));

  ck_assert(current_window_for_angle(&conf.window, &window, 540));
  ck_assert(window == 510);
  ck_assert(!current_window_for_angle(&conf.window, &window, 590));

  ck_assert(current_window_for_angle(&conf.window, &window, 630));
  ck_assert(window == 600);
  ck_assert(!current_window_for_angle(&conf.window, &window, 680));

  ck_assert(current_window_for_angle(&conf.window, &window, 710));
  ck_assert(window == 690);

}
END_TEST

START_TEST(check_knock_configure) {
  struct knock_sensor_config conf = {
    .frequency = 7000,
  };
  struct knock_sensor sensor = {
    .config = &conf,
  };

  knock_configure(&sensor);
  ck_assert_int_eq(sensor.width, 64);
  ck_assert_int_eq(sensor.freq, 9);
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
