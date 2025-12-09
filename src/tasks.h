#ifndef TASKS_H
#define TASKS_H

#include <stdint.h>

#include "platform.h"
#include "table.h"

struct boost_control_config {
  struct table_1d pwm_duty_vs_rpm;
  float enable_threshold_kpa;
  float control_threshold_kpa;
  float control_threshold_tps;
  uint32_t pin;
  float overboost;
};

struct cel_config {
  uint32_t pin;

  float lean_boost_kpa;
  float lean_boost_ego;
};

typedef enum {
  CEL_NONE,
  CEL_CONSTANT,
  CEL_SLOWBLINK,
  CEL_FASTBLINK,
} cel_state_t;

struct tasks {
  timeval_t fuelpump_last_valid;

  cel_state_t cel_state;
  timeval_t cel_time;
};

void handle_emergency_shutdown(void);

struct viaems;
struct engine_update;
struct platform_plan;

void run_tasks(struct viaems *viaems,
               const struct engine_update *update,
               struct platform_plan *plan);

#ifdef UNITTEST
#include <check.h>
TCase *setup_tasks_tests(void);
#endif

#endif
