#include <assert.h>
#include <math.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "calculations.h"
#include "config.h"
#include "console.h"
#include "decoder.h"
#include "platform.h"
#include "sensors.h"
#include "spsc.h"

#include "pb_encode.h"
#include "interface/types.pb.h"

TriggerUpdate trigger_update_queue_data[32];
struct spsc_queue trigger_update_queue = { .size = 32 };

EngineUpdate engine_update_queue_data[8];
struct spsc_queue engine_update_queue = { .size = 8 };

void console_publish_trigger_update(const struct decoder_event *ev) {
  static uint32_t seq = 0;

  int32_t idx = spsc_allocate(&trigger_update_queue);
  if (idx < 0) {
    return;
  }
  TriggerUpdate *msg = &trigger_update_queue_data[idx];
  msg->has_header = true;
  msg->header.seq = seq;
  msg->header.timestamp = ev->time;
  msg->trigger = ev->trigger;
  seq += 1;

  spsc_push(&trigger_update_queue);
}

static void populate_engine_position(DecoderUpdate *u) {
  u->has_header = true;
  u->header.timestamp = config.decoder.last_trigger_time;
  u->header.seq = 0;
  u->valid_before_timestamp = config.decoder.expiration;
  switch (config.decoder.state) {
    case DECODER_NOSYNC:
      u->state = DecoderState_NoSync;
      break;
    case DECODER_RPM:
      u->state = DecoderState_RpmOnly;
      break;
    case DECODER_SYNC:
      u->state = DecoderState_Sync;
      break;
  }
  switch (config.decoder.loss) {
    case DECODER_NO_LOSS:
      u->state_cause = DecoderLossReason_NoLoss;
      break;
    case DECODER_VARIATION:
      u->state_cause = DecoderLossReason_ToothVariation;
      break;
    case DECODER_TRIGGERCOUNT_HIGH:
      u->state_cause = DecoderLossReason_TriggerCountTooHigh;
      break;
    case DECODER_TRIGGERCOUNT_LOW:
      u->state_cause = DecoderLossReason_TriggerCountTooLow;
      break;
    case DECODER_EXPIRED:
      u->state_cause = DecoderLossReason_Expired;
      break;
    case DECODER_OVERFLOW:
      u->state_cause = DecoderLossReason_Overflowed;
      break;
  }
  u->last_angle = config.decoder.last_trigger_angle;
  u->instantaneous_rpm = config.decoder.tooth_rpm;
  u->average_rpm = config.decoder.rpm;
}

static SensorUpdate render_sensor_update(const struct sensor_input *si) {
  SensorUpdate update = { 0 };
  update.value = si->value;
  switch (si->fault) {
    case FAULT_NONE:
      update.fault = SensorFault_NoFault;
      break;
    case FAULT_RANGE:
      update.fault = SensorFault_RangeFault;
      break;
    case FAULT_CONN:
      update.fault = SensorFault_ConnectionFault;
      break;
  }
  return update;
}

static void populate_sensors_update(SensorsUpdate *u) {
  *u = (SensorsUpdate){ 0 };
  u->has_header = true,
  u->header.timestamp = config.sensors[SENSOR_MAP].time;
  u->header.seq = 0;
  u->has_ManifoldPressure = true;
  u->ManifoldPressure = render_sensor_update(&config.sensors[SENSOR_MAP]);
  u->has_IntakeTemperature = true;
  u->IntakeTemperature = render_sensor_update(&config.sensors[SENSOR_IAT]);
  u->has_CoolantTemperature = true;
  u->CoolantTemperature = render_sensor_update(&config.sensors[SENSOR_CLT]);
  u->has_BatteryVoltage = true;
  u->BatteryVoltage = render_sensor_update(&config.sensors[SENSOR_BRV]);
  u->has_ThrottlePosition = true;
  u->ThrottlePosition = render_sensor_update(&config.sensors[SENSOR_TPS]);
  u->has_AmbientPressure = true;
  u->AmbientPressure = render_sensor_update(&config.sensors[SENSOR_AAP]);
  u->has_FuelRailTemperature = true;
  u->FuelRailTemperature = render_sensor_update(&config.sensors[SENSOR_FRP]);
  u->has_ExhaustGasOxygen = true;
  u->ExhaustGasOxygen = render_sensor_update(&config.sensors[SENSOR_EGO]);
  u->has_FuelRailPressure = true;
  u->FuelRailPressure = render_sensor_update(&config.sensors[SENSOR_FRP]);
  u->has_EthanolContent = true;
  u->EthanolContent = render_sensor_update(&config.sensors[SENSOR_ETH]);
}

