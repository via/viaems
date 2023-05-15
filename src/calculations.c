#include <math.h>
#include <stdbool.h>

#include "calculations.h"
#include "config.h"
struct calculated_values calculated_values;

static bool fuel_overduty() {
  /* Maximum pulse width */
  timeval_t max_pw = time_from_rpm_diff(config.decoder.rpm, 720) /
                     config.fueling.injections_per_cycle;

  calculated_values.fuel_overduty_cut =
    time_from_us(calculated_values.fueling_us) >= max_pw;
  return calculated_values.fuel_overduty_cut;
}

static bool rpm_cut() {
  if (config.decoder.rpm >= config.rpm_stop) {
    calculated_values.rpm_limit_cut = 1;
  }
  if (config.decoder.rpm < config.rpm_start) {
    calculated_values.rpm_limit_cut = 0;
  }
  return calculated_values.rpm_limit_cut;
}

static bool boost_cut() {
  calculated_values.boost_cut =
    config.sensors[SENSOR_MAP].value > config.boost_control.overboost;
  return calculated_values.boost_cut;
}

bool ignition_cut() {
  return rpm_cut() || fuel_overduty() || boost_cut();
}

bool fuel_cut() {
  return ignition_cut();
}

void calculate_ignition() {
  calculated_values.timing_advance = interpolate_table_twoaxis(
    config.timing, config.decoder.rpm, config.sensors[SENSOR_MAP].value);
  switch (config.ignition.dwell) {
  case DWELL_FIXED_DUTY:
    calculated_values.dwell_us =
      time_from_rpm_diff(config.decoder.rpm, 45) / (TICKRATE / 1000000);
    break;
  case DWELL_FIXED_TIME:
    calculated_values.dwell_us = config.ignition.dwell_us;
    break;
  case DWELL_BRV:
    calculated_values.dwell_us =
      1000 *
      interpolate_table_oneaxis(config.dwell, config.sensors[SENSOR_BRV].value);
    break;
  }
}

/* Compute air density from temperature, returns g/cm^3 */
static float air_density(float iat_celsius) {
  const float kelvin_offset = 273.15f;
  float temp_factor = kelvin_offset / (iat_celsius + kelvin_offset);
  return config.fueling.density_of_air_stp * temp_factor;
}

/* Compute fuel density from temperature, returns g/cm^3 */
static float fuel_density(float fuel_celsius) {
  const float beta = 950.0; /* Gasoline 10^-6/K */
  float delta_temp = fuel_celsius - 15.0f;
  return config.fueling.density_of_fuel - (delta_temp * beta / 1000000.0f);
}

/* Returns mass (g) of air injested into a cylinder */
static float calculate_airmass(float ve, float map, float iat) {

  float injested_air_volume_per_cycle =
    (ve / 100.0f) * (map / 100.0f) * config.fueling.cylinder_cc;

  float injested_air_mass_per_cycle =
    injested_air_volume_per_cycle * air_density(iat);

  return injested_air_mass_per_cycle;
}

/* Given an airmass and a fuel temperature, returns stoich amount of fuel volume
 */
static float calculate_fuel_volume(float airmass, float frt) {
  float fuel_mass = airmass / config.fueling.fuel_stoich_ratio;
  float fuel_volume = fuel_mass / fuel_density(frt);

  return fuel_volume;
}

/* Returns mm^3 of fuel per cycle to add */
static float calculate_tipin_enrichment(float tps, float tpsrate, int rpm) {
  static struct {
    timeval_t time;
    timeval_t length;
    float amount;
    int active;
  } current = { 0 };

  if (!config.tipin_enrich_amount || !config.tipin_enrich_duration) {
    return 0.0;
  }

  float new_tipin_amount =
    interpolate_table_twoaxis(config.tipin_enrich_amount, tpsrate, tps);

  /* Update status flag */
  if (current.active && !time_in_range(current_time(),
                                       current.time,
                                       current.time + current.length)) {
    current.active = 0;
  }

  if ((new_tipin_amount > current.amount) || !current.active) {
    /* Overwrite our event */
    current.time = current_time();
    current.length = time_from_us(
      interpolate_table_oneaxis(config.tipin_enrich_duration, rpm) * 1000);
    current.amount = new_tipin_amount;
    current.active = 1;
  }

  return current.amount;
}

