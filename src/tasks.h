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
  struct table *ego_response_time;
  float K_u; /* Ulimate gain */
  float max_correction;
  float max_cumulative_error;
};

void handle_fuel_pump();
void handle_boost_control();
void handle_check_engine_light();
void handle_idle_control();
void handle_closed_loop_feedback();
void handle_emergency_shutdown();

#ifdef UNITTEST
#include <check.h>
TCase *setup_tasks_tests();
#endif

#endif
