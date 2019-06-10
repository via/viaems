#include <math.h>
#include <stdio.h>

#include "platform.h"
#include "config.h"
#include "decoder.h"
#include "util.h"
#include "tasks.h"

void handle_fuel_pump() {
  static timeval_t last_valid = 0;

  /* If engine is turning, keep pump on */
  if ((config.decoder.state == DECODER_RPM) ||
      (config.decoder.state == DECODER_SYNC)) {
    last_valid = current_time();
    set_gpio(config.fueling.fuel_pump_pin, 1);
    return;
  }

  /* Allow 4 seconds of fueling */
  if (time_diff(current_time(), last_valid) < time_from_us(4000000)) {
    set_gpio(config.fueling.fuel_pump_pin, 1);
  } else {
    set_gpio(config.fueling.fuel_pump_pin, 0);
    /* Keep last valid 5 seconds behind to prevent rollover bugs */
    last_valid = current_time() - time_from_us(5000000);
  }
}

void handle_boost_control() {
}

void handle_idle_control() {
}

void handle_emergency_shutdown() {

  /* Fuel pump off */
  set_gpio(config.fueling.fuel_pump_pin, 0);

  /* Stop events */
  invalidate_scheduled_events(config.events, config.num_events);

  /* TODO evaluate what to do about ignition outputs
   * for now, make sure fuel injectors are off */
  for (unsigned int i = 0; i < config.num_events; ++i) {
    if (config.events[i].type == FUEL_EVENT) {
      set_output(config.events[i].pin, config.events[i].inverted);
    }
  }
}

struct auto_calibration_data {
  float map;
  float rpm;
  float tps;
  float ego;
  float lambda;
};

static int auto_calibration_data_is_stable(struct auto_calibration_data *old,
    struct auto_calibration_data *new) {
  if (fabsf(old->map - new->map) > 2.0) {
    return 0;
  }
  if (fabsf(old->rpm - new->rpm) > 100) {
    return 0;
  }
  if (fabsf(old->tps - new->tps) > 5) {
    return 0;
  }
  if (fabsf(old->ego - new->ego) > 0.03) {
    return 0;
  }
  if ((new->ego < 0.65) || (new->ego > 1.4)) {
    return 0;
  }
  if (fabsf(calculated_values.tipin) > 20) {
    return 0;
  }
  if (fabsf(calculated_values.ete - 1.0) > 0.01) {
    return 0;
  }
  return 1;
}

void handle_auto_calibration() {
  static timeval_t last_run = 0;
  static struct auto_calibration_data last_reading, current_reading;

  /* Run about every 1/10th second */
  if (current_time() - last_run < time_from_us(100000)) {
    return;
  }
  last_run = current_time();

  current_reading.map = config.sensors[SENSOR_MAP].processed_value;
  current_reading.rpm = config.decoder.rpm;
  current_reading.tps = config.sensors[SENSOR_TPS].processed_value;
  current_reading.ego = config.sensors[SENSOR_EGO].processed_value;
  current_reading.lambda = calculated_values.lambda;

  if (auto_calibration_data_is_stable(&last_reading, &current_reading)) {
    float new_ve = (last_reading.ego / last_reading.lambda) * 
      calculated_values.ve;

    /* Lag filter the VE change */
    new_ve = ((calculated_values.ve * 90.0) +
      (new_ve * (100.0 - 90.0))) / 100.0;


   table_apply_correction_twoaxis(config.ve, new_ve,
       last_reading.rpm, last_reading.map, 800, 30);

  }

  last_reading = current_reading;
}

#ifdef UNITTEST
#include <check.h>

START_TEST(check_tasks_handle_fuel_pump) {

  /* Initial conditions */
  config.fueling.fuel_pump_pin = 1;
  ck_assert_int_eq(get_gpio(1), 0);

  set_current_time(time_from_us(500));
  handle_fuel_pump();  
  ck_assert_int_eq(get_gpio(1), 1);

  /* Wait 4 seconds, should turn off */
  set_current_time(time_from_us(4000000));
  handle_fuel_pump();  
  ck_assert_int_eq(get_gpio(1), 0);

  /* Wait 4 more seconds, should still be off */
  set_current_time(time_from_us(8000000));
  handle_fuel_pump();  
  ck_assert_int_eq(get_gpio(1), 0);

  /* Get RPM sync */
  config.decoder.state = DECODER_RPM;
  handle_fuel_pump();  
  ck_assert_int_eq(get_gpio(1), 1);

  /* Wait 10 more seconds, should still be on */
  set_current_time(time_from_us(18000000));
  handle_fuel_pump();  
  ck_assert_int_eq(get_gpio(1), 1);

  /* Lose RPM sync */
  config.decoder.state = DECODER_NOSYNC;
  set_current_time(time_from_us(19000000));
  handle_fuel_pump();  
  ck_assert_int_eq(get_gpio(1), 1);

  /* Keep waiting, should shut off */
  set_current_time(time_from_us(25000000));
  handle_fuel_pump();  
  ck_assert_int_eq(get_gpio(1), 0);

  /* Keep waiting, should stay shut off */
  set_current_time(time_from_us(40000000));
  handle_fuel_pump();  
  ck_assert_int_eq(get_gpio(1), 0);

} END_TEST

TCase *setup_tasks_tests() {
  TCase *tasks_tests = tcase_create("tasks");
  tcase_add_test(tasks_tests, check_tasks_handle_fuel_pump);
  return tasks_tests;
}

#endif

