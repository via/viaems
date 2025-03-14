#include "stream.h"
#define BENCHMARK
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "calculations.h"
#include "config.h"
#include "decoder.h"
#include "platform.h"
#include "scheduler.h"
#include "sensors.h"
#include "table.h"
#include "tasks.h"
#include "util.h"
#include "crc.h"

static void do_fuel_calculation_bench() {
  platform_load_config();

  uint64_t start = cycle_count();
  calculate_fueling();
  uint64_t end = cycle_count();
  printf("calculate_fueling: %d ns\r\n", (int)cycles_to_ns(end - start));
}

static void do_schedule_ignition_event_bench() {
  platform_load_config();

  config.events[0].start.state = SCHED_UNSCHEDULED;
  config.events[0].stop.state = SCHED_UNSCHEDULED;
  config.events[0].angle = 180;

  config.decoder.valid = 1;
  calculated_values.timing_advance = 20.0f;
  calculated_values.dwell_us = 2000;

  uint64_t start = cycle_count();
  schedule_event(&config.events[0]);
  uint64_t end = cycle_count();
  assert(config.events[0].start.state == SCHED_SCHEDULED);

  printf("fresh ignition schedule_event: %d ns\r\n",
         (int)cycles_to_ns(end - start));

  calculated_values.timing_advance = 25.0f;
  start = cycle_count();
  schedule_event(&config.events[0]);
  end = cycle_count();

  printf("backward schedule_event: %d ns\r\n", (int)cycles_to_ns(end - start));

  calculated_values.timing_advance = 15.0f;
  start = cycle_count();
  schedule_event(&config.events[0]);
  end = cycle_count();

  printf("forward schedule_event: %d ns\r\n", (int)cycles_to_ns(end - start));
}

static void do_sensor_adc_calcs() {
  platform_load_config();
  struct adc_update u = {
    .time = current_time(),
    .valid = true,
  };
  for (int i = 0; i < 16; i++) {
    u.values[i] = 2.5f;
  }
  uint64_t start = cycle_count();
  sensor_update_adc(&u);
  uint64_t end = cycle_count();

  printf("process_sensor(adc): %d ns\r\n", (int)cycles_to_ns(end - start));
}

static void do_sensor_single_therm() {
  platform_load_config();
  for (int i = 0; i < NUM_SENSORS; i++) {
    config.sensors[i].source = SENSOR_NONE;
  }
  config.sensors[0] = (struct sensor_input){
    .pin = 0,
    .source = SENSOR_ADC,
    .method = METHOD_THERM,
    .therm = {
      .bias=2490,
      .a=0.00146167419060305,
      .b=0.00022887572003919,
      .c=1.64484831669638E-07,
    },
    .fault_config = {
      .min = 0,
      .max = 4095,
    },
  };

  struct adc_update u = {
    .time = current_time(),
    .valid = true,
    .values = { 2.5f },
  };

  uint64_t start = cycle_count();
  sensor_update_adc(&u);
  uint64_t end = cycle_count();

  printf("process_sensor(single-therm): %d ns\r\n",
         (int)cycles_to_ns(end - start));
}

uint8_t crc_payload[] = {
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
};

static void do_crc16() {
  /* Calculate a CRC for the 240 byte payload above */
  uint64_t start = cycle_count();
  uint16_t crc = CRC16_INIT;
  for (unsigned int i = 0; i < sizeof(crc_payload); i++) {
    crc16_add_byte(&crc, crc_payload[i]);
  }
  uint64_t end = cycle_count();
  printf("crc16: %d ns  (%x)\r\n", (int)cycles_to_ns(end - start), crc);
}

static void do_crc32() {
  /* Calculate a CRC for the 240 byte payload above */
  uint64_t start = cycle_count();
  uint32_t crc = CRC32_INIT;
  for (unsigned int i = 0; i < sizeof(crc_payload); i++) {
    crc32_add_byte(&crc, crc_payload[i]);
  }
  uint64_t end = cycle_count();
  printf("crc32: %d ns  (%x)\r\n", (int)cycles_to_ns(end - start), crc);
}

#if 0
static void do_encode_decode() {
  static uint8_t buffer[MAX_ENCODED_FRAGMENT_SIZE];
  uint16_t encoded_size;

  uint64_t start = cycle_count();
  encode_cobs_fragment(sizeof(crc_payload), crc_payload,
                       &encoded_size, buffer);
  uint64_t end = cycle_count();

  printf("encode_fragment: %d ns\r\n", (int)cycles_to_ns(end - start));

  start = cycle_count();
  decode_cobs_fragment_in_place(&encoded_size, buffer);
  end = cycle_count();
  printf("decode_fragment: %d ns\r\n", (int)cycles_to_ns(end - start));
}
#endif


int main() {
  /* Preparations for all benchmarks */
  platform_benchmark_init();

  config.sensors[SENSOR_IAT].value = 15.0f;
  config.sensors[SENSOR_BRV].value = 14.8f;
  config.sensors[SENSOR_MAP].value = 100.0f;
  config.sensors[SENSOR_CLT].value = 90.0f;
  config.decoder.rpm = 3000;

  do {
    do_fuel_calculation_bench();
    do_schedule_ignition_event_bench();
    do_sensor_adc_calcs();
    do_sensor_single_therm();
    do_crc16();
    do_crc32();
  } while (1);
  return 0;
}
