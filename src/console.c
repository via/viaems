/* For strtok_r */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 1
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "calculations.h"
#include "config.h"
#include "console.h"
#include "decoder.h"
#include "platform.h"
#include "sensors.h"
#include "stats.h"

static struct console console;

uint32_t console_current_time;

#define MAX_CONSOLE_FEED_NODES 128
struct console_feed_node console_feed_nodes[MAX_CONSOLE_FEED_NODES] = {
  { .id = "cputime", .uint32_ptr = &console_current_time },

  /* Fueling */
  {.id = "ve", .float_ptr = &calculated_values.ve},
  {.id = "lambda", .float_ptr = &calculated_values.lambda},
  {.id = "fuel_pulsewidth_us", .uint32_ptr = &calculated_values.fueling_us},
  {.id = "temp_enrich_percent", .float_ptr = &calculated_values.ete},
  {.id = "injector_dead_time", .float_ptr = &calculated_values.idt},
  {.id = "accel_enrich_percent", .float_ptr = &calculated_values.tipin},

  /* Ignition */
  {.id = "advance", .float_ptr = &calculated_values.timing_advance},
  {.id = "dwell", .uint32_ptr = &calculated_values.dwell_us},

  {.id = "sensor_map", .float_ptr = &config.sensors[SENSOR_MAP].processed_value},
  {.id = "sensor_iat", .float_ptr = &config.sensors[SENSOR_IAT].processed_value},
  {.id = "sensor_clt", .float_ptr = &config.sensors[SENSOR_CLT].processed_value},
  {.id = "sensor_brv", .float_ptr = &config.sensors[SENSOR_BRV].processed_value},
  {.id = "sensor_tps", .float_ptr = &config.sensors[SENSOR_TPS].processed_value},
  {.id = "sensor_aap", .float_ptr = &config.sensors[SENSOR_AAP].processed_value},
  {.id = "sensor_frt", .float_ptr = &config.sensors[SENSOR_FRT].processed_value},
  {.id = "sensor_ego", .float_ptr = &config.sensors[SENSOR_EGO].processed_value},

};

void console_add_feed_node(struct console_feed_node *node) {
  for (int i = 0; i < MAX_CONSOLE_FEED_NODES; i++) {
    if (!console_feed_nodes[i].id) {
      console_feed_nodes[i] = *node;
      return;
    }
  }
}

#ifndef GIT_DESCRIBE
#define GIT_DESCRIBE "unknown"
static const char *git_describe = GIT_DESCRIBE;
#endif

void console_record_event(struct logged_event ev) {}

void console_init() {}

void render_type_field(CborEncoder *enc, const char *type) {
  cbor_encode_text_stringz(enc, "_type");
  cbor_encode_text_stringz(enc, type);
}

void render_description_field(CborEncoder *enc, const char *description) {
  cbor_encode_text_stringz(enc, "description");
  cbor_encode_text_stringz(enc, description);
}

void console_describe_choices(CborEncoder *enc, const char **choices) {
  cbor_encode_text_stringz(enc, "choices");
  CborEncoder list_encoder;
  cbor_encoder_create_array(enc, &list_encoder, CborIndefiniteLength);
  for (const char **i = &choices[0]; *i != NULL; i++) {
    cbor_encode_text_stringz(&list_encoder, *i);
  }
  cbor_encoder_close_container(enc, &list_encoder);
}

static size_t console_feed_line_keys(uint8_t *dest, size_t bsize) {
  CborEncoder encoder;
  cbor_encoder_init(&encoder, dest, bsize, 0);

  CborEncoder top_encoder;
  cbor_encoder_create_map(&encoder, &top_encoder, 2);
  cbor_encode_text_stringz(&top_encoder, "type");
  cbor_encode_text_stringz(&top_encoder, "description");

  cbor_encode_text_stringz(&top_encoder, "keys");
  CborEncoder key_list_encoder;
  cbor_encoder_create_array(
    &top_encoder, &key_list_encoder, CborIndefiniteLength);
  for (int i = 0; i < MAX_CONSOLE_FEED_NODES; i++) {
    const struct console_feed_node *node = &console_feed_nodes[i];
    if (!node->id) {
      break;
    }
    cbor_encode_text_stringz(&key_list_encoder, node->id);
  }
  cbor_encoder_close_container(&top_encoder, &key_list_encoder);
  cbor_encoder_close_container(&encoder, &top_encoder);
  return cbor_encoder_get_buffer_size(&encoder, dest);
}

