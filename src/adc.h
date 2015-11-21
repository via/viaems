#ifndef _ADC_H
#define _ADC_H

#include "platform.h"

typedef enum {
  ADC_MAP,
  ADC_IAT,
  ADC_CLT,
} adc_input_type;

struct adc_input {
  int pin;
  void (*process)(struct adc_input *);

  float min, max;

  uint16_t raw_value;
  float processed_value;
};

/* Process functions */
void adc_process_linear(struct adc_input *);
void adc_process();
void adc_notify();

#endif

