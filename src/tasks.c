#include "tasks.h"
#include "config.h"
#include "decoder.h"
#include "platform.h"
#include "util.h"

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
  float duty;
  if ((config.sensors[SENSOR_MAP].processed_value <
       config.boost_control.threshhold_kpa)) {
    if (config.sensors[SENSOR_TPS].processed_value > 90.0) {
      duty = 100.0;
    } else {
      duty = 0.0;
    }
  } else {
    duty = interpolate_table_oneaxis(config.boost_control.pwm_duty_vs_rpm,
                                     config.decoder.rpm);
  }
  set_pwm(config.boost_control.pin, duty);
}

void handle_idle_control() {}

/* Checks for a variety of failure conditions, and produces a check engine
 * output:
 *
 * Sensor input in fault - 3s constant steady light
 * Decoder loss - 3s of 1/2s blink
 * Lean in boost - 3s of 1/5s blink
 */

void handle_check_engine_light() {
  static enum {
    CEL_NONE,
    CEL_CONSTANT,
    CEL_SLOWBLINK,
    CEL_FASTBLINK,
  } cel_state = CEL_NONE, next_cel_state = CEL_NONE;

  static timeval_t last_cel = 0;

  /* Determine CEL conditions */
  int sensor_in_fault = (sensor_fault_status() > 0);
  int decode_loss = !config.decoder.valid && (config.decoder.rpm > 0);
  int lean_in_boost =
    (config.sensors[SENSOR_MAP].processed_value > config.cel.lean_boost_kpa) &&
    (config.sensors[SENSOR_EGO].processed_value > config.cel.lean_boost_ego);

  /* Translate CEL condition to CEL blink state */
  if (lean_in_boost) {
    next_cel_state = CEL_FASTBLINK;
  } else if (decode_loss) {
    next_cel_state = CEL_SLOWBLINK;
  } else if (sensor_in_fault) {
    next_cel_state = CEL_CONSTANT;
  } else {
    next_cel_state = CEL_NONE;
  }

  /* Handle 3s reset of CEL state */
  if (time_diff(current_time(), last_cel) > time_from_us(3000000)) {
    cel_state = CEL_NONE;
  }

  /* Are we transitioning to a higher state? If so reset blink timer */
  if (next_cel_state > cel_state) {
    cel_state = next_cel_state;
    last_cel = current_time();
  }

  int pin_state;
  switch (cel_state) {
    case CEL_NONE:
      pin_state = 0;
      break;
    case CEL_CONSTANT:
      pin_state = 1;
      break;
    case CEL_SLOWBLINK:
      pin_state = (current_time() - last_cel) / time_from_us(500000) % 2;
      break;
    case CEL_FASTBLINK:
      pin_state = (current_time() - last_cel) / time_from_us(200000) % 2;
      break;
    default:
      pin_state = 1;
  }
  set_gpio(config.cel.pin, pin_state);

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
}
END_TEST

TCase *setup_tasks_tests() {
  TCase *tasks_tests = tcase_create("tasks");
  tcase_add_test(tasks_tests, check_tasks_handle_fuel_pump);
  return tasks_tests;
}

#endif
