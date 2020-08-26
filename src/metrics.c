#include <metrics.h>

#define NUM_METRICS 32
static struct counter_metric *counters[NUM_METRICS];
static struct timer_metric *timers[NUM_METRICS];

static void reset_counter(struct counter_metric *m) {
  m->last_gather = current_time();
  m->acc = 0;
}

void register_counter_metric(struct counter_metric *m, const char *name) { 
  m->name = name;
  for (int i = 0; i < NUM_METRICS; i++) {
    if (!counters[i]) {
      counters[i] = m;
      break;
    }
  }
  reset_counter(m);
}

static void reset_timer(struct timer_metric *m) {
  m->total_cycles = 0;
  m->min = -1;
  m->max = 0;
  m->reports = 0;
}

void register_timer_metric(struct timer_metric *m, const char *name) {
  for (int i = 0; i < NUM_METRICS; i++) {
    if (!timers[i]) {
      timers[i] = m;
      break;
    }
  }
  m->name = name;
  reset_timer(m);
}

void metric_time(struct timer_metric *m, uint32_t cycles) {
  m->reports += 1;
  m->total_cycles += cycles;
  if (cycles > m->max) {
    m->max = cycles;
  }
  if (cycles < m->min) {
    m->min = cycles;
  }
}

void metric_count(struct counter_metric *m, uint32_t amt) {
  m->acc += amt;
}

static void render_timer_metric(struct console_request_context *ctx, void *_timer) {
  struct timer_metric *m = _timer;
  render_type_field(ctx->response, "timer");
  render_uint32_map_field(ctx, "min", "", &m->min);
  render_uint32_map_field(ctx, "max", "", &m->max);
  uint32_t avg = m->reports ? (m->total_cycles / m->reports) : 0;
  render_uint32_map_field(ctx, "avg", "", &avg);
  render_uint32_map_field(ctx, "count", "", &m->reports);
  reset_timer(m);
}

static void render_counter_metric(struct console_request_context *ctx, void *_counter) {
  struct counter_metric *m = _counter;
  render_type_field(ctx->response, "counter");
  uint32_t time = current_time() - m->last_gather;
  uint32_t rate = time ? (TICKRATE * m->acc / time) : 0;
  render_uint32_map_field(ctx, "rate", "", &rate);
  reset_counter(m);
}

void render_metrics(struct console_request_context *ctx, void *ptr) {
  (void)ptr;
  for (int i = 0; i < NUM_METRICS; i++) {
    if (timers[i] && timers[i]->reports) {
      render_map_map_field(ctx, timers[i]->name, render_timer_metric, timers[i]);
    }
  }
  for (int i = 0; i < NUM_METRICS; i++) {
    if (counters[i]) {
      render_map_map_field(ctx, counters[i]->name, render_counter_metric, counters[i]);
    }
  }
}


