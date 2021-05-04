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

#define MAX_EVENTS 24

struct config {
  /* Event list */
  unsigned int num_events;
  struct output_event events[MAX_EVENTS];

  /* Trigger setup */
  struct decoder decoder;

  /* Analog inputs */
  struct sensor_input sensors[NUM_SENSORS];

  /* Frequency inputs */
  struct freq_input freq_inputs[4];

  /* Main Fuel/Ign */
  struct fueling_config fueling;
  struct ignition_config ignition;

  /* Tasks */
  struct boost_control_config boost_control;
  struct cel_config cel;

  /* Cutoffs */
  unsigned int rpm_stop;
  unsigned int rpm_start;
};

extern struct config config;

int config_valid();

#endif
