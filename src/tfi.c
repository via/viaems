#include "platform.h"
#include "util.h"
#include "decoder.h"
#include "scheduler.h"
#include "config.h"
#include "table.h"
#include "sensors.h"
#include "calculations.h"

int main() {
  decoder_init(&config.decoder);
  platform_init();

  enable_test_trigger(FORD_TFI, 1300);

  while (1) {
    sensors_process();
    if (config.decoder.needs_decoding_t0 || config.decoder.needs_decoding_t1) {
      config.decoder.decode(&config.decoder);

      if (config.decoder.valid) {

        calculate_ignition();

        for (unsigned int e = 0; e < config.num_events; ++e) {
          switch(config.events[e].type) {
            case IGNITION_EVENT:
              if (ignition_cut()) {
                invalidate_scheduled_events(config.events, config.num_events);
                continue;
              }
              schedule_ignition_event(&config.events[e], &config.decoder, 
                  (degrees_t)calculated_values.timing_advance, 
                  calculated_values.dwell_us);
              break;

            case FUEL_EVENT:
              schedule_fuel_event(&config.events[e], &config.decoder, 0);
              break;

            case ADC_EVENT:
              schedule_adc_event(&config.events[e], &config.decoder);
              break;
          }

        }
      }
    }
    console_process();
  }

  return 0;
}

