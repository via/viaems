#include "tasks.h"
#include "calculations.h"
#include "config.h"
#include "decoder.h"
#include "platform.h"
#include "scheduler.h"
#include "util.h"
#include "viaems.h"

static bool determine_fuel_pump_state(struct tasks *state,
                                      const struct engine_update *update) {

  const struct engine_position *d = &update->position;

  /* If engine is actively turning, fuel pump is on */
  if (d->has_rpm &&
      time_in_range(update->current_time, d->time, d->valid_until)) {
    state->fuelpump_last_valid = update->current_time;
    return true;
  }

  /* Otherwise, if we're within four seconds of the last time the fuel pump was
   * turned on (due to engine turning), keep it on */
  if (time_diff(update->current_time, state->fuelpump_last_valid) <
      time_from_us(4000000)) {
    return true;
  } else {
    /* Update last valid time to 5 seconds ago so that we don't turn it on due
     * to time rolling over */
    state->fuelpump_last_valid = update->current_time - time_from_us(5000000);
    return false;
  }
}

static float handle_boost_control(const struct boost_control_config *config,
                                  const struct engine_update *u) {
  float duty;
  float map = u->sensors.MAP.value;
  float tps = u->sensors.TPS.value;
  if (map < config->enable_threshold_kpa) {
    /* Below the "enable" threshold, keep the valve off */
    duty = 0.0f;
  } else if (map < config->control_threshold_kpa &&
             tps > config->control_threshold_tps) {
    /* Above "enable", but below "control", and WOT, so keep wastegate at
     * atmostpheric pressure to improve spool time */
    duty = 100.0f;
  } else {
    /* We are controlling the wastegate with PWM */
    duty = interpolate_table_oneaxis(&config->pwm_duty_vs_rpm, u->position.rpm);
  }
  return duty;
}

/* Checks for a variety of failure conditions, and produces a check engine
 * output:
 *
 * Sensor input in fault - 3s constant steady light
 * Decoder loss - 3s of 1/2s blink
 * Lean in boost - 3s of 1/5s blink
 */

static cel_state_t determine_next_cel_state(const struct cel_config *config,
                                            const struct engine_update *u) {
  cel_state_t next_cel_state = CEL_NONE;

  float map = u->sensors.MAP.value;
  float ego = u->sensors.EGO.value;

  bool sensor_in_fault = sensor_has_faults(&u->sensors);
  bool decode_loss = !u->position.has_position;
  bool lean_in_boost =
    (map > config->lean_boost_kpa) && (ego > config->lean_boost_ego);

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

static bool handle_check_engine_light(const struct cel_config *config,
                                      struct tasks *state,
                                      const struct engine_update *u) {

  cel_state_t next_cel_state = determine_next_cel_state(config, u);

  /* Handle 3s reset of CEL state */
  if (time_diff(u->current_time, state->cel_time) > time_from_us(3000000)) {
    state->cel_state = CEL_NONE;
  }

  /* Are we transitioning to a higher state? If so reset blink timer */
  if (next_cel_state > state->cel_state) {
    state->cel_state = next_cel_state;
    state->cel_time = u->current_time;
  }

  return determine_cel_pin_state(
    state->cel_state, u->current_time, state->cel_time);
}

void run_tasks(struct viaems *viaems,
               const struct engine_update *update,
               struct platform_plan *plan) {
  plan_set_gpio(plan,
                viaems->config->fueling.fuel_pump_pin,
                determine_fuel_pump_state(&viaems->tasks, update));

  plan_set_gpio(
    plan,
    viaems->config->cel.pin,
    handle_check_engine_light(&viaems->config->cel, &viaems->tasks, update));

  plan_set_pwm(plan,
               viaems->config->boost_control.pin,
               handle_boost_control(&viaems->config->boost_control, update));
}

#ifdef UNITTEST
#include <check.h>

START_TEST(check_tasks_handle_fuel_pump) {

  /* Initial conditions */
  struct engine_update update = {
    .position = { 
      .time = 0, 
      .valid_until = -1, 
      .has_position = false, 
      .has_rpm = false ,
    },
    .current_time = 500,
  };

  struct tasks state = { 0 };

  ck_assert(determine_fuel_pump_state(&state, &update) == true);
  ck_assert_int_eq(state.fuelpump_last_valid, 0);

  /* Wait 4 seconds, should turn off */
  update.current_time = time_from_us(4001000);
  ck_assert(determine_fuel_pump_state(&state, &update) == false);

  /* Wait 4 more seconds, should still be off */
  update.current_time = time_from_us(8000000);
  ck_assert(determine_fuel_pump_state(&state, &update) == false);

  /* Have a recent trigger */
  update.position.has_rpm = true;
  update.position.tooth_rpm = update.position.rpm = 500;
  update.position.time = time_from_us(9000000);
  update.position.valid_until = time_from_us(11000000);
  update.current_time = time_from_us(10000000);
  ck_assert(determine_fuel_pump_state(&state, &update) == true);

  update.current_time = time_from_us(15000000);
  ck_assert(determine_fuel_pump_state(&state, &update) == false);

  /* Keep waiting, should stay shut off */
  update.current_time = time_from_us(20000000);
  ck_assert(determine_fuel_pump_state(&state, &update) == false);
}
END_TEST

START_TEST(check_tasks_next_cel_state) {
  /* no fault */
  struct engine_update update = { 
    .position = { 
      .time = 0, 
      .valid_until = -1, 
      .has_position = true, 
      .has_rpm = true,
    },
    .sensors = { 0 },
  };
  const struct cel_config config = {
    .lean_boost_ego = 0.85,
    .lean_boost_kpa = 150,
    .pin = 1,
  };

  ck_assert_int_eq(determine_next_cel_state(&config, &update), CEL_NONE);

  /* Sensor fault */
  update.sensors.MAP.fault = FAULT_RANGE;
  ck_assert_int_eq(determine_next_cel_state(&config, &update), CEL_CONSTANT);

  /* Decoder loss */
  update.position.has_position = update.position.has_rpm = false;
  update.sensors.MAP.fault = FAULT_NONE;
  ck_assert_int_eq(determine_next_cel_state(&config, &update), CEL_SLOWBLINK);

  /* Still decoder loss, add sensor fault */
  update.sensors.MAP.fault = FAULT_RANGE;
  ck_assert_int_eq(determine_next_cel_state(&config, &update), CEL_SLOWBLINK);

  /* Lean in boost */
  update.sensors.MAP.fault = FAULT_NONE;
  update.position.has_position = update.position.has_rpm = true;
  update.sensors.MAP.value = 180;
  update.sensors.EGO.value = 1.1;
  ck_assert_int_eq(determine_next_cel_state(&config, &update), CEL_FASTBLINK);
}

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
