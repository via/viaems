#include <assert.h>
#include <math.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "calculations.h"
#include "table.h"
#include "config.h"
#include "console.h"
#include "decoder.h"
#include "platform.h"
#include "scheduler.h"
#include "sensors.h"
#include "spsc.h"

#include "crc.h"
#include "types.pb.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include "stream.h"

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
  u->has_header = true, u->header.timestamp = config.sensors[SENSOR_MAP].time;
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

static void load_table_axis_from_config(const struct table_axis *config, TableAxis *axis) {
  strcpy(axis->name, config->name);
  axis->values_count = config->num;
  memcpy(axis->values, config->values, sizeof(axis->values));
}

static void load_table_1d_from_config(const struct table_1d *config, Table1d *table) {
  strcpy(table->name, config->title);
  table->has_cols = true;
  load_table_axis_from_config(&config->cols, &table->cols);
  table->has_data = true;
  table->data.values_count = config->cols.num;
  memcpy(table->data.values, config->data, sizeof(config->data));
}

static void load_table_2d_from_config(const struct table_2d *config, Table2d *table) {
  strcpy(table->name, config->title);
  table->has_cols = true;
  load_table_axis_from_config(&config->cols, &table->cols);
  table->has_rows = true;
  load_table_axis_from_config(&config->rows, &table->rows);
  table->data_count = config->rows.num;
  for (int row = 0; row < config->rows.num; row++) {
    table->data[row].values_count = config->cols.num;
    memcpy(table->data[row].values, config->data[row], sizeof(config->data[row]));
  }
}

static void save_outputs_to_config(size_t count, const Output outputs[count]) {
  for (size_t i = 0; (i < MAX_EVENTS) && (i < count); i++) {
    config.events[i].angle = outputs[i].angle;
    config.events[i].inverted = outputs[i].inverted;
    config.events[i].pin = outputs[i].pin;
    switch (outputs[i].type) {
      case Output_OutputType_OutputDisabled:
        config.events[i].type = DISABLED_EVENT;
        break;
      case Output_OutputType_Fuel:
        config.events[i].type = FUEL_EVENT;
        break;
      case Output_OutputType_Ignition:
        config.events[i].type = IGNITION_EVENT;
        break;
    }
  }
}

static void load_outputs_from_config(Output outputs[MAX_EVENTS]) {
  for (size_t i = 0; i < MAX_EVENTS; i++) {
    outputs[i].angle = config.events[i].angle;
    outputs[i].inverted = config.events[i].inverted;
    outputs[i].pin = config.events[i].pin;
    switch (config.events[i].type) {
      case DISABLED_EVENT:
        outputs[i].type = Output_OutputType_OutputDisabled;
        break;
      case FUEL_EVENT:
        outputs[i].type = Output_OutputType_Fuel;
        break;
      case IGNITION_EVENT:
        outputs[i].type = Output_OutputType_Ignition;
        break;
    }
  }
}

static void load_freq_inputs_from_config(FreqInput inputs[4]) {
  for (size_t i = 0; i < 4; i++) {
    switch (config.freq_inputs[i].edge) {
      case RISING_EDGE:
        inputs[i].edge = InputEdge_Rising;
        break;
      case FALLING_EDGE:
        inputs[i].edge = InputEdge_Falling;
        break;
      case BOTH_EDGES:
        inputs[i].edge = InputEdge_Both;
        break;
    }
    switch (config.freq_inputs[i].type) {
      case NONE:
        inputs[i].type = InputType_InputDisabled;
        break;
      case FREQ:
        inputs[i].type = InputType_Freq;
        break;
      case TRIGGER:
        inputs[i].type = InputType_Trigger;
        break;
    }
  }
}

