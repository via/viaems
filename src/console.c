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
#include "sim.h"
#include "spsc.h"
#include "util.h"
#include "viaems.h"
#include "crc.h"

#include "console.pb.h"

#define EVENT_LOG_SIZE 32

static struct viaems_console_Message console_message;

static bool pb_write(const uint8_t *data, unsigned len, void *user) {
  struct console_tx_message *txmsg = (struct console_tx_message *)user;
  return platform_message_writer_write(txmsg, data, len);
}

static bool pb_read(uint8_t *data, unsigned len, void *user) {
  struct console_rx_message *rxmsg = (struct console_rx_message *)user;
  return platform_message_reader_read(rxmsg, data, len);
}

static struct {
  bool enabled;
  _Atomic uint32_t read;
  _Atomic uint32_t write;
  _Atomic uint32_t seq;
  struct logged_event events[EVENT_LOG_SIZE];
} event_log = { .enabled = true };

static uint32_t event_log_next_idx(uint32_t idx) {
  idx += 1;
  if (idx >= EVENT_LOG_SIZE) {
    return 0;
  } else {
    return idx;
  }
}

static struct logged_event get_logged_event() {
  uint32_t read_pos = event_log.read;
  uint32_t write_pos = event_log.write;

  if (!event_log.enabled) {
    return (struct logged_event){ .type = EVENT_NONE };
  }

  if (read_pos == write_pos) {
    return (struct logged_event){ .type = EVENT_NONE };
  }

  struct logged_event ret = event_log.events[read_pos];
  event_log.read = event_log_next_idx(read_pos);
  return ret;
}

void console_record_event(struct logged_event ev) {
  if (!event_log.enabled) {
    return;
  }

  uint32_t seq = atomic_fetch_add(&event_log.seq, 1);
  uint32_t read_pos = event_log.read;
  uint32_t write_pos = event_log.write;
  uint32_t next_write_pos = event_log_next_idx(write_pos);

  if (next_write_pos != read_pos) {
    event_log.events[write_pos] = ev;
    event_log.events[write_pos].seq = seq;
    event_log.write = next_write_pos;
  }
}

static void console_event_message(struct logged_event *ev) {
  struct viaems_console_Event pb_ev = { 
    .has_header = true,
    .header = {
      .timestamp = ev->time,
      .seq = ev->seq,
    },
  };
  switch(ev->type) {
    case EVENT_NONE:
      return;
    case EVENT_GPIO:
      pb_ev.which_type = PB_TAG_viaems_console_Event_gpio_pins;
      pb_ev.gpio_pins = ev->value;
      break;
    case EVENT_OUTPUT:
      pb_ev.which_type = PB_TAG_viaems_console_Event_output_pins;
      pb_ev.output_pins = ev->value;
      break;
    case EVENT_TRIGGER:
      pb_ev.which_type = PB_TAG_viaems_console_Event_trigger;
      pb_ev.trigger = ev->value;
      break;
  };
  console_message.which_msg = PB_TAG_viaems_console_Message_event,
  console_message.event = pb_ev;

  struct console_tx_message writer;
  unsigned length = pb_sizeof_viaems_console_Message(&console_message);
  if (!platform_message_writer_new(&writer, length)) {
    return;
  }

  if (!pb_encode_viaems_console_Message(&console_message, pb_write, &writer)) {
    platform_message_writer_abort(&writer);
  }
}


struct spsc_queue engine_update_queue = {
  .size = 4,
};
struct viaems_console_EngineUpdate engine_update_msgs[4];
static uint32_t engine_update_seq = 0;

static viaems_console_SensorFault convert_sensor_fault(const sensor_fault fault) {
  switch (fault) {
    case FAULT_NONE: return viaems_console_SensorFault_SENSOR_NO_FAULT;
    case FAULT_RANGE: return viaems_console_SensorFault_SENSOR_RANGE_FAULT;
    case FAULT_CONN: return viaems_console_SensorFault_SENSOR_CONNECTION_FAULT;
  }
  return viaems_console_SensorFault_SENSOR_NO_FAULT;
}