void calculate_fueling() {

  float ve;
  float lambda;
  float idt;
  float pwc;
  float ete;

  float iat = config.sensors[SENSOR_IAT].value;
  float brv = config.sensors[SENSOR_BRV].value;
  float map = config.sensors[SENSOR_MAP].value;
  float frt = config.sensors[SENSOR_FRT].value;
  float clt = config.sensors[SENSOR_CLT].value;

  float tps = config.sensors[SENSOR_TPS].value;
  float tpsrate = config.sensors[SENSOR_TPS].derivative;

  if (config.ve) {
    ve = interpolate_table_twoaxis(config.ve, config.decoder.rpm, map);
  } else {
    ve = 100.0;
  }

  if (config.commanded_lambda) {
    lambda = interpolate_table_twoaxis(
      config.commanded_lambda, config.decoder.rpm, map);
  } else {
    lambda = 1.0;
  }

  if (config.injector_deadtime_offset) {
    idt = interpolate_table_oneaxis(config.injector_deadtime_offset, brv);
  } else {
    idt = 1.0;
  }

  if (config.engine_temp_enrich) {
    ete = interpolate_table_twoaxis(config.engine_temp_enrich, clt, map);
  } else {
    ete = 1.0;
  }

  /* Cranking enrichment config overrides ETE */
  if ((config.decoder.rpm < config.fueling.crank_enrich_config.crank_rpm) &&
      (clt < config.fueling.crank_enrich_config.cutoff_temperature)) {
    ete = config.fueling.crank_enrich_config.enrich_amt;
  }

  calculated_values.tipin =
    calculate_tipin_enrichment(tps, tpsrate, config.decoder.rpm);

  calculated_values.airmass_per_cycle = calculate_airmass(ve, map, iat);

  float fuel_vol_at_stoich =
    calculate_fuel_volume(calculated_values.airmass_per_cycle, frt);

  calculated_values.fuelvol_per_cycle = fuel_vol_at_stoich / lambda;

  float raw_pw_us =
    (calculated_values.fuelvol_per_cycle +
     (calculated_values.tipin / 1000)) / /* Tipin unit is mm^3 */
    config.fueling.injector_cc_per_minute *
    60000000 /                           /* uS per minute */
    config.fueling.injections_per_cycle; /* This many pulses */

  if (config.injector_pw_correction) {
    pwc = interpolate_table_oneaxis(config.injector_pw_correction,
                                    raw_pw_us / 1000.0f);
  } else {
    pwc = 0.0f;
  }

  calculated_values.ete = ete;
  calculated_values.idt = idt;
  calculated_values.pwc = pwc;
  calculated_values.ve = ve;
  calculated_values.lambda = lambda;
  calculated_values.fueling_us =
    (raw_pw_us * ete) + (pwc * 1000.0f) + (idt * 1000.0f);
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
  ck_assert_float_eq_tol(fuel_density(15), 0.755, 0.001);
  ck_assert_float_eq_tol(fuel_density(50), 0.721, 0.001);
}
END_TEST

START_TEST(check_calculate_airmass) {

  /* Airmass for perfect VE, full map, 0 C*/
  config.sensors[SENSOR_IAT].value = 0.0f;
  config.fueling.cylinder_cc = 500;
  float airmass = calculate_airmass(100, 100, 0);
  ck_assert_float_eq_tol(airmass, 0.646100, 0.001);

  /* 70 MAP should be 70% of previous airmass */
  ck_assert_float_eq_tol(calculate_airmass(100, 70, 0), 0.7 * airmass, 0.001);

  /* 80 VE should be 80% of first airmass */
  ck_assert_float_eq_tol(calculate_airmass(80, 100, 0), 0.8 * airmass, 0.001);
}
END_TEST

START_TEST(check_fuel_overduty) {

  /* 20 ms for two complete revolutions */
  config.decoder.rpm = 6000;
  config.fueling.injections_per_cycle = 1;
  calculated_values.fueling_us = 18000;
  ck_assert(!fuel_overduty());

  calculated_values.fueling_us = 21000;
  ck_assert(fuel_overduty());

  /* Test batch injection */
  config.fueling.injections_per_cycle = 2;
  calculated_values.fueling_us = 11000;
  ck_assert(fuel_overduty());

  calculated_values.fueling_us = 8000;
  ck_assert(!fuel_overduty());
}
END_TEST

START_TEST(check_calculate_ignition_cut) {
  config.rpm_stop = 5000;
  config.rpm_start = 4500;

  config.decoder.rpm = 1000;
  ck_assert_int_eq(ignition_cut(), 0);
  ck_assert_int_eq(calculated_values.rpm_limit_cut, 0);

  config.decoder.rpm = 5500;
  ck_assert_int_eq(ignition_cut(), 1);
  ck_assert_int_eq(calculated_values.rpm_limit_cut, 1);

  config.decoder.rpm = 4000;
  ck_assert_int_eq(ignition_cut(), 0);
  ck_assert_int_eq(calculated_values.rpm_limit_cut, 0);

  config.decoder.rpm = 4800;
  ck_assert_int_eq(ignition_cut(), 0);
  ck_assert_int_eq(calculated_values.rpm_limit_cut, 0);

  config.decoder.rpm = 5500;
  ck_assert_int_eq(ignition_cut(), 1);
  ck_assert_int_eq(calculated_values.rpm_limit_cut, 1);
}
END_TEST

