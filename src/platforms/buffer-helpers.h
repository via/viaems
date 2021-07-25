#ifndef COMMON_H
#define COMMON_H

inline static void output_buffer_fired(struct output_buffer *buf) {
  struct output_event *oev;
  int i;
  for (i = 0; i < MAX_EVENTS; ++i) {
    oev = &config.events[i];

    if (atomic_load_explicit(&oev->start.state, memory_order_relaxed) == SCHED_SUBMITTED &&
        time_in_range(oev->start.time, buf->first_time, buf->last_time)) {
      platform_output_buffer_unset(buf, &oev->start);
      atomic_store_explicit(&oev->start.state, SCHED_FIRED, memory_order_relaxed);
    }
    if (atomic_load_explicit(&oev->stop.state, memory_order_relaxed) == SCHED_SUBMITTED &&
        time_in_range(oev->stop.time, buf->first_time, buf->last_time)) {
      platform_output_buffer_unset(buf, &oev->stop);
      atomic_store_explicit(&oev->stop.state, SCHED_FIRED, memory_order_relaxed);
    }
  }
}

inline static void output_buffer_ready(struct output_buffer *buf) {
  struct output_event *oev;
  int i;
  for (i = 0; i < MAX_EVENTS; ++i) {
    oev = &config.events[i];
    /* Is this an event that is scheduled for this time window? */
    if (atomic_load_explicit(&oev->start.state, memory_order_relaxed) == SCHED_SCHEDULED &&
        time_in_range(oev->start.time, buf->first_time, buf->last_time)) {
      platform_output_buffer_set(buf, &oev->start);
      atomic_store_explicit(&oev->start.state, SCHED_SUBMITTED, memory_order_relaxed);
    }
    if (atomic_load_explicit(&oev->stop.state, memory_order_relaxed) == SCHED_SCHEDULED &&
        time_in_range(oev->stop.time, buf->first_time, buf->last_time)) {
      platform_output_buffer_set(buf, &oev->stop);
      atomic_store_explicit(&oev->stop.state, SCHED_SUBMITTED, memory_order_relaxed);
    }
  }
}


#endif
