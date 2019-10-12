#ifndef _SENSORS_H
#define _SENSORS_H

#include "platform.h"
#define SENSOR_FREQ_DIVIDER 4096

typedef enum {
  SENSOR_MAP, /* Manifold Absolute Pressure */
  SENSOR_IAT, /* Intake Air Temperature */
  SENSOR_CLT, /* Coolant Temperature */
  SENSOR_BRV, /* Battery Reference Voltage */
  SENSOR_TPS, /* Throttle Position */
  SENSOR_AAP, /* Atmospheric Absolute Pressure */
  SENSOR_FRT, /* Fuel Rail Temperature */
  SENSOR_EGO, /* Exhaust Gas Oxygen */
  NUM_SENSORS,
} sensor_input_type;

typedef enum {
  SENSOR_NONE,
  SENSOR_ADC,
  SENSOR_FREQ,
  SENSOR_DIGITAL,
  SENSOR_PWM,
  SENSOR_CONST,
} sensor_source;

typedef enum {
  METHOD_LINEAR,
  METHOD_TABLE,
  METHOD_THERM,
} sensor_method;

typedef enum {
  FAULT_NONE,
  FAULT_RANGE,
  FAULT_CONN,
} sensor_fault;

struct thermistor_config {
  float bias;
  float a, b, c;
};

struct sensor_input {
  int pin;
  sensor_source source;
  sensor_method method;

  union {
    struct {
      float min;
      float max;
    } range;
    struct table *table;
    float fixed_value;
    struct thermistor_config therm;
  } params;

  uint32_t raw_value;
  float processed_value;
  float lag;
  timeval_t process_time;
  float derivative;
  struct {
    uint32_t min;
    uint32_t max;
    float fault_value;
  } fault_config;
  sensor_fault fault;
};

typedef enum {
  RISING_EDGE,
  FALLING_EDGE,
  BOTH_EDGES,
} trigger_edge;

typedef enum {
  FREQ,  /* Only generate frequency information */
  TRIGGER, /* Use as source of decoder position */
  SYNC, /* Use for position sync */
} freq_type;

struct freq_input {
  trigger_edge edge;
  freq_type type;
};

void sensors_process(sensor_source source);
uint32_t sensor_fault_status();

#ifdef UNITTEST
#include <check.h>
TCase *setup_sensor_tests();
#endif

#endif
