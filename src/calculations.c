#include <math.h>
#include <stdbool.h>

#include "calculations.h"
#include "config.h"
#include "util.h"
#include "viaems.h"

/* Constants */
/* Compute air density from temperature, returns g/cm^3 */
static float air_density(float iat_celsius) {
  const float kelvin_offset = 273.15f;
  const float density_of_air_stp = 1.2922e-3; /* g/cm^3 at 0C */

  float temp_factor = kelvin_offset / (iat_celsius + kelvin_offset);
  return density_of_air_stp * temp_factor;
}

/* Corrects a provided fuel density at 15C and 1 atm for temperature,
 * returns g/cm^3 */
static float fuel_density(float stp_density, float fuel_celsius) {
  const float beta = 950.0; /* Gasoline 10^-6/K */
  float delta_temp = fuel_celsius - 15.0f;
  return stp_density - (delta_temp * beta / 1000000.0f);
}

/* Returns mass (g) of air injested into a cylinder */
static float calculate_airmass(float cylinder_cc,
                               float ve,
                               float map,
                               float iat) {

  float injested_air_volume_per_cycle =
    (ve / 100.0f) * (map / 100.0f) * cylinder_cc;

  float injested_air_mass_per_cycle =
    injested_air_volume_per_cycle * air_density(iat);

  return injested_air_mass_per_cycle;
}

/* Given an airmass and a fuel temperature, returns stoich amount of fuel volume
 */
static float calculate_fuel_volume(float fuel_stp_density,
                                   float stoich_ratio,
                                   float airmass,
                                   float frt) {
  float fuel_mass = airmass / stoich_ratio;
  float fuel_volume = fuel_mass / fuel_density(fuel_stp_density, frt);

  return fuel_volume;
}

/* Returns mm^3 of fuel per cycle to add */
static float calculate_tipin_enrichment(const struct config *config,
                                        struct tipin_state *state,
                                        timeval_t now,
                                        float tps,
                                        float tpsrate,
                                        int rpm) {

  float new_tipin_amount =
    interpolate_table_twoaxis(&config->tipin_enrich_amount, tpsrate, tps);

  /* Update status flag */
  if (state->active &&
      !time_in_range(now, state->time, state->time + state->length)) {
    state->active = false;
  }

  if ((new_tipin_amount > state->amount) || !state->active) {
    /* Overwrite our event */
    state->time = now;
    state->length = time_from_us(
      interpolate_table_oneaxis(&config->tipin_enrich_duration, rpm) * 1000);
    state->amount = new_tipin_amount;
    state->active = true;
  }

  return state->amount;
}

