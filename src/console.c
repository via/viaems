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

typedef enum {
  MESSAGE_PENDING,
  MESSAGE_COMPLETE,
  MESSAGE_TIMEOUT,
  MESSAGE_BAD_SIZE,
  MESSAGE_BAD_CRC,
} console_message_status;

struct console_message {
  console_message_status status;
  timeval_t time;
  uint32_t length;
  uint32_t crc;
};

/* Determine is a new message is ready to read. If so, returns true and
 * populates the provided pointer to a message */
bool console_message_read_ready(struct console_message *msg);

/* Read size bytes for the provided message into buffer. If able to fulfill the
 * requested size, returns true.  Otherwise returns false, and the reason is
 * available in the status field.  */
bool platform_message_read(struct console_message *msg, timeval_t timeout, uint8_t *buffer, size_t size);

/* Start a new message. Any message currently being written will be aborted, any
 * underlying frame will end a new one will start.  Length and CRC must be
 * provided up front */
bool platform_message_new(struct console_message *msg, uint32_t length, uint32_t crc);

bool platform_message_write(struct console_message *msg, timeval_t timeout, uint8_t *buffer, size_t size);


static int console_write_full(const uint8_t *buf, size_t len) {
  size_t remaining = len;
  const uint8_t *ptr = buf;
  while (remaining) {
    size_t written = platform_stream_write(remaining, ptr);
    if (written > 0) {
      remaining -= written;
      ptr += written;
    }
  }
  return 1;
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

TCase *setup_console_tests() {
  TCase *console_tests = tcase_create("console");
  /* Object primitives */

  return console_tests;
}

#endif