static size_t console_feed_line(uint8_t *dest, size_t bsize) {
  CborEncoder encoder;

  console_current_time = current_time();
  cbor_encoder_init(&encoder, dest, bsize, 0);

  CborEncoder top_encoder;
  cbor_encoder_create_map(&encoder, &top_encoder, 2);
  cbor_encode_text_stringz(&top_encoder, "type");
  cbor_encode_text_stringz(&top_encoder, "feed");

  cbor_encode_text_stringz(&top_encoder, "values");
  CborEncoder value_list_encoder;
  cbor_encoder_create_array(
    &top_encoder, &value_list_encoder, CborIndefiniteLength);
  for (int i = 0; i < MAX_CONSOLE_FEED_NODES; i++) {
    const struct console_feed_node *node = &console_feed_nodes[i];
    if (!node->id) {
      break;
    }
    if (node->uint32_ptr) {
      cbor_encode_uint(&value_list_encoder, *node->uint32_ptr);
    } else if (node->float_ptr) {
      cbor_encode_float(&value_list_encoder, *node->float_ptr);
    } else {
      cbor_encode_text_stringz(&value_list_encoder, "invalid");
    }
  }
  cbor_encoder_close_container(&top_encoder, &value_list_encoder);
  cbor_encoder_close_container(&encoder, &top_encoder);
  return cbor_encoder_get_buffer_size(&encoder, dest);
}

int console_write_full(const char *buf, size_t len) {
  size_t remaining = len;
  const char *ptr = buf;
  while (remaining) {
    ssize_t written = console_write(ptr, remaining);
    if (written > 0) {
      remaining -= written;
      ptr += written;
    }
  }
  return 1;
}

static uint8_t rx_buffer[4096];
static uint8_t *rx_buffer_ptr = rx_buffer;

static void console_shift_rx_buffer(size_t amt) {
  memmove(&rx_buffer[0], &rx_buffer[amt], (rx_buffer_ptr - rx_buffer));
  rx_buffer_ptr -= amt;
}

static size_t console_try_read() {
  size_t remaining = sizeof(rx_buffer) - (rx_buffer_ptr - rx_buffer);

  size_t read_amt = console_read(rx_buffer_ptr, remaining);

  rx_buffer_ptr += read_amt;

  size_t len = rx_buffer_ptr - rx_buffer;
  if (len == 0) {
    return 0;
  }

  CborParser parser;
  CborValue value;

  /* We want to determine if we have a valid cbor object in buffer.  If the
   * buffer doesn't start with a map, it is not valid, and we want to advance
   * byte-by-byte until we find a start of map */
  do {
    if (cbor_parser_init(rx_buffer, len, 0, &parser, &value) == CborNoError) {
      if (cbor_value_is_map(&value)) {
        break;
      }
    }
    console_shift_rx_buffer(1);

    /* We've exhausted the buffer */
    if (rx_buffer_ptr == rx_buffer) {
      return 0;
    }
  } while (1);

  /* Maybe we have a valid object. if so, return.
   * If we don't, it could be that we haven't read enough. We will read until
   * the end of the buffer, but if its maxed out, reset everything
   */
  if (cbor_value_validate_basic(&value) == CborNoError) {
    cbor_value_advance(&value);
    const uint8_t *next = cbor_value_get_next_byte(&value);
    return (next - rx_buffer);
  }

  if (len == sizeof(rx_buffer)) {
    rx_buffer_ptr = rx_buffer;
  }
  return 0;
}

