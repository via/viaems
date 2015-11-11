#include "util.h"
#include "queue.h"
#include "decoder.h"
#include "platform.h"
#include "scheduler.h"

static struct scheduler_head schedule = LIST_HEAD_INITIALIZER(schedule);

unsigned char event_is_active(struct output_event *ev) {
  return (ev->start->fired && !ev->stop->fired);
}

timeval_t
schedule_insert(struct scheduler_head *sched, timeval_t curtime, struct sched_entry *en) {
  struct sched_entry *cur;

  if (LIST_EMPTY(sched)) {
    LIST_INSERT_HEAD(sched, en, entries);
    en->scheduled = 1;
    en->fired = 0;
    return en->time;
  }

  /* If the entry is already in here somewhere, remove it first */
  if (en->scheduled) {
    LIST_REMOVE(en, entries);
  }

  /* Find where to insert the entry */
  LIST_FOREACH(cur, sched, entries) {
    if (time_in_range(en->time, curtime, cur->time)) {
      LIST_INSERT_BEFORE(cur, en, entries);
      en->scheduled = 1;
      en->fired = 0;
      return LIST_FIRST(sched)->time;
    }
    if (LIST_NEXT(cur, entries) == NULL) {
      /* This is the last entry in the list, insert here */
      LIST_INSERT_AFTER(cur, en, entries);
      en->scheduled = 1;
      en->fired = 0;
      return LIST_FIRST(sched)->time;
    }
  }

  /* Should not be reached */
  return LIST_FIRST(sched)->time;
}

void
schedule_ignition_event(struct output_event *ev, 
                        struct decoder *d,
                        degrees_t advance, 
                        unsigned int usecs_dwell) {
  
  degrees_t fire_angle = ev->angle - advance;
  timeval_t stop_time = d->last_trigger_time + 
    time_from_rpm_diff(d->rpm, fire_angle - d->last_trigger_rpm);  
    /*Fix to handle wrapping angle */
  timeval_t start_time = stop_time - (TICKRATE / 1000000) * usecs_dwell;

  timeval_t curtime = current_time();
  disable_interrupts();
  if (!event_is_active(ev)) {
    ev->start->time = start_time;
    schedule_insert(&schedule, curtime, ev->start);
    ev->stop->time = stop_time;
    schedule_insert(&schedule, curtime, ev->stop);
  }
  enable_interrupts();

}

void
schedule_fuel_event(struct output_event *ev, 
                    struct decoder *d, 
                    unsigned int usecs_pw) {

}

void
scheduler_execute(struct scheduler_head *ent) {

  timeval_t schedtime = get_event_timer();
  timeval_t cur;
  struct sched_entry *en = LIST_FIRST(ent);
  do {
    clear_event_timer();
    set_output(en->output_id, en->output_val);
    en->fired = 1;
    en->scheduled = 0;
    LIST_REMOVE(en, entries);
    en = LIST_FIRST(ent);
    if (en == NULL) {
      disable_event_timer();
      break;
    }

    set_event_timer(en->time);
    /* Has that time already passed? */
    cur = current_time();
  } while (time_in_range(en->time, schedtime, cur));

}
