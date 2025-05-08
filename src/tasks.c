#include "tasks.h"
#include "config.h"
#include "decoder.h"
#include "platform.h"
#include "util.h"

static void handle_fuel_pump() {
  static timeval_t last_valid = 0;

  /* If engine is turning (as defined by seeing a trigger in the last second),
   * keep pump on */
  timeval_t last_trigger = config.decoder.last_trigger_time;
  if (time_in_range(
        current_time(), last_trigger, last_trigger + time_from_us(1000000))) {
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

static void handle_boost_control() {
  float duty;
  float map = config.sensors[SENSOR_MAP].value;
  float tps = config.sensors[SENSOR_TPS].value;
  if (map < config.boost_control.enable_threshold_kpa) {
    /* Below the "enable" threshold, keep the valve off */
    duty = 0.0f;
  } else if (map < config.boost_control.control_threshold_kpa &&
             tps > config.boost_control.control_threshold_tps) {
    /* Above "enable", but below "control", and WOT, so keep wastegate at
     * atmostpheric pressure to improve spool time */
    duty = 100.0f;
  } else {
    /* We are controlling the wastegate with PWM */
    duty = interpolate_table_oneaxis(config.boost_control.pwm_duty_vs_rpm,
                                     config.decoder.rpm);
  }
  set_pwm(config.boost_control.pin, duty);
}

enum stepper_state {
  STEPPER_UNINIT,   /* Default state on start */
  STEPPER_INIT,     /* Position unknown, travelling to the low (0) extreme */
  STEPPER_ACTIVE,   /* Position known, tracking target */
};

struct unipolar_stepper_state {
  enum stepper_state state;
  uint32_t current_step;
};

static struct unipolar_stepper_state uni_stepper_state = {
  .state = STEPPER_UNINIT,
  .current_step = 0,
};

/* Unipolar stepper idle control via bitbang of gpio pins */
static void handle_unipolar_stepper_idle_control(float target) {

 switch (uni_stepper_state.state) {
    case STEPPER_UNINIT:
      /* Travel `stepper_steps` (plus a few in case we're on the wrong phase)
       * in down to get us into a position we know */
      uni_stepper_state.current_step = config.idle_control.stepper_steps + 4;
      uni_stepper_state.state = STEPPER_INIT;
      break;
    case STEPPER_INIT:
      if (uni_stepper_state.current_step == 0) {
        uni_stepper_state.current_step = config.idle_control.stepper_steps;
        uni_stepper_state.state = STEPPER_ACTIVE;
      } else {
        uni_stepper_state.current_step -= 1;
      }
      break;
    case STEPPER_ACTIVE: {
      if (target > config.idle_control.stepper_steps) {
        target = config.idle_control.stepper_steps;
      }
      if (target > uni_stepper_state.current_step) {
        uni_stepper_state.current_step += 1;
      } else if (target < uni_stepper_state.current_step) {
        uni_stepper_state.current_step -= 1;
      } else {
        /* No holding torque, just turn everything off */
        set_gpio(config.idle_control.pin_phase_a, 0);
        set_gpio(config.idle_control.pin_phase_b, 0);
        set_gpio(config.idle_control.pin_phase_c, 0);
        set_gpio(config.idle_control.pin_phase_d, 0);
        return;
      }
    }
    break;
  }


  unsigned int phase = uni_stepper_state.current_step % 4;
  set_gpio(config.idle_control.pin_phase_a, (phase == 0));
  set_gpio(config.idle_control.pin_phase_b, (phase == 1));
  set_gpio(config.idle_control.pin_phase_c, (phase == 2));
  set_gpio(config.idle_control.pin_phase_d, (phase == 3));
}

static void handle_idle_control() {
  float target = 0.0f;
  switch (config.idle_control.method) {
    case IDLE_METHOD_DISABED:
      target = 0.0f;
      break;
    case IDLE_METHOD_FIXED:
      target = config.idle_control.fixed_value;
      break;
    case IDLE_METHOD_OPEN_LOOP_CLT:
      break;
  }

  switch (config.idle_control.interface_type) {
    case IDLE_INTERFACE_NONE:
      break;
    case IDLE_INTERFACE_UNIPOLAR_STEPPER:
      handle_unipolar_stepper_idle_control(target);
      break;
  }

}

/* Checks for a variety of failure conditions, and produces a check engine
 * output:
 *
 * Sensor input in fault - 3s constant steady light
 * Decoder loss - 3s of 1/2s blink
 * Lean in boost - 3s of 1/5s blink
 */

typedef enum {
  CEL_NONE,
  CEL_CONSTANT,
  CEL_SLOWBLINK,
  CEL_FASTBLINK,
} cel_state_t;

static cel_state_t cel_state = CEL_NONE;
static timeval_t last_cel = 0;

static cel_state_t determine_next_cel_state() {
  static cel_state_t next_cel_state;

  int sensor_in_fault = (sensor_fault_status() > 0);
  int decode_loss = !config.decoder.valid && (config.decoder.rpm > 0);
  int lean_in_boost =
    (config.sensors[SENSOR_MAP].value > config.cel.lean_boost_kpa) &&
    (config.sensors[SENSOR_EGO].value > config.cel.lean_boost_ego);

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
  return next_cel_state;
}

static int determine_cel_pin_state(cel_state_t state,
                                   timeval_t cur_time,
                                   timeval_t cel_start_time) {
  switch (state) {
  case CEL_NONE:
    return 0;
  case CEL_CONSTANT:
    return 1;
  case CEL_SLOWBLINK:
    return (cur_time - cel_start_time) / time_from_us(500000) % 2;
  case CEL_FASTBLINK:
    return (cur_time - cel_start_time) / time_from_us(200000) % 2;
  default:
    return 1;
  }
}

static void handle_check_engine_light() {

  cel_state_t next_cel_state = determine_next_cel_state();

  /* Handle 3s reset of CEL state */
  if (time_diff(current_time(), last_cel) > time_from_us(3000000)) {
    cel_state = CEL_NONE;
  }

  /* Are we transitioning to a higher state? If so reset blink timer */
  if (next_cel_state > cel_state) {
    cel_state = next_cel_state;
    last_cel = current_time();
  }

  set_gpio(config.cel.pin,
           determine_cel_pin_state(cel_state, current_time(), last_cel));
}

void handle_emergency_shutdown() {

  /* Fuel pump off */
  set_gpio(config.fueling.fuel_pump_pin, 0);

  /* Stop events */
  invalidate_scheduled_events(config.events, MAX_EVENTS);

  /* TODO evaluate what to do about ignition outputs
   * for now, make sure fuel injectors are off */
  for (unsigned int i = 0; i < MAX_EVENTS; ++i) {
    if (config.events[i].type == FUEL_EVENT) {
      set_output(config.events[i].pin, config.events[i].inverted);
    }
  }
}

void run_tasks() {
  handle_fuel_pump();
  handle_boost_control();
  handle_idle_control();
  handle_check_engine_light();
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
  set_current_time(time_from_us(4001000));
  handle_fuel_pump();
  ck_assert_int_eq(get_gpio(1), 0);

  /* Wait 4 more seconds, should still be off */
  set_current_time(time_from_us(8000000));
  handle_fuel_pump();
  ck_assert_int_eq(get_gpio(1), 0);

  /* Have a recent trigger */
  config.decoder.state = DECODER_RPM;
  config.decoder.last_trigger_time = current_time() - 500000;
  handle_fuel_pump();
  ck_assert_int_eq(get_gpio(1), 1);

  /* Keep waiting, should shut off */
  set_current_time(time_from_us(15000000));
  handle_fuel_pump();
  ck_assert_int_eq(get_gpio(1), 0);

  /* Keep waiting, should stay shut off */
  set_current_time(time_from_us(200000000));
  handle_fuel_pump();
  ck_assert_int_eq(get_gpio(1), 0);
}
END_TEST

START_TEST(check_tasks_next_cel_state) {
  /* no fault */
  config.decoder.valid = 1;
  ck_assert_int_eq(determine_next_cel_state(), CEL_NONE);

  /* Sensor fault */
  config.sensors[SENSOR_MAP].fault = FAULT_RANGE;
  ck_assert_int_eq(determine_next_cel_state(), CEL_CONSTANT);

  /* Decoder loss */
  config.decoder.valid = 0;
  config.decoder.rpm = 1000;
  config.sensors[SENSOR_MAP].fault = FAULT_NONE;
  ck_assert_int_eq(determine_next_cel_state(), CEL_SLOWBLINK);

  /* Still decoder loss, add sensor fault */
  config.sensors[SENSOR_MAP].fault = FAULT_RANGE;
  ck_assert_int_eq(determine_next_cel_state(), CEL_SLOWBLINK);

  /* Lean in boost */
  config.sensors[SENSOR_MAP].fault = FAULT_NONE;
  config.decoder.valid = 1;
  config.sensors[SENSOR_MAP].value = 180;
  config.sensors[SENSOR_EGO].value = 1.1;
  ck_assert_int_eq(determine_next_cel_state(), CEL_FASTBLINK);
}
END_TEST

START_TEST(check_tasks_cel_pin_state) {

  /* with no cel, time is irrelevent */
  ck_assert_int_eq(determine_cel_pin_state(CEL_NONE, 0, 0), 0);

  /* With constant, time still irrelevent */
  ck_assert_int_eq(determine_cel_pin_state(CEL_CONSTANT, 0, 0), 1);

  /* Slowblink should be on at .75 s, off at 1.25 s */
  ck_assert_int_eq(determine_cel_pin_state(CEL_SLOWBLINK,
                                           time_from_us(10750000),
                                           time_from_us(10000000)),
                   1);
  ck_assert_int_eq(determine_cel_pin_state(CEL_SLOWBLINK,
                                           time_from_us(11250000),
                                           time_from_us(10000000)),
                   0);

  /* Fastblink should be on at 1.1 s, off at 1.3 s */
  ck_assert_int_eq(determine_cel_pin_state(CEL_FASTBLINK,
                                           time_from_us(11100000),
                                           time_from_us(10000000)),
                   1);
  ck_assert_int_eq(determine_cel_pin_state(CEL_FASTBLINK,
                                           time_from_us(11300000),
                                           time_from_us(10000000)),
                   0);
}
END_TEST

TCase *setup_tasks_tests() {
  TCase *tasks_tests = tcase_create("tasks");
  tcase_add_test(tasks_tests, check_tasks_handle_fuel_pump);
  tcase_add_test(tasks_tests, check_tasks_next_cel_state);
  tcase_add_test(tasks_tests, check_tasks_cel_pin_state);
  return tasks_tests;
}

#endif