static void report_success(CborEncoder *enc, bool success) {
  cbor_encode_text_stringz(enc, "success");
  cbor_encode_boolean(enc, success);
}

void report_cbor_parsing_error(CborEncoder *enc,
                               const char *msg,
                               CborError err) {
  char txtbuf[256];
  strncat(txtbuf, msg, sizeof(txtbuf) - 1);
  strncat(txtbuf, ": ", sizeof(txtbuf) - 1);
  strncat(txtbuf, cbor_error_string(err), sizeof(txtbuf) - 1);

  cbor_encode_text_stringz(enc, "error");
  cbor_encode_text_stringz(enc, txtbuf);
  report_success(enc, false);
}

void report_parsing_error(CborEncoder *enc, const char *msg) {
  cbor_encode_text_stringz(enc, "error");
  cbor_encode_text_stringz(enc, msg);
  report_success(enc, false);
}

static void console_request_ping(CborEncoder *enc) {
  cbor_encode_text_stringz(enc, "response");
  cbor_encode_text_stringz(enc, "pong");
  report_success(enc, true);
}

static bool console_path_match(CborValue *path, const char *id) {
  bool is_match = false;
  if (cbor_value_text_string_equals(path, id, &is_match) != CborNoError) {
    is_match = false;
  }
  return is_match;
}

static bool console_field_check_skip(const struct console_request_context *ctx,
                                     const char *id) {
  if (ctx->is_completed) {
    return true;
  }
  if (ctx->is_filtered && !console_path_match(ctx->path, id)) {
    return true;
  }
  return false;
}

void render_uint32_field(struct console_request_context *ctx,
                         const char *id,
                         const char *description,
                         uint32_t *ptr) {
  switch (ctx->type) {
  case CONSOLE_GET:
    if (console_field_check_skip(ctx, id)) {
      break;
    }
    if (!ctx->is_filtered) {
      cbor_encode_text_stringz(ctx->response, id);
    }
    cbor_encode_int(ctx->response, *ptr);
    if (ctx->is_filtered) {
      ctx->is_completed = true;
    }
    break;
  case CONSOLE_DESCRIBE:
  case CONSOLE_STRUCTURE: {
    cbor_encode_text_stringz(ctx->response, id);
    CborEncoder desc;
    cbor_encoder_create_map(ctx->response, &desc, 2);
    render_type_field(&desc, "uint32");
    render_description_field(&desc, description);
    cbor_encoder_close_container(ctx->response, &desc);
    break;
  }
  default:
    break;
  }
}

void render_float_field(struct console_request_context *ctx,
                        const char *id,
                        const char *description,
                        float *ptr) {

  switch (ctx->type) {
  case CONSOLE_GET:
    if (console_field_check_skip(ctx, id)) {
      break;
    }
    if (!ctx->is_filtered) {
      cbor_encode_text_stringz(ctx->response, id);
    }
    cbor_encode_float(ctx->response, *ptr);
    if (ctx->is_filtered) {
      ctx->is_completed = true;
    }
    break;
  case CONSOLE_STRUCTURE:
  case CONSOLE_DESCRIBE: {
    cbor_encode_text_stringz(ctx->response, id);
    CborEncoder desc;
    cbor_encoder_create_map(ctx->response, &desc, 2);
    render_type_field(&desc, "float");
    render_description_field(&desc, description);
    cbor_encoder_close_container(ctx->response, &desc);
    break;
  }
  default:
    break;
  }
}