struct calculated_values calculate_ignition_and_fuel(
  const struct config *config,
  const struct engine_update *update,
  struct calculations *state) {

  const struct sensor_values *sensors = &update->sensors;
  const struct engine_position *pos = &update->position;
  const float iat = sensors->IAT.value;
  const float brv = sensors->BRV.value;
  const float map = sensors->MAP.value;
  const float frt = sensors->FRT.value;
  const float clt = sensors->CLT.value;

  const float tps = sensors->TPS.value;
  const float tpsrate = sensors->TPS.derivative;

  float timing_advance =
    interpolate_table_twoaxis(&config->timing, pos->rpm, map);
  float dwell_us = 0.0f;
  switch (config->ignition.dwell) {
  case DWELL_FIXED_DUTY:
    dwell_us = time_from_rpm_diff(pos->rpm, 45) / (TICKRATE / 1000000);
    break;
  case DWELL_FIXED_TIME:
    dwell_us = config->ignition.dwell_us;
    break;
  case DWELL_BRV:
    dwell_us = 1000 * interpolate_table_oneaxis(&config->dwell, brv);
    break;
  }

  float ve = interpolate_table_twoaxis(&config->ve, pos->rpm, map);
  float lambda =
    interpolate_table_twoaxis(&config->commanded_lambda, pos->rpm, map);
  float idt = interpolate_table_oneaxis(&config->injector_deadtime_offset, brv);
  float ete = interpolate_table_twoaxis(&config->engine_temp_enrich, clt, map);

  /* Cranking enrichment config overrides ETE */
  if ((pos->rpm < config->fueling.crank_enrich_config.crank_rpm) &&
      (clt < config->fueling.crank_enrich_config.cutoff_temperature)) {
    ete = config->fueling.crank_enrich_config.enrich_amt;
  }

  float tipin = calculate_tipin_enrichment(
    config, &state->tipin, update->current_time, tps, tpsrate, pos->rpm);

  float airmass_per_cycle =
    calculate_airmass(config->fueling.cylinder_cc, ve, map, iat);

  float fuel_vol_at_stoich =
    calculate_fuel_volume(config->fueling.density_of_fuel,
                          config->fueling.fuel_stoich_ratio,
                          airmass_per_cycle,
                          frt);

  float fuelvol_per_cycle = fuel_vol_at_stoich / lambda;

  float raw_pw_us =
    (fuelvol_per_cycle + (tipin / 1000)) /              /* Tipin unit is mm^3 */
    config->fueling.injector_cc_per_minute * 60000000 / /* uS per minute */
    config->fueling.injections_per_cycle;               /* This many pulses */

  float pwc = interpolate_table_oneaxis(&config->injector_pw_correction,
                                        raw_pw_us / 1000.0f);
  float commanded_pw_us = (raw_pw_us * ete) + (pwc * 1000.0f) + (idt * 1000.0f);

  bool rpm_cut;
  if (state->rpm_cut_triggered) {
    rpm_cut = pos->rpm > config->rpm_start;
  } else {
    rpm_cut = pos->rpm > config->rpm_stop;
  }
  state->rpm_cut_triggered = rpm_cut;

  timeval_t max_allowable_pw = time_from_rpm_diff(pos->rpm, 720) /
                               config->fueling.injections_per_cycle *
                               (config->fueling.max_duty_cycle / 100.0f);

  bool fuel_overduty_cut_enabled = false;
  if (time_from_us(commanded_pw_us) >= max_allowable_pw) {
    fuel_overduty_cut_enabled = true;
  }

  timeval_t max_allowable_dwell_us =
    us_from_time(time_from_rpm_diff(pos->rpm, 720) /
                 config->ignition.ignitions_per_cycle) -
    config->ignition.min_coil_cooldown_us;

  bool dwell_overduty_cut_enabled = false;
  if (dwell_us >= max_allowable_dwell_us) {
    dwell_us = max_allowable_dwell_us;
  }
  if (dwell_us < config->ignition.min_dwell_us) {
    dwell_overduty_cut_enabled = true;
  }

  bool boost_cut = map > config->boost_control.overboost;

  return (struct calculated_values){
    .rpm_limit_cut = rpm_cut,
    .boost_cut = boost_cut,
    .fuel_overduty_cut = fuel_overduty_cut_enabled,
    .dwell_overduty_cut = dwell_overduty_cut_enabled,

    .timing_advance = timing_advance,
    .dwell_us = dwell_us,
    .fueling_us = commanded_pw_us,

    .ete = ete,
    .idt = idt,
    .pwc = pwc,
    .ve = ve,
    .lambda = lambda,
    .fuelvol_per_cycle = fuelvol_per_cycle,
    .tipin = tipin,
    .airmass_per_cycle = airmass_per_cycle,
  };
}

#ifdef UNITTEST
#include <check.h>
#include <stdlib.h>

START_TEST(check_air_density) {
  ck_assert_float_eq_tol(air_density(0), 1.2922e-3, 0.000001);
  ck_assert_float_eq_tol(air_density(30), 1.1644e-3, 0.000001);
}
END_TEST

START_TEST(check_fuel_density) {
  const float stp_density = 0.755;
  ck_assert_float_eq_tol(fuel_density(stp_density, 15), 0.755, 0.001);
  ck_assert_float_eq_tol(fuel_density(stp_density, 50), 0.721, 0.001);
}
END_TEST

START_TEST(check_calculate_airmass) {
  const float cylinder_cc = 500;

  /* Airmass for perfect VE, full map, 0 C*/
  float airmass = calculate_airmass(cylinder_cc, 100, 100, 0);
  ck_assert_float_eq_tol(airmass, 0.646100, 0.001);

  /* 70 MAP should be 70% of previous airmass */
  ck_assert_float_eq_tol(
    calculate_airmass(cylinder_cc, 100, 70, 0), 0.7 * airmass, 0.001);

  /* 80 VE should be 80% of first airmass */
  ck_assert_float_eq_tol(
    calculate_airmass(cylinder_cc, 80, 100, 0), 0.8 * airmass, 0.001);
}
END_TEST

START_TEST(check_ign_fuel_calcs) {

  struct calculations state = { 0 };
  struct engine_update update = {
    .position = { .valid_until = - 1, .has_rpm = true, .rpm = 6000, .tooth_rpm = 6000 },
    .sensors = { 
      .IAT = { .value = 30 },
      .BRV = { .value = 14 },
      .MAP = { .value = 100 },
      .FRT = { .value = 15 },
      .CLT = { .value = 85 },
      .TPS = { .value = 0, .derivative = 0},
    },
  };

  struct calculated_values results =
    calculate_ignition_and_fuel(&default_config, &update, &state);
  // Make sure ignition values are reasonable
  ck_assert((results.timing_advance > 5) && (results.timing_advance < 45));
  ck_assert((results.dwell_us > 500) && (results.dwell_us < 5000));

  ck_assert_float_eq(results.ete, 1.0);   // No ETE since we're at temp
  ck_assert_float_eq(results.tipin, 0.0); // No tipin since no TPS movement
  ck_assert((results.fueling_us > 1000) && (results.fueling_us < 10000));

  // No cuts
  ck_assert(!results.boost_cut);
  ck_assert(!results.rpm_limit_cut);
  ck_assert(!results.fuel_overduty_cut);
}
END_TEST

