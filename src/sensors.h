#ifndef _SENSORS_H
#define _SENSORS_H

#include "platform.h"
#define SENSOR_FREQ_DIVIDER 4096
#define MAX_ADC_PINS 16
#define MAX_KNOCK_SAMPLES 16

typedef enum {
  SENSOR_MAP, /* Manifold Absolute Pressure */
  SENSOR_IAT, /* Intake Air Temperature */
  SENSOR_CLT, /* Coolant Temperature */
  SENSOR_BRV, /* Battery Reference Voltage */
  SENSOR_TPS, /* Throttle Position */
  SENSOR_AAP, /* Atmospheric Absolute Pressure */
  SENSOR_FRT, /* Fuel Rail Temperature */
  SENSOR_EGO, /* Exhaust Gas Oxygen */
  SENSOR_FRP, /* Fuel Rail Pressure */
  SENSOR_ETH, /* Ethanol percentage */
  NUM_SENSORS,
} sensor_input_type;

typedef enum {
  SENSOR_NONE,
  SENSOR_ADC,
  SENSOR_FREQ,
  SENSOR_PULSEWIDTH,
  SENSOR_CONST,
} sensor_source;

typedef enum {
  METHOD_LINEAR,
  METHOD_LINEAR_WINDOWED,
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
  uint32_t pin;
  sensor_source source;
  sensor_method method;

  struct {
    float min;
    float max;
  } range;
  float raw_min;
  float raw_max;
  struct table *table;
  struct thermistor_config therm;

  float lag;
  struct {
    float min;
    float max;
    float fault_value;
  } fault_config;
  struct window {
    uint32_t capture_width;
    uint32_t total_width;
    uint32_t offset;

    float accumulator;
    uint32_t samples;
    uint8_t collecting;
    degrees_t collection_start_angle;
  } window;

  timeval_t time;
  float raw_value;
  float value;
  float derivative;

  sensor_fault fault;
};

typedef enum {
  RISING_EDGE,
  FALLING_EDGE,
  BOTH_EDGES,
} trigger_edge;

typedef enum {
  NONE,
  FREQ,    /* Only generate frequency information */
  TRIGGER, /* Use as source of decoder position */
} freq_type;

struct freq_input {
  trigger_edge edge;
  freq_type type;
};

struct knock_input_state {
  uint32_t width;
  uint32_t freq;
  float w;
  float cr;

  uint32_t n_samples;
  float sprev;
  float sprev2;
};

struct knock_input {
  float frequency;
  float threshold;

  struct knock_input_state state;
  float value;
};

struct freq_update {
  timeval_t time;
  bool valid;

  uint32_t pin;
  float frequency;  /* Hz */
  float pulsewidth; /* seconds */
};

struct adc_update {
  timeval_t time;
  bool valid;
  float values[MAX_ADC_PINS]; /* Pin values in V */
};

struct knock_update {
  timeval_t time; /* Time of first sample in set */
  bool valid;
  int pin;
  int n_samples;
  float samples[MAX_KNOCK_SAMPLES];
};

void sensor_update_freq(const struct freq_update *);
void sensor_update_adc(const struct adc_update *);
void sensor_update_knock(const struct knock_update *);

uint32_t sensor_fault_status();

void knock_configure(struct knock_input *);

#ifdef UNITTEST
#include <check.h>
TCase *setup_sensor_tests();
#endif

#endif
