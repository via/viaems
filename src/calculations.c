#include <math.h>

#include "config.h"
#include "calculations.h"
#include "stats.h"
struct calculated_values calculated_values;

static int fuel_overduty() {
  /* Maximum pulse width */
  timeval_t max_pw = time_from_rpm_diff(config.decoder.rpm, 720) / config.fueling.injections_per_cycle;

  return time_from_us(calculated_values.fueling_us) >= max_pw;
}

static int rpm_limit() {
  if (config.decoder.rpm >= config.rpm_stop) {
    calculated_values.rpm_limit_cut = 1;
  }
  if (config.decoder.rpm < config.rpm_start) {
    calculated_values.rpm_limit_cut = 0;
  }
  return calculated_values.rpm_limit_cut;
}

int boost_cut() {
  return (config.sensors[SENSOR_MAP].processed_value > 200.0);
}

int ignition_cut() {
  return rpm_limit() || fuel_overduty() || boost_cut();
}

int fuel_cut() {
  return ignition_cut();
}

void calculate_ignition() {
  calculated_values.timing_advance = 
    interpolate_table_twoaxis(config.timing, config.decoder.rpm, 
        config.sensors[SENSOR_MAP].processed_value);
  switch (config.ignition.dwell) {
    case DWELL_FIXED_DUTY:
      calculated_values.dwell_us = 
        time_from_rpm_diff(config.decoder.rpm, 45) / (TICKRATE / 1000000);
      break;
    case DWELL_FIXED_TIME:
      calculated_values.dwell_us = config.ignition.dwell_us;
      break;
  }
}

static float air_density(float iat_celsius, float atmos_kpa) {
  const float kelvin_offset = 273.15;
  float temp_factor =  kelvin_offset / (iat_celsius + kelvin_offset);
  return (atmos_kpa / 100) * config.fueling.density_of_air_stp * temp_factor;
}

static float fuel_density(float fuel_celsius) {
  const float beta = 950.0; /* Gasoline 10^-6/K */
  float delta_temp = fuel_celsius - 15.0;
  return config.fueling.density_of_fuel - (delta_temp * beta / 1000000.0);
}

/* Returns mass of air injested into a cylinder */
static float calculate_airmass(float ve, float map, float aap, float iat) {

  float injested_air_volume_per_cycle = (ve / 100.0) * 
    (map / aap) * 
    config.fueling.cylinder_cc;

  float injested_air_mass_per_cycle = injested_air_volume_per_cycle *
    air_density(iat, aap);

  return injested_air_mass_per_cycle;
}

/* Given an airmass and a fuel temperature, returns stoich amount of fuel volume */
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
  } current = {0};

  if (!config.tipin_enrich_amount || !config.tipin_enrich_duration) {
    return 0.0;
  }

  float new_tipin_amount = interpolate_table_twoaxis(config.tipin_enrich_amount, 
      tpsrate, tps);

  /* Update status flag */
  if (current.active &&
      !time_in_range(current_time(), current.time, current.time +
        current.length)) {
    current.active = 0;
  }

  if ((new_tipin_amount > current.amount) ||
       !current.active) {
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
  stats_start_timing(STATS_FUELCALC_TIME);

  float ve;
  float lambda;
  float idt;
  float ete;

  float iat = config.sensors[SENSOR_IAT].processed_value;
  float brv = config.sensors[SENSOR_BRV].processed_value;
  float map = config.sensors[SENSOR_MAP].processed_value;
  float aap = config.sensors[SENSOR_AAP].processed_value;
  float frt = config.sensors[SENSOR_FRT].processed_value;
  float clt = config.sensors[SENSOR_CLT].processed_value;

  float tps = config.sensors[SENSOR_TPS].processed_value;
  float tpsrate = config.sensors[SENSOR_TPS].derivative;

  if (config.ve) {
    float load = map / aap * 100.0;
    ve = interpolate_table_twoaxis(config.ve, config.decoder.rpm, load);
  } else {
    ve = 100.0;
  }

  if (config.commanded_lambda) {
    lambda = interpolate_table_twoaxis(config.commanded_lambda, 
      config.decoder.rpm, map);
  } else {
    lambda = 1.0;
  }

  if (config.injector_pw_compensation) {
    idt = interpolate_table_oneaxis(config.injector_pw_compensation, brv);
  } else {
    idt = 1.0;
  }

  if (config.engine_temp_enrich) {
    ete = interpolate_table_twoaxis(config.engine_temp_enrich, clt, map);
  } else {
    ete = 1.0;
  }

  calculated_values.tipin = calculate_tipin_enrichment(tps, tpsrate, 
      config.decoder.rpm);

  calculated_values.airmass_per_cycle = calculate_airmass(ve, map, aap, iat);

  float fuel_vol_at_stoich = calculate_fuel_volume(
      calculated_values.airmass_per_cycle,
      frt);

  calculated_values.fuelvol_per_cycle = fuel_vol_at_stoich / lambda;

  float raw_pw_us = (calculated_values.fuelvol_per_cycle + 
      (calculated_values.tipin / 1000)) / /* Tipin unit is mm^3 */
    config.fueling.injector_cc_per_minute * 60000000 / /* uS per minute */
    config.fueling.injections_per_cycle; /* This many pulses */

  calculated_values.ete = ete;
  calculated_values.idt = idt;
  calculated_values.ve = ve;
  calculated_values.lambda = lambda;
  calculated_values.fueling_us = (raw_pw_us * ete) + (idt * 1000);

  stats_finish_timing(STATS_FUELCALC_TIME);
}

