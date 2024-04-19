#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "platform.h"
#include <stdatomic.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"

typedef enum {
  DISABLED_EVENT,
  FUEL_EVENT,
  IGNITION_EVENT,
} event_type_t;

typedef enum {
  SCHED_UNSCHEDULED, /* Blank event, not scheduled, not valid time */
  SCHED_SCHEDULED,   /* Event is set and scheduled, but is still changable as
  long as interrupts are disabled to prevent it from being submitted  */
  SCHED_SUBMITTED,   /* Event is submitted to platform and cannot be undone, but
                        may or may not have actually fired yet */
  SCHED_FIRED,       /* Event is confirmed fired */
} sched_state_t;

struct sched_entry {
  timeval_t time;
  uint8_t pin;
  bool val;
  sched_state_t state;
};

static inline sched_state_t sched_entry_get_state(struct sched_entry *s) {
  return s->state;
}

static inline void sched_entry_set_state(struct sched_entry *s,
                                         sched_state_t state) {
  s->state = state;
}

struct engine_pump_update {
  timeval_t retire_start_time;
  timeval_t retire_length;

  timeval_t populate_start_time;
  timeval_t populate_length;
};

struct timer_callback {
  TaskHandle_t task;
  timeval_t time;
  bool scheduled;
};

struct output_event {
  event_type_t type;
  degrees_t angle;
  uint32_t pin;
  bool inverted;

  struct sched_entry start;
  struct sched_entry stop;
  struct timer_callback callback;
};

void schedule_event(struct output_event *ev);
void deschedule_event(struct output_event *);

int schedule_callback(struct timer_callback *tcb, timeval_t time);

void scheduler_callback_timer_execute(void);
void invalidate_scheduled_events(struct output_event *, int);

#ifdef UNITTEST
#include <check.h>
void check_add_buffer_tests(TCase *);
TCase *setup_scheduler_tests(void);
#endif

#endif
