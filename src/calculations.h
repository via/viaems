#ifndef _CALCULATIONS_H
#define _CALCULATIONS_H

struct fueling_config {
  float injector_cc_per_minute;
  float cylinder_cc;
  float fuel_stoich_ratio; 
  unsigned int injections_per_cycle; /* fuel quantity per shot divisor */
  unsigned int fuel_pump_pin;

  /* Constants */
  float density_of_air_stp; /* g/cc at 0C 100 kpa */
  float density_of_fuel; /* At 15 C */

  /* Intermediate values for debugging below */
};

typedef enum {
  DWELL_FIXED_DUTY,
  DWELL_FIXED_TIME,
} dwell_type;

struct ignition_config {
  dwell_type dwell;
  float dwell_duty;
  float dwell_us;
  int min_fire_time_us;
};

struct calculated_values {
  /* Ignition */
  float timing_advance;
  unsigned int dwell_us;
  int rpm_limit_cut;

  /* Fueling */
  unsigned int fueling_us;
  float tipin_factor;
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
int ignition_cut();
int fuel_cut();

#ifdef UNITTEST
#include <check.h>
TCase *setup_calculations_tests();
#endif

#endif
