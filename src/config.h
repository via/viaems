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

  /* knock inputs */
  struct knock_input knock_inputs[2];

  /* Tables */
  struct table_2d *timing;
  struct table_2d *ve;
  struct table_2d *commanded_lambda;
  struct table_1d *injector_deadtime_offset;
  struct table_1d *injector_pw_correction;
  struct table_2d *engine_temp_enrich;
  struct table_1d *dwell;
  struct table_2d *tipin_enrich_amount;
  struct table_1d *tipin_enrich_duration;

  /* Fuel information */
  struct fueling_config fueling;
  struct ignition_config ignition;
  struct boost_control_config boost_control;
  struct cel_config cel;
  struct idle_control_config idle_control;

  /* Cutoffs */
  uint32_t rpm_stop;
  uint32_t rpm_start;
};

extern struct config config;

int config_valid(void);

#endif
