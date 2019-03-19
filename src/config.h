#ifndef _CONFIG_H
#define _CONFIG_H

#include "platform.h"
#include "decoder.h"
#include "util.h"
#include "scheduler.h"
#include "table.h"
#include "console.h"
#include "sensors.h"
#include "calculations.h"

#define MAX_EVENTS 24

struct config {
  /* Event list */
  unsigned int num_events;
  struct output_event events[MAX_EVENTS];

  /* Trigger setup */
  struct decoder decoder;

  /* Analog inputs */
  struct sensor_input sensors[NUM_SENSORS];

  /* Tables */
  struct table *timing;
  struct table *iat_timing_adjust;
  struct table *clt_timing_adjust;
  struct table *clt_pw_adjust;
  struct table *ve;
  struct table *commanded_lambda;
  struct table *injector_pw_compensation;
  struct table *engine_temp_enrich;
  struct table *tipin_enrich_factor;
  struct table *tipin_enrich_duration;

  /* Fuel information */
  struct fueling_config fueling;
  struct ignition_config ignition;

  /* Cutoffs */
  unsigned int rpm_stop;
  unsigned int rpm_start;

  /* Console config */
  struct console console;
};

extern struct config config;

/* Externally make-available tables for console */
extern struct table timing_vs_rpm_and_map;
extern struct table lambda_vs_rpm_and_map;
extern struct table ve_vs_rpm_and_map;
extern struct table injector_dead_time;
extern struct table enrich_vs_temp_and_map;
extern struct table tipin_vs_tpsrate_and_tps;
extern struct table tipin_duration_vs_rpm;

int config_valid();

#endif
