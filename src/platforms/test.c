#include <check.h>
#include <stdlib.h>

#include "calculations.h"
#include "config.h"
#include "console.h"
#include "decoder.h"
#include "platform.h"
#include "scheduler.h"
#include "sensors.h"
#include "table.h"
#include "tasks.h"
#include "util.h"

static timeval_t curtime = 0;
static timeval_t event_timer_time = 0;
static bool event_timer_enabled = false;
bool event_timer_pending = false;
static int int_disables = 0;
static int output_states[16] = { 0 };
static int gpio_states[16] = { 0 };

/* Disable leak detection in asan. There are several convenience allocations,
 * but they should be single ones for the lifetime of the program */
const char *__asan_default_options(void) {
  return "detect_leaks=0";
}

void platform_reset_into_bootloader() {}

void set_pwm(int pin, float val) {
  (void)pin;
  (void)val;
}

void schedule_event_timer(timeval_t t) {
  event_timer_pending = false;
  event_timer_time = t;
  event_timer_enabled = true;
  if (time_before_or_equal(t, current_time())) {
    event_timer_pending = true;
  }
}

void check_platform_reset() {
  curtime = 0;
  memset(config.events, 0, sizeof(config.events));
}

void disable_interrupts() {
  int_disables += 1;
}

void enable_interrupts() {
  int_disables -= 1;
  ck_assert(int_disables >= 0);
}

timeval_t current_time() {
  return curtime;
}

uint64_t cycle_count() {
  return 0;
}

uint64_t cycles_to_ns(uint64_t cycles) {
  return cycles;
}

timeval_t platform_output_earliest_schedulable_time() {
  /* Round/floor to nearest 128-time buffer start, then use next one */
  return curtime + 1;
}

void set_current_time(timeval_t t) {
  /* All events between old time and new time that are scheduled get fired */
  for (int i = 0; i < MAX_EVENTS; i++) {
    struct output_event *oev = &config.events[i];
    if (sched_entry_get_state(&oev->start) == SCHED_SCHEDULED &&
        time_in_range(oev->start.time, current_time(), t)) {
      sched_entry_set_state(&oev->start, SCHED_FIRED);
    }
    if (sched_entry_get_state(&oev->stop) == SCHED_SCHEDULED &&
        time_in_range(oev->stop.time, current_time(), t)) {
      sched_entry_set_state(&oev->stop, SCHED_FIRED);
    }
  }
  curtime = t;
}

bool interrupts_enabled() {
  return int_disables == 0;
}

void set_output(int output, char value) {
  output_states[output] = value;
}

int get_output(int output) {
  return output_states[output];
}

void set_gpio(int output, char value) {
  gpio_states[output] = value;
}

int get_gpio(int output) {
  return gpio_states[output];
}

void adc_gather(void *_adc) {
  (void)_adc;
}

uint32_t platform_adc_samplerate(void) {
  return 5000;
}

uint32_t platform_knock_samplerate(void) {
  return 50000;
}

void platform_save_config() {}

void platform_load_config() {}

size_t console_read(void *ptr, size_t max) {
  (void)ptr;
  (void)max;
  return 0;
}

size_t console_write(const void *ptr, size_t max) {
  (void)ptr;
  (void)max;
  return 0;
}
void platform_benchmark_init() {}

void platform_init(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  Suite *viaems_suite = suite_create("ViaEMS");

  suite_add_tcase(viaems_suite, setup_util_tests());
  suite_add_tcase(viaems_suite, setup_table_tests());
  suite_add_tcase(viaems_suite, setup_sensor_tests());
  suite_add_tcase(viaems_suite, setup_decoder_tests());
  suite_add_tcase(viaems_suite, setup_scheduler_tests());
  suite_add_tcase(viaems_suite, setup_calculations_tests());
  suite_add_tcase(viaems_suite, setup_console_tests());
  suite_add_tcase(viaems_suite, setup_tasks_tests());
  SRunner *sr = srunner_create(viaems_suite);
  srunner_run_all(sr, CK_VERBOSE);
  exit(srunner_ntests_failed(sr));
}