void render_map_field(struct console_request_context *ctx,
                      const char *id,
                      console_renderer rend,
                      void *ptr) {
  switch (ctx->type) {
  case CONSOLE_GET: {
    if (console_field_check_skip(ctx, id)) {
      break;
    }
    if (!ctx->is_filtered) {
      cbor_encode_text_stringz(ctx->response, id);
    } else {
      cbor_value_advance(ctx->path);
    }

    if (cbor_value_at_end(ctx->path)) {
      struct console_request_context deeper_ctx = *ctx;
      CborEncoder map;
      cbor_encoder_create_map(ctx->response, &map, CborIndefiniteLength);

      deeper_ctx.is_filtered = false;
      deeper_ctx.response = &map;
      rend(&deeper_ctx, ptr);
      cbor_encoder_close_container(ctx->response, &map);
      if (ctx->is_filtered) {
        ctx->is_completed = true;
      }
    } else {
      rend(ctx, ptr);
      if (!ctx->is_completed) {
        cbor_encode_null(ctx->response);
        ctx->is_completed = true;
      }
    }
  } break;
  case CONSOLE_STRUCTURE:
  case CONSOLE_DESCRIBE: {
    cbor_encode_text_stringz(ctx->response, id);
    struct console_request_context deeper_ctx = *ctx;
    CborEncoder map;
    cbor_encoder_create_map(ctx->response, &map, CborIndefiniteLength);
    deeper_ctx.response = &map;
    rend(&deeper_ctx, ptr);
    cbor_encoder_close_container(ctx->response, &map);
    break;
  }
  }
}

void render_custom_field(struct console_request_context *ctx,
                         const char *id,
                         console_renderer rend,
                         void *ptr) {
  switch (ctx->type) {
  case CONSOLE_GET: {
    if (console_field_check_skip(ctx, id)) {
      break;
    }
    if (!ctx->is_filtered) {
      cbor_encode_text_stringz(ctx->response, id);
    }
    rend(ctx, ptr);
  } break;
  case CONSOLE_STRUCTURE:
  case CONSOLE_DESCRIBE: {

    cbor_encode_text_stringz(ctx->response, id);
    rend(ctx, ptr);
    break;
  }
  }
}

static void decoder_console_type_renderer(struct console_request_context *ctx,
                                          void *ptr) {
  (void)ptr;

  switch (ctx->type) {
  case CONSOLE_GET:
    switch (config.decoder.type) {
    case FORD_TFI:
      cbor_encode_text_stringz(ctx->response, "tfi");
      break;
    case TOYOTA_24_1_CAS:
      cbor_encode_text_stringz(ctx->response, "cam24+1");
      break;
    }
    return;
  case CONSOLE_STRUCTURE:
  case CONSOLE_DESCRIBE: {
    CborEncoder map;
    cbor_encoder_create_map(ctx->response, &map, 3);
    render_type_field(&map, "string");
    render_description_field(&map, "decoder wheel type");
    console_describe_choices(&map, (const char *[]){ "cam24+1", "tfi", NULL });
    cbor_encoder_close_container(ctx->response, &map);
  }
    return;
  }
}

static void decoder_console_renderer(struct console_request_context *ctx,
                                     void *ptr) {
  (void)ptr;

  render_uint32_field(ctx,
                      "rpm_window_size",
                      "averaging window (N teeth)",
                      &config.decoder.rpm_window_size);
  render_float_field(
    ctx, "offset", "offset past TDC for sync pulse", &config.decoder.offset);

  render_custom_field(ctx, "type", decoder_console_type_renderer, NULL);
}

void console_toplevel_request(struct console_request_context *ctx, void *ptr) {
  if (ctx->type == CONSOLE_GET) {
    ctx->is_filtered = !cbor_value_at_end(ctx->path);
  }
  render_map_field(ctx, "decoder", decoder_console_renderer, ptr);
  render_map_field(ctx, "sensors", sensor_console_renderer, ptr);
}

void console_toplevel_types(struct console_request_context *ctx, void *ptr) {

  render_map_field(
    ctx, "sensor", render_sensor_input_field, &config.sensors[0]);
}

static void console_request_structure(CborEncoder *enc) {

  struct console_request_context structure_ctx = {
    .type = CONSOLE_STRUCTURE,
    .response = enc,
    .is_filtered = false,
  };
  render_map_field(&structure_ctx, "response", console_toplevel_request, NULL);

  struct console_request_context type_ctx = {
    .type = CONSOLE_DESCRIBE,
    .response = enc,
    .is_filtered = false,
  };
  render_map_field(&type_ctx, "types", console_toplevel_types, NULL);

  report_success(enc, true);
}

