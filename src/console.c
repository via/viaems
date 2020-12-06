/* For strtok_r */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 1
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>

#include "calculations.h"
#include "config.h"
#include "console.h"
#include "decoder.h"
#include "platform.h"
#include "sensors.h"
#include "stats.h"

const struct console_feed_node console_feed_nodes[] = {
  { .id = "cputime", .uint32_fptr = current_time },

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

  { .id = "sensor.map",
    .float_ptr = &config.sensors[SENSOR_MAP].processed_value },
  { .id = "sensor.iat",
    .float_ptr = &config.sensors[SENSOR_IAT].processed_value },
  { .id = "sensor.clt",
    .float_ptr = &config.sensors[SENSOR_CLT].processed_value },
  { .id = "sensor.brv",
    .float_ptr = &config.sensors[SENSOR_BRV].processed_value },
  { .id = "sensor.tps",
    .float_ptr = &config.sensors[SENSOR_TPS].processed_value },
  { .id = "sensor.aap",
    .float_ptr = &config.sensors[SENSOR_AAP].processed_value },
  { .id = "sensor.frt",
    .float_ptr = &config.sensors[SENSOR_FRT].processed_value },
  { .id = "sensor.ego",
    .float_ptr = &config.sensors[SENSOR_EGO].processed_value },
  { .id = "sensor.frp",
    .float_ptr = &config.sensors[SENSOR_FRP].processed_value },

  { .id = "sensor_faults", .uint32_fptr = sensor_fault_status },

  /* Decoder */
  { .id = "rpm", .uint32_ptr = &config.decoder.rpm },
  { .id = "tooth_rpm", .uint32_ptr = &config.decoder.tooth_rpm },
  { .id = "sync", .uint32_ptr = &config.decoder.valid },
  { .id = "rpm_variance", .float_ptr = &config.decoder.trigger_cur_rpm_change },
  { .id = "last_trigger_angle",
    .float_ptr = &config.decoder.last_trigger_angle },
  { .id = "t0_count", .uint32_ptr = &config.decoder.t0_count },
  { .id = "t1_count", .uint32_ptr = &config.decoder.t1_count },
  { 0 },
};

#ifndef GIT_DESCRIBE
#define GIT_DESCRIBE "unknown"
static const char *git_describe = GIT_DESCRIBE;
#endif

static struct {
  uint32_t enabled;
  struct logged_event events[32];
  volatile uint32_t read;
  volatile uint32_t write;
} event_log = { .enabled = 1 };

static struct logged_event get_logged_event() {
  if (!event_log.enabled || (event_log.read == event_log.write)) {
    return (struct logged_event){ .type = EVENT_NONE };
  }
  struct logged_event ret = event_log.events[event_log.read];
  event_log.read = (event_log.read + 1) %
                   (sizeof(event_log.events) / sizeof(event_log.events[0]));
  return ret;
}

void console_record_event(struct logged_event ev) {
  if (!event_log.enabled) {
    return;
  }

  int size = (sizeof(event_log.events) / sizeof(event_log.events[0]));
  if ((event_log.write + 1) % size == event_log.read) {
    return;
  }

  event_log.events[event_log.write] = ev;
  event_log.write = (event_log.write + 1) % size;
}

static size_t console_event_message(uint8_t *dest,
                                    size_t bsize,
                                    struct logged_event *ev) {
  CborEncoder encoder;

  cbor_encoder_init(&encoder, dest, bsize, 0);

  CborEncoder top_encoder;
  cbor_encoder_create_map(&encoder, &top_encoder, 3);
  cbor_encode_text_stringz(&top_encoder, "type");
  cbor_encode_text_stringz(&top_encoder, "event");

  cbor_encode_text_stringz(&top_encoder, "time");
  cbor_encode_int(&top_encoder, ev->time);

  cbor_encode_text_stringz(&top_encoder, "event");

  CborEncoder event_encoder;

  switch (ev->type) {
  case EVENT_OUTPUT:
    cbor_encoder_create_map(&top_encoder, &event_encoder, 2);
    cbor_encode_text_stringz(&event_encoder, "type");
    cbor_encode_text_stringz(&event_encoder, "output");
    cbor_encode_text_stringz(&event_encoder, "outputs");
    cbor_encode_int(&event_encoder, ev->value);
    cbor_encoder_close_container(&top_encoder, &event_encoder);
    break;
  case EVENT_GPIO:
    cbor_encoder_create_map(&top_encoder, &event_encoder, 2);
    cbor_encode_text_stringz(&event_encoder, "type");
    cbor_encode_text_stringz(&event_encoder, "gpio");
    cbor_encode_text_stringz(&event_encoder, "outputs");
    cbor_encode_int(&event_encoder, ev->value);
    cbor_encoder_close_container(&top_encoder, &event_encoder);
    break;
  case EVENT_TRIGGER:
    cbor_encoder_create_map(&top_encoder, &event_encoder, 2);
    cbor_encode_text_stringz(&event_encoder, "type");
    cbor_encode_text_stringz(&event_encoder, "trigger");
    cbor_encode_text_stringz(&event_encoder, "pin");
    cbor_encode_int(&event_encoder, ev->value);
    cbor_encoder_close_container(&top_encoder, &event_encoder);
    break;
  default:
    break;
  }
  cbor_encoder_close_container(&encoder, &top_encoder);
  return cbor_encoder_get_buffer_size(&encoder, dest);
}

