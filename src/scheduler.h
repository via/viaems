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
  SCHED_SCHEDULED,   /* Event is set and scheduled, but is still changable */
  SCHED_SUBMITTED,   /* Event is submitted to platform and cannot be undone, but
                        may or may not have actually fired yet */
  SCHED_FIRED,       /* Event is confirmed fired */
} sched_state_t;

struct sched_entry {
  timeval_t time;
  uint8_t pin;
  bool val;
  _Atomic sched_state_t state;
};

struct timed_callback {
  void (*callback)(void *);
  void *data;
  timeval_t time;
  bool scheduled;
};

struct output_event {
  event_type_t type;
  degrees_t angle;
  uint32_t pin;
  uint32_t inverted;

  struct sched_entry start;
  struct sched_entry stop;
  struct timed_callback callback;
};

void schedule_event(struct output_event *ev);
void deschedule_event(struct output_event *);

int schedule_callback(struct timed_callback *tcb, timeval_t time);

void scheduler_callback_timer_execute();
void initialize_scheduler();
void scheduler_output_buffer_ready(struct output_buffer *);
void invalidate_scheduled_events(struct output_event *, int);

#ifdef UNITTEST
#include <check.h>
void check_add_buffer_tests(TCase *);
TCase *setup_scheduler_tests();
#endif

#endif
