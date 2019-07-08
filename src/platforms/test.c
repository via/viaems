#include <stdlib.h>
#include <check.h>

#include "scheduler.h"
#include "platform.h"
#include "decoder.h"
#include "util.h"
#include "sensors.h"
#include "table.h"
#include "config.h"
#include "calculations.h"
#include "console.h"
#include "tasks.h"


static timeval_t curtime = 0;
static timeval_t cureventtime = 0;
static int int_on = 1;
static int output_states[16] = {0};
static int gpio_states[16] = {0};
static int current_buffer = 0;

void platform_enable_event_logging() {
}

void platform_disable_event_logging() {
}

void platform_reset_into_bootloader() {
}

void set_pwm(int pin, float val) {
}

void check_platform_reset() {
  curtime = 0;
  current_buffer = 0;
  initialize_scheduler();
}

int disable_interrupts() {
  int old = int_on;
  int_on = 0;
  return old;
}

void enable_interrupts() {
  int_on = 1;
}

timeval_t current_time() {
  return curtime;
}

timeval_t cycle_count() {
  return 0;
}

void set_current_time(timeval_t t) {
  timeval_t c = curtime;
  curtime = t;

  /* Swap buffers until we're at time t */
  while (c < ((t / 512) * 512)) {
    current_buffer = (current_buffer + 1) % 2;
    scheduler_buffer_swap();
    c += 512;
  }
}

void set_event_timer(timeval_t t) {
  cureventtime = t;
}

timeval_t get_event_timer() {
  return cureventtime;
}

void clear_event_timer() {
  cureventtime = 0;
}

void disable_event_timer() {
  cureventtime = 0;
}

int interrupts_enabled() {
  return int_on;
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
}

int current_output_buffer() {
  return current_buffer;
}

timeval_t init_output_thread(uint32_t *b0, uint32_t *b1, uint32_t len) {
  return 0;
}


void enable_test_trigger(trigger_type trig, unsigned int rpm) {
}

void platform_save_config() {

}

void platform_load_config() {

}

size_t console_read(void *ptr, size_t max) {
  return 0;
}

size_t console_write(const void *ptr, size_t max) {
  return 0;
}

void platform_init() {
  Suite *tfi_suite = suite_create("TFI");

  suite_add_tcase(tfi_suite, setup_util_tests());
  suite_add_tcase(tfi_suite, setup_table_tests());
  suite_add_tcase(tfi_suite, setup_sensor_tests());
  suite_add_tcase(tfi_suite, setup_decoder_tests());
  suite_add_tcase(tfi_suite, setup_scheduler_tests());
  suite_add_tcase(tfi_suite, setup_calculations_tests());
  suite_add_tcase(tfi_suite, setup_console_tests());
  suite_add_tcase(tfi_suite, setup_tasks_tests());
  SRunner *sr = srunner_create(tfi_suite);
  srunner_run_all(sr, CK_VERBOSE);
  exit(srunner_ntests_failed(sr));
}

