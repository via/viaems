#include "config.h"
#include "calculations.h"

struct calculated_values calculated_values;

int ignition_cut() {
  if (config.decoder.rpm >= config.rpm_stop) {
    calculated_values.rpm_limit_cut = 1;
  }
  if (config.decoder.rpm < config.rpm_start) {
    calculated_values.rpm_limit_cut = 0;
  }
  return calculated_values.rpm_limit_cut;
}

void calculate_ignition() {
  calculated_values.timing_advance = 
    interpolate_table_twoaxis(config.timing, config.decoder.rpm, 
        config.adc[ADC_MAP].processed_value);
  switch (config.dwell) {
    case DWELL_FIXED_DUTY:
      calculated_values.dwell_us = time_from_rpm_diff(config.decoder.rpm, 45) / 84;
      break;
  }
}
