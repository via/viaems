#include "platform.h"
#include "stats.h"
#include <stdio.h>

struct stats_entry entries[] __attribute__((externally_visible)) = {
	[STATS_INT_RATE] = {
		.name = "interrupt_rate",
	},
	[STATS_INT_PWM_RATE] = {
		.name = "pwm interrupt_rate",
	},
	[STATS_INT_USART_RATE] = {
		.name = "usart interrupt_rate",
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
	[STATS_FUELCALC_TIME] = {
		.name = "fuelcalc_time",
	},
  [STATS_INT_TOTAL_TIME] = {
    .name = "total interrupt time",
    .is_interrupt = 1,
  },
	[STATS_LAST] = {},
};

static timeval_t ticks_per_sec;
static timeval_t interrupt_time;

void stats_init(timeval_t ticks) {
  ticks_per_sec = ticks;

  for (int i = 0; i < STATS_LAST; ++i) {
    entries[i].min = (timeval_t)-1;
    entries[i].max = 0;
    entries[i].avg = 0;
    entries[i].counter = 0;
    entries[i]._window = 0;
    entries[i]._prev[0] = 0;
    entries[i]._prev[1] = 0;
    entries[i]._prev[2] = 0;
    entries[i]._prev[3] = 0;
  }
}

static void stats_update(stats_field_t type, timeval_t val) {
  if (val < entries[type].min) {
    entries[type].min = val;
  }

  if (val > entries[type].max) {
    entries[type].max = val;
  }

  entries[type]._prev[3] = entries[type]._prev[2];
  entries[type]._prev[2] = entries[type]._prev[1];
  entries[type]._prev[1] = entries[type]._prev[0];
  entries[type]._prev[0] = val;

  timeval_t total = entries[type]._prev[3] + entries[type]._prev[2] +
    entries[type]._prev[1] + entries[type]._prev[0];
  entries[type].avg = total / 4;
}

void stats_start_timing(stats_field_t type) {
  entries[type]._window = cycle_count();
  entries[type].cur_interrupt_time = interrupt_time;
}

void stats_finish_timing(stats_field_t type) {
  timeval_t time = cycle_count();
  time -= entries[type]._window;
  stats_update(type, time);

  if (entries[type].is_interrupt) {
    interrupt_time += time;
  }
}

void stats_increment_counter(stats_field_t type) {

  if (cycle_count() - entries[type]._window > ticks_per_sec) {
    /* We've reached the window edge, calculate and reset */
    stats_update(type, entries[type].counter);
    entries[type].counter = 0;
    entries[type]._window = cycle_count();
  }
  entries[type].counter++;
}
