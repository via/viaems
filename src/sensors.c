#include "sensors.h"
#include "config.h"
#include "platform.h"

volatile static int adc_data_ready;
volatile static int freq_data_ready;

void
sensor_process_linear(struct sensor_input *in) {
  float partial = in->raw_value / 4096.0f;
  in->processed_value = in->min + partial * (in->max - in->min);
}
  
void
sensors_process() {
  if (adc_data_ready) {
    adc_data_ready = 0;
    for (int i = 0; i < MAX_ADC_INPUTS; ++i) {
      if ((config.sensors[i].method == SENSORS_ADC) &&
          config.sensors[i].process) {
        config.sensors[i].process(&config.sensors[i]);
      }
    }
  }

}

void sensor_adc_new_data() {
  adc_data_ready = 1;
}

void sensor_freq_new_data() {
  freq_data_ready = 1;
}
