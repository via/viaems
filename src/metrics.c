#include <metrics.h>

#define NUM_METRICS 32
static struct counter_metric *counters[NUM_METRICS];
static struct timer_metric *timers[NUM_METRICS];

void register_counter_metric(struct counter_metric *m, const char *name) { 
  for (int i = 0; i < NUM_METRICS; i++) {
    if (!counters[i]) {
      counters[i] = m;
    }
  }

  m->name = name;
  m->last_gather = current_time();
  m->acc = 0;
}

void register_timer_metric(struct timer_metric *m, const char *name) {
  for (int i = 0; i < NUM_METRICS; i++) {
    if (!timers[i]) {
      timers[i] = m;
    }
  }
  m->name = name;
  m->total_cycles = 0;
  m->min = -1;
  m->max = 0;
}

void metric_time(struct timer_metric *m, uint32_t cycles) {
  m->total_cycles += cycles;
  if (cycles > m->max) {
    m->max = cycles;
  }
  if (cycles < m->min) {
    m->min = cycles;
  }
}

void metric_counter(struct counter_metric *m, uint32_t amt) {
  m->acc += amt;
}
