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
  NUM_SENSORS,
} sensor_input_type;

typedef enum {
  SENSOR_NONE,
  SENSOR_ADC,
  SENSOR_FREQ,
  SENSOR_DIGITAL,
  SENSOR_PWM,
  SENSOR_CONST,
} sensor_method;

struct sensor_input {
  int pin;
  sensor_method method;
  void (*process)(struct sensor_input *);

  union {
    struct {
      float min;
      float max;
    } range;
    struct table *table;
    float fixed_value;
  } params;

  uint32_t raw_value;
  float processed_value;
};

/* Process functions */
void sensor_process_linear(struct sensor_input *);
void sensor_process_freq(struct sensor_input *);
void sensor_process_table(struct sensor_input *);

void sensors_process();

void sensor_adc_new_data();
void sensor_freq_new_data();

#endif