void record_engine_update(const struct engine_update *eng_update,
                          const struct calculated_values *calcs) {

  uint32_t seq = engine_update_seq;
  engine_update_seq++;

  int idx = spsc_allocate(&engine_update_queue);
  if (idx < 0) {
    return;
  }

  struct viaems_console_EngineUpdate *update = &engine_update_msgs[idx];

  *update = (struct viaems_console_EngineUpdate){ 0 };
  update->has_header = true;
  update->header.timestamp = eng_update->current_time;
  update->header.seq = seq;

  update->has_position = true;
  update->position.time = eng_update->position.time;
  update->position.average_rpm = eng_update->position.rpm;
  update->position.instantaneous_rpm = eng_update->position.tooth_rpm;
  update->position.has_position = eng_update->position.has_position;
  update->position.synced = engine_position_is_synced(&eng_update->position, eng_update->current_time);

  update->position.last_angle = eng_update->position.last_trigger_angle;

  if (calcs != NULL) {
    update->has_calculations = true;
    update->calculations.ve = calcs->ve;
    update->calculations.lambda = calcs->lambda;
    update->calculations.fuel_us = calcs->fueling_us;
    update->calculations.engine_temp_enrichment = calcs->ete;
    update->calculations.injector_dead_time = calcs->idt;
    update->calculations.pulse_width_correction = calcs->pwc;
    update->calculations.tipin_percent = calcs->tipin;
    update->calculations.airmass_per_cycle = calcs->airmass_per_cycle;

    update->calculations.advance = calcs->timing_advance;
    update->calculations.dwell_us = calcs->dwell_us;
    update->calculations.rpm_limit_cut = calcs->rpm_limit_cut;
    update->calculations.boost_cut = calcs->boost_cut;
    update->calculations.fuel_overduty_cut = calcs->fuel_overduty_cut;
    update->calculations.dwell_overduty_cut = calcs->dwell_overduty_cut;
  }

  update->has_sensors = true;
  update->sensors.map = eng_update->sensors.MAP.value;
  update->sensors.iat = eng_update->sensors.IAT.value;
  update->sensors.clt = eng_update->sensors.CLT.value;
  update->sensors.brv = eng_update->sensors.BRV.value;
  update->sensors.tps = eng_update->sensors.TPS.value;
  update->sensors.aap = eng_update->sensors.AAP.value;
  update->sensors.frt = eng_update->sensors.FRT.value;
  update->sensors.ego = eng_update->sensors.EGO.value;
  update->sensors.frp = eng_update->sensors.FRP.value;
  update->sensors.eth = eng_update->sensors.ETH.value;
  update->sensors.knock1 = eng_update->sensors.KNK1;
  update->sensors.knock2 = eng_update->sensors.KNK2;

  update->sensors.map_fault = convert_sensor_fault(eng_update->sensors.MAP.fault);
  update->sensors.iat_fault = convert_sensor_fault(eng_update->sensors.IAT.fault);
  update->sensors.clt_fault = convert_sensor_fault(eng_update->sensors.CLT.fault);
  update->sensors.brv_fault = convert_sensor_fault(eng_update->sensors.BRV.fault);
  update->sensors.tps_fault = convert_sensor_fault(eng_update->sensors.TPS.fault);
  update->sensors.aap_fault = convert_sensor_fault(eng_update->sensors.AAP.fault);
  update->sensors.frt_fault = convert_sensor_fault(eng_update->sensors.FRT.fault);
  update->sensors.ego_fault = convert_sensor_fault(eng_update->sensors.EGO.fault);
  update->sensors.frp_fault = convert_sensor_fault(eng_update->sensors.FRP.fault);
  update->sensors.eth_fault = convert_sensor_fault(eng_update->sensors.ETH.fault);

  update->sensors.tps_rate = eng_update->sensors.TPS.derivative;
  spsc_push(&engine_update_queue);
}

static void console_feed_line(const struct viaems_console_EngineUpdate *update) {
  console_message.which_msg = PB_TAG_viaems_console_Message_engine_update;
  console_message.engine_update = *update;

  struct console_tx_message writer;
  unsigned length = pb_sizeof_viaems_console_Message(&console_message);
  if (!platform_message_writer_new(&writer, length)) {
    return;
  }

  if (!pb_encode_viaems_console_Message(&console_message, pb_write, &writer)) {
    platform_message_writer_abort(&writer);
  }

}

#ifndef GIT_DESCRIBE
#define GIT_DESCRIBE "unknown"
#endif
static const char *fw_version = GIT_DESCRIBE;
static const char *fw_platform = FW_PLATFORM;

static struct viaems_console_Response_FirmwareInfo console_fwinfo(void) {
  struct viaems_console_Response_FirmwareInfo fwinfo;
  fwinfo.version.len = strlen(fw_version);
  memcpy(fwinfo.version.str, fw_version, fwinfo.version.len);

  fwinfo.platform.len = strlen(fw_platform);
  memcpy(fwinfo.platform.str, fw_platform, fwinfo.platform.len);

  fwinfo.proto.len = 0;
  return fwinfo;
}