static void load_sensor_from_config(const struct sensor_input *c, Sensor *sensor) {
  *sensor = (Sensor)Sensor_init_default;
  sensor->pin = c->pin;
  sensor->lag = c->lag;
  switch (c->source) {
    default:
    case SENSOR_NONE:
      sensor->source = SensorSource_None;
      break;
    case SENSOR_ADC:
      sensor->source = SensorSource_Adc;
      break;
    case SENSOR_FREQ:
      sensor->source = SensorSource_Frequency;
      break;
    case SENSOR_PULSEWIDTH:
      sensor->source = SensorSource_Pulsewidth;
      break;
    case SENSOR_CONST:
      sensor->source = SensorSource_Const;
      break;
  }
  switch (c->method) {
    default:
    case METHOD_LINEAR:
      sensor->method = SensorMethod_Linear;
      break;
    case METHOD_LINEAR_WINDOWED:
      sensor->method = SensorMethod_LinearWindowed;
      break;
    case METHOD_THERM:
      sensor->method = SensorMethod_Thermistor;
      break;
  }

  if (sensor->method == SensorMethod_Thermistor) {
    sensor->has_thermistor_config = true;
    sensor->thermistor_config = (Sensor_ThermistorConfig){
      .A = c->therm.a,
      .B = c->therm.b,
      .C = c->therm.c,
      .bias = c->therm.bias
    };
  }

  if (sensor->method == SensorMethod_LinearWindowed) {
    sensor->has_window_config = true;
    sensor->window_config = (Sensor_WindowConfig){
      .capture_width = c->window.capture_width,
      .total_width = c->window.total_width,
      .offset = c->window.offset,
    };
  }

  if (sensor->source == SensorSource_Const) {
    sensor->has_const_config = true;
    sensor->const_config = (Sensor_ConstConfig){
      .fixed_value = c->value,
    };
  }
        
  if ((sensor->method == SensorMethod_Linear) ||
      (sensor->method == SensorMethod_LinearWindowed)) {
    sensor->has_linear_config = true;
    sensor->linear_config = (Sensor_LinearConfig){
      .output_min = c->range.min,
      .output_max = c->range.max,
      .input_min = c->raw_min,
      .input_max = c->raw_max
    };
  }

  if ((c->fault_config.min != 0.0f) && (c->fault_config.max != 0.0f)) {
    sensor->has_fault_config = true;
    sensor->fault_config = (Sensor_FaultConfig){
      .min = c->fault_config.min,
      .max = c->fault_config.max,
      .value = c->fault_config.fault_value
    };
  }
}

static void load_fueling_from_config(Fueling *fueling) {
  *fueling = (Fueling)Fueling_init_default;
  fueling->fuel_pump_pin = config.fueling.fuel_pump_pin;
  fueling->cylinder_cc = config.fueling.cylinder_cc;
  fueling->fuel_density = config.fueling.density_of_fuel;
  fueling->fuel_stoich_ratio = config.fueling.fuel_stoich_ratio;
  fueling->injector_cc = config.fueling.injector_cc_per_minute;

  fueling->has_crank_enrich = true;
  fueling->crank_enrich.cranking_rpm = config.fueling.crank_enrich_config.crank_rpm;
  fueling->crank_enrich.cranking_temp = config.fueling.crank_enrich_config.cutoff_temperature;
  fueling->crank_enrich.enrich_amt = config.fueling.crank_enrich_config.enrich_amt;

  fueling->has_PulseWidthCompensation = true;
  load_table_1d_from_config(config.injector_pw_correction, &fueling->PulseWidthCompensation);

  fueling->has_InjectorDeadTime = true;
  load_table_1d_from_config(config.injector_deadtime_offset, &fueling->InjectorDeadTime);

  fueling->has_EngineTempEnrichment = true;
  load_table_2d_from_config(config.engine_temp_enrich, &fueling->EngineTempEnrichment);

  fueling->has_CrankingEnrichment = false;  // TODO
                                            
  fueling->has_commanded_lambda = true;
  load_table_2d_from_config(config.commanded_lambda, &fueling->commanded_lambda);
  fueling->has_ve = true;
  load_table_2d_from_config(config.ve, &fueling->ve);

  fueling->has_tipin_enrich_amount = true;
  load_table_2d_from_config(config.tipin_enrich_amount, &fueling->tipin_enrich_amount);

  fueling->has_tipin_enrich_duration = true;
  load_table_1d_from_config(config.tipin_enrich_duration, &fueling->tipin_enrich_duration);

}

/* Reconfigure the ECU with a new configuration message */
static bool set_configuration(const Configuration *new_config) {
  save_outputs_to_config(new_config->outputs_count, new_config->outputs);

  return false;
}

