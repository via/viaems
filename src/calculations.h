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

  struct table *crank_enrich_vs_temp;
  struct table *ve;
  struct table *commanded_lambda;
  struct table *injector_pw_compensation;
  struct table *engine_temp_enrich;
  struct table *tipin_enrich_amount;
  struct table *tipin_enrich_duration;

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
  dwell_type dwell_type;
  float dwell_duty;
  float dwell_us;
  uint32_t min_fire_time_us;

  struct table *timing;
  struct table *dwell;
};

struct calculated_values {
  /* Ignition */
  float timing_advance;
  uint32_t dwell_us;
  uint32_t rpm_limit_cut;

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

void calculate_ignition();
void calculate_fueling();
bool ignition_cut();
bool fuel_cut();

#ifdef UNITTEST
#include <check.h>
TCase *setup_calculations_tests();
#endif

#endif
