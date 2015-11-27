#ifndef _CONFIG_H
#define _CONFIG_H

#include "platform.h"
#include "decoder.h"
#include "util.h"
#include "scheduler.h"
#include "table.h"
#include "console.h"
#include "adc.h"

#define MAX_EVENTS 24
#define MAX_ADC_INPUTS 8

typedef enum {
  FORD_TFI,
  TOYOTA_24_1_CAS,
} trigger_type;

typedef enum {
  DWELL_FIXED_DUTY,
} dwell_type;

struct config {
  /* Event list */
  unsigned int num_events;
  struct output_event events[MAX_EVENTS];
  dwell_type dwell;
  float dwell_duty;

  /* Trigger setup */
  trigger_type trigger;
  struct decoder decoder;

  /* Analog inputs */
  struct adc_input adc[MAX_ADC_INPUTS];

  /* Tables */
  struct table *timing;
  struct table *iat_timing_adjust;
  struct table *clt_timing_adjust;

  /* Cutoffs */
  unsigned int rpm_stop;
  unsigned int rpm_start;

  /* Console config */
  struct console console;
};

extern struct config config;

#endif