void render_type_field(CborEncoder *enc, const char *type) {
  cbor_encode_text_stringz(enc, "_type");
  cbor_encode_text_stringz(enc, type);
}

void render_description_field(CborEncoder *enc, const char *description) {
  cbor_encode_text_stringz(enc, "description");
  cbor_encode_text_stringz(enc, description);
}

static void report_success(CborEncoder *enc, bool success) {
  cbor_encode_text_stringz(enc, "success");
  cbor_encode_boolean(enc, success);
}

static void report_parsing_error(CborEncoder *enc, const char *msg) {
  cbor_encode_text_stringz(enc, "error");
  cbor_encode_text_stringz(enc, msg);
  report_success(enc, false);
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

static bool console_string_matches(CborValue *val, const char *str) {
  bool match;
  if (cbor_value_text_string_equals(val, str, &match) != CborNoError) {
    return false;
  }
  return match;
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
  for (const struct console_feed_node *node = &console_feed_nodes[0];
       node->id != NULL;
       node++) {
    cbor_encode_text_stringz(&key_list_encoder, node->id);
  }
  cbor_encoder_close_container(&top_encoder, &key_list_encoder);
  cbor_encoder_close_container(&encoder, &top_encoder);
  return cbor_encoder_get_buffer_size(&encoder, dest);
}

static size_t console_feed_line(uint8_t *dest, size_t bsize) {
  CborEncoder encoder;

  cbor_encoder_init(&encoder, dest, bsize, 0);

  CborEncoder top_encoder;
  cbor_encoder_create_map(&encoder, &top_encoder, 2);
  cbor_encode_text_stringz(&top_encoder, "type");
  cbor_encode_text_stringz(&top_encoder, "feed");

  cbor_encode_text_stringz(&top_encoder, "values");
  CborEncoder value_list_encoder;
  cbor_encoder_create_array(
    &top_encoder, &value_list_encoder, CborIndefiniteLength);
  for (const struct console_feed_node *node = &console_feed_nodes[0];
       node->id != NULL;
       node++) {
    if (node->uint32_ptr) {
      cbor_encode_uint(&value_list_encoder, *node->uint32_ptr);
    } else if (node->float_ptr) {
      cbor_encode_float(&value_list_encoder, *node->float_ptr);
    } else if (node->uint32_fptr) {
      cbor_encode_uint(&value_list_encoder, node->uint32_fptr());
    } else if (node->float_fptr) {
      cbor_encode_float(&value_list_encoder, node->float_fptr());
    } else {
      cbor_encode_text_stringz(&value_list_encoder, "invalid");
    }
  }
  cbor_encoder_close_container(&top_encoder, &value_list_encoder);
  cbor_encoder_close_container(&encoder, &top_encoder);
  return cbor_encoder_get_buffer_size(&encoder, dest);
}

static int console_write_full(const uint8_t *buf, size_t len) {
  size_t remaining = len;
  const uint8_t *ptr = buf;
  while (remaining) {
    size_t written = console_write(ptr, remaining);
    if (written > 0) {
      remaining -= written;
      ptr += written;
    }
  }
  return 1;
}

static uint8_t rx_buffer[16384];
static size_t rx_buffer_size = 0;
static timeval_t rx_start_time = 0;

static void console_shift_rx_buffer(size_t amt) {
  assert(amt <= rx_buffer_size);
  memmove(&rx_buffer[0], &rx_buffer[amt], (rx_buffer_size - amt));
  rx_buffer_size -= amt;
}

static size_t console_try_read() {
  size_t remaining = sizeof(rx_buffer) - rx_buffer_size;

  if (rx_buffer_size == 0) {
    rx_start_time = current_time();
  }

  size_t read_amt = console_read(&rx_buffer[rx_buffer_size], remaining);
  rx_buffer_size += read_amt;

  if (rx_buffer_size == 0) {
    return 0;
  }

  CborParser parser;
  CborValue value;

  /* We want to determine if we have a valid cbor object in buffer.  If the
   * buffer doesn't start with a map, it is not valid, and we want to advance
   * byte-by-byte until we find a start of map */
  do {
    if (cbor_parser_init(rx_buffer, rx_buffer_size, 0, &parser, &value) ==
        CborNoError) {
      if (cbor_value_is_map(&value)) {
        break;
      }
    }
    /* Reset timer since we have a new start */
    console_shift_rx_buffer(1);
    rx_start_time = current_time();

    /* We've exhausted the buffer */
    if (!rx_buffer_size) {
      return 0;
    }
  } while (1);

  switch (cbor_value_validate_basic(&value)) {
    /* If we have a valid object, return its length */
  case CborNoError:
  case CborErrorGarbageAtEnd: {
    cbor_value_advance(&value);
    const uint8_t *next = cbor_value_get_next_byte(&value);
    return (next - rx_buffer);
  }
  /* If we have the start of a valid object (as per the cbor_value_is_map check
   * above, but not enough to decode the whole object, return but do not reset
   * unless 1s has passed. This is a balance between allowing time for messages
   * to arrive but not locking up communications due to some garbage */
  case CborErrorAdvancePastEOF:
  case CborErrorUnexpectedEOF:
    if (current_time() - rx_start_time > time_from_us(1000000)) {
      rx_buffer_size = 0;
    }
    /* Alternatively, if we've filled up, waiting longer will not work, reset */
    if (rx_buffer_size == sizeof(rx_buffer)) {
      rx_buffer_size = 0;
    }
    break;
  /* Assume garbage input, reset */
  default:
    rx_buffer_size = 0;
    break;
  }
  return 0;
}

static void console_request_ping(CborEncoder *enc) {
  cbor_encode_text_stringz(enc, "response");
  cbor_encode_text_stringz(enc, "pong");
  report_success(enc, true);
}

void render_uint32_map_field(struct console_request_context *ctx,
                             const char *id,
                             const char *description,
                             uint32_t *ptr) {
  struct console_request_context deeper;
  if (descend_map_field(ctx, &deeper, id)) {
    render_uint32_object(&deeper, description, ptr);
  }
}

void render_uint32_object(struct console_request_context *ctx,
                          const char *description,
                          uint32_t *ptr) {
  switch (ctx->type) {
  case CONSOLE_SET:
    if (cbor_value_is_integer(&ctx->value)) {
      cbor_value_get_int_checked(&ctx->value, (int *)ptr);
    }
    /* Fall through */
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
  struct console_request_context deeper;
  if (descend_map_field(ctx, &deeper, id)) {
    render_float_object(&deeper, description, ptr);
  }
}
/* This function loosely borrowed from tinycbor's internal helpers until
 * https://github.com/intel/tinycbor/issues/149 is merged
 */
static float decode_half(uint16_t half) {
  int exp = (half >> 10) & 0x1f;
  int mant = half & 0x3ff;
  float val;
  if (exp == 0) {
    val = ldexpf(mant, -24);
  } else if (exp != 31) {
    val = ldexpf(mant + 1024, exp - 25);
  } else {
    val = mant == 0 ? INFINITY : NAN;
  }
  return half & 0x8000 ? -val : val;
}

void render_float_object(struct console_request_context *ctx,
                         const char *description,
                         float *ptr) {

  switch (ctx->type) {
  case CONSOLE_SET:
    if (cbor_value_is_float(&ctx->value)) {
      cbor_value_get_float(&ctx->value, ptr);
    } else if (cbor_value_is_half_float(&ctx->value)) {
      /* Support f16 for client support */
      uint16_t dest;
      cbor_value_get_half_float(&ctx->value, &dest);
      *ptr = decode_half(dest);
    } else if (cbor_value_is_double(&ctx->value)) {
      /* Support doubles for client support */
      double val;
      cbor_value_get_double(&ctx->value, &val);
      *ptr = val;
    }
    /* Fall through */
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
  struct console_request_context deeper;
  if (descend_map_field(ctx, &deeper, id)) {
    render_map_object(&deeper, rend, ptr);
  }
}

void render_custom_map_field(struct console_request_context *ctx,
                             const char *id,
                             console_renderer rend,
                             void *ptr) {
  struct console_request_context deeper;
  if (descend_map_field(ctx, &deeper, id)) {
    rend(&deeper, ptr);
  }
}

bool descend_array_field(struct console_request_context *ctx,
                         struct console_request_context *deeper_ctx,
                         int index) {
  if (ctx->is_completed) {
    return false;
  }

  *deeper_ctx = *ctx;

  switch (ctx->type) {
  case CONSOLE_SET:
    if (!ctx->is_filtered) {
      if (!cbor_value_is_array(&ctx->value)) {
        return false;
      }
      CborValue newvalue;
      cbor_value_enter_container(&ctx->value, &newvalue);
      for (int i = 0; i < index; i++) {
        if (cbor_value_at_end(&newvalue)) {
          return false;
        }
        cbor_value_advance(&newvalue);
      }
      deeper_ctx->value = newvalue;
    }
    /* intentional fallthrough */
  case CONSOLE_GET:
    /* Short circuit all future rendering if we've already matched a path */
    if (ctx->is_filtered) {
      /* If we're filtering, we need to match the path */
      if (!console_path_match_int(deeper_ctx->path, index)) {
        return false;
      }
      cbor_value_advance(deeper_ctx->path);
      ctx->is_completed = true;
      deeper_ctx->is_filtered = !cbor_value_at_end(deeper_ctx->path);
      deeper_ctx->is_completed = false;
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

  bool wrapped = !ctx->is_filtered;

  CborEncoder array;
  CborEncoder *outer = ctx->response;

  if (wrapped) {
    cbor_encoder_create_array(outer, &array, CborIndefiniteLength);
    ctx->response = &array;
  }

  rend(ctx, ptr);
  if (ctx->is_filtered && !ctx->is_completed) {
    cbor_encode_null(ctx->response);
  }

  if (wrapped) {
    cbor_encoder_close_container(outer, &array);
  }
  ctx->response = outer;
}

void render_map_array_field(struct console_request_context *ctx,
                            int index,
                            console_renderer rend,
                            void *ptr) {
  struct console_request_context deeper;
  if (descend_array_field(ctx, &deeper, index)) {
    render_map_object(&deeper, rend, ptr);
  }
}

void render_array_map_field(struct console_request_context *ctx,
                            const char *id,
                            console_renderer rend,
                            void *ptr) {
  struct console_request_context deeper;
  if (descend_map_field(ctx, &deeper, id)) {
    render_array_object(&deeper, rend, ptr);
  }
}

void render_map_object(struct console_request_context *ctx,
                       console_renderer rend,
                       void *ptr) {

  bool wrapped = !ctx->is_filtered;

  CborEncoder map;
  CborEncoder *outer = ctx->response;

  if (wrapped) {
    cbor_encoder_create_map(outer, &map, CborIndefiniteLength);
    ctx->response = &map;
  }

  rend(ctx, ptr);
  if (ctx->is_filtered && !ctx->is_completed) {
    cbor_encode_null(ctx->response);
  }

  if (wrapped) {
    cbor_encoder_close_container(outer, &map);
  }

  ctx->response = outer;
}

bool descend_map_field(struct console_request_context *ctx,
                       struct console_request_context *deeper_ctx,
                       const char *id) {

  if (ctx->is_completed) {
    return false;
  }

  *deeper_ctx = *ctx;

  switch (ctx->type) {
  case CONSOLE_SET:
    /* If unfiltered, start descending into the value object */
    if (!ctx->is_filtered) {
      if (!cbor_value_is_map(&ctx->value)) {
        return false;
      }
      CborValue newvalue;
      if (cbor_value_map_find_value(&ctx->value, id, &newvalue) !=
          CborNoError) {
        return false;
      }
      deeper_ctx->value = newvalue;
    }
    /* Intentional fallthrough */
  case CONSOLE_GET:
    /* Short circuit all future rendering if we've already matched a path */
    if (ctx->is_filtered) {
      /* If we're filtering, we need to match the path */
      if (!console_path_match_str(deeper_ctx->path, id)) {
        return false;
      }
      cbor_value_advance(deeper_ctx->path);
      ctx->is_completed = true;
      deeper_ctx->is_filtered = !cbor_value_at_end(deeper_ctx->path);
      deeper_ctx->is_completed = false;
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

void render_enum_map_field(struct console_request_context *ctx,
                           const char *id,
                           const char *desc,
                           const struct console_enum_mapping *mapping,
                           void *ptr) {
  struct console_request_context deeper;
  if (descend_map_field(ctx, &deeper, id)) {
    render_enum_object(&deeper, desc, mapping, ptr);
  }
}

void render_enum_object(struct console_request_context *ctx,
                        const char *desc,
                        const struct console_enum_mapping *mapping,
                        void *ptr) {
  int *type = ptr;
  bool success = false;
  switch (ctx->type) {
  case CONSOLE_SET:
    for (const struct console_enum_mapping *m = mapping; m->s; m++) {
      if (console_string_matches(&ctx->value, m->s)) {
        *type = m->e;
      }
    }
    /* Intentional Fallthrough */
  case CONSOLE_GET:
    for (const struct console_enum_mapping *m = mapping; m->s; m++) {
      if (*type == m->e) {
        success = true;
        cbor_encode_text_stringz(ctx->response, m->s);
      }
    }
    if (!success) {
      cbor_encode_null(ctx->response);
    }
    return;
  case CONSOLE_STRUCTURE:
  case CONSOLE_DESCRIBE: {
    CborEncoder map;
    cbor_encoder_create_map(ctx->response, &map, 3);
    render_type_field(&map, "string");
    render_description_field(&map, desc);
    cbor_encode_text_stringz(&map, "choices");
    CborEncoder list_encoder;
    cbor_encoder_create_array(&map, &list_encoder, CborIndefiniteLength);
    for (const struct console_enum_mapping *m = mapping; m->s; m++) {
      cbor_encode_text_stringz(&list_encoder, m->s);
    }
    cbor_encoder_close_container(&map, &list_encoder);
    cbor_encoder_close_container(ctx->response, &map);
  }
    return;
  }
}

static void render_table_title(struct console_request_context *ctx, void *_t) {

  struct table *t = _t;
  switch (ctx->type) {
  case CONSOLE_SET:
    if (cbor_value_is_text_string(&ctx->value)) {
      size_t len = sizeof(t->title);
      cbor_value_copy_text_string(&ctx->value, t->title, &len, NULL);
    }
    /* Fall through */
  case CONSOLE_GET:
    cbor_encode_text_stringz(ctx->response, t->title);
    break;
  case CONSOLE_STRUCTURE:
  case CONSOLE_DESCRIBE: {
    CborEncoder desc;
    cbor_encoder_create_map(ctx->response, &desc, 2);
    render_type_field(&desc, "string");
    render_description_field(&desc, "table title");
    cbor_encoder_close_container(ctx->response, &desc);
    break;
  }
  default:
    break;
  }
}

static void render_table_axis_name(struct console_request_context *ctx,
                                   void *_axis) {

  struct table_axis *axis = _axis;
  switch (ctx->type) {
  case CONSOLE_SET:
    if (cbor_value_is_text_string(&ctx->value)) {
      size_t len = sizeof(axis->name);
      cbor_value_copy_text_string(&ctx->value, axis->name, &len, NULL);
    }
    /* Fall through */
  case CONSOLE_GET:
    cbor_encode_text_stringz(ctx->response, axis->name);
    break;
  case CONSOLE_STRUCTURE:
  case CONSOLE_DESCRIBE: {
    CborEncoder desc;
    cbor_encoder_create_map(ctx->response, &desc, 2);
    render_type_field(&desc, "string");
    render_description_field(&desc, "axis title");
    cbor_encoder_close_container(ctx->response, &desc);
    break;
  }
  default:
    break;
  }
}

static void render_table_axis_values(struct console_request_context *ctx,
                                     void *_a) {

  struct table_axis *axis = _a;
  size_t len = axis->num;
  if (ctx->type == CONSOLE_SET && cbor_value_is_array(&ctx->value)) {
    if (cbor_value_get_array_length(&ctx->value, &len) != CborNoError) {
      len = axis->num;
    }
    if (len > MAX_AXIS_SIZE) {
      len = MAX_AXIS_SIZE;
    }
    axis->num = len;
  }
  for (int i = 0; i < (axis->num > MAX_AXIS_SIZE ? MAX_AXIS_SIZE : axis->num);
       i++) {
    struct console_request_context deeper;
    if (descend_array_field(ctx, &deeper, i)) {
      render_float_object(&deeper, "axis value", &axis->values[i]);
    }
  }
}

static void render_table_axis_description(struct console_request_context *ctx,
                                          void *ptr) {
  (void)ptr;
  CborEncoder desc;
  cbor_encoder_create_map(ctx->response, &desc, 3);
  render_type_field(&desc, "[float]");
  render_description_field(&desc, "list of axis values");
  cbor_encode_text_stringz(&desc, "len");
  cbor_encode_int(&desc, MAX_AXIS_SIZE);
  cbor_encoder_close_container(ctx->response, &desc);
}

static void render_table_axis(struct console_request_context *ctx, void *_t) {
  struct table_axis *axis = _t;
  render_custom_map_field(ctx, "name", render_table_axis_name, axis);
  if (ctx->type == CONSOLE_DESCRIBE) {
    render_custom_map_field(ctx, "values", render_table_axis_description, NULL);
  } else {
    render_array_map_field(ctx, "values", render_table_axis_values, axis);
  }
}

struct nested_table_context {
  struct table *t;
  int row;
};

static void render_table_second_axis_data(struct console_request_context *ctx,
                                          void *_ntc) {
  struct nested_table_context *ntc = _ntc;
  struct table *t = ntc->t;
  for (int j = 0; j < t->axis[0].num; j++) {
    struct console_request_context deeper;
    if (descend_array_field(ctx, &deeper, j)) {
      render_float_object(&deeper, "axis value", &t->data.two[ntc->row][j]);
    }
  }
}

static void render_table_data_description(struct console_request_context *ctx,
                                          void *ptr) {
  (void)ptr;
  CborEncoder desc;
  cbor_encoder_create_map(ctx->response, &desc, 3);
  render_type_field(&desc, "[[float]]");
  render_description_field(&desc, "list of lists of table values");
  cbor_encode_text_stringz(&desc, "len");
  cbor_encode_int(&desc, MAX_AXIS_SIZE);
  cbor_encoder_close_container(ctx->response, &desc);
}

static void render_table_data(struct console_request_context *ctx, void *_t) {
  struct table *t = _t;
  if (t->num_axis == 1) {
    for (int i = 0; i < t->axis[0].num; i++) {
      struct console_request_context deeper;
      if (descend_array_field(ctx, &deeper, i)) {
        render_float_object(&deeper, "axis value", &t->data.one[i]);
      }
    }
  } else if (t->num_axis == 2) {
    for (int i = 0; i < t->axis[1].num; i++) {
      struct console_request_context deeper;
      if (descend_array_field(ctx, &deeper, i)) {
        struct nested_table_context ntc = { .t = t, .row = i };
        render_array_object(&deeper, render_table_second_axis_data, &ntc);
      }
    }
  }
}

static void render_table_object(struct console_request_context *ctx, void *_t) {
  struct table *t = _t;
  if (ctx->type == CONSOLE_STRUCTURE) {
    render_type_field(ctx->response, "table");
    return;
  }
  render_custom_map_field(ctx, "title", render_table_title, t);
  render_uint32_map_field(
    ctx, "num-axis", "number of axis (1 or 2)", &t->num_axis);
  render_map_map_field(ctx, "horizontal-axis", render_table_axis, &t->axis[0]);
  render_map_map_field(ctx, "vertical-axis", render_table_axis, &t->axis[1]);

  if ((ctx->type != CONSOLE_DESCRIBE)) {
    render_array_map_field(ctx, "data", render_table_data, t);
  } else {
    render_custom_map_field(ctx, "data", render_table_data_description, NULL);
  }
}

static void render_tables(struct console_request_context *ctx, void *ptr) {
  (void)ptr;
  render_map_map_field(ctx, "ve", render_table_object, config.ve);
  render_map_map_field(
    ctx, "lambda", render_table_object, config.commanded_lambda);
  render_map_map_field(ctx, "dwell", render_table_object, config.dwell);
  render_map_map_field(ctx, "timing", render_table_object, config.timing);
  render_map_map_field(ctx,
                       "injector_dead_time",
                       render_table_object,
                       config.injector_pw_compensation);
  render_map_map_field(
    ctx, "temp-enrich", render_table_object, config.engine_temp_enrich);
  render_map_map_field(
    ctx, "tipin-amount", render_table_object, config.tipin_enrich_amount);
  render_map_map_field(
    ctx, "tipin-time", render_table_object, config.tipin_enrich_duration);
}

static void render_decoder(struct console_request_context *ctx, void *ptr) {
  (void)ptr;

  render_float_map_field(
    ctx, "offset", "offset past TDC for sync pulse", &config.decoder.offset);

  render_float_map_field(ctx,
                         "max-variance",
                         "inter-tooth max time variation (0-1)",
                         &config.decoder.trigger_max_rpm_change);

  render_uint32_map_field(
    ctx, "min-rpm", "minimum RPM for sync", &config.decoder.trigger_min_rpm);

  int type = config.decoder.type;
  render_enum_map_field(
    ctx,
    "trigger-type",
    "Primary trigger decoder method",
    (struct console_enum_mapping[]){ { TRIGGER_EVEN_NOSYNC, "even" },
                                     { TRIGGER_EVEN_CAMSYNC, "even+camsync" },
                                     { 0, NULL } },
    &type);
  config.decoder.type = type;
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

  int type = ev->type;
  render_enum_map_field(
    ctx,
    "type",
    "output type",
    (struct console_enum_mapping[]){ { DISABLED_EVENT, "disabled" },
                                     { FUEL_EVENT, "fuel" },
                                     { IGNITION_EVENT, "ignition" },
                                     { 0, NULL } },
    &type);
  ev->type = type;
}

static void render_outputs(struct console_request_context *ctx, void *ptr) {
  (void)ptr;
  for (int i = 0; i < MAX_EVENTS; i++) {
    render_map_array_field(ctx, i, output_console_renderer, &config.events[i]);
  }
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
  case SENSOR_FRP:
    return "frp";
  default:
    return "invalid";
  }
}

static void render_sensor_object(struct console_request_context *ctx,
                                 void *ptr) {

  if (ctx->type == CONSOLE_STRUCTURE) {
    render_type_field(ctx->response, "sensor");
    return;
  }

  struct sensor_input *input = ptr;

  render_uint32_map_field(ctx, "pin", "adc sensor input pin", &input->pin);
  render_float_map_field(
    ctx, "lag", "lag filter coefficient (0-1)", &input->lag);

  int source = input->source;
  render_enum_map_field(
    ctx,
    "source",
    "sensor data source",
    (struct console_enum_mapping[]){ { SENSOR_NONE, "none" },
                                     { SENSOR_ADC, "adc" },
                                     { SENSOR_FREQ, "freq" },
                                     { SENSOR_DIGITAL, "digital" },
                                     { SENSOR_CONST, "const" },
                                     { 0, NULL } },
    &source);
  input->source = source;

  int method = input->method;
  render_enum_map_field(ctx,
                        "method",
                        "sensor processing method",
                        (struct console_enum_mapping[]){
                          { METHOD_LINEAR, "linear" },
                          { METHOD_LINEAR_WINDOWED, "linear-window" },
                          { METHOD_TABLE, "table" },
                          { METHOD_THERM, "therm" },
                          { 0, NULL } },
                        &method);
  input->method = method;

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

static void render_sensors(struct console_request_context *ctx, void *ptr) {
  (void)ptr;
  for (sensor_input_type i = 0; i < NUM_SENSORS; i++) {
    render_map_map_field(
      ctx, sensor_name_from_type(i), render_sensor_object, &config.sensors[i]);
  }
}

static void render_crank_enrich(struct console_request_context *ctx,
                                void *ptr) {
  (void)ptr;
  render_float_map_field(ctx,
                         "crank-rpm",
                         "Upper RPM limit for crank enrich",
                         &config.fueling.crank_enrich_config.crank_rpm);
  render_float_map_field(
    ctx,
    "crank-temp",
    "Upper temperature (C) for crank enrich",
    &config.fueling.crank_enrich_config.cutoff_temperature);
  render_float_map_field(ctx,
                         "enrich-amt",
                         "crank enrichment multiplier",
                         &config.fueling.crank_enrich_config.enrich_amt);
}

static void render_fueling(struct console_request_context *ctx, void *ptr) {
  (void)ptr;
  render_float_map_field(ctx,
                         "injector-cc",
                         "fuel injector size cc/min",
                         &config.fueling.injector_cc_per_minute);
  render_float_map_field(
    ctx, "cylinder-cc", "cylinder size in cc", &config.fueling.cylinder_cc);
  render_float_map_field(ctx,
                         "fuel-stoich-ratio",
                         "stoich ratio of fuel to air",
                         &config.fueling.fuel_stoich_ratio);
  render_uint32_map_field(ctx,
                          "injections-per-cycle",
                          "Number of times injectors fire per cycle (batching)",
                          &config.fueling.injections_per_cycle);
  render_uint32_map_field(ctx,
                          "fuel-pump-pin",
                          "GPIO pin for fuel pump control",
                          &config.fueling.fuel_pump_pin);
  render_float_map_field(ctx,
                         "fuel-density",
                         "Fuel density (g/cc) at 15C",
                         &config.fueling.density_of_fuel);
  render_map_map_field(ctx, "crank-enrich", render_crank_enrich, NULL);
}

static void render_ignition(struct console_request_context *ctx, void *ptr) {
  (void)ptr;
  render_uint32_map_field(ctx,
                          "min-fire-time",
                          "minimum wait period for coil output (uS)",
                          &config.ignition.min_fire_time_us);
  render_float_map_field(ctx,
                         "dwell-time",
                         "dwell time for fixed-duty (uS)",
                         &config.ignition.dwell_us);
}

static void render_boost_control(struct console_request_context *ctx,
                                 void *ptr) {
  (void)ptr;
  render_uint32_map_field(
    ctx, "pin", "GPIO pin for boost control output", &config.boost_control.pin);
  render_float_map_field(ctx,
                         "threshold",
                         "Boost low threshold to enable boost control",
                         &config.boost_control.threshhold_kpa);
  render_float_map_field(ctx,
                         "overboost",
                         "High threshold for boost cut (kpa)",
                         &config.boost_control.overboost);
  render_map_map_field(
    ctx, "pwm", render_table_object, config.boost_control.pwm_duty_vs_rpm);
}

static void render_cel(struct console_request_context *ctx, void *ptr) {
  (void)ptr;
  render_uint32_map_field(
    ctx, "pin", "GPIO pin for CEL output", &config.cel.pin);
  render_float_map_field(ctx,
                         "lean-boost-kpa",
                         "Boost low threshold for lean boost warning",
                         &config.cel.lean_boost_kpa);
  render_float_map_field(ctx,
                         "lean-boost-ego",
                         "EGO high threshold for lean boost warning",
                         &config.cel.lean_boost_ego);
}

static void render_freq_object(struct console_request_context *ctx, void *ptr) {
  struct freq_input *f = ptr;

  int edge = f->edge;
  int type = f->type;

  render_enum_map_field(
    ctx,
    "edge",
    "type of edge to trigger",
    (const struct console_enum_mapping[]){ { RISING_EDGE, "rising" },
                                           { FALLING_EDGE, "falling" },
                                           { BOTH_EDGES, "both" },
                                           { 0, NULL } },
    &edge);
  render_enum_map_field(
    ctx,
    "type",
    "input interpretation",
    (const struct console_enum_mapping[]){
      { FREQ, "freq" }, { TRIGGER, "trigger" }, { 0, NULL } },
    &type);

  f->edge = edge;
  f->type = type;
}

static void render_freq_list(struct console_request_context *ctx, void *ptr) {
  (void)ptr;
  for (int i = 0; i < 4; i++) {
    render_map_array_field(ctx, i, render_freq_object, &config.freq_inputs[i]);
  }
}

static void render_test(struct console_request_context *ctx, void *ptr) {
  (void)ptr;
  render_uint32_map_field(
    ctx, "event-logging", "Enable event logging", &event_log.enabled);

  /* Workaround to support the getter/setters */
  uint32_t old_rpm = get_test_trigger_rpm();
  uint32_t rpm = old_rpm;
  render_uint32_map_field(
    ctx, "test-trigger-rpm", "Test trigger output rpm (0 to disable)", &rpm);
  if ((ctx->type == CONSOLE_SET) && (rpm != old_rpm)) {
    set_test_trigger_rpm(rpm);
  }
}

#ifndef GIT_DESCRIBE
#define GIT_DESCRIBE "unknown"
#endif

static void render_version(struct console_request_context *ctx, void *_t) {

  (void)_t;
  switch (ctx->type) {
  case CONSOLE_SET:
  case CONSOLE_GET:
    cbor_encode_text_stringz(ctx->response, GIT_DESCRIBE);
    break;
  case CONSOLE_STRUCTURE:
  case CONSOLE_DESCRIBE: {
    CborEncoder desc;
    cbor_encoder_create_map(ctx->response, &desc, 2);
    render_type_field(&desc, "string");
    render_description_field(&desc, "ViaEMS version (RO)");
    cbor_encoder_close_container(ctx->response, &desc);
    break;
  }
  default:
    break;
  }
}

static void render_info(struct console_request_context *ctx, void *ptr) {
  (void)ptr;
  render_custom_map_field(ctx, "version", render_version, NULL);
}

static void console_toplevel_request(struct console_request_context *ctx,
                                     void *ptr) {
  (void)ptr;
  render_map_map_field(ctx, "decoder", render_decoder, NULL);
  render_map_map_field(ctx, "sensors", render_sensors, NULL);
  render_array_map_field(ctx, "outputs", render_outputs, NULL);
  render_map_map_field(ctx, "fueling", render_fueling, NULL);
  render_map_map_field(ctx, "ignition", render_ignition, NULL);
  render_map_map_field(ctx, "tables", render_tables, NULL);
  render_map_map_field(ctx, "boost-control", render_boost_control, NULL);
  render_map_map_field(ctx, "check-engine-light", render_cel, NULL);
  render_array_map_field(ctx, "freq", render_freq_list, NULL);
  render_map_map_field(ctx, "test", render_test, NULL);
  render_map_map_field(ctx, "info", render_info, NULL);
}

static void console_toplevel_types(struct console_request_context *ctx,
                                   void *ptr) {
  (void)ptr;
  render_map_map_field(ctx, "sensor", render_sensor_object, &config.sensors[0]);
  render_map_map_field(
    ctx, "output", output_console_renderer, &config.events[1]);
  render_map_map_field(ctx, "table", render_table_object, config.ve);
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
  report_success(enc, true);
}

static void console_request_set(CborEncoder *enc,
                                CborValue *pathlist,
                                CborValue *value) {
  struct console_request_context ctx = {
    .type = CONSOLE_SET,
    .response = enc,
    .path = pathlist,
    .is_filtered = !cbor_value_at_end(pathlist),
    .value = *value,
  };
  cbor_encode_text_stringz(enc, "response");
  render_map_object(&ctx, console_toplevel_request, NULL);
  report_success(enc, true);
}

static void console_request_flash(CborEncoder *response) {
  platform_save_config();
  report_success(response, true);
}

static void console_request_bootloader(CborEncoder *response) {
  report_success(response, true);
  platform_reset_into_bootloader();
}

static void console_process_request(CborValue *request, CborEncoder *response) {
  cbor_encode_text_stringz(response, "type");
  cbor_encode_text_stringz(response, "response");

  int req_id = -1;
  CborValue request_id_value;
  if (!cbor_value_map_find_value(request, "id", &request_id_value)) {
    if (cbor_value_is_integer(&request_id_value)) {
      cbor_value_get_int(&request_id_value, &req_id);
    }
  }

  cbor_encode_text_stringz(response, "id");
  cbor_encode_int(response, req_id);

  CborValue request_method_value;
  cbor_value_map_find_value(request, "method", &request_method_value);
  if (cbor_value_get_type(&request_method_value) == CborInvalidType) {
    report_parsing_error(response, "request has no 'method'");
    report_success(response, false);
    return;
  }

  bool match;
  cbor_value_text_string_equals(&request_method_value, "ping", &match);
  if (match) {
    console_request_ping(response);
    return;
  }

  cbor_value_text_string_equals(&request_method_value, "structure", &match);
  if (match) {
    console_request_structure(response);
    return;
  }

  cbor_value_text_string_equals(&request_method_value, "flash", &match);
  if (match) {
    console_request_flash(response);
    return;
  }

  cbor_value_text_string_equals(&request_method_value, "bootloader", &match);
  if (match) {
    console_request_bootloader(response);
    return;
  }

  CborValue path, pathlist;
  cbor_value_map_find_value(request, "path", &path);
  if (cbor_value_is_array(&path)) {
    cbor_value_enter_container(&path, &pathlist);
  } else {
    report_parsing_error(response, "no 'path' provided'");
    return;
  }

  cbor_value_text_string_equals(&request_method_value, "get", &match);
  if (match) {
    console_request_get(response, &pathlist);
    return;
  }

  CborValue set_value;
  cbor_value_map_find_value(request, "value", &set_value);
  if (cbor_value_get_type(&set_value) == CborInvalidType) {
    report_parsing_error(response, "invalid 'value' provided");
    return;
  }

  cbor_value_text_string_equals(&request_method_value, "set", &match);
  if (match) {
    console_request_set(response, &pathlist, &set_value);
    return;
  }
}

static void console_process_request_raw(int len) {
  CborParser parser;
  CborValue value;
  CborError err;

  uint8_t response[16384];
  CborEncoder encoder;

  cbor_encoder_init(&encoder, response, sizeof(response), 0);

  err = cbor_parser_init(rx_buffer, len, 0, &parser, &value);
  if (err) {
    return;
  }

  CborEncoder response_map;
  if (cbor_encoder_create_map(&encoder, &response_map, CborIndefiniteLength) !=
      CborNoError) {
    return;
  }

  console_process_request(&value, &response_map);

  if (cbor_encoder_close_container(&encoder, &response_map)) {
    return;
  }

  size_t write_size = cbor_encoder_get_buffer_size(&encoder, response);
  console_write_full(response, write_size);
}

void console_process() {
  static timeval_t last_desc_time = 0;
  uint8_t txbuffer[16384];

  size_t read_size;
  if ((read_size = console_try_read())) {
    /* Parse a request from the client */
    console_process_request_raw(read_size);
    console_shift_rx_buffer(read_size);
  }

  /* Process any outstanding event messages */
  struct logged_event ev = get_logged_event();
  while (ev.type != EVENT_NONE) {
    size_t write_size = console_event_message(txbuffer, sizeof(txbuffer), &ev);
    console_write_full(txbuffer, write_size);
    ev = get_logged_event();
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
  uint8_t buf[16384];
  CborEncoder top_encoder;
  CborParser top_parser;
  CborValue top_value;

  uint8_t pathbuf[512];
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
  ck_assert(cbor_value_get_int(&test_ctx.top_value, &result) == CborNoError);
  ck_assert_int_eq(result, field);
}
END_TEST

START_TEST(test_render_uint32_object_get_large) {
  struct console_request_context ctx = {
    .type = CONSOLE_GET,
    .response = &test_ctx.top_encoder,
  };

  /* Chosen to exceed signed 32 bit integer */
  uint32_t field = 3000000000;
  render_uint32_object(&ctx, "desc", &field);
  finish_writing();

  int result;
  ck_assert(cbor_value_get_int(&test_ctx.top_value, &result) == CborNoError);
  ck_assert_int_eq((uint32_t)result, field);
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
  tcase_add_test(console_tests, test_render_uint32_object_get_large);
  tcase_add_test(console_tests, test_render_float_object_get);

  /* Containers */

  /* Real access / integration tests */
  tcase_add_test(console_tests, test_smoke_console_request_structure);
  tcase_add_test(console_tests, test_smoke_console_request_get_full);
  return console_tests;
}

#endif
