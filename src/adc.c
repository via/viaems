#include "adc.h"
#include "config.h"
#include "platform.h"

volatile int adc_data_ready;

void
adc_process_linear(struct adc_input *in) {
  float partial = in->raw_value / 4096.0f;
  in->processed_value = in->min + partial * (in->max - in->min);
}
  
void
adc_process() {
  if (!adc_data_ready) {
    return;
  }
  adc_data_ready = 0;
  for (int i = 0; i < MAX_ADC_INPUTS; ++i) {
    if (config.adc[i].process) {
      config.adc[i].process(&config.adc[i]);
    }
  }
}

void adc_notify() {
  adc_data_ready = 1;
}
