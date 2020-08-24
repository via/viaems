#include <math.h>

#include "config.h"
#include "decoder.h"
#include "platform.h"
#include "sensors.h"
#include "stats.h"

static float sensor_convert_linear(struct sensor_input *in, float raw) {
  float partial = raw / 4096.0f;
  return in->params.range.min +
         partial * (in->params.range.max - in->params.range.min);
}

static int current_angle_in_window(struct sensor_input *in, degrees_t angle) {
  degrees_t cur_angle = clamp_angle(angle - in->window.offset, 720);
  for (uint32_t i = 0; i < 720; i += in->window.total_width) {
    if ((cur_angle >= i) && (cur_angle < i + in->window.capture_width)) {
      return 1;
    }
  }
  return 0;
}

static float sensor_convert_linear_windowed(struct sensor_input *in,
                                            degrees_t angle,
                                            float raw) {
  if (!config.decoder.rpm) {
    /* If engine not turning, don't window */
    return sensor_convert_linear(in, raw);
  }

  float result = in->processed_value;

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
    in->window.accumulator += sensor_convert_linear(in, raw);
    in->window.samples += 1;
  }

  /* Return previous processed value */
  return result;
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
  float logf_r = logf(r);
  float t = 1 / (tc->a + tc->b * logf_r + tc->c * logf_r * logf_r * logf_r);
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

  switch (in->method) {
  case METHOD_LINEAR:
    in->processed_value = sensor_convert_linear(in, raw);
    break;
  case METHOD_LINEAR_WINDOWED:
    in->processed_value =
      sensor_convert_linear_windowed(in, current_angle(), raw);
    break;
  case METHOD_TABLE:
    in->processed_value = interpolate_table_oneaxis(in->params.table, raw);
    break;
  case METHOD_THERM:
    in->processed_value = sensor_convert_thermistor(&in->params.therm, raw);
    break;
  }

  /* Do lag filtering over 10ish ms derivative window */
  in->processed_value = ((in->derivative.last_sample_value * in->lag) +
                         (in->processed_value * (100.0f - in->lag))) /
                        100.0f;

  /* Process derivative */
  timeval_t process_time = current_time();

  /* We want derivatives at a minimum of 10 ms dt to keep noise at a minimum */
  if (process_time - in->derivative.last_sample_time > time_from_us(10000)) {
    in->derivative.value =
      TICKRATE * (in->processed_value - in->derivative.last_sample_value) /
      (process_time - in->derivative.last_sample_time);
    in->derivative.last_sample_time = process_time;
    in->derivative.last_sample_value = in->processed_value;
  }
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

struct sensor_input *sensor_input_from_type(sensor_input_type t) {
  return &config.sensors[t];
}

static const char *sensor_name_from_type(sensor_input_type t) {
  switch (t) {
  case SENSOR_MAP:
    return "map";
  case SENSOR_IAT:
    return "iat";
  case SENSOR_CLT:
    return "clt";
  case SENSOR_BRV:
    return "brv";
  case SENSOR_TPS:
    return "tps";
  case SENSOR_AAP:
    return "aap";
  case SENSOR_FRT:
    return "frt";
  case SENSOR_EGO:
    return "ego";
  default:
    return "invalid";
  }
}

static void render_sensor_source_field(struct console_request_context *ctx,
                                       void *ptr) {
  sensor_source *src = ptr;
  switch (ctx->type) {
  case CONSOLE_GET:
    switch (*src) {
    case SENSOR_NONE:
      cbor_encode_text_stringz(ctx->response, "none");
      break;
    case SENSOR_ADC:
      cbor_encode_text_stringz(ctx->response, "adc");
      break;
    case SENSOR_FREQ:
      cbor_encode_text_stringz(ctx->response, "freq");
      break;
    case SENSOR_DIGITAL:
      cbor_encode_text_stringz(ctx->response, "digital");
      break;
    case SENSOR_CONST:
      cbor_encode_text_stringz(ctx->response, "const");
      break;
    }
    return;
  case CONSOLE_DESCRIBE: {
    CborEncoder map;
    cbor_encoder_create_map(ctx->response, &map, 3);
    render_type_field(&map, "string");
    render_description_field(&map, "sensor source");
    console_describe_choices(
      &map,
      (const char *[]){
        "none", "adc", "freq", "digital", "pwm", "const", NULL });
    cbor_encoder_close_container(ctx->response, &map);
  }
    return;
  }
}

