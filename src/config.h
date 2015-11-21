#ifndef _CONFIG_H
#define _CONFIG_H

#include "util.h"
#include "scheduler.h"
#include "table.h"
#include "adc.h"

#define MAX_EVENTS 24
#define MAX_ADC_INPUTS 8

typedef enum {
  FORD_TFI,
  TOYOTA_24_1_CAS,
} trigger_type;

struct config {
  /* Event list */
  unsigned int num_events;
  struct output_event events[MAX_EVENTS];

  /* Trigger setup */
  trigger_type trigger;
  degrees_t trigger_offset;
  float trigger_max_rpm_change;
  unsigned int trigger_min_rpm;
  unsigned int t0_pin;
  unsigned int t1_pin;

  /* Analog inputs */
  struct adc_input adc[MAX_ADC_INPUTS];

  /* Tables */
  struct table *timing;
  struct table *iat_timing_adjust;
  struct table *clt_timing_adjust;

  /* Cutoffs */
  unsigned int rpm_stop;
  unsigned int rpm_start;
};

extern struct config config;

#endif