#ifdef UNITTEST
#include <check.h>
#include <stdlib.h>

START_TEST(check_air_density) {

  /* Confirm STP */
  ck_assert_float_eq_tol(air_density(0.0, 100), 1.2754e-3, 0.000001);

  ck_assert_float_eq_tol(air_density(20, 101.325), 1.2041e-3, 0.000001);
} END_TEST

START_TEST(check_fuel_density) {
  ck_assert_float_eq_tol(fuel_density(15), 0.755, 0.001);
  ck_assert_float_eq_tol(fuel_density(50), 0.721, 0.001);
} END_TEST


START_TEST(check_calculate_airmass) {

  /* Airmass for perfect VE, full map, 0 C*/
  float airmass = calculate_airmass(100, 100, 100, 0);

  /* 70 MAP should be 70% of previous airmass */
  ck_assert_float_eq_tol(calculate_airmass(100, 70, 100, 0), 
      0.7 * airmass, 0.001);

  /* 80 VE should be 80% of first airmass */
  ck_assert_float_eq_tol(calculate_airmass(80, 100, 100, 0), 
      0.8 * airmass, 0.001);

  /* 70 MAP when aap is 70 as well should be the same as 70 %*/
  ck_assert_float_eq_tol(calculate_airmass(100, 70, 70, 0), 
      0.7 * airmass, 0.001);
} END_TEST

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

} END_TEST

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
} END_TEST

START_TEST(check_calculate_ignition_fixedduty) {
  struct table t = { 
    .num_axis = 2,
    .axis = { { .num = 2, .values = {5, 10} }, { .num = 2, .values = {5, 10}} },
    .data = { .two = { {10, 10}, {10, 10} } },
  };
  config.timing = &t;
  config.ignition.dwell = DWELL_FIXED_DUTY;
  config.decoder.rpm = 6000;

  calculate_ignition();
  
  ck_assert(calculated_values.timing_advance == 10);
  /* 10 ms per rev, dwell should be 1/8 of rotation,
   * fuzzy estimate because math */
  ck_assert(abs(calculated_values.dwell_us - (10000 / 8)) < 5);
} END_TEST

static struct table tipin_amount = {
    .num_axis = 2,
    .axis = { { .num = 2, .values = {0, 100} }, { .num = 2, .values = {0, 100}} },
    .data = { .two = { {0, 0}, {0, 2000} } },
};

static struct table tipin_duration = {
    .num_axis = 1,
    .axis = { { .num = 2, .values = {100, 6000}}},
    .data = { .one = {1.0, 5.0}},
};


START_TEST(check_calculate_tipin_newevent) {

  config.tipin_enrich_amount = &tipin_amount;
  config.tipin_enrich_duration = &tipin_duration;

  set_current_time(0);

  ck_assert_float_eq_tol(calculate_tipin_enrichment(0, 0, 100), 0, 0.001);
  ck_assert_float_eq_tol(calculate_tipin_enrichment(100, 100, 100), 2000.0, 0.001);
  /* At 100 rpm, should last 1 ms */

  set_current_time(time_from_us(900));
  ck_assert_float_eq_tol(calculate_tipin_enrichment(0, 0, 100), 2000.0, 0.001);
  set_current_time(time_from_us(1005));
  ck_assert_float_eq_tol(calculate_tipin_enrichment(0, 0, 100), 0.0, 0.001);

} END_TEST

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

} END_TEST

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