static void console_process_request(struct console_rx_message *rxmsg, struct config *config) {
  if (!pb_decode_viaems_console_Message(&console_message, pb_read, rxmsg)) {
//    platform_message_reader_abort(rxmsg);
    return;
  }
  if (console_message.which_msg != PB_TAG_viaems_console_Message_request) {
    return;
  }

  switch (console_message.request.which_request) {
    case PB_TAG_viaems_console_Request_ping:
      console_message.which_msg = PB_TAG_viaems_console_Message_response;
      console_message.response.which_response = PB_TAG_viaems_console_Response_pong;
      break;
    case PB_TAG_viaems_console_Request_fwinfo:
      console_message.which_msg = PB_TAG_viaems_console_Message_response;
      console_message.response.which_response = PB_TAG_viaems_console_Response_fwinfo;
      console_message.response.fwinfo = console_fwinfo();
      break;
    case PB_TAG_viaems_console_Request_getconfig:
      console_message.which_msg = PB_TAG_viaems_console_Message_response;
      console_message.response.which_response = PB_TAG_viaems_console_Response_getconfig;
      console_message.response.getconfig.has_config = true;
      config_store_to_console_pbtype(config, &console_message.response.getconfig.config);
      break;
    case PB_TAG_viaems_console_Request_setconfig: {
     bool success = false;
      if (console_message.request.setconfig.has_config) {
        config_load_result r = config_load_from_console_pbtype(config, &console_message.request.setconfig.config);
        success = (r == CONFIG_SAVED);
      }
      console_message.which_msg = PB_TAG_viaems_console_Message_response;
      console_message.response.which_response = PB_TAG_viaems_console_Response_setconfig;
      console_message.response.setconfig.has_config = true;
      console_message.response.setconfig.success = success;
      config_store_to_console_pbtype(config, &console_message.response.setconfig.config);
      break;
                                                  }
    default:
      break;
  }

  struct console_tx_message writer;
  unsigned length = pb_sizeof_viaems_console_Message(&console_message);
  if (!platform_message_writer_new(&writer, length)) {
    return;
  }

  if (!pb_encode_viaems_console_Message(&console_message, pb_write, &writer)) {
    platform_message_writer_abort(&writer);
  }

}

void console_process(struct config *config) {

  struct console_rx_message rxmsg;
  if (platform_message_reader_new(&rxmsg)) {
      console_process_request(&rxmsg, config);
  }

  /* Process any outstanding event messages */
  struct logged_event ev = get_logged_event();
  if (ev.type != EVENT_NONE) {
    do {
      console_event_message(&ev);
      ev = get_logged_event();
    } while (ev.type != EVENT_NONE);
  }

  int idx = spsc_next(&engine_update_queue);
  if (idx >= 0) {
    struct viaems_console_EngineUpdate *update = &engine_update_msgs[idx];
    console_feed_line(update);
    spsc_release(&engine_update_queue);
  }
}

#ifdef UNITTEST
#include <check.h>
#include <stdarg.h>

START_TEST(test_console_event_log) {
  event_log.enabled = true;

  ck_assert(event_log.read == 0);
  ck_assert(event_log.write == 0);

  /* Empty queue should return none */
  ck_assert(get_logged_event().type == EVENT_NONE);
  ck_assert(get_logged_event().type == EVENT_NONE);

  uint32_t seq = 0;
  /* Push and pop single event, confirm empty after */
  console_record_event((struct logged_event){
    .type = EVENT_TRIGGER,
    .time = 1,
  });
  struct logged_event ret = get_logged_event();
  ck_assert(ret.type == EVENT_TRIGGER);
  ck_assert(ret.time == 1);
  ck_assert(ret.seq == seq);
  ck_assert(get_logged_event().type == EVENT_NONE);
  seq += 1;

  /* Push 16 and pop 16 */
  for (int i = 0; i < 16; i++) {
    console_record_event((struct logged_event){
      .type = EVENT_TRIGGER,
      .time = i,
    });
  }
  for (int i = 0; i < 16; i++) {
    ret = get_logged_event();
    ck_assert(ret.type == EVENT_TRIGGER);
    ck_assert(ret.time == (uint32_t)i);
    ck_assert(ret.seq == (uint32_t)i + seq);
  }
  seq += 16;

  ck_assert(get_logged_event().type == EVENT_NONE);

  /* Push 64 to trigger overflow behavior */
  for (int i = 0; i < 64; i++) {
    console_record_event((struct logged_event){
      .type = EVENT_TRIGGER,
      .time = i,
    });
  }
  for (int i = 0; i < EVENT_LOG_SIZE - 1; i++) {
    ret = get_logged_event();
    ck_assert(ret.type == EVENT_TRIGGER);
    ck_assert(ret.time == (uint32_t)i);
    ck_assert(ret.seq == (uint32_t)i + seq);
  }
  ck_assert(get_logged_event().type == EVENT_NONE);
  seq += 64;

  console_record_event((struct logged_event){
    .type = EVENT_TRIGGER,
    .time = 99,
  });

  ret = get_logged_event();
  ck_assert(ret.type == EVENT_TRIGGER);
  ck_assert(ret.time == 99);
  ck_assert(ret.seq == seq);
  ck_assert(get_logged_event().type == EVENT_NONE);
  seq += 1;
}
END_TEST

TCase *setup_console_tests() {
  TCase *console_tests = tcase_create("console");

  tcase_add_test(console_tests, test_console_event_log);
  return console_tests;
}

#endif
