#include "platform.h"
#include "util.h"
#include "decoder.h"
#include "scheduler.h"
#include "config.h"
#include "table.h"
#include "adc.h"
#include "calculations.h"


int main() {
  decoder_init(&config.decoder);
  platform_init(&config.decoder);
  while (1) {
    adc_process();
    if (config.decoder.needs_decoding) {
      config.decoder.decode(&config.decoder);

      if (decoder_valid(&config.decoder)) {

        calculate_ignition();

        for (unsigned int e = 0; e < config.num_events; ++e) {
          switch(config.events[e].type) {
            case IGNITION_EVENT:
              if (ignition_cut()) {
                invalidate_scheduled_events();
                continue;
              }
              schedule_ignition_event(&config.events[e], &config.decoder, 
                  (degrees_t)calculated_values.timing_advance, 2000);
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

