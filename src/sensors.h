#ifndef _SENSORS_H
#define _SENSORS_H

#include "platform.h"
#define SENSOR_FREQ_DIVIDER 4096
#define MAX_ADC_PINS 16
#define MAX_KNOCK_SAMPLES 16

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

struct sensor_config {
  uint32_t pin;
  sensor_source source;
  sensor_method method;

  struct {
    float min;
    float max;
  } range;
  float raw_min;
  float raw_max;
  struct thermistor_config therm;

  float lag;
  float const_value;

  struct {
    float min;
    float max;
    float fault_value;
  } fault_config;

  struct window_config {
    uint32_t windows_per_cycle;
    degrees_t window_opening;
    degrees_t window_offset;
  } window;
};

struct sensor_value {
  timeval_t time;
  float value;
  float derivative;
  sensor_fault fault;
};

struct sensor_state {
  const struct sensor_config *config;
  struct sensor_value output;

  bool window_collecting;
  degrees_t window_start;
  float window_accumulator;
  uint32_t window_sample_count;
};

struct knock_sensor_config {
  float frequency;
  float threshold;
};

struct knock_sensor {
  const struct knock_sensor_config *config;
  uint32_t width;
  uint32_t freq;
  float w;
  float cr;

  uint32_t n_samples;
  float sprev;
  float sprev2;
  float value;
};

struct sensor_configs {
  struct sensor_config MAP;
  struct sensor_config BRV;
  struct sensor_config IAT;
  struct sensor_config CLT;
  struct sensor_config EGO;
  struct sensor_config AAP;
  struct sensor_config TPS;
  struct sensor_config FRT;
  struct sensor_config FRP;
  struct sensor_config ETH;
  struct knock_sensor_config KNK1;
  struct knock_sensor_config KNK2;
};

struct sensors {
  struct sensor_state MAP;
  struct sensor_state BRV;
  struct sensor_state IAT;
  struct sensor_state CLT;
  struct sensor_state EGO;
  struct sensor_state AAP;
  struct sensor_state TPS;
  struct sensor_state FRT;
  struct sensor_state FRP;
  struct sensor_state ETH;
  struct knock_sensor KNK1;
  struct knock_sensor KNK2;
};

struct sensor_values {
  struct sensor_value MAP;
  struct sensor_value BRV;
  struct sensor_value IAT;
  struct sensor_value CLT;
  struct sensor_value EGO;
  struct sensor_value AAP;
  struct sensor_value TPS;
  struct sensor_value FRT;
  struct sensor_value FRP;
  struct sensor_value ETH;
  float KNK1;
  float KNK2;
};

typedef enum {
  RISING_EDGE,
  FALLING_EDGE,
  BOTH_EDGES,
} trigger_edge;

typedef enum {
  NONE,
  FREQ,    /* Only generate frequency information */
  TRIGGER, /* Use as source of decoder position and RPM */
  SYNC,    /* Use to synchronize cam or crank phase */
} trigger_type;

struct trigger_input {
  trigger_edge edge;
  trigger_type type;
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

struct engine_position;
float sensor_convert_thermistor(const struct sensor_config *in,
                                const float raw);
float sensor_convert_linear(const struct sensor_config *conf, const float raw);

void sensors_init(const struct sensor_configs *configs, struct sensors *);
void sensor_update_freq(struct sensors *,
                        const struct engine_position *,
                        const struct freq_update *);
void sensor_update_adc(struct sensors *,
                       const struct engine_position *,
                       const struct adc_update *);
void sensor_update_knock(struct sensors *, const struct knock_update *);
void knock_configure(struct knock_sensor *k);

struct sensor_values sensors_get_values(const struct sensors *);

bool sensor_has_faults(const struct sensor_values *);

#ifdef UNITTEST
#include <check.h>
TCase *setup_sensor_tests();
#endif

#endif
