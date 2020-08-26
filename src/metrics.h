#ifndef METRICS_H
#define METRICS_H

#include <stdint.h>
#include <console.h>

struct counter_metric {
  const char *name;
  timeval_t last_gather;
  uint32_t acc;
};

struct timer_metric {
  const char *name;
  uint64_t total_cycles;
  uint32_t min;
  uint32_t max;
};

void register_counter_metric(struct counter_metric *metric, const char *name);
void register_timer_metric(struct timer_metric *metric, const char *name);

void metric_time(struct timer_metric *metric,  uint32_t cycles);
void metric_count(struct counter_metric *metric,  uint32_t amt);

void render_metrics(struct console_request_context *ctx, void *ptr);

#endif

