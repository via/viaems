#ifndef STATS_H
#define STATS_H

struct stats_entry {
  const char *name;
  timeval_t min;
  timeval_t avg;
  timeval_t max;
  timeval_t counter;

  timeval_t _prev[4];
  timeval_t _window;

  int is_interrupt;
  timeval_t cur_interrupt_time;
};

typedef enum {
  STATS_INT_RATE,
  STATS_INT_BUFFERSWAP_TIME,
  STATS_INT_EVENTTIMER_RATE,
  STATS_INT_BUFFERSWAP_RATE,
  STATS_INT_DISABLE_TIME,
  STATS_INT_TOTAL_TIME,
  STATS_FUELCALC_TIME,
  STATS_SENSOR_THERM_TIME,
  STATS_DECODE_TIME,
  STATS_SCHED_TOTAL_TIME,
  STATS_SCHED_FIRED_TIME,
  STATS_SCHED_SINGLE_TIME,
  STATS_CONSOLE_TIME,
  STATS_CONSOLE_READ_TIME,
  STATS_MAINLOOP_RATE,
  STATS_USB_INTERRUPT_TIME,
  STATS_USB_INTERRUPT_RATE,
  STATS_SCHEDULE_LATENCY,
  STATS_LAST,
} stats_field_t;

void stats_init(timeval_t ticks_per_us);

void stats_increment_counter(stats_field_t type);
void stats_start_timing(stats_field_t type);
void stats_finish_timing(stats_field_t type);

extern struct stats_entry stats_entries[];

#endif
