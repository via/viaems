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
#include "fiber.h"

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

struct fiber_condition decoder_ready __attribute__((externally_visible)) = {0};
void decode_loop(int argc) {
  while (1) {
    fiber_wait(&decoder_ready);
    if (config.decoder.needs_decoding_t0 || config.decoder.needs_decoding_t1) {
      stats_finish_timing(STATS_TRIGGER_LATENCY);
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

    }
  }
}

void reschedule_finished_loop(int argc) {
  while (1) {
    stats_start_timing(STATS_SCHED_FIRED_TIME);
    for (unsigned int e = 0; e < config.num_events; ++e) {
      if (config.decoder.valid && event_has_fired(&config.events[e])) {
        schedule(&config.events[e]);
      }
    }
    stats_finish_timing(STATS_SCHED_FIRED_TIME);
    fiber_yield();
  }
}

void console_loop(int argc) {
  while (1) {                   
    stats_increment_counter(STATS_MAINLOOP_RATE);

    sensors_process();
    if (!config.decoder.valid) {
      adc_gather();
    }
    console_process();
    fiber_yield();
  }
}

struct fiber fibers[] = {
  {.entry = console_loop, .runnable = 1, .priority = 0},
  {.entry = reschedule_finished_loop, .runnable = 1, .priority = 0},
  {.entry = decode_loop, .runnable = 1, .priority = 6},
};

int main(int argc, char *argv[]) {
  platform_load_config();
  decoder_init(&config.decoder);
  console_init();
  platform_init(argc, argv);
  initialize_scheduler();

  enable_test_trigger(FORD_TFI, 6000);

  fibers_init(fibers, 3);
  return 0;
}
