#ifndef _CONFIG_H
#define _CONFIG_H

#include "calculations.h"
#include "decoder.h"
#include "platform.h"
#include "scheduler.h"
#include "sensors.h"
#include "table.h"
#include "tasks.h"

struct config {
  /* Event list */
  struct output_event_config outputs[MAX_EVENTS];

  /* Trigger setup */
  struct decoder_config decoder;

  /* Analog inputs */
  struct sensor_configs sensors;

  /* Frequency and wheel inputs */
  struct trigger_input trigger_inputs[4];

  /* Tables */
  struct table_2d timing;
  struct table_2d ve;
  struct table_2d commanded_lambda;
  struct table_1d injector_deadtime_offset;
  struct table_1d injector_pw_correction;
  struct table_2d engine_temp_enrich;
  struct table_1d dwell;
  struct table_2d tipin_enrich_amount;
  struct table_1d tipin_enrich_duration;

  /* Fuel information */
  struct fueling_config fueling;
  struct ignition_config ignition;
  struct boost_control_config boost_control;
  struct cel_config cel;

  /* Cutoffs */
  uint32_t rpm_stop;
  uint32_t rpm_start;
};

extern struct config default_config;

#endif
