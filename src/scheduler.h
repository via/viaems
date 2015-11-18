#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "platform.h"
#include "queue.h"


typedef enum {
  FUEL_EVENT,
  IGNITION_EVENT,
} event_type_t;

struct sched_entry {
  timeval_t time;
  unsigned char output_id;
  unsigned char output_val;
  unsigned char fired;
  unsigned char scheduled;
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


/*
struct output_event events[] = {
  {IGNITION_EVENT, 0, 0,  NULL, NULL},
  {IGNITION_EVENT, 90, 0, NULL, NULL},
  {IGNITION_EVENT, 180, 0, NULL, NULL},
  {IGNITION_EVENT, 270, 0, NULL, NULL},
  {IGNITION_EVENT, 360, 0, NULL, NULL},
  {IGNITION_EVENT, 450, 0, NULL, NULL},
  {IGNITION_EVENT, 540, 0, NULL, NULL},
  {IGNITION_EVENT, 630, 0, NULL, NULL} 
};
*/

int schedule_ignition_event(struct output_event *, struct decoder *d, 
    degrees_t advance, unsigned int usecs_dwell);
void schedule_fuel_event(struct output_event *, struct decoder *d, 
    unsigned int usecs_pw);
void deschedule_event(struct output_event *);
timeval_t schedule_insert(struct scheduler_head *, timeval_t, struct sched_entry *);

void scheduler_execute();

unsigned char event_is_active(struct output_event *);



#endif 

