#include <assert.h>
#include <math.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>

#include "calculations.h"
#include "config.h"
#include "console.h"
#include "decoder.h"
#include "platform.h"
#include "sensors.h"
#include "spsc.h"

#include "stream.h"
#include "crc.h"
#include "pb_encode.h"
#include "pb_decode.h"
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

/* Attempt to write n bytes from data using platform_stream_write.
 * arg is expected to be a pointer to a uin32_t representing a deadline, after
 * which the function will return false. 
 */
static bool platform_stream_write_timeout(size_t n, const uint8_t data[n], void *arg) {

  timeval_t continue_before_time = *((uint32_t *)arg);
  const uint8_t *ptr = data;
  size_t remaining = n;

  while (remaining > 0) {
    bool timed_out = time_before(continue_before_time, current_time());
    if (timed_out) {
      return false;
    }
    size_t written = platform_stream_write(remaining, ptr);
    ptr += written;
    remaining -= written;
  }

  return true;
}


/* Attempt to read n bytes into data using plattform_stream_read.
 * arg is expected to be a pointer to a uin32_t representing a deadline, after
 * which the function will return false. 
 */
static bool platform_stream_read_timeout(size_t n, uint8_t data[n], void *arg) {

  timeval_t continue_before_time = *((uint32_t*)arg);
  uint8_t *ptr = data;
  size_t remaining = n;

  while (remaining > 0) {
    bool timed_out = time_before(continue_before_time, current_time());
    if (timed_out) {
      return false;
    }
    size_t amt_read = platform_stream_read(remaining, ptr);
    ptr += amt_read;
    remaining -= amt_read;
  }

  return true;
}

/* write callback for pb_ostream_t implemented using stream_message_write.
 * The stream state must be a pointer to a stream_message_writer.
 */
static bool pb_ostream_write_callback(
    pb_ostream_t *stream, 
    const uint8_t *buf, 
    size_t count) {
  struct stream_message_writer *msg = (struct stream_message_writer *)stream->state;
  return stream_message_write(msg, count, buf);
}

/* write callback for pb_ostream_t that is used to calculate a length and CRC.
 * It does not write the payload anywhere, and the stream state must be a
 * pointer to a uint32_t for the current CRC
 */
bool pb_ostream_crc_callback(pb_ostream_t *stream,
    const uint8_t *buf, size_t count) {
  uint32_t *crc = (uint32_t *)stream->state;
  for (unsigned int i = 0; i < count; i++) {
    crc32_add_byte(crc, buf[i]);
  }
  return true;
}

/* Calculate a length and crc for a target message.
 * This necessarily encodes the full target message to get these values, but
 * does not write the payload anywhere.
 */
static void calculate_msg_length_and_crc(TargetMessage *r, uint32_t *length, uint32_t *crc) {

  *crc = CRC32_INIT;
  pb_ostream_t ostream = {
    .callback = pb_ostream_crc_callback,
    .max_size = SIZE_MAX,
    .state = crc,
  };
  pb_encode(&ostream, TargetMessage_fields, r);
  crc32_finish(crc);
  *length = ostream.bytes_written;
}

/* State structure for the pb_istream_t read callback for storing the accumulated
 * CRC of the read. 
 */
struct pb_istream_state {
  struct stream_message_reader *reader;
  uint32_t actual_crc;
};

/* read callback for pb_istream_t implemented using stream_message_read.
 * The stream state must be a pointer to a pb_istream_state, which itself must
 * contain a pointer to the stream_message_reader.
 */
static bool pb_istream_read_callback(
    pb_istream_t *stream, 
    uint8_t *buf, 
    size_t count) {
  struct pb_istream_state *state = (struct pb_istream_state *)stream->state;
  if (!stream_message_read(state->reader, count, buf)) {
    return false;
  }
  for (unsigned int i = 0; i < count; i++) {
    crc32_add_byte(&state->actual_crc, buf[i]);
  }
  return true;
}


/* Attempt to read a full request message via platform_stream_read. 
 * This function blocks, so in order to not halt target messages being sent out
 * in the event of a slow or failed incoming request, we use an overall timeout
 * of 100 mS.  If the read times out, the decoding fails, or the CRC check
 * fails, false is returned.
 */
static bool read_request(Request *req) {
  uint32_t timeout = current_time() + time_from_us(100000);
  struct stream_message_reader reader;
  if (!stream_message_reader_new(&reader, platform_stream_read_timeout, &timeout)) {
    return false;
  }

  struct pb_istream_state state = { .reader = &reader, .actual_crc = CRC32_INIT };
  pb_istream_t istream = {
    .state = &state,
    .bytes_left = reader.length,
    .callback = pb_istream_read_callback,
  };

  if (!pb_decode(&istream, Request_fields, req)) {
    return false;
  }

  crc32_finish(&state.actual_crc);
  if (state.actual_crc != reader.crc) {
    return false;
  }
  return true;
}

/* Act on a request message, and optionally provide a response.
 * Returns true if a response is set
 */
static bool process_request(Request *req, Response *resp) {
  *resp = (Response)Response_init_default;
  switch (req->which_type) {
    case Request_ping_tag:
      resp->has_header = true;
      resp->header.seq = req->seq;
      resp->header.timestamp = current_time();
      resp->which_type = Response_ping_tag;
      return true;
  }
  return false;
}

void console_process() {
  static Request request_msg;
  static TargetMessage target_message;

  bool has_message_to_send = false;

  /* Try to receive a byte */
  uint8_t rx_byte;
  size_t resp = platform_stream_read(1, &rx_byte);
  if ((resp == 1) && (rx_byte == 0)) {
    /* If it was a COBS delimiter, now try to read an entire request message */
    if (read_request(&request_msg)) {
      if (process_request(&request_msg, &target_message.msg.request_response)) {
        target_message.which_msg = TargetMessage_request_response_tag;
        has_message_to_send = true;
      }
    }
  }

  if (!has_message_to_send) {
    if (!spsc_is_empty(&trigger_update_queue)) {
      has_message_to_send = true;
      int32_t idx = spsc_next(&trigger_update_queue);
      TriggerUpdate *msg = &trigger_update_queue_data[idx];

      target_message.which_msg = TargetMessage_trigger_update_tag;
      target_message.msg.trigger_update = *msg;

      spsc_release(&trigger_update_queue);
    } else if (!spsc_is_empty(&engine_update_queue)) {
      has_message_to_send = true;
      int32_t idx = spsc_next(&engine_update_queue);
      EngineUpdate *msg = &engine_update_queue_data[idx];

      target_message.which_msg = TargetMessage_engine_update_tag;
      target_message.msg.engine_update = *msg;

      spsc_release(&engine_update_queue);
    }
  }

  if (has_message_to_send) {
    uint32_t length;
    uint32_t crc;
    calculate_msg_length_and_crc(&target_message, &length, &crc);

    struct stream_message_writer msg;
    pb_ostream_t ostream = {
      .state = &msg,
      .callback = pb_ostream_write_callback,
      .max_size = SIZE_MAX,
    };

    /* Send target message with a 100 ms timeout */
    uint32_t timeout = current_time() + time_from_us(100000);
    stream_message_writer_new(&msg, platform_stream_write_timeout, &timeout, length, crc);
    pb_encode(&ostream, TargetMessage_fields, &target_message);
  }

}

#ifdef UNITTEST
#include <check.h>
#include <stdarg.h>

TCase *setup_console_tests() {
  TCase *console_tests = tcase_create("console");
  /* Object primitives */

  return console_tests;
}

#endif
