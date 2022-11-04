#include "platform.h"
#include "decoder.h"
#include "config.h"
#include "sim.h"

static struct timer_callback test_trigger_callback;

struct test_wheel_event {
  float degrees;
  uint32_t trigger;
};

static struct test_wheel_event test_wheel_Nminus1_next(void) {
  static int tooth = 0;

  const int N = config.decoder.num_triggers;
  const float deg_per_tooth = 720.0f / (2 * N);

  struct test_wheel_event ev;
  if (tooth == 1) { /* Gap, but use the opportunity to make a cam sync pulse */
    ev = (struct test_wheel_event){ .degrees = deg_per_tooth, .trigger = 1 };
  } else if (tooth == 36) {
    /* Or if its tooth 1 of the second rotation, make a gap, ... */
    ev =
      (struct test_wheel_event){ .degrees = 2 * deg_per_tooth, .trigger = 0 };
    /* And skip a tooth */
  } else {
    ev = (struct test_wheel_event){ .degrees = deg_per_tooth, .trigger = 0 };
  }

  tooth += 1;
  if (tooth >= (N * 2 - 1)) {
    tooth = 0;
  }
  return ev;
}

static uint32_t test_trigger_rpm = 0;

static void execute_test_trigger(void *_w) {
  (void)_w;

  if (test_trigger_rpm == 0) {
    return;
  }

  timeval_t ev_time = test_trigger_callback.time;
  struct test_wheel_event wheel_ev = test_wheel_Nminus1_next();
  struct decoder_event ev = {
    .trigger = wheel_ev.trigger,
    .time = ev_time,
  };

  /* Handle current */
  decoder_update_scheduling(&ev, 1);

  /* Schedule next */
  timeval_t delay = time_from_rpm_diff(test_trigger_rpm, wheel_ev.degrees);
  timeval_t next = ev_time + delay;

  schedule_callback(&test_trigger_callback, next);
}

void set_test_trigger_rpm(uint32_t rpm) {
  test_trigger_rpm = rpm;
  test_trigger_callback.callback = execute_test_trigger;
  test_trigger_callback.data = NULL;
  schedule_callback(&test_trigger_callback, current_time());
}

uint32_t get_test_trigger_rpm() {
  return test_trigger_rpm;
}