static void render_sensor_method_field(struct console_request_context *ctx,
                                       void *ptr) {
  sensor_method *method = ptr;
  switch (ctx->type) {
  case CONSOLE_GET:
    switch (*method) {
    case METHOD_LINEAR:
      cbor_encode_text_stringz(ctx->response, "linear");
      break;
    case METHOD_LINEAR_WINDOWED:
      cbor_encode_text_stringz(ctx->response, "linear-window");
      break;
    case METHOD_TABLE:
      cbor_encode_text_stringz(ctx->response, "table");
      break;
    case METHOD_THERM:
      cbor_encode_text_stringz(ctx->response, "therm");
      break;
    }
    return;
  case CONSOLE_DESCRIBE: {
    CborEncoder map;
    cbor_encoder_create_map(ctx->response, &map, 3);
    render_type_field(&map, "string");
    render_description_field(&map, "sensor processing method");
    console_describe_choices(
      &map,
      (const char *[]){ "linear", "linear-window", "table", "therm", NULL });
    cbor_encoder_close_container(ctx->response, &map);
    return;
  }
  }
}

void render_sensor_input_field(struct console_request_context *ctx, void *ptr) {

  if (ctx->type == CONSOLE_STRUCTURE) {
    render_type_field(ctx->response, "sensor");
    return;
  }

  struct sensor_input *input = ptr;

  render_uint32_map_field(ctx, "pin", "adc sensor input pin", &input->pin);
  render_float_map_field(
    ctx, "lag", "lag filter coefficient (0-1)", &input->lag);
  render_custom_map_field(
    ctx, "source", render_sensor_source_field, &input->source);
  render_custom_map_field(
    ctx, "method", render_sensor_method_field, &input->method);

  render_float_map_field(
    ctx, "range-min", "min for linear mapping", &input->params.range.min);
  render_float_map_field(
    ctx, "range-max", "max for linear mapping", &input->params.range.max);
  render_float_map_field(ctx,
                         "fixed-value",
                         "value to hold for const input",
                         &input->params.fixed_value);
  render_float_map_field(
    ctx, "therm-a", "thermistor A", &input->params.therm.a);
  render_float_map_field(
    ctx, "therm-b", "thermistor B", &input->params.therm.b);
  render_float_map_field(
    ctx, "therm-c", "thermistor C", &input->params.therm.c);
  render_float_map_field(ctx,
                         "therm-bias",
                         "thermistor resistor bias value (ohms)",
                         &input->params.therm.bias);
  render_uint32_map_field(ctx,
                          "fault-min",
                          "Lower bound for raw sensor input",
                          &input->fault_config.min);
  render_uint32_map_field(ctx,
                          "fault-max",
                          "Upper bound for raw sensor input",
                          &input->fault_config.max);
  render_float_map_field(ctx,
                         "fault-value",
                         "Value to assume in fault condition",
                         &input->fault_config.fault_value);

  render_uint32_map_field(ctx,
                          "window-capture-width",
                          "Crank degrees in window to average samples over",
                          &input->window.capture_width);
  render_uint32_map_field(ctx,
                          "window-total-width",
                          "Crank degrees per window",
                          &input->window.total_width);
  render_uint32_map_field(ctx,
                          "window-offset",
                          "Crank degree into window to start averagine",
                          &input->window.total_width);
}

