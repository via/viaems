#include <assert.h>

#include "config.h"
#include "decoder.h"
#include "platform.h"
#include "sim.h"
#include "util.h"

static uint32_t test_trigger_rpm = 0;
static timeval_t test_trigger_time = 0;

struct test_wheel_event {
  float degrees;
  trigger_type type;
};

static struct test_wheel_event test_wheel_Nminus1_next(void) {
  static int tooth = 0;

  const int N = 36;
  const float deg_per_tooth = 720.0f / (2 * N);

  struct test_wheel_event ev;
  if (tooth == 1) { /* Gap, but use the opportunity to make a cam sync pulse */
    ev = (struct test_wheel_event){ .degrees = deg_per_tooth, .type = SYNC };
  } else if (tooth == 36) {
    /* Or if its tooth 1 of the second rotation, make a gap, ... */
    ev = (struct test_wheel_event){ .degrees = 2 * deg_per_tooth,
                                    .type = TRIGGER };
    /* And skip a tooth */
  } else {
    ev = (struct test_wheel_event){ .degrees = deg_per_tooth, .type = TRIGGER };
  }

  tooth += 1;
  if (tooth >= (N * 2 - 1)) {
    tooth = 0;
  }
  return ev;
}

void sim_wakeup_callback(struct decoder *decoder) {
  if (test_trigger_rpm == 0) {
    return;
  }

  struct test_wheel_event wheel_ev = test_wheel_Nminus1_next();

  /* Handle current */
  struct trigger_event tev = { .time = test_trigger_time,
                               .type = wheel_ev.type };
  decoder_update(decoder, &tev);

  /* Schedule next */
  timeval_t delay = time_from_rpm_diff(test_trigger_rpm, wheel_ev.degrees);
  test_trigger_time += delay;

  set_sim_wakeup(test_trigger_time);
}

void set_test_trigger_rpm(uint32_t rpm) {
  test_trigger_rpm = rpm;
  test_trigger_time = current_time() + 10000;
  set_sim_wakeup(test_trigger_time);
}

uint32_t get_test_trigger_rpm() {
  return test_trigger_rpm;
}
