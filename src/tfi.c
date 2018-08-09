#include <stdio.h>
#include "platform.h"
#include "util.h"
#include "decoder.h"
#include "scheduler.h"
#include "config.h"
#include "table.h"
#include "sensors.h"
#include "calculations.h"
#include "stats.h"

static void schedule(struct output_event *ev) {
  switch(ev->type) {
    case IGNITION_EVENT:
      if (ignition_cut() || !config.decoder.valid) {
        invalidate_scheduled_events(config.events, config.num_events);
        return;
      }
      schedule_ignition_event(ev, &config.decoder, 
          (degrees_t)calculated_values.timing_advance, 
          calculated_values.dwell_us);
      break;

    case FUEL_EVENT:
      if (fuel_cut() || !config.decoder.valid) {
        invalidate_scheduled_events(config.events, config.num_events);
        return;
      }
      schedule_fuel_event(ev, &config.decoder, 
        calculated_values.fueling_us);
      break;

    case ADC_EVENT:
      schedule_adc_event(ev, &config.decoder);
      break;
    default:
      break;
  }
}

int main(int argc, char *argv[]) {
  platform_load_config();
  decoder_init(&config.decoder);
  console_init();
  platform_init(argc, argv);
  initialize_scheduler();


  enable_test_trigger(FORD_TFI, 2000);

  while (1) {                   
    stats_increment_counter(STATS_MAINLOOP_RATE);

    sensors_process();
    if (config.decoder.needs_decoding_t0 || config.decoder.needs_decoding_t1) {
      stats_finish_timing(STATS_SCHED_LATENCY);
      config.decoder.decode(&config.decoder);

      if (config.decoder.valid) {
        calculate_ignition();
        calculate_fueling();
        stats_start_timing(STATS_SCHED_TOTAL_TIME);
        for (unsigned int e = 0; e < config.num_events; ++e) {
          schedule(&config.events[e]);
        }
        stats_finish_timing(STATS_SCHED_TOTAL_TIME);
      }

    } else {
      /* Check to see if any events have fired. These should be rescheduled
       * now to allow 100% duty utilization */
      stats_start_timing(STATS_SCHED_FIRED_TIME);
      for (unsigned int e = 0; e < config.num_events; ++e) {
        if (config.decoder.valid && event_has_fired(&config.events[e])) {
          schedule(&config.events[e]);
        }
      }
      stats_finish_timing(STATS_SCHED_FIRED_TIME);

      /* We want to continue adc sampling when the engine isn't spinning.
       * TODO: do this more infrequently */
      if (!config.decoder.valid) {
        adc_gather();
      }
    }
   
    console_process();
  }

  return 0;
}

