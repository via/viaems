#include "util.h"
#include "queue.h"
#include "decoder.h"
#include "platform.h"
#include "scheduler.h"

static struct scheduler_head schedule = LIST_HEAD_INITIALIZER(schedule);

unsigned char event_is_active(struct output_event *ev) {
  return (ev->start.fired && !ev->stop.fired);
}

void invalidate_scheduled_events() {
  struct sched_entry *cur;
  LIST_FOREACH(cur, &schedule, entries) {
    /* For now, the hacky way to do this is to just disable the ones with 'val'
     * set to 1, so still allow current events to stop */
    if (cur->output_val) {
      cur->scheduled = 0;
    }
  }
}

static timeval_t reschedule_head(timeval_t new, timeval_t old) {
  /* Did we miss an event when interrupts were off? */
  set_event_timer(new);
  if (time_in_range(new, old, current_time())) {
    scheduler_execute();
  }
  return new;
}

timeval_t
schedule_insert(timeval_t curtime, struct sched_entry *en) {
  struct sched_entry *cur;
  timeval_t new_sched;

  if (LIST_EMPTY(&schedule)) {
    LIST_INSERT_HEAD(&schedule, en, entries);
    en->scheduled = 1;
    en->fired = 0;
    return reschedule_head(en->time, curtime);
  }

  /* If the entry is already in here somewhere, remove it first */
  if (en->scheduled) {
    LIST_REMOVE(en, entries);
  }

  /* Find where to insert the entry */
  LIST_FOREACH(cur, &schedule, entries) {
    if (time_in_range(en->time, curtime, cur->time)) {
      LIST_INSERT_BEFORE(cur, en, entries);
      en->scheduled = 1;
      en->fired = 0;
      return reschedule_head(LIST_FIRST(&schedule)->time, curtime);
    }
    if (LIST_NEXT(cur, entries) == NULL) {
      /* This is the last entry in the list, insert here */
      LIST_INSERT_AFTER(cur, en, entries);
      en->scheduled = 1;
      en->fired = 0;
      return reschedule_head(LIST_FIRST(&schedule)->time, curtime);
    }
  }

  return reschedule_head(LIST_FIRST(&schedule)->time, curtime);
}


int
schedule_ignition_event(struct output_event *ev, 
                        struct decoder *d,
                        degrees_t advance, 
                        unsigned int usecs_dwell) {
  
  timeval_t stop_time;
  timeval_t start_time;
  timeval_t max_time;
  timeval_t curtime;
  int firing_angle;

  firing_angle = clamp_angle(ev->angle - advance - 
      d->last_trigger_angle + d->offset, 720);

  stop_time = d->last_trigger_time + 
    time_from_rpm_diff(d->rpm, (degrees_t)firing_angle);
    /*Fix to handle wrapping angle */
  start_time = stop_time - (TICKRATE / 1000000) * usecs_dwell;

  curtime = current_time();
  max_time = curtime + time_from_rpm_diff(d->rpm, 720);


  disable_interrupts();
  if (!event_is_active(ev)) {
    /* If we cant schedule this event, don't try */
    if (!time_in_range(start_time, curtime, max_time)){
      enable_interrupts();
      return 0;
    }
    ev->start.time = start_time;
    ev->start.output_id = ev->output_id;
    ev->start.output_val = 1;
    schedule_insert(curtime, &ev->start);

    ev->stop.time = stop_time;
    ev->stop.output_id = ev->output_id;
    ev->stop.output_val = 0;
    schedule_insert(curtime, &ev->stop);

  }
  enable_interrupts();
  return 1;
}

void
schedule_fuel_event(struct output_event *ev, 
                    struct decoder *d, 
                    unsigned int usecs_pw) {

}

void
scheduler_execute() {

  timeval_t schedtime = get_event_timer();
  timeval_t cur;
  struct sched_entry *en = LIST_FIRST(&schedule);
  do {
    clear_event_timer();
    if (en->scheduled) {
      set_output(en->output_id, en->output_val);
      if (en->callback) {
        en->callback(en->ptr);
      }
    }
    en->fired = 1;
    en->scheduled = 0;
    LIST_REMOVE(en, entries);
    en = LIST_FIRST(&schedule);
    if (en == NULL) {
      disable_event_timer();
      break;
    }

    set_event_timer(en->time);
    /* Has that time already passed? */
    cur = current_time();
  } while (time_in_range(en->time, schedtime, cur));

}
