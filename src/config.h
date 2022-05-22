#ifndef _CONFIG_H
#define _CONFIG_H

#include "calculations.h"
#include "console.h"
#include "decoder.h"
#include "platform.h"
#include "scheduler.h"
#include "sensors.h"
#include "table.h"
#include "tasks.h"
#include "util.h"

#define MAX_EVENTS 16

struct config {
  /* Event list */
  struct output_event events[MAX_EVENTS];

  /* Trigger setup */
  struct decoder decoder;

  /* Analog inputs */
  struct sensor_input sensors[NUM_SENSORS];

  /* Frequency inputs */
  struct freq_input freq_inputs[4];

  /* Tables */
  struct table *timing;
  struct table *ve;
  struct table *commanded_lambda;
  struct table *injector_pw_compensation;
  struct table *engine_temp_enrich;
  struct table *dwell;
  struct table *tipin_enrich_amount;
  struct table *tipin_enrich_duration;

  /* Fuel information */
  struct fueling_config fueling;
  struct ignition_config ignition;
  struct boost_control_config boost_control;
  struct cel_config cel;

  /* Cutoffs */
  uint32_t rpm_stop;
  uint32_t rpm_start;
};

extern struct config config;

int config_valid(void);

#endif
