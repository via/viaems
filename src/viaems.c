#include <assert.h>

#include "config.h"
#include "console.h"
#include "util.h"
#include "viaems.h"

static void populate_plan_from_events(
  struct platform_plan *plan,
  struct output_event_schedule_state *events) {
  plan->n_events = 0;
  for (int i = 0; i < MAX_EVENTS; i++) {
    {
      struct schedule_entry *start = &events[i].start;
      if ((start->state == SCHED_SCHEDULED) &&
          time_in_range(
            start->time, plan->schedulable_start, plan->schedulable_end)) {
        plan->schedule[plan->n_events++] = start;
      }
    }

    {
      struct schedule_entry *stop = &events[i].stop;
      if ((stop->state == SCHED_SCHEDULED) &&
          time_in_range(
            stop->time, plan->schedulable_start, plan->schedulable_end)) {
        plan->schedule[plan->n_events++] = stop;
      }
    }
  }
}

void viaems_reschedule(struct viaems *viaems,
                       const struct engine_update *u,
                       struct platform_plan *plan) {

  const struct config *config = viaems->config;

  run_tasks(viaems, u, plan);

  if (engine_position_is_synced(&u->position, u->current_time)) {
    struct calculated_values calcs =
      calculate_ignition_and_fuel(config, u, &viaems->calculations);

    if (calculated_values_has_cuts(&calcs)) {
      invalidate_scheduled_events(viaems->events, MAX_EVENTS);
    } else {
      schedule_events(config,
                      &calcs,
                      &u->position,
                      viaems->events,
                      MAX_EVENTS,
                      plan->schedulable_start);
    }
    record_engine_update(viaems, u, &calcs);
  } else {
    invalidate_scheduled_events(viaems->events, MAX_EVENTS);
    record_engine_update(viaems, u, NULL);
  }

  populate_plan_from_events(plan, viaems->events);
}

void viaems_init(struct viaems *v, struct config *config) {
  *v = (struct viaems){
    .config = config,
  };

  decoder_init(&config->decoder, &v->decoder);
  sensors_init(&config->sensors, &v->sensors);
  scheduler_init(v->events, MAX_EVENTS, config);
}

void viaems_idle(struct viaems *viaems, timeval_t time) {
  console_process(viaems->config, time);
}
