#include "config.h"
#include "calculations.h"

struct calculated_values calculated_values;

int ignition_cut() {
  if (config.decoder.rpm >= config.rpm_stop) {
    calculated_values.rpm_limit_cut = 1;
  }
  if (config.decoder.rpm < config.rpm_start) {
    calculated_values.rpm_limit_cut = 0;
  }
  return calculated_values.rpm_limit_cut;
}

int fuel_cut() {
  return 0;
}

void calculate_ignition() {
  calculated_values.timing_advance = 
    interpolate_table_twoaxis(config.timing, config.decoder.rpm, 
        config.sensors[SENSOR_MAP].processed_value);
  switch (config.dwell) {
    case DWELL_FIXED_DUTY:
      calculated_values.dwell_us = 
        time_from_rpm_diff(config.decoder.rpm, 45) / (TICKRATE / 1000000);
      break;
  }
}

static float air_density(float iat_celsius, float atmos_kpa) {
  const float kelvin_offset = 273.15;
  float temp_factor =  kelvin_offset / (iat_celsius - kelvin_offset);
  return (atmos_kpa / 100) * config.fueling.density_of_air_stp / temp_factor;
}

static float fuel_density(float fuel_celsius) {
  const float kelvin_offset = 273.15;
  float temp_factor =  kelvin_offset / (fuel_celsius - kelvin_offset);
  return config.fueling.density_of_fuel / temp_factor;
}

void calculate_fueling() {
  float ve;
  float lambda;
  float atmos_kpa = 100;

  if (config.ve) {
    ve = interpolate_table_twoaxis(config.ve, config.decoder.rpm, 
      config.sensors[SENSOR_MAP].processed_value);
  } else {
    ve = 100.0;
  }

  if (config.commanded_lambda) {
    lambda = interpolate_table_twoaxis(config.commanded_lambda, 
      config.decoder.rpm, config.sensors[SENSOR_MAP].processed_value);
  } else {
    lambda = 1.0;
  }

  float injested_volume_per_cycle = (ve / 100.0) * 
    (config.sensors[SENSOR_MAP].processed_value / atmos_kpa) * 
    config.fueling.cylinder_cc;

  float injested_mass_per_cycle = injested_volume_per_cycle *
    air_density(config.sensors[SENSOR_IAT].processed_value, atmos_kpa);
  config.fueling.airmass_per_cycle = injested_mass_per_cycle;

  float required_fuelvolume_per_cycle = injested_mass_per_cycle / 
    config.fueling.fuel_stoich_ratio / fuel_density(0.0) / lambda;
  config.fueling.fuelvol_per_cycle = required_fuelvolume_per_cycle;

  float raw_pw_us = required_fuelvolume_per_cycle / 
    config.fueling.injector_cc_per_minute * 60000000 / /* uS per minute */
    config.fueling.injections_per_cycle; /* This many pulses */
  
  calculated_values.fueling_us = raw_pw_us;
}
