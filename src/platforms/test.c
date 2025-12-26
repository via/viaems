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

/* Disable leak detection in asan. There are several convenience allocations,
 * but they should be single ones for the lifetime of the program */
const char *__asan_default_options(void) {
  return "detect_leaks=0";
}

void platform_reset_into_bootloader() {}

void set_sim_wakeup(timeval_t t) {
  (void)t;
}

timeval_t current_time() {
  return 0;
}

uint64_t cycle_count() {
  return 0;
}

void platform_save_config(void) {}

uint64_t cycles_to_ns(uint64_t cycles) {
  (void)cycles;
  return 0;
}

uint32_t platform_adc_samplerate(void) {
  return 5000;
}

uint32_t platform_knock_samplerate(void) {
  return 50000;
}

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

void main(int argc, char *argv[]) {
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