void console_publish_engine_update() {
  static uint32_t seq = 0;

  int32_t idx = spsc_allocate(&engine_update_queue);
  if (idx < 0) {
    return;
  }
  EngineUpdate *msg = &engine_update_queue_data[idx];
  msg->has_header = true;
  msg->header.seq = seq;
  msg->header.timestamp = current_time();

  msg->has_position = true;
  populate_engine_position(&msg->position);

  msg->has_sensors = true;
  populate_sensors_update(&msg->sensors);

  seq += 1;
  spsc_push(&engine_update_queue);
}


static uint32_t render_loss_reason() {
  return (uint32_t)config.decoder.loss;
};

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
    } else if (cbor_value_is_integer(&ctx->value)) {
      int val;
      cbor_value_get_int(&ctx->value, &val);
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

void render_bool_map_field(struct console_request_context *ctx,
                           const char *id,
                           const char *description,
                           bool *ptr) {
  struct console_request_context deeper;
  if (descend_map_field(ctx, &deeper, id)) {
    render_bool_object(&deeper, description, ptr);
  }
}

void render_bool_object(struct console_request_context *ctx,
                        const char *description,
                        bool *ptr) {
  switch (ctx->type) {
  case CONSOLE_SET:
    if (cbor_value_is_boolean(&ctx->value)) {
      cbor_value_get_boolean(&ctx->value, ptr);
    }
    /* Fall through */
  case CONSOLE_GET:
    cbor_encode_boolean(ctx->response, *ptr);
    break;
  case CONSOLE_DESCRIBE:
  case CONSOLE_STRUCTURE: {
    CborEncoder desc;
    cbor_encoder_create_map(ctx->response, &desc, 2);
    render_type_field(&desc, "bool");
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

  char *t = _t;
  switch (ctx->type) {
  case CONSOLE_SET:
    if (cbor_value_is_text_string(&ctx->value)) {
      size_t len = MAX_TABLE_TITLE_SIZE;
      cbor_value_copy_text_string(&ctx->value, t, &len, NULL);
      t[MAX_TABLE_TITLE_SIZE] = '\0';
    }
    /* Fall through */
  case CONSOLE_GET:
    cbor_encode_text_stringz(ctx->response, t);
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
      size_t len = MAX_TABLE_TITLE_SIZE;
      cbor_value_copy_text_string(&ctx->value, axis->name, &len, NULL);
      axis->name[MAX_TABLE_TITLE_SIZE] = '\0';
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
  for (int i = 0; (i < axis->num) && (i < MAX_AXIS_SIZE); i++) {
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
  struct table_2d *t;
  int row;
};

static void render_table_second_axis_data(struct console_request_context *ctx,
                                          void *_ntc) {
  struct nested_table_context *ntc = _ntc;
  struct table_2d *t = ntc->t;
  for (int c = 0; c < t->cols.num; c++) {
    struct console_request_context deeper;
    if (descend_array_field(ctx, &deeper, c)) {
      render_float_object(&deeper, "axis value", &t->data[ntc->row][c]);
    }
  }
}

static void render_table_2d_data_description(
  struct console_request_context *ctx,
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

static void render_table_1d_data_description(
  struct console_request_context *ctx,
  void *ptr) {
  (void)ptr;
  CborEncoder desc;
  cbor_encoder_create_map(ctx->response, &desc, 3);
  render_type_field(&desc, "[float]");
  render_description_field(&desc, "list of table values");
  cbor_encode_text_stringz(&desc, "len");
  cbor_encode_int(&desc, MAX_AXIS_SIZE);
  cbor_encoder_close_container(ctx->response, &desc);
}

static void render_table_1d_data(struct console_request_context *ctx,
                                 void *_t) {
  struct table_1d *t = _t;
  for (int i = 0; i < t->cols.num; i++) {
    struct console_request_context deeper;
    if (descend_array_field(ctx, &deeper, i)) {
      render_float_object(&deeper, "axis value", &t->data[i]);
    }
  }
}

static void render_table_2d_data(struct console_request_context *ctx,
                                 void *_t) {
  struct table_2d *t = _t;
  for (int r = 0; r < t->rows.num; r++) {
    struct console_request_context deeper;
    if (descend_array_field(ctx, &deeper, r)) {
      struct nested_table_context ntc = { .t = t, .row = r };
      render_array_object(&deeper, render_table_second_axis_data, &ntc);
    }
  }
}

static void render_table_2d_object(struct console_request_context *ctx,
                                   void *_t) {
  struct table_2d *t = _t;
  if (ctx->type == CONSOLE_STRUCTURE) {
    render_type_field(ctx->response, "table2d");
    return;
  }
  render_custom_map_field(ctx, "title", render_table_title, t->title);
  render_map_map_field(ctx, "horizontal-axis", render_table_axis, &t->cols);
  render_map_map_field(ctx, "vertical-axis", render_table_axis, &t->rows);

  if ((ctx->type != CONSOLE_DESCRIBE)) {
    render_array_map_field(ctx, "data", render_table_2d_data, t);
  } else {
    render_custom_map_field(
      ctx, "data", render_table_2d_data_description, NULL);
  }
}

static void render_table_1d_object(struct console_request_context *ctx,
                                   void *_t) {
  struct table_1d *t = _t;
  if (ctx->type == CONSOLE_STRUCTURE) {
    render_type_field(ctx->response, "table1d");
    return;
  }
  render_custom_map_field(ctx, "title", render_table_title, t->title);
  render_map_map_field(ctx, "horizontal-axis", render_table_axis, &t->cols);

  if ((ctx->type != CONSOLE_DESCRIBE)) {
    render_array_map_field(ctx, "data", render_table_1d_data, t);
  } else {
    render_custom_map_field(
      ctx, "data", render_table_1d_data_description, NULL);
  }
}

static void render_tables(struct console_request_context *ctx, void *ptr) {
  (void)ptr;
  render_map_map_field(ctx, "ve", render_table_2d_object, config.ve);
  render_map_map_field(
    ctx, "lambda", render_table_2d_object, config.commanded_lambda);
  render_map_map_field(ctx, "dwell", render_table_1d_object, config.dwell);
  render_map_map_field(ctx, "timing", render_table_2d_object, config.timing);
  render_map_map_field(ctx,
                       "injector_dead_time",
                       render_table_1d_object,
                       config.injector_deadtime_offset);
  render_map_map_field(ctx,
                       "injector_pw_correction",
                       render_table_1d_object,
                       config.injector_pw_correction);
  render_map_map_field(
    ctx, "temp-enrich", render_table_2d_object, config.engine_temp_enrich);
  render_map_map_field(
    ctx, "tipin-amount", render_table_2d_object, config.tipin_enrich_amount);
  render_map_map_field(
    ctx, "tipin-time", render_table_1d_object, config.tipin_enrich_duration);
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

  render_uint32_map_field(
    ctx, "rpm-limit-stop", "ignition and fuel cut rpm limit", &config.rpm_stop);
  render_uint32_map_field(
    ctx, "rpm-limit-start", "rpm limit lower hysteresis", &config.rpm_start);

  int type = config.decoder.type;
  render_enum_map_field(ctx,
                        "trigger-type",
                        "Primary trigger decoder method",
                        (struct console_enum_mapping[]){
                          { TRIGGER_EVEN_NOSYNC, "even" },
                          { TRIGGER_EVEN_CAMSYNC, "even+camsync" },
                          { TRIGGER_MISSING_NOSYNC, "missing" },
                          { TRIGGER_MISSING_CAMSYNC, "missing+camsync" },
                          { 0, NULL } },
                        &type);
  config.decoder.type = type;

  render_uint32_map_field(ctx,
                          "num-triggers",
                          "number of teeth on primary wheel",
                          &config.decoder.num_triggers);
  render_float_map_field(ctx,
                         "degrees-per-trigger",
                         "angle a single tooth represents",
                         &config.decoder.degrees_per_trigger);

  render_uint32_map_field(
    ctx,
    "min-triggers-rpm",
    "minimum teeth required to generate rpm for even tooth wheel",
    &config.decoder.required_triggers_rpm);

  render_uint32_map_field(ctx,
                          "rpm-window-size",
                          "rpm average window for even tooth wheel",
                          &config.decoder.rpm_window_size);
}

static void output_console_renderer(struct console_request_context *ctx,
                                    void *ptr) {
  if (ctx->type == CONSOLE_STRUCTURE) {
    render_type_field(ctx->response, "output");
    return;
  }

  struct output_event *ev = ptr;
  render_uint32_map_field(ctx, "pin", "pin", &ev->pin);
  render_bool_map_field(ctx, "inverted", "inverted", &ev->inverted);
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
  case SENSOR_ETH:
    return "eth";
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
                                     { SENSOR_PULSEWIDTH, "pulsewidth" },
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
                          { METHOD_THERM, "therm" },
                          { 0, NULL } },
                        &method);
  input->method = method;

  render_float_map_field(
    ctx, "range-min", "min for linear mapping", &input->range.min);
  render_float_map_field(
    ctx, "range-max", "max for linear mapping", &input->range.max);
  render_float_map_field(
    ctx, "raw-min", "min raw input for linear mapping", &input->raw_min);
  render_float_map_field(
    ctx, "raw-max", "max raw input for linear mapping", &input->raw_max);
  render_float_map_field(
    ctx, "fixed-value", "value to hold for const input", &input->value);
  render_float_map_field(ctx, "therm-a", "thermistor A", &input->therm.a);
  render_float_map_field(ctx, "therm-b", "thermistor B", &input->therm.b);
  render_float_map_field(ctx, "therm-c", "thermistor C", &input->therm.c);
  render_float_map_field(ctx,
                         "therm-bias",
                         "thermistor resistor bias value (ohms)",
                         &input->therm.bias);
  render_float_map_field(ctx,
                         "fault-min",
                         "Lower bound for raw sensor input",
                         &input->fault_config.min);
  render_float_map_field(ctx,
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
                          &input->window.offset);
}

static void render_knock_object(struct console_request_context *ctx,
                                void *ptr) {

  struct knock_input *input = ptr;

  render_float_map_field(
    ctx, "frequency", "knock filter center frequency (Hz)", &input->frequency);
  render_float_map_field(
    ctx, "threshold", "input level indicating knock", &input->threshold);

  if (ctx->type == CONSOLE_SET) {
    /* Reconfigure knock */
    knock_configure(&config.knock_inputs[0]);
    knock_configure(&config.knock_inputs[1]);
  }
}

static void render_sensors(struct console_request_context *ctx, void *ptr) {
  (void)ptr;
  for (sensor_input_type i = 0; i < NUM_SENSORS; i++) {
    render_map_map_field(
      ctx, sensor_name_from_type(i), render_sensor_object, &config.sensors[i]);
  }
  render_map_map_field(
    ctx, "knock1", render_knock_object, &config.knock_inputs[0]);
  render_map_map_field(
    ctx, "knock2", render_knock_object, &config.knock_inputs[1]);
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
                         "enable-threshold",
                         "MAP low threshold to enable boost control",
                         &config.boost_control.enable_threshold_kpa);
  render_float_map_field(
    ctx,
    "control-threshold",
    "MAP low threshold to keep valve open if TPS setting is met",
    &config.boost_control.control_threshold_kpa);
  render_float_map_field(ctx,
                         "control-threshold-tps",
                         "TPS threshold for valve-wide-open mode",
                         &config.boost_control.control_threshold_tps);
  render_float_map_field(ctx,
                         "overboost",
                         "High threshold for boost cut (kpa)",
                         &config.boost_control.overboost);
  render_map_map_field(
    ctx, "pwm", render_table_1d_object, config.boost_control.pwm_duty_vs_rpm);
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
      { NONE, "none" }, { FREQ, "freq" }, { TRIGGER, "trigger" }, { 0, NULL } },
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
  render_map_map_field(ctx, "table1d", render_table_1d_object, config.ve);
  render_map_map_field(ctx, "table2d", render_table_2d_object, config.ve);
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


void console_process() {
  uint8_t txbuffer[4096];

  Response response_msg = { 0 };
  pb_ostream_t stream = pb_ostream_from_buffer(txbuffer + 4, sizeof(txbuffer) - 4);

  if (!spsc_is_empty(&trigger_update_queue)) {
    int32_t idx = spsc_next(&trigger_update_queue);
    TriggerUpdate *msg = &trigger_update_queue_data[idx];

    response_msg.which_response = Response_trigger_update_tag;
    response_msg.response.trigger_update = msg;

    pb_encode(&stream, Response_fields, &response_msg);
    spsc_release(&trigger_update_queue);
  } else if (!spsc_is_empty(&engine_update_queue)) {
    int32_t idx = spsc_next(&engine_update_queue);
    EngineUpdate *msg = &engine_update_queue_data[idx];

    response_msg.which_response = Response_engine_update_tag;
    response_msg.response.engine_update = msg;

    pb_encode(&stream, Response_fields, &response_msg);
    spsc_release(&engine_update_queue);
  }

  uint32_t write_size = stream.bytes_written;
  if (write_size > 0) {
    memcpy(txbuffer, &write_size, 4);
    console_write_full(txbuffer, write_size + 4);
  }
#if 0
/* Otherwise a feed message */
  if (current_time > 4000000) {
    size_t write_size = console_feed_line(txbuffer+4, sizeof(txbuffer)-4);
    memcpy(txbuffer, &write_size, 4);
    console_write_full(txbuffer, write_size+4);
  }
#endif
}

#if 0

static void encode_cobs(int bufsize, uint8_t buffer[bufsize], uint32_t datasize) {

  int src_idx = 1;
  uint32_t current_value = buffer[0];
  int last_zero_idx = 0;

  while (src_idx <= datasize)  {
    if (current_value == 0) {
      buffer[last_zero_idx] = src_idx - last_zero_idx;
      last_zero_idx = src_idx;
    } else {
    }
    uint8_t next_value = buffer[src_idx];
    buffer[src_idx] = current_value;
    src_idx += 1;
    current_value = next_value;
  }
  buffer[last_zero_idx] = src_idx - last_zero_idx;
}
#endif

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


static bool memeql(size_t len, const uint8_t src[len], const uint8_t dst[len]) {
  for (int i = 0; i < len; i++) {
    if (src[i] != dst[i]) {
      return false;
    }
  }
  return true;
}

static void dump(size_t len, const uint8_t src[len]) {
  for (int i = 0; i < len; i++) {
    fprintf(stderr, "%d\n", src[i]);
  }

}

#if 0
START_TEST(test_encode_cobs) {

  const uint8_t case1[] = {1, 2, 3, 4};

  uint8_t buffer[256];
  memcpy(buffer, case1, sizeof(case1));
  encode_cobs(256, buffer, 4);

  dump(5, buffer);
  ck_assert(memeql(sizeof(case1) + 1, buffer,
      (uint8_t[]){5, 1, 2, 3, 4}));


  const uint8_t case2[] = {1, 2, 0, 3, 4};

  memcpy(buffer, case2, sizeof(case2));
  encode_cobs(256, buffer, 5);

  dump(6, buffer);
  ck_assert(memeql(sizeof(case2) + 1, buffer,
      (uint8_t[]){3, 1, 2, 3, 3, 4}));

} END_TEST
#endif

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
