#ifndef _CALCULATIONS_H
#define _CALCULATIONS_H

#include "platform.h"

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

  float density_of_fuel; /* At 15 C */
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
  uint32_t min_coil_cooldown_us;
  uint32_t min_dwell_us;
};

struct calculations {
  bool rpm_cut_triggered;

  struct tipin_state {
    timeval_t time;
    timeval_t length;
    float amount;
    int active;
  } tipin;
};

struct calculated_values {
  /* Cuts */
  bool rpm_limit_cut;
  bool boost_cut;
  bool fuel_overduty_cut;

  /* Ignition */
  float timing_advance;
  uint32_t dwell_us;

  /* Fueling */
  uint32_t fueling_us;
  float tipin;
  float airmass_per_cycle;
  float fuelvol_per_cycle;
  float idt;
  float pwc;
  float lambda;
  float ve;
  float ete;
};

struct config;
struct engine_update;

struct calculated_values calculate_ignition_and_fuel(
  const struct config *config,
  const struct engine_update *update,
  struct calculations *state);

#ifdef UNITTEST
#include <check.h>
TCase *setup_calculations_tests();
#endif

#endif
