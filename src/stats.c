#include <assert.h>
#include <stdio.h>

#include "platform.h"
#include "stats.h"

struct stats_entry stats_entries[] = {
	[STATS_INT_RATE] = {
		.name = "interrupt_rate",
	},
	[STATS_INT_EVENTTIMER_RATE] = {
		.name = "event timer interrupt_rate",
	},
	[STATS_INT_BUFFERSWAP_RATE] = {
		.name = "buffer swap interrupt_rate",
	},
	[STATS_CONSOLE_TIME] = {
		.name = "console processing time",
	},
	[STATS_MAINLOOP_RATE] = {
		.name = "main loop rate",
	},
	[STATS_INT_DISABLE_TIME] = {
		.name = "interrupt_disabled_time",
	},
	[STATS_INT_BUFFERSWAP_TIME] = {
		.name = "output buffer reload time",
	},
	[STATS_SCHED_TOTAL_TIME] = {
		.name = "total scheduling time after decode",
	},
	[STATS_SCHED_FIRED_TIME] = {
		.name = "total scheduling of just-fired events",
	},
	[STATS_SCHED_SINGLE_TIME] = {
		.name = "individual event scheduling time",
	},
	[STATS_DECODE_TIME] = {
		.name = "decode_time",
	},
	[STATS_SENSOR_THERM_TIME] = {
		.name = "sensor_therm_time",
	},
	[STATS_FUELCALC_TIME] = {
		.name = "fuelcalc_time",
	},
  [STATS_INT_TOTAL_TIME] = {
    .name = "total interrupt time",
    .is_interrupt = 1,
  },
	[STATS_USB_INTERRUPT_TIME] = {
		.name = "time spent in usb interrupt",
	},
	[STATS_USB_INTERRUPT_RATE] = {
		.name = "usb interrupt rate",
	},
	[STATS_CONSOLE_READ_TIME] = {
		.name = "time spent moving data from rx buffer",
	},
	[STATS_SCHEDULE_LATENCY] = {
		.name = "time from trigger to finish scheduling",
	},
	[STATS_LAST] = {},
};

static timeval_t ticks_per_sec = 1000000;

void stats_init(timeval_t ticks) {
  ticks_per_sec = ticks;

  for (int i = 0; i < STATS_LAST; ++i) {
    stats_entries[i].min = (timeval_t)-1;
    stats_entries[i].max = 0;
    stats_entries[i].avg = 0;
    stats_entries[i].counter = 0;
    stats_entries[i]._window = 0;
    stats_entries[i]._prev[0] = 0;
    stats_entries[i]._prev[1] = 0;
    stats_entries[i]._prev[2] = 0;
    stats_entries[i]._prev[3] = 0;
  }
}

static void stats_update(stats_field_t type, timeval_t val) {

  if (val < stats_entries[type].min) {
    stats_entries[type].min = val;
  }

  if (val > stats_entries[type].max) {
    stats_entries[type].max = val;
  }

  stats_entries[type]._prev[3] = stats_entries[type]._prev[2];
  stats_entries[type]._prev[2] = stats_entries[type]._prev[1];
  stats_entries[type]._prev[1] = stats_entries[type]._prev[0];
  stats_entries[type]._prev[0] = val;

  timeval_t total = stats_entries[type]._prev[3] +
                    stats_entries[type]._prev[2] +
                    stats_entries[type]._prev[1] + stats_entries[type]._prev[0];
  stats_entries[type].avg = total / 4;
}

void stats_start_timing(stats_field_t type) {
  stats_entries[type]._window = cycle_count();
}

void stats_finish_timing(stats_field_t type) {

  timeval_t time;
  time = cycle_count();
  time -= stats_entries[type]._window;

  stats_update(type, time / (ticks_per_sec / 1000000));
}

void stats_increment_counter(stats_field_t type) {

  if (cycle_count() - stats_entries[type]._window > ticks_per_sec) {
    /* We've reached the window edge, calculate and reset */
    stats_update(type, stats_entries[type].counter);
    stats_entries[type].counter = 0;
    stats_entries[type]._window = cycle_count();
  }
  stats_entries[type].counter++;
}