void sensor_console_renderer(struct console_request_context *ctx, void *ptr) {
  for (sensor_input_type i = 0; i < NUM_SENSORS; i++) {
    render_map_map_field(ctx,
                         sensor_name_from_type(i),
                         render_sensor_input_field,
                         &config.sensors[i]);
  }
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

START_TEST(check_sensor_convert_linear_windowed) {
  struct sensor_input si = {
    .processed_value = 12.0,
    .params = {
      .range = { .min=0, .max=4096.0},
    },
    .window = {
      .total_width = 90,
      .capture_width = 45,
    },
  };

  config.decoder.rpm = 1000;

  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&si, 0, 2048), 12.0, 0.01);
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&si, 10, 2500), 12.0, 0.01);
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&si, 40, 2600), 12.0, 0.01);
  /* End of window, should average first three results together */
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&si, 50, 2700), 2382.666, 0.1);

  /* All values in this non-capture window should be ignored */
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&si, 60, 2800), 12, 0.1);
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&si, 80, 2800), 12, 0.1);
}
END_TEST

START_TEST(check_sensor_convert_linear_windowed_skipped) {
  struct sensor_input si = {
    .processed_value = 12.0,
    .params = {
      .range = { .min=0, .max=4096.0},
    },
    .window = {
      .total_width = 90,
      .capture_width = 45,
    },
  };

  config.decoder.rpm = 1000;

  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&si, 0, 2048), 12.0, 0.01);
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&si, 10, 2500), 12.0, 0.01);
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&si, 40, 2600), 12.0, 0.01);

  /* Move into next capture window, it should still produce average */
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&si, 100, 2700), 2382.666, 0.1);

  /* Remaining samples in this window should add to average */
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&si, 120, 1000), 12, 0.1);
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&si, 130, 1500), 12, 0.1);

  /* Reaches end of window, should average two prior samples */
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&si, 140, 2000), 1733.33, 0.1);
}
END_TEST

START_TEST(check_sensor_convert_linear_windowed_wide) {
  struct sensor_input si = {
    .processed_value = 12.0,
    .params = {
      .range = { .min=0, .max=4096.0},
    },
    .window = {
      .total_width = 90,
      .capture_width = 90,
    },
  };

  config.decoder.rpm = 1000;

  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&si, 0, 2048), 12.0, 0.01);
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&si, 45, 2500), 12.0, 0.01);
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&si, 89, 2600), 12.0, 0.01);

  /* Move into next capture window, it should still produce average of all
   * three*/
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&si, 100, 2700), 2382.666, 0.1);

  /* Remaining samples in this window should add to average at end */
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&si, 120, 1000), 12, 0.1);
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&si, 130, 1500), 12, 0.1);

  /* TODO: right now this needs to be capture_width degrees past the first
   * sample in the last window, which is not ideal. We should just treat all
   * samples in the window as acceptable */
  /* Reaches end of window, should average all three prior samples */
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&si, 210, 2000), 1733.33, 0.1);
}
END_TEST

START_TEST(check_sensor_convert_linear_windowed_offset) {
  struct sensor_input si = {
    .processed_value = 12.0,
    .params = {
      .range = { .min=0, .max=4096.0},
    },
    .window = {
      .total_width = 90,
      .capture_width = 45,
      .offset = 45,
    },
  };

  config.decoder.rpm = 1000;

  /* All values should be ignored */
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&si, 0, 2048), 12.0, 0.01);
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&si, 10, 2500), 12.0, 0.01);
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&si, 40, 2600), 12.0, 0.01);

  /* Start of real window */
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&si, 50, 2700), 12, 0.1);
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&si, 80, 2400), 12, 0.1);

  /* End of window, should produce average of two prior samples */
  ck_assert_float_eq_tol(
    sensor_convert_linear_windowed(&si, 90, 2000), 2550, 0.1);
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

TCase *setup_sensor_tests() {
  TCase *sensor_tests = tcase_create("sensors");
  tcase_add_test(sensor_tests, check_sensor_convert_linear);
  tcase_add_test(sensor_tests, check_sensor_convert_linear_windowed);
  tcase_add_test(sensor_tests, check_sensor_convert_linear_windowed_skipped);
  tcase_add_test(sensor_tests, check_sensor_convert_linear_windowed_wide);
  tcase_add_test(sensor_tests, check_sensor_convert_linear_windowed_offset);
  tcase_add_test(sensor_tests, check_sensor_convert_freq);
  tcase_add_test(sensor_tests, check_sensor_convert_therm);

  tcase_add_test(sensor_tests, check_current_angle_in_window);
  return sensor_tests;
}

#endif
