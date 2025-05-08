#ifndef TASKS_H
#define TASKS_H

#include <stdint.h>

struct boost_control_config {
  struct table_1d *pwm_duty_vs_rpm;
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
  IDLE_INTERFACE_NONE,
  IDLE_INTERFACE_UNIPOLAR_STEPPER,
} idle_interface_type;

typedef enum {
  IDLE_METHOD_DISABED,
  IDLE_METHOD_FIXED,
  IDLE_METHOD_OPEN_LOOP_CLT,
} idle_control_method;

struct idle_control_config {
  idle_interface_type interface_type;
  idle_control_method method;

  float fixed_value;
  uint32_t stepper_steps;

  uint32_t pin_phase_a;
  uint32_t pin_phase_b;
  uint32_t pin_phase_c;
  uint32_t pin_phase_d;

};

void handle_emergency_shutdown(void);
void run_tasks(void);

#ifdef UNITTEST
#include <check.h>
TCase *setup_tasks_tests(void);
#endif

#endif