START_TEST(check_calculate_ignition_fixedduty) {
  struct table t = {
    .num_axis = 2,
    .axis = { { .num = 2, .values = { 5, 10 } },
              { .num = 2, .values = { 5, 10 } } },
    .data = { .two = { { 10, 10 }, { 10, 10 } } },
  };
  config.timing = &t;
  config.ignition.dwell = DWELL_FIXED_DUTY;
  config.decoder.rpm = 6000;

  calculate_ignition();

  ck_assert(calculated_values.timing_advance == 10);
  /* 10 ms per rev, dwell should be 1/8 of rotation,
   * fuzzy estimate because math */
  ck_assert(abs((signed)calculated_values.dwell_us - (10000 / 8)) < 5);
}
END_TEST

static struct table tipin_amount = {
  .num_axis = 2,
  .axis = { { .num = 2, .values = { 0, 100 } },
            { .num = 2, .values = { 0, 100 } } },
  .data = { .two = { { 0, 0 }, { 0, 2000 } } },
};

static struct table tipin_duration = {
  .num_axis = 1,
  .axis = { { .num = 2, .values = { 100, 6000 } } },
  .data = { .one = { 1.0, 5.0 } },
};

START_TEST(check_calculate_tipin_newevent) {

  config.tipin_enrich_amount = &tipin_amount;
  config.tipin_enrich_duration = &tipin_duration;

  set_current_time(0);

  ck_assert_float_eq_tol(calculate_tipin_enrichment(0, 0, 100), 0, 0.001);
  ck_assert_float_eq_tol(
    calculate_tipin_enrichment(100, 100, 100), 2000.0, 0.001);
  /* At 100 rpm, should last 1 ms */

  set_current_time(time_from_us(900));
  ck_assert_float_eq_tol(calculate_tipin_enrichment(0, 0, 100), 2000.0, 0.001);
  set_current_time(time_from_us(1005));
  ck_assert_float_eq_tol(calculate_tipin_enrichment(0, 0, 100), 0.0, 0.001);
}
END_TEST

START_TEST(check_calculate_tipin_overriding_event) {

  config.tipin_enrich_amount = &tipin_amount;
  config.tipin_enrich_duration = &tipin_duration;

  set_current_time(0);

  ck_assert_float_eq_tol(calculate_tipin_enrichment(0, 0, 100), 0, 0.001);
  ck_assert_float_eq_tol(calculate_tipin_enrichment(100, 50, 100), 1000, 0.001);

  /* At 100 rpm, should last 1 ms */
  set_current_time(time_from_us(500));
  /* Milder tipin should not affect anything */
  ck_assert_float_eq_tol(calculate_tipin_enrichment(100, 20, 100), 1000, 0.001);

  set_current_time(time_from_us(1005));
  ck_assert_float_eq_tol(calculate_tipin_enrichment(0, 0, 100), 0.0, 0.001);

  /* Test a higher overriding event */
  ck_assert_float_eq_tol(calculate_tipin_enrichment(100, 20, 100), 400, 0.001);

  set_current_time(time_from_us(1500));
  /* New tipin should override, since its higher */
  ck_assert_float_eq_tol(calculate_tipin_enrichment(100, 50, 100), 1000, 0.001);

  /* and should still be ongoing for the full time */
  set_current_time(time_from_us(2400));
  ck_assert_float_eq_tol(calculate_tipin_enrichment(0, 0, 100), 1000, 0.001);

  /*and now finished */
  set_current_time(time_from_us(2600));
  ck_assert_float_eq_tol(calculate_tipin_enrichment(0, 0, 100), 0.0, 0.001);
}
END_TEST

TCase *setup_calculations_tests() {
  TCase *tc = tcase_create("calculations");
  tcase_add_test(tc, check_air_density);
  tcase_add_test(tc, check_fuel_density);
  tcase_add_test(tc, check_calculate_airmass);
  tcase_add_test(tc, check_fuel_overduty);
  tcase_add_test(tc, check_calculate_ignition_cut);
  tcase_add_test(tc, check_calculate_ignition_fixedduty);

  tcase_add_test(tc, check_calculate_tipin_newevent);
  tcase_add_test(tc, check_calculate_tipin_overriding_event);
  return tc;
}
#endif
