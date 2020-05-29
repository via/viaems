#ifndef TASKS_H
#define TASKS_H

struct boost_control_config {
  struct table *pwm_duty_vs_rpm;
  float threshhold_kpa;
  int pin;
  float overboost;
};

struct cel_config {
  int pin;

  float lean_boost_kpa;
  float lean_boost_ego;
};

struct closed_loop_config {
  /* Full table VE correction */
  struct table *ve_correction;
  struct table *ego_response_time;
  float ve_correction_max_percent;

  /* PID Pulse width correction for low load */
  float low_load_K_p;
  float low_load_K_i;
  float low_load_max_correction_us;
  float low_load_map;
};

void handle_emergency_shutdown();
void run_tasks();

#ifdef UNITTEST
#include <check.h>
TCase *setup_tasks_tests();
#endif

#endif
