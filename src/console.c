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

static uint32_t console_current_time;

#define MAX_CONSOLE_FEED_NODES 128
struct console_feed_node console_feed_nodes[MAX_CONSOLE_FEED_NODES] = {
  { .id = "cputime", .uint32_ptr = &console_current_time },

  /* Fueling */
  { .id = "ve", .float_ptr = &calculated_values.ve },
  { .id = "lambda", .float_ptr = &calculated_values.lambda },
  { .id = "fuel_pulsewidth_us", .uint32_ptr = &calculated_values.fueling_us },
  { .id = "temp_enrich_percent", .float_ptr = &calculated_values.ete },
  { .id = "injector_dead_time", .float_ptr = &calculated_values.idt },
  { .id = "accel_enrich_percent", .float_ptr = &calculated_values.tipin },

  /* Ignition */
  { .id = "advance", .float_ptr = &calculated_values.timing_advance },
  { .id = "dwell", .uint32_ptr = &calculated_values.dwell_us },

  { .id = "sensor_map",
    .float_ptr = &config.sensors[SENSOR_MAP].processed_value },
  { .id = "sensor_iat",
    .float_ptr = &config.sensors[SENSOR_IAT].processed_value },
  { .id = "sensor_clt",
    .float_ptr = &config.sensors[SENSOR_CLT].processed_value },
  { .id = "sensor_brv",
    .float_ptr = &config.sensors[SENSOR_BRV].processed_value },
  { .id = "sensor_tps",
    .float_ptr = &config.sensors[SENSOR_TPS].processed_value },
  { .id = "sensor_aap",
    .float_ptr = &config.sensors[SENSOR_AAP].processed_value },
  { .id = "sensor_frt",
    .float_ptr = &config.sensors[SENSOR_FRT].processed_value },
  { .id = "sensor_ego",
    .float_ptr = &config.sensors[SENSOR_EGO].processed_value },

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

void console_record_event(struct logged_event ev) {
  (void)ev;
}

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

int console_write_full(const uint8_t *buf, size_t len) {
  size_t remaining = len;
  const uint8_t *ptr = buf;
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
  char *txtptr = txtbuf;
  txtptr = strncat(txtptr, msg, sizeof(txtbuf) - 1);
  txtptr = strncat(txtptr, ": ", sizeof(txtbuf) - strlen(txtbuf) - 1);
  strncat(txtptr, cbor_error_string(err), sizeof(txtbuf) - strlen(txtbuf) - 1);

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

static bool console_path_match_str(CborValue *path, const char *id) {
  bool is_match = false;
  if (cbor_value_text_string_equals(path, id, &is_match) != CborNoError) {
    is_match = false;
  }
  return is_match;
}

static bool console_path_match_int(CborValue *path, int index) {
  if (!cbor_value_is_integer(path)) {
    return false;
  }
  int path_int;
  if (cbor_value_get_int_checked(path, &path_int) != CborNoError) {
    return false;
  }
  return (path_int == index);
}

void render_uint32_map_field(struct console_request_context *ctx,
                             const char *id,
                             const char *description,
                             uint32_t *ptr) {
  if (descend_map_field(ctx, id)) {
    render_uint32_object(ctx, description, ptr);
  }
}

void render_uint32_object(struct console_request_context *ctx,
                          const char *description,
                          uint32_t *ptr) {
  switch (ctx->type) {
  case CONSOLE_GET:
    cbor_encode_int(ctx->response, *ptr);
    break;
  case CONSOLE_DESCRIBE:
  case CONSOLE_STRUCTURE: {
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

void render_float_map_field(struct console_request_context *ctx,
                            const char *id,
                            const char *description,
                            float *ptr) {
  if (descend_map_field(ctx, id)) {
    render_float_object(ctx, description, ptr);
  }
}

void render_float_object(struct console_request_context *ctx,
                         const char *description,
                         float *ptr) {

  switch (ctx->type) {
  case CONSOLE_GET:
    cbor_encode_float(ctx->response, *ptr);
    break;
  case CONSOLE_STRUCTURE:
  case CONSOLE_DESCRIBE: {
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

void render_map_map_field(struct console_request_context *ctx,
                          const char *id,
                          console_renderer rend,
                          void *ptr) {
  if (descend_map_field(ctx, id)) {
    render_map_object(ctx, rend, ptr);
  }
}

void render_custom_map_field(struct console_request_context *ctx,
                             const char *id,
                             console_renderer rend,
                             void *ptr) {
  if (descend_map_field(ctx, id)) {
    rend(ctx, ptr);
  }
}

bool descend_array_field(struct console_request_context *ctx, int index) {
  switch (ctx->type) {
  case CONSOLE_GET:
    /* Short circuit all future rendering if we've already matched a path */
    if (ctx->is_completed) {
      return false;
    }
    if (ctx->is_filtered) {
      /* If we're filtering, we need to match the path */
      if (!console_path_match_int(ctx->path, index)) {
        return false;
      }
      cbor_value_advance(ctx->path);
      ctx->is_completed = true;
    }
    return true;
  case CONSOLE_STRUCTURE:
  case CONSOLE_DESCRIBE:
    return true;
  }
  return false;
}

void render_array_object(struct console_request_context *ctx,
                         console_renderer rend,
                         void *ptr) {
  struct console_request_context deeper_ctx = *ctx;
  deeper_ctx.is_completed = false;
  if (ctx->type == CONSOLE_GET) {
    deeper_ctx.is_filtered = !cbor_value_at_end(ctx->path);
  }

  bool wrapped = !deeper_ctx.is_filtered;

  CborEncoder array;
  if (wrapped) {
    cbor_encoder_create_array(ctx->response, &array, CborIndefiniteLength);
    deeper_ctx.response = &array;
  }

  rend(&deeper_ctx, ptr);
  if (deeper_ctx.is_filtered && !deeper_ctx.is_completed) {
    cbor_encode_null(deeper_ctx.response);
  }

  if (wrapped) {
    cbor_encoder_close_container(ctx->response, &array);
  }
}

void render_map_array_field(struct console_request_context *ctx,
                            int index,
                            console_renderer rend,
                            void *ptr) {
  if (descend_array_field(ctx, index)) {
    render_map_object(ctx, rend, ptr);
  }
}

void render_array_map_field(struct console_request_context *ctx,
                            const char *id,
                            console_renderer rend,
                            void *ptr) {
  if (descend_map_field(ctx, id)) {
    render_array_object(ctx, rend, ptr);
  }
}

void render_map_object(struct console_request_context *ctx,
                       console_renderer rend,
                       void *ptr) {

  struct console_request_context deeper_ctx = *ctx;
  deeper_ctx.is_completed = false;
  if (ctx->type == CONSOLE_GET) {
    deeper_ctx.is_filtered = !cbor_value_at_end(ctx->path);
  }

  bool wrapped = !deeper_ctx.is_filtered;

  CborEncoder map;
  if (wrapped) {
    cbor_encoder_create_map(ctx->response, &map, CborIndefiniteLength);
    deeper_ctx.response = &map;
  }

  rend(&deeper_ctx, ptr);
  if (deeper_ctx.is_filtered && !deeper_ctx.is_completed) {
    cbor_encode_null(deeper_ctx.response);
  }

  if (wrapped) {
    cbor_encoder_close_container(ctx->response, &map);
  }
}

bool descend_map_field(struct console_request_context *ctx, const char *id) {
  switch (ctx->type) {
  case CONSOLE_GET:
    /* Short circuit all future rendering if we've already matched a path */
    if (ctx->is_completed) {
      return false;
    }
    if (ctx->is_filtered) {
      /* If we're filtering, we need to match the path */
      if (!console_path_match_str(ctx->path, id)) {
        return false;
      }
      cbor_value_advance(ctx->path);
      ctx->is_completed = true;
    } else {
      cbor_encode_text_stringz(ctx->response, id);
    }
    return true;
  case CONSOLE_STRUCTURE:
  case CONSOLE_DESCRIBE:

    cbor_encode_text_stringz(ctx->response, id);
    return true;
  }
  return false;
}

static void render_decoder_type_object(struct console_request_context *ctx,
                                       trigger_type *type) {

  switch (ctx->type) {
  case CONSOLE_GET:
    switch (*type) {
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

static void render_output_type_object(struct console_request_context *ctx,
                                      void *_type) {

  event_type_t *type = _type;

  switch (ctx->type) {
  case CONSOLE_GET:
    switch (*type) {
    case DISABLED_EVENT:
      cbor_encode_text_stringz(ctx->response, "disabled");
      break;
    case FUEL_EVENT:
      cbor_encode_text_stringz(ctx->response, "fuel");
      break;
    case IGNITION_EVENT:
      cbor_encode_text_stringz(ctx->response, "ignition");
      break;
    }
    return;
  case CONSOLE_STRUCTURE:
  case CONSOLE_DESCRIBE: {
    CborEncoder map;
    cbor_encoder_create_map(ctx->response, &map, 3);
    render_type_field(&map, "string");
    render_description_field(&map, "output type");
    console_describe_choices(
      &map, (const char *[]){ "disabled", "fuel", "ignition", NULL });
    cbor_encoder_close_container(ctx->response, &map);
  }
    return;
  }
}

static void output_console_renderer(struct console_request_context *ctx,
                                    void *ptr) {
  if (ctx->type == CONSOLE_STRUCTURE) {
    render_type_field(ctx->response, "output");
    return;
  }

  struct output_event *ev = ptr;
  render_uint32_map_field(ctx, "pin", "pin", &ev->pin);
  render_uint32_map_field(ctx, "inverted", "inverted", &ev->inverted);
  render_float_map_field(
    ctx, "angle", "angle past TDC to trigger event", &ev->angle);
  render_custom_map_field(ctx, "type", render_output_type_object, &ev->type);
}

static void output_array_console_renderer(struct console_request_context *ctx,
                                          void *ptr) {
  (void)ptr;
  for (int i = 0; i < MAX_EVENTS; i++) {
    render_map_array_field(ctx, i, output_console_renderer, &config.events[i]);
  }
}

static void decoder_map_console_renderer(struct console_request_context *ctx,
                                         void *ptr) {
  (void)ptr;

  render_uint32_map_field(ctx,
                          "rpm_window_size",
                          "averaging window (N teeth)",
                          &config.decoder.rpm_window_size);

  render_float_map_field(
    ctx, "offset", "offset past TDC for sync pulse", &config.decoder.offset);

  if (descend_map_field(ctx, "type")) {
    render_decoder_type_object(ctx, &config.decoder.type);
  }
}

void console_toplevel_request(struct console_request_context *ctx, void *ptr) {
  (void)ptr;
  render_map_map_field(ctx, "decoder", decoder_map_console_renderer, NULL);
  render_map_map_field(ctx, "sensors", sensor_console_renderer, NULL);
  render_array_map_field(ctx, "outputs", output_array_console_renderer, NULL);
}

void console_toplevel_types(struct console_request_context *ctx, void *ptr) {
  (void)ptr;
  render_map_map_field(
    ctx, "sensor", render_sensor_input_field, &config.sensors[0]);
  render_map_map_field(
    ctx, "output", output_console_renderer, &config.events[1]);
}

static void console_request_structure(CborEncoder *enc) {

  struct console_request_context structure_ctx = {
    .type = CONSOLE_STRUCTURE,
    .response = enc,
    .is_filtered = false,
  };
  render_map_map_field(
    &structure_ctx, "response", console_toplevel_request, NULL);

  struct console_request_context type_ctx = {
    .type = CONSOLE_DESCRIBE,
    .response = enc,
    .is_filtered = false,
  };
  render_map_map_field(&type_ctx, "types", console_toplevel_types, NULL);

  report_success(enc, true);
}

static void console_request_get(CborEncoder *enc, CborValue *pathlist) {
  struct console_request_context ctx = {
    .type = CONSOLE_GET,
    .response = enc,
    .path = pathlist,
    .is_filtered = !cbor_value_at_end(pathlist),
  };
  cbor_encode_text_stringz(enc, "response");
  render_map_object(&ctx, console_toplevel_request, NULL);
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

    CborValue path, pathlist;
    cbor_value_map_find_value(&value, "path", &path);
    bool path_is_present = false;
    if (cbor_value_is_array(&path)) {
      cbor_value_enter_container(&path, &pathlist);
      path_is_present = true;
    }

    cbor_value_text_string_equals(&request_method_value, "get", &match);
    if (match) {
      if (path_is_present) {
        console_request_get(&response_map, &pathlist);
      } else {
        /* TODO: give an error */
      }
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
  uint8_t txbuffer[4096];

  size_t read_size;
  if ((read_size = console_try_read())) {
    /* Parse a request from the client */
    console_process_request(read_size);
    console_shift_rx_buffer(read_size);
  }

  /* Has it been 100ms since the last description? */
  if (time_diff(current_time(), last_desc_time) > time_from_us(100000)) {
    /* If so, print a description message */
    size_t write_size = console_feed_line_keys(txbuffer, sizeof(txbuffer));
    console_write_full(txbuffer, write_size);
    last_desc_time = current_time();
  } else {
    /* Otherwise a feed message */
    size_t write_size = console_feed_line(txbuffer, sizeof(txbuffer));
    console_write_full(txbuffer, write_size);
  }
  stats_finish_timing(STATS_CONSOLE_TIME);
}

#ifdef UNITTEST
#include <check.h>
#include <stdarg.h>

struct cbor_test_context {
  char buf[16384];
  CborEncoder top_encoder;
  CborParser top_parser;
  CborValue top_value;

  char pathbuf[512];
  CborParser path_parser;
  CborValue path_value;
};

struct cbor_test_context test_ctx;

static void init_console_tests() {
  cbor_encoder_init(
    &test_ctx.top_encoder, test_ctx.buf, sizeof(test_ctx.buf), 0);
}

static void deinit_console_tests() {}

static void render_path(const char *fmt, ...) {
  CborEncoder enc, array;
  cbor_encoder_init(&enc, test_ctx.pathbuf, sizeof(test_ctx.pathbuf), 0);
  cbor_encoder_create_array(&enc, &array, CborIndefiniteLength);
  va_list va_fmt;
  va_start(va_fmt, fmt);

  for (const char *i = fmt; *i != 0; i++) {
    uint32_t u;
    const char *s;
    switch (*i) {
    case 'u':
      u = va_arg(va_fmt, uint32_t);
      cbor_encode_int(&array, u);
      break;
    case 's':
      s = va_arg(va_fmt, const char *);
      cbor_encode_text_stringz(&array, s);
      break;
    default:
      cbor_encode_null(&array);
      break;
    }
  }
  va_end(va_fmt);
  cbor_encoder_close_container(&enc, &array);

  CborValue top_value;
  cbor_parser_init(test_ctx.pathbuf,
                   sizeof(test_ctx.pathbuf),
                   0,
                   &test_ctx.path_parser,
                   &top_value);
  cbor_value_enter_container(&top_value, &test_ctx.path_value);
}

static void finish_writing() {
  cbor_parser_init(test_ctx.buf,
                   sizeof(test_ctx.buf),
                   0,
                   &test_ctx.top_parser,
                   &test_ctx.top_value);
}

START_TEST(test_render_uint32_object_get) {
  struct console_request_context ctx = {
    .type = CONSOLE_GET,
    .response = &test_ctx.top_encoder,
  };

  uint32_t field = 12;
  render_uint32_object(&ctx, "desc", &field);
  finish_writing();

  int result;
  ck_assert(cbor_value_get_int_checked(&test_ctx.top_value, &result) ==
            CborNoError);
  ck_assert_int_eq(result, field);
}
END_TEST

START_TEST(test_render_float_object_get) {
  struct console_request_context ctx = {
    .type = CONSOLE_GET,
    .response = &test_ctx.top_encoder,
  };

  float field = 3.2f;
  render_float_object(&ctx, "desc", &field);
  finish_writing();

  float result;
  ck_assert(cbor_value_get_float(&test_ctx.top_value, &result) == CborNoError);
  ck_assert_float_eq(result, field);
}
END_TEST

START_TEST(test_smoke_console_request_structure) {

  CborEncoder structure_enc;
  cbor_encoder_create_map(&test_ctx.top_encoder, &structure_enc, 1);
  console_request_structure(&structure_enc);
  cbor_encoder_close_container(&test_ctx.top_encoder, &structure_enc);
  finish_writing();

  ck_assert(cbor_value_validate_basic(&test_ctx.top_value) == CborNoError);
  ck_assert(cbor_value_is_map(&test_ctx.top_value));

  /* Test that decoder has an offset field with a type and description */
  CborValue response_value;
  ck_assert(cbor_value_map_find_value(
              &test_ctx.top_value, "response", &response_value) == CborNoError);
  ck_assert(cbor_value_is_map(&response_value));

  CborValue decoder_value;
  ck_assert(cbor_value_map_find_value(
              &response_value, "decoder", &decoder_value) == CborNoError);
  ck_assert(cbor_value_is_map(&decoder_value));

  CborValue offset_value;
  ck_assert(cbor_value_map_find_value(
              &decoder_value, "offset", &offset_value) == CborNoError);
  ck_assert(cbor_value_is_map(&offset_value));

  CborValue type_value, description_value;
  ck_assert(cbor_value_map_find_value(&offset_value, "_type", &type_value) ==
            CborNoError);
  ck_assert(cbor_value_map_find_value(
              &offset_value, "description", &description_value) == CborNoError);

  ck_assert(cbor_value_is_text_string(&type_value));
  ck_assert(cbor_value_is_text_string(&description_value));
}
END_TEST

START_TEST(test_smoke_console_request_get_full) {

  render_path("");

  CborEncoder get_enc;
  cbor_encoder_create_map(&test_ctx.top_encoder, &get_enc, 1);
  console_request_get(&get_enc, &test_ctx.path_value);
  cbor_encoder_close_container(&test_ctx.top_encoder, &get_enc);
  finish_writing();

  ck_assert(cbor_value_validate_basic(&test_ctx.top_value) == CborNoError);
  ck_assert(cbor_value_is_map(&test_ctx.top_value));

  /* Test that decoder has an offset field with a type and description */
  CborValue response_value;
  ck_assert(cbor_value_map_find_value(
              &test_ctx.top_value, "response", &response_value) == CborNoError);
  ck_assert(cbor_value_is_map(&response_value));

  CborValue decoder_value;
  ck_assert(cbor_value_map_find_value(
              &response_value, "decoder", &decoder_value) == CborNoError);
  ck_assert(cbor_value_is_map(&decoder_value));

  CborValue offset_value;
  ck_assert(cbor_value_map_find_value(
              &decoder_value, "offset", &offset_value) == CborNoError);
  ck_assert(cbor_value_is_float(&offset_value));
}
END_TEST

TCase *setup_console_tests() {
  TCase *console_tests = tcase_create("console");
  tcase_add_checked_fixture(
    console_tests, init_console_tests, deinit_console_tests);
  /* Object primitives */
  tcase_add_test(console_tests, test_render_uint32_object_get);
  tcase_add_test(console_tests, test_render_float_object_get);

  /* Containers */

  /* Real access / integration tests */
  tcase_add_test(console_tests, test_smoke_console_request_structure);
  tcase_add_test(console_tests, test_smoke_console_request_get_full);
  return console_tests;
}

#endif