static void console_request_get(CborEncoder *enc, CborValue *value) {
  CborValue path;
  CborValue pathlist;
  CborError err;
  err = cbor_value_map_find_value(value, "path", &path);
  if (err) {
    report_cbor_parsing_error(enc, "request has no 'path'", err);
    return;
  }
  if (!cbor_value_is_array(&path)) {
    report_parsing_error(enc, "request 'path' not an array");
    return;
  }
  err = cbor_value_enter_container(&path, &pathlist);
  if (err) {
    report_cbor_parsing_error(enc, "could not read 'path'", err);
    return;
  }

  struct console_request_context ctx = {
    .type = CONSOLE_GET,
    .response = enc,
    .path = &pathlist,
    .is_filtered = false,
  };
  render_map_field(&ctx, "response", console_toplevel_request, NULL);
}

static void console_process_request(int len) {
  CborParser parser;
  CborValue value;
  CborError err;
  int req_id = -1;

  uint8_t response[4096];
  CborEncoder encoder;
  CborEncoder response_map;

  cbor_encoder_init(&encoder, response, sizeof(response), 0);

  if (cbor_encoder_create_map(&encoder, &response_map, CborIndefiniteLength) !=
      CborNoError) {
    return;
  }
  if (cbor_encode_text_stringz(&response_map, "type")) {
    return;
  }
  if (cbor_encode_text_stringz(&response_map, "response")) {
    return;
  }

  err = cbor_parser_init(rx_buffer, len, 0, &parser, &value);
  if (err) {
    return;
  }

  CborValue request_id_value;
  if (!cbor_value_map_find_value(&value, "id", &request_id_value)) {
    if (cbor_value_is_integer(&request_id_value)) {
      cbor_value_get_int(&request_id_value, &req_id);
    }
  }
  if (cbor_encode_text_stringz(&response_map, "id")) {
    return;
  }
  if (cbor_encode_int(&response_map, req_id)) {
    return;
  }

  CborValue request_method_value;
  err = cbor_value_map_find_value(&value, "method", &request_method_value);
  if (cbor_value_get_type(&request_method_value) == CborInvalidType) {
    report_cbor_parsing_error(&response_map, "request has no 'method'", err);
  } else {
    bool match;
    cbor_value_text_string_equals(&request_method_value, "ping", &match);
    if (match) {
      console_request_ping(&response_map);
    }

    cbor_value_text_string_equals(&request_method_value, "structure", &match);
    if (match) {
      console_request_structure(&response_map);
    }

    cbor_value_text_string_equals(&request_method_value, "get", &match);
    if (match) {
      console_request_get(&response_map, &value);
    }
  }

  if (cbor_encoder_close_container(&encoder, &response_map)) {
    return;
  }

  size_t write_size = cbor_encoder_get_buffer_size(&encoder, response);
  console_write_full(response, write_size);
}

void console_process() {
  static timeval_t last_desc_time = 0;

  size_t read_size;
  if ((read_size = console_try_read())) {
    /* Parse a request from the client */
    console_process_request(read_size);
    console_shift_rx_buffer(read_size);
  }

  /* Has it been 100ms since the last description? */
  if (time_diff(current_time(), last_desc_time) > time_from_us(100000)) {
    /* If so, print a description message */
    size_t len =
      console_feed_line_keys((uint8_t *)console.txbuffer, CONSOLE_BUFFER_SIZE);
    console_write_full(console.txbuffer, len);
    last_desc_time = current_time();
  } else {
    /* Otherwise a feed message */
    size_t len =
      console_feed_line((uint8_t *)console.txbuffer, CONSOLE_BUFFER_SIZE);
    console_write_full(console.txbuffer, len);
  }
  stats_finish_timing(STATS_CONSOLE_TIME);
}

#ifdef UNITTEST
#include <check.h>

TCase *setup_console_tests() {
  TCase *console_tests = tcase_create("console");
  return console_tests;
}

#endif
