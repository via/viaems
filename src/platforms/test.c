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
static timeval_t cureventtime = 0;
static int int_disables = 0;
static int output_states[16] = { 0 };
static int gpio_states[16] = { 0 };

void platform_enable_event_logging() {}

void platform_disable_event_logging() {}

void platform_reset_into_bootloader() {}

void set_pwm(int pin, float val) {
  (void)pin;
  (void)val;
}

void check_platform_reset() {
  curtime = 0;
  memset(config.events, 0, sizeof(config.events));
  initialize_scheduler();
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

timeval_t cycle_count() {
  return 0;
}

struct sched_entry log_entries[128];
int log_entry_count = 0;

void check_reset_sched_log() {
  memset(log_entries, 0, sizeof(log_entries));
  log_entry_count = 0;
}

struct sched_entry *check_get_sched_log() {
  return log_entries;
}

static void append_sched_entry(struct sched_entry e) {
  log_entries[log_entry_count] = e;
  log_entry_count++;
}

struct slot {
  bool used;
  bool on[16];
  bool off[16];
};

void platform_output_buffer_set(struct output_buffer *b,
                                struct sched_entry *s) {
  int pos = b->first_time - curtime;
  ck_assert((pos >= 0) && (pos < 128));
  struct slot *slot = &((struct slot *)b->buf)[pos];
  if (s->val) {
    slot->on[s->pin] = true;
  } else {
    slot->off[s->pin] = true;
  }
  slot->used = true;
}

timeval_t platform_output_earliest_schedulable_time() {
  /* Round/floor to nearest 128-time buffer start, then use next one */
  return curtime / 128 * 128 + 128;
}

void set_current_time(timeval_t t) {
  /* Swap buffers until we're at time t */
  while (time_before(curtime, t)) {
    curtime++;
    if (curtime % 128 == 0) {
      struct slot slots[128];
      memset(slots, 0, sizeof(slots));
      struct output_buffer buf = {
        .first_time = curtime,
        .last_time = curtime + 128 - 1,
        .buf = slots,
      };
      scheduler_output_buffer_ready(&buf);
      for (int i = 0; i < 128; i++) {
        if (!slots[i].used) {
          continue;
        }
        for (int pin = 0; pin < 16; pin++) {
          bool pin_on = slots[i].on[pin];
          bool pin_off = slots[i].off[pin];
          /* We can't both turn on and off at the same time */
          ck_assert(!(pin_on && pin_off));
          if (pin_off || pin_on) {
            append_sched_entry((struct sched_entry){
                .time = buf.first_time + i,
                .pin = pin,
                .val = pin_on ? 1 : 0,
                });
          }
        }
      }
    }
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

void set_test_trigger_rpm(uint32_t rpm) {
  (void)rpm;
}

uint32_t get_test_trigger_rpm() {
  return 0;
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

void platform_init() {
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
