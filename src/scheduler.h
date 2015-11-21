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

  /* Treat event as a callback */
  void (*callback)(void *);
  void *ptr;

  /* Otherwise an output change */
  unsigned char output_id;
  unsigned char output_val;

  volatile unsigned char fired;
  volatile unsigned char scheduled;
  LIST_ENTRY(sched_entry) entries;
};
LIST_HEAD(scheduler_head, sched_entry);

struct output_event {
  event_type_t type;
  degrees_t angle;
  unsigned char output_id;

  struct sched_entry start;
  struct sched_entry stop;
};

int schedule_ignition_event(struct output_event *, struct decoder *d, 
    degrees_t advance, unsigned int usecs_dwell);
int schedule_fuel_event(struct output_event *, struct decoder *d, 
    unsigned int usecs_pw);
int schedule_adc_event(struct output_event *, struct decoder *);
void deschedule_event(struct output_event *);
timeval_t schedule_insert(timeval_t, struct sched_entry *);

void scheduler_execute();

unsigned char event_is_active(struct output_event *);
void invalidate_scheduled_events();



#endif 

