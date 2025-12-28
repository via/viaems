#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "platform.h"
#include <stdatomic.h>
#include <stdbool.h>

typedef enum {
  DISABLED_EVENT,
  FUEL_EVENT,
  IGNITION_EVENT,
} event_type_t;

typedef enum {
  SCHED_UNSCHEDULED, /* Blank event, not scheduled, not valid time */

  SCHED_SCHEDULED, /* Event is set and scheduled with a valid time, but
                      can still be changed */

  SCHED_SUBMITTED, /* Event is submitted to platform and cannot be changed,
                      but may or may not have actually fired yet */

  SCHED_FIRED, /* Event is confirmed fired */
} sched_state_t;

/* Represents a single rising or falling edge for an output */
struct schedule_entry {
  timeval_t time;      /* Time specified for the event */
  uint8_t pin;         /* Pin (in the high-speed output block) specified */
  bool val;            /* True if rising edge, false if falling edge */
  sched_state_t state; /* State of the entry */
};

/* Configuration for an output event */
struct output_event_config {
  event_type_t type; /* Type of event, fuel or ignition */
  degrees_t angle;   /* Angle for event: for fuel and ignition,
                        this is the target angle to end the event */
  uint32_t pin;      /* Which pin to use for the event */
  bool inverted;     /* If true, output is active-low */
};

/* Stores the scheduling state for a single output */
struct output_event_schedule_state {
  const struct output_event_config
    *config; /* Reference to the configuration for the event */
  struct schedule_entry start; /* Schedule state for event start */
  struct schedule_entry stop;  /* Schedule state for event stop */
};

struct config;
struct calculated_values;
struct engine_position;
struct sensor_values;

void schedule_events(const struct config *config,
                     const struct calculated_values *calcs,
                     const struct engine_position *pos,
                     struct output_event_schedule_state *evs,
                     size_t n_events,
                     timeval_t start_of_schedulable_time);

void invalidate_scheduled_events(struct output_event_schedule_state evs[],
                                 int n_events);

void scheduler_init(struct output_event_schedule_state evs[], int n_evs, const struct config *config);

#ifdef UNITTEST
#include <check.h>
void check_add_buffer_tests(TCase *);
TCase *setup_scheduler_tests(void);
#endif

#endif
