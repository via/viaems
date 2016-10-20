#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "platform.h"
#include "queue.h"


typedef enum {
  FUEL_EVENT,
  IGNITION_EVENT,
  ADC_EVENT,
} event_type_t;

struct sched_entry {
  /* scheduled time of an event */
  timeval_t time;
  int32_t jitter;
  /* Treat event as a callback */
  void (*callback)();

  /* Otherwise an output change */
  unsigned char output_id;
  unsigned char output_val;

  volatile unsigned char fired;
  volatile unsigned char scheduled; /* current time is valid */
  struct output_buffer *buffer;
};

/* Meaning of scheduled/fired:
 *   fired   |  scheduled  |  meaning
 *     0     |      0      |  new event
 *     1     |      1      |  event fired, time not updated since
 *     0     |      1      |  time updated, waiting to fire
 *     1     |      0      |  event fired, not meaningful
 */

struct output_event {
  event_type_t type;
  degrees_t angle;
  unsigned char output_id;
  unsigned int inverted;

  struct sched_entry start;
  struct sched_entry stop;
};

int schedule_ignition_event(struct output_event *, struct decoder *d, 
    degrees_t advance, unsigned int usecs_dwell);
int schedule_fuel_event(struct output_event *, struct decoder *d, 
    unsigned int usecs_pw);
int schedule_adc_event(struct output_event *, struct decoder *);
void deschedule_event(struct output_event *);
//timeval_t schedule_insert(timeval_t, timeval_t, struct sched_entry *);

void scheduler_execute();
void initialize_scheduler();
void scheduler_buffer_swap();

int event_is_active(struct output_event *);
int event_has_fired(struct output_event *);
void invalidate_scheduled_events(struct output_event *, int);

#ifdef UNITTEST
#include <check.h>
void check_add_buffer_tests(TCase *);
#endif

#endif 

