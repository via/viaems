#ifndef VIAEMS_H
#define VIAEMS_H

#include "calculations.h"
#include "decoder.h"
#include "scheduler.h"
#include "sensors.h"
#include "tasks.h"

struct config;
struct schedule_entry;

/* ViaEMS state structure */
struct viaems {
  struct config *config;
  struct decoder decoder;
  struct sensors sensors;
  struct calculations calculations;
  struct tasks tasks;
  struct output_event_schedule_state events[MAX_EVENTS];
};

/* Convenience struct representing the current engine state to run engine
 * calculations and scheduling with */
struct engine_update {
  timeval_t current_time;
  struct engine_position position;
  struct sensor_values sensors;
};

/* Primary entry point for viaems engine update logic.
 * config is a pointer to a config struct
 * update is the latest engine position and sensor values
 * plan is the engine control outputs for the provided upcoming time range
 */
void viaems_reschedule(struct viaems *viaems,
                       const struct engine_update *update,
                       struct platform_plan *plan);

void viaems_init(struct viaems *v, struct config *config);

void viaems_idle(struct viaems *viaems);
#endif
