#ifndef _CALCULATIONS_H
#define _CALCULATIONS_H

#include <stdbool.h>
#include <stdint.h>

struct fueling_config {
  float injector_cc_per_minute;
  float cylinder_cc;
  float fuel_stoich_ratio;
  uint32_t injections_per_cycle; /* fuel quantity per shot divisor */
  uint32_t fuel_pump_pin;

  struct {
    float crank_rpm;
    float cutoff_temperature;
    float enrich_amt;
  } crank_enrich_config;

  /* Constants */
  float density_of_air_stp; /* g/cc at 0C */
  float density_of_fuel;    /* At 15 C */
};

typedef enum {
  DWELL_FIXED_DUTY,
  DWELL_FIXED_TIME,
  DWELL_BRV,
} dwell_type;

struct ignition_config {
  dwell_type dwell;
  float dwell_duty;
  float dwell_us;
  uint32_t min_fire_time_us;
};

struct calculated_values {
  /* Ignition */
  float timing_advance;
  uint32_t dwell_us;
  uint32_t rpm_limit_cut;
  uint32_t boost_cut;
  uint32_t fuel_overduty_cut;

  /* Fueling */
  uint32_t fueling_us;
  float tipin;
  float airmass_per_cycle;
  float fuelvol_per_cycle;
  float idt;
  float lambda;
  float ve;
  float ete;
};

extern struct calculated_values calculated_values;

void calculate_ignition(void);
void calculate_fueling(void);
bool ignition_cut(void);
bool fuel_cut(void);

#ifdef UNITTEST
#include <check.h>
TCase *setup_calculations_tests();
#endif

#endif