/* Populate a configuration message with the ECU config */
static void get_configuration(Configuration *dest) {
  dest->outputs_count = MAX_EVENTS;
  load_outputs_from_config(dest->outputs);

  dest->freq_count = 4;
  load_freq_inputs_from_config(dest->freq);

  dest->has_sensors = true;
  dest->sensors.has_AbsoluteAirPressure = true;
  load_sensor_from_config(&config.sensors[SENSOR_AAP], &dest->sensors.AbsoluteAirPressure);
  dest->sensors.has_BatteryReferenceVoltage = true;
  load_sensor_from_config(&config.sensors[SENSOR_BRV], &dest->sensors.BatteryReferenceVoltage);
  dest->sensors.has_CoolantTemperature = true;
  load_sensor_from_config(&config.sensors[SENSOR_CLT], &dest->sensors.CoolantTemperature);
  dest->sensors.has_ExhaustGasOxygen = true;
  load_sensor_from_config(&config.sensors[SENSOR_EGO], &dest->sensors.ExhaustGasOxygen);
  dest->sensors.has_FuelRailTemperature = true;
  load_sensor_from_config(&config.sensors[SENSOR_FRT], &dest->sensors.FuelRailTemperature);
  dest->sensors.has_IntakeAirTemperature = true;
  load_sensor_from_config(&config.sensors[SENSOR_IAT], &dest->sensors.IntakeAirTemperature);
  dest->sensors.has_ManifoldPressure = true;
  load_sensor_from_config(&config.sensors[SENSOR_MAP], &dest->sensors.ManifoldPressure);
  dest->sensors.has_ThrottlePosition = true;
  load_sensor_from_config(&config.sensors[SENSOR_TPS], &dest->sensors.ThrottlePosition);
  dest->sensors.has_ThrottlePosition = true;
  load_sensor_from_config(&config.sensors[SENSOR_TPS], &dest->sensors.ThrottlePosition);

  dest->sensors.has_knock1 = true;
  dest->sensors.knock1.enabled = (config.knock_inputs[0].frequency != 0.0);
  dest->sensors.knock1.frequency = config.knock_inputs[0].frequency;
  dest->sensors.knock1.threshold = config.knock_inputs[0].threshold;

  dest->sensors.has_knock2 = true;
  dest->sensors.knock2.enabled = (config.knock_inputs[1].frequency != 0.0);
  dest->sensors.knock2.frequency = config.knock_inputs[1].frequency;
  dest->sensors.knock2.threshold = config.knock_inputs[1].threshold;

  dest->has_fueling = true;
  load_fueling_from_config(&dest->fueling);
}

/* Attempt to write n bytes from data using platform_stream_write.
 * arg is expected to be a pointer to a uin32_t representing a deadline, after
 * which the function will return false.
 */
static bool platform_stream_write_timeout(size_t n,
                                          const uint8_t data[n],
                                          void *arg) {

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

  timeval_t continue_before_time = *((uint32_t *)arg);
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
static bool pb_ostream_write_callback(pb_ostream_t *stream,
                                      const uint8_t *buf,
                                      size_t count) {
  struct stream_message_writer *msg =
    (struct stream_message_writer *)stream->state;
  return stream_message_write(msg, count, buf);
}

/* write callback for pb_ostream_t that is used to calculate a length and CRC.
 * It does not write the payload anywhere, and the stream state must be a
 * pointer to a uint32_t for the current CRC
 */
bool pb_ostream_crc_callback(pb_ostream_t *stream,
                             const uint8_t *buf,
                             size_t count) {
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
static void calculate_msg_length_and_crc(TargetMessage *r,
                                         uint32_t *length,
                                         uint32_t *crc) {

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

/* State structure for the pb_istream_t read callback for storing the
 * accumulated CRC of the read.
 */
struct pb_istream_state {
  struct stream_message_reader *reader;
  uint32_t actual_crc;
};

/* read callback for pb_istream_t implemented using stream_message_read.
 * The stream state must be a pointer to a pb_istream_state, which itself must
 * contain a pointer to the stream_message_reader.
 */
static bool pb_istream_read_callback(pb_istream_t *stream,
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

static Configuration configuration;
static Request request_msg;
static TargetMessage target_message;

/* Attempt to read a full request message via platform_stream_read.
 * This function blocks, so in order to not halt target messages being sent out
 * in the event of a slow or failed incoming request, we use an overall timeout
 * of 100 mS.  If the read times out, the decoding fails, or the CRC check
 * fails, false is returned.
 */
static bool read_request(Request *req) {
  uint32_t timeout = current_time() + time_from_us(100000);
  struct stream_message_reader reader;
  if (!stream_message_reader_new(
        &reader, platform_stream_read_timeout, &timeout)) {
    return false;
  }

  struct pb_istream_state state = { .reader = &reader,
                                    .actual_crc = CRC32_INIT };
  pb_istream_t istream = {
    .state = &state,
    .bytes_left = reader.length,
    .callback = pb_istream_read_callback,
  };

  req->type.set_configuration = &configuration;
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
  resp->has_header = true;
  resp->header.seq = req->seq;
  resp->header.timestamp = current_time();
  switch (req->which_type) {
  case Request_ping_tag:
    resp->which_type = Response_ping_tag;
    return true;
  case Request_get_configuration_tag:
    resp->which_type = Response_configuration_tag;
    resp->type.configuration = &configuration;
    get_configuration(resp->type.configuration);
    return true;
  }
  return false;
}

void console_process() {

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
    stream_message_writer_new(
      &msg, platform_stream_write_timeout, &timeout, length, crc);
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
