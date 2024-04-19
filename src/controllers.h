#ifndef TASKS_H
#define TASKS_H

#include <stdint.h>
#include "decoder.h"
#include "sensors.h"

#include "FreeRTOS.h"
#include "message_buffer.h"
#include "task.h"
#include "queue.h"

struct boost_control_config {
  struct table *pwm_duty_vs_rpm;
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

void handle_emergency_shutdown(void);
void run_tasks(void);

extern TaskHandle_t sim_task_handle;

struct engine_pump_update;
void publish_trigger_event(const struct trigger_event *ev);
void publish_raw_adc(const struct adc_update *ev);
void trigger_engine_pump(struct engine_pump_update *ev);

void start_controllers(void);

#ifdef UNITTEST
#include <check.h>
TCase *setup_tasks_tests(void);
#endif

#endif
