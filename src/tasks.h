#ifndef TASKS_H
#define TASKS_H

enum pid_windup {
  WINDUP_NONE,
  WINDUP_CLAMP,
  WINDUP_ZERO,
};

struct pid_controller {
  float p;
  float i;
  float i_max;
  float d;

  enum pid_windup windup;

  float i_accum;
  float previous_error;

  float p_val;
  float i_val;
  float d_val;
};

struct boost_control_config {
  float target_kpa;
  float min_tps;
  float min_kpa;
  struct pid_controller pid;
  int pin;
  float overboost;
  float duty;
};

struct cel_config {
  int pin;

  float lean_boost_kpa;
  float lean_boost_ego;
};

void handle_emergency_shutdown();
void run_tasks();

float perform_pid(struct pid_controller *pid, float error);

#ifdef UNITTEST
#include <check.h>
TCase *setup_tasks_tests();
#endif

#endif
