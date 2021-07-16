#ifndef TASKS_H
#define TASKS_H

#include <stdint.h>

struct boost_control_config {
  struct table *pwm_duty_vs_rpm;
  float threshhold_kpa;
  uint32_t pin;
  float overboost;
};

struct cel_config {
  uint32_t pin;

  float lean_boost_kpa;
  float lean_boost_ego;
};

void handle_emergency_shutdown(void);
void run_tasks(void);

#ifdef UNITTEST
#include <check.h>
TCase *setup_tasks_tests(void);
#endif

#endif
