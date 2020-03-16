#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "platform.h"
//#include "queue.h"

typedef enum {
  DISABLED_EVENT,
  FUEL_EVENT,
  IGNITION_EVENT,
} event_type_t;

struct sched_entry {
  /* scheduled time of an event */
  timeval_t time;

  /* Otherwise an output change */
  unsigned char pin;
  unsigned char val;

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

struct timed_callback {
  void (*callback)(void *);
  void *data;
  timeval_t time;
  int scheduled;
};

struct output_event {
  event_type_t type;
  degrees_t angle;
  unsigned char pin;
  unsigned int inverted;

  struct sched_entry start;
  struct sched_entry stop;
  struct timed_callback callback;
};

void schedule_event(struct output_event *ev);
void deschedule_event(struct output_event *);

int schedule_callback(struct timed_callback *tcb, timeval_t time);

void scheduler_callback_timer_execute();
void initialize_scheduler();
void scheduler_buffer_swap();

int event_is_active(struct output_event *);
int event_has_fired(struct output_event *);
void invalidate_scheduled_events(struct output_event *, int);

#ifdef UNITTEST
#include <check.h>
void check_add_buffer_tests(TCase *);
TCase *setup_scheduler_tests();
#endif

#endif