START_TEST(check_cuts) {

  struct calculations state = { 0 };
  struct engine_update update = {
    .position = { .valid_until = - 1, .has_rpm = true, .rpm = 8000, .tooth_rpm = 8000 },
    .sensors = { 
      .IAT = { .value = 30 },
      .BRV = { .value = 14 },
      .MAP = { .value = 500 },
      .FRT = { .value = 15 },
      .CLT = { .value = 85 },
      .TPS = { .value = 0, .derivative = 0},
    },
  };

  struct calculated_values results =
    calculate_ignition_and_fuel(&default_config, &update, &state);
  // At 6000 rpms we only have 20 ms, and with a MAP of 500, we should be
  // hitting that limit

  ck_assert_float_gt(results.fueling_us, 20000);
  ck_assert(results.fuel_overduty_cut);

  ck_assert(results.rpm_limit_cut);

  ck_assert(results.boost_cut);
}
END_TEST

static const struct config tipin_test_config = {
  .tipin_enrich_amount = {
    .cols = { .num = 2, .values = { 0, 100 }, },
    .rows = { .num = 2, .values = { 0, 100 },  },
    .data = { { 0, 0 }, { 0, 2000 } },
  },
  .tipin_enrich_duration = {
    .cols = { .num = 2, .values = { 100, 6000 } },
    .data = { 1.0, 5.0 },
  },
};

START_TEST(check_calculate_tipin_newevent) {

  timeval_t time = 0;
  struct tipin_state state = { 0 };

  ck_assert_float_eq_tol(
    calculate_tipin_enrichment(&tipin_test_config, &state, time, 0, 0, 100),
    0,
    0.001);
  ck_assert_float_eq_tol(
    calculate_tipin_enrichment(&tipin_test_config, &state, time, 100, 100, 100),
    2000.0,
    0.001);
  /* At 100 rpm, should last 1 ms */

  time = time_from_us(900);
  ck_assert_float_eq_tol(
    calculate_tipin_enrichment(&tipin_test_config, &state, time, 0, 0, 100),
    2000.0,
    0.001);

  time = time_from_us(1005);
  ck_assert_float_eq_tol(
    calculate_tipin_enrichment(&tipin_test_config, &state, time, 0, 0, 100),
    0.0,
    0.001);
}
END_TEST

START_TEST(check_calculate_tipin_overriding_event) {

  timeval_t time = 0;
  struct tipin_state state = { 0 };

  ck_assert_float_eq_tol(
    calculate_tipin_enrichment(&tipin_test_config, &state, time, 0, 0, 100),
    0,
    0.001);
  ck_assert_float_eq_tol(
    calculate_tipin_enrichment(&tipin_test_config, &state, time, 100, 50, 100),
    1000,
    0.001);

  /* At 100 rpm, should last 1 ms */
  time = time_from_us(500);
  /* Milder tipin should not affect anything */
  ck_assert_float_eq_tol(
    calculate_tipin_enrichment(&tipin_test_config, &state, time, 100, 20, 100),
    1000,
    0.001);

  time = time_from_us(1005);
  ck_assert_float_eq_tol(
    calculate_tipin_enrichment(&tipin_test_config, &state, time, 0, 0, 100),
    0.0,
    0.001);

  /* Test a higher overriding event */
  ck_assert_float_eq_tol(
    calculate_tipin_enrichment(&tipin_test_config, &state, time, 100, 20, 100),
    400,
    0.001);

  time = time_from_us(1500);
  /* New tipin should override, since its higher */
  ck_assert_float_eq_tol(
    calculate_tipin_enrichment(&tipin_test_config, &state, time, 100, 50, 100),
    1000,
    0.001);

  /* and should still be ongoing for the full time */
  time = time_from_us(2400);
  ck_assert_float_eq_tol(
    calculate_tipin_enrichment(&tipin_test_config, &state, time, 0, 0, 100),
    1000,
    0.001);

  /*and now finished */
  time = time_from_us(2600);
  ck_assert_float_eq_tol(
    calculate_tipin_enrichment(&tipin_test_config, &state, time, 0, 0, 100),
    0.0,
    0.001);
}
END_TEST

TCase *setup_calculations_tests() {
  TCase *tc = tcase_create("calculations");
  tcase_add_test(tc, check_air_density);
  tcase_add_test(tc, check_fuel_density);
  tcase_add_test(tc, check_calculate_airmass);
  tcase_add_test(tc, check_cuts);

  tcase_add_test(tc, check_ign_fuel_calcs);
  tcase_add_test(tc, check_calculate_tipin_newevent);
  tcase_add_test(tc, check_calculate_tipin_overriding_event);
  return tc;
}
#endif
