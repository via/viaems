#include "controllers.h"
#include "config.h"
#include "decoder.h"
#include "platform.h"
#include "util.h"

#include "fiber.h"

int32_t decoder_queue;
struct trigger_event decoder_queue_data[2];

int32_t adc_queue;
struct adc_update adc_queue_data[2];

int32_t engine_pump_queue;
struct engine_pump_update engine_pump_queue_data[2];

uint32_t before_cycles __attribute__((externally_visible)) = 0;
volatile uint32_t duration __attribute__((externally_visible)) = 0;

void publish_trigger_event(const struct trigger_event *ev) {
  uak_queue_put(decoder_queue, ev);
}

__attribute__((externally_visible)) 
__attribute__((used)) 
bool do_thing(uint32_t vals) {
  int8_t val1 = vals >> 24;
  int8_t val2 = (vals & 0xff0000)  >> 16;
  int8_t val3 = (vals & 0xff00)  >> 8;
  int8_t val4 = vals & 0xff;
  val1 -= 1;
  val2 -= 1;
  val3 -= 1;
  val4 -= 1;
  if ((val1 == 0) ||
      (val2 == 0) ||
      (val3 == 0) ||
      (val4 == 0)) { 
    return true;
  } 
  return false;
}


void publish_raw_adc(const struct adc_update *ev) {
  uak_queue_put(adc_queue, ev);
}


int32_t decoder_thread;
uint32_t decoder_thread_stack[128];

static void decoder_loop(void *unused) {
  while (true) {
    struct trigger_event ev;
    uak_queue_get(decoder_queue, &ev);

    set_gpio(2, 1);
    decoder_decode(&ev);
    set_gpio(2, 0);
  }
}

int32_t sensor_thread;
uint32_t sensor_thread_stack[128];

static void sensor_loop(void *unused) {
  while (true) {
    struct adc_update ev;
    uak_queue_get(adc_queue, &ev);
    set_gpio(1, 1);
    sensor_update_adc(&ev);
    set_gpio(1, 0);
  }
}

int32_t engine_pump_thread;
uint32_t engine_pump_stack[128];

void trigger_engine_pump(struct engine_pump_update *ev) {
  uak_queue_put(engine_pump_queue, ev);
}

static void engine_loop(void *unused) {

  while (true) {
    struct engine_pump_update update;
    uak_queue_get(engine_pump_queue, &update);
    set_gpio(3, 1);

    set_gpio(5, 1);
    platform_retire_schedule(update.retire_start, update.retire_duration);
    set_gpio(5, 0);
    
    calculate_ignition();
    calculate_fueling();

    for (unsigned int e = 0; e < MAX_EVENTS; ++e) {
      schedule_event(&config.events[e]);
    }

    // Retire all events for time X-Y
    set_gpio(5, 1);
    platform_submit_schedule(update.plan_start, update.plan_duration);
    set_gpio(5, 0);
    set_gpio(3, 0);

  }
}

int32_t console_thread;
uint32_t console_thread_stack[512];

void trigger_console() {
  uak_notify_set(console_thread, 0x1);
}

static void console_loop(void *_unused) {
  (void)_unused;
  while (true) {
    uak_wait_for_notify();
    console_process();
  }
}

int32_t sim_thread;
uint32_t sim_thread_stack[128];

void trigger_sim() {
  before_cycles = cycle_count();
  uak_notify_set(sim_thread, 0x1);
}

static void sim_loop(void *_unused) {
  (void)_unused;
  while (true) {
    uak_wait_for_notify();
    duration = cycle_count() - before_cycles;
    execute_test_trigger(NULL);
  }
}

static void handle_fuel_pump() {
  static timeval_t last_valid = 0;

  /* If engine is turning (as defined by seeing a trigger in the last second),
   * keep pump on */
  timeval_t last_trigger = config.decoder.last_trigger_time;
  if (time_in_range(
        current_time(), last_trigger, last_trigger + time_from_us(1000000))) {
    last_valid = current_time();
    set_gpio(config.fueling.fuel_pump_pin, 1);
    return;
  }

  /* Allow 4 seconds of fueling */
  if (time_diff(current_time(), last_valid) < time_from_us(4000000)) {
    set_gpio(config.fueling.fuel_pump_pin, 1);
  } else {
    set_gpio(config.fueling.fuel_pump_pin, 0);
    /* Keep last valid 5 seconds behind to prevent rollover bugs */
    last_valid = current_time() - time_from_us(5000000);
  }
}

static void handle_boost_control() {
  float duty;
  float map = config.sensors[SENSOR_MAP].value;
  float tps = config.sensors[SENSOR_TPS].value;
  if (map < config.boost_control.enable_threshold_kpa) {
    /* Below the "enable" threshold, keep the valve off */
    duty = 0.0f;
  } else if (map < config.boost_control.control_threshold_kpa &&
             tps > config.boost_control.control_threshold_tps) {
    /* Above "enable", but below "control", and WOT, so keep wastegate at
     * atmostpheric pressure to improve spool time */
    duty = 100.0f;
  } else {
    /* We are controlling the wastegate with PWM */
    duty = interpolate_table_oneaxis(config.boost_control.pwm_duty_vs_rpm,
                                     config.decoder.rpm);
  }
  set_pwm(config.boost_control.pin, duty);
}

static void handle_idle_control() {}

/* Checks for a variety of failure conditions, and produces a check engine
 * output:
 *
 * Sensor input in fault - 3s constant steady light
 * Decoder loss - 3s of 1/2s blink
 * Lean in boost - 3s of 1/5s blink
 */

typedef enum {
  CEL_NONE,
  CEL_CONSTANT,
  CEL_SLOWBLINK,
  CEL_FASTBLINK,
} cel_state_t;

static cel_state_t cel_state = CEL_NONE;
static timeval_t last_cel = 0;

static cel_state_t determine_next_cel_state() {
  static cel_state_t next_cel_state;

  int sensor_in_fault = (sensor_fault_status() > 0);
  int decode_loss = !config.decoder.valid && (config.decoder.rpm > 0);
  int lean_in_boost =
    (config.sensors[SENSOR_MAP].value > config.cel.lean_boost_kpa) &&
    (config.sensors[SENSOR_EGO].value > config.cel.lean_boost_ego);

  /* Translate CEL condition to CEL blink state */
  if (lean_in_boost) {
    next_cel_state = CEL_FASTBLINK;
  } else if (decode_loss) {
    next_cel_state = CEL_SLOWBLINK;
  } else if (sensor_in_fault) {
    next_cel_state = CEL_CONSTANT;
  } else {
    next_cel_state = CEL_NONE;
  }
  return next_cel_state;
}

static int determine_cel_pin_state(cel_state_t state,
                                   timeval_t cur_time,
                                   timeval_t cel_start_time) {
  switch (state) {
  case CEL_NONE:
    return 0;
  case CEL_CONSTANT:
    return 1;
  case CEL_SLOWBLINK:
    return (cur_time - cel_start_time) / time_from_us(500000) % 2;
  case CEL_FASTBLINK:
    return (cur_time - cel_start_time) / time_from_us(200000) % 2;
  default:
    return 1;
  }
}

static void handle_check_engine_light() {

  cel_state_t next_cel_state = determine_next_cel_state();

  /* Handle 3s reset of CEL state */
  if (time_diff(current_time(), last_cel) > time_from_us(3000000)) {
    cel_state = CEL_NONE;
  }

  /* Are we transitioning to a higher state? If so reset blink timer */
  if (next_cel_state > cel_state) {
    cel_state = next_cel_state;
    last_cel = current_time();
  }

  set_gpio(config.cel.pin,
           determine_cel_pin_state(cel_state, current_time(), last_cel));
}

void handle_emergency_shutdown() {

  /* Fuel pump off */
  set_gpio(config.fueling.fuel_pump_pin, 0);

  /* Stop events */
  invalidate_scheduled_events(config.events, MAX_EVENTS);

  /* TODO evaluate what to do about ignition outputs
   * for now, make sure fuel injectors are off */
  for (unsigned int i = 0; i < MAX_EVENTS; ++i) {
    if (config.events[i].type == FUEL_EVENT) {
      set_output(config.events[i].pin, config.events[i].inverted);
    }
  }
}

void tasks_loop() {
  struct timer_callback timer = {
    .time = current_time(),
  };

  while (true) {
    schedule_callback(&timer, timer.time + time_from_us(10000));

    handle_fuel_pump();
    handle_boost_control();
    handle_idle_control();
    handle_check_engine_light();
  }
}

int32_t t1, t2;
uint32_t t1_stack[128];
uint32_t t2_stack[128];

int32_t q1;
struct adc_update q1_data[2];

__attribute__((naked))
static inline uint32_t syscall2(uint32_t number,
                      uint32_t arg1,
                      uint32_t arg2) {
  __asm__("svc 0\n"
          "bx lr\n");
}

__attribute__((naked))

static inline uint32_t syscall1(uint32_t number,
                      uint32_t arg1) {
  __asm__("svc 0\n"
          "bx lr\n");
}

void t1_loop(void *) {
  while (true) {
    uint32_t c = cycle_count();
//    uak_queue_put(q1, &u);
//    uak_notify_set(t2, c);
      syscall2(1, t2, c);
#if 0
    __asm__("mov r0, %0\n"
            "mov r1, %1\n"
            "svc 0\n"
            : : "r"(t2), "r"(c) : "r0", "r1");
#endif

    uak_wait_for_notify();

  }
}

void t2_loop(void *) {
  while (true) {
//    uint32_t value;
//    struct adc_update u;
//    uak_queue_get(q1, &u);
    uint32_t value = syscall2(2, t2, 0xdeadbeef);
    duration = cycle_count() - value;
    uak_notify_set(t1, 1);
  }
}

int32_t idle_thread;
uint32_t idle_loop_stack[32];
static void idle_loop(void *unused) {
  while (true) {
  }
}

void start_controllers(void) {
#if 0
  decoder_queue = uak_queue_create(decoder_queue_data, sizeof(struct trigger_event), 2);
  adc_queue = uak_queue_create(adc_queue_data, sizeof(struct adc_update), 2);
  engine_pump_queue = uak_queue_create(engine_pump_queue_data, sizeof(struct engine_pump_update), 2);

  decoder_thread = uak_fiber_create(decoder_loop, 0, 1, decoder_thread_stack, sizeof(decoder_thread_stack));
  sensor_thread = uak_fiber_create(sensor_loop, 0, 1, sensor_thread_stack, sizeof(sensor_thread_stack));
  engine_pump_thread = uak_fiber_create(engine_loop, 0, 1, engine_pump_stack, sizeof(engine_pump_stack));

  console_thread = uak_fiber_create(console_loop, 0, 2, console_thread_stack, sizeof(console_thread_stack));
  sim_thread = uak_fiber_create(sim_loop, 0, 0, sim_thread_stack, sizeof(sim_thread_stack));

  idle_thread = uak_fiber_create(idle_loop, 0, 3, idle_loop_stack, sizeof(idle_loop_stack));

#endif

#if 1
  q1 = uak_queue_create(q1_data, sizeof(struct adc_update), 2);
  t1 = uak_fiber_create(t1_loop, 0, 2, t1_stack, sizeof(t1_stack));
  t2 = uak_fiber_create(t2_loop, 0, 1, t2_stack, sizeof(t2_stack));
#endif

  platform_init(0, NULL);
//  set_test_trigger_rpm(5000);

  void fiber_md_start(void);
  fiber_md_start();
}





#ifdef UNITTEST
#include <check.h>

START_TEST(check_tasks_handle_fuel_pump) {

  /* Initial conditions */
  config.fueling.fuel_pump_pin = 1;
  ck_assert_int_eq(get_gpio(1), 0);

  set_current_time(time_from_us(500));
  handle_fuel_pump();
  ck_assert_int_eq(get_gpio(1), 1);

  /* Wait 4 seconds, should turn off */
  set_current_time(time_from_us(4001000));
  handle_fuel_pump();
  ck_assert_int_eq(get_gpio(1), 0);

  /* Wait 4 more seconds, should still be off */
  set_current_time(time_from_us(8000000));
  handle_fuel_pump();
  ck_assert_int_eq(get_gpio(1), 0);

  /* Have a recent trigger */
  config.decoder.state = DECODER_RPM;
  config.decoder.last_trigger_time = current_time() - 500000;
  handle_fuel_pump();
  ck_assert_int_eq(get_gpio(1), 1);

  /* Keep waiting, should shut off */
  set_current_time(time_from_us(15000000));
  handle_fuel_pump();
  ck_assert_int_eq(get_gpio(1), 0);

  /* Keep waiting, should stay shut off */
  set_current_time(time_from_us(200000000));
  handle_fuel_pump();
  ck_assert_int_eq(get_gpio(1), 0);
}
END_TEST

START_TEST(check_tasks_next_cel_state) {
  /* no fault */
  config.decoder.valid = 1;
  ck_assert_int_eq(determine_next_cel_state(), CEL_NONE);

  /* Sensor fault */
  config.sensors[SENSOR_MAP].fault = FAULT_RANGE;
  ck_assert_int_eq(determine_next_cel_state(), CEL_CONSTANT);

  /* Decoder loss */
  config.decoder.valid = 0;
  config.decoder.rpm = 1000;
  config.sensors[SENSOR_MAP].fault = FAULT_NONE;
  ck_assert_int_eq(determine_next_cel_state(), CEL_SLOWBLINK);

  /* Still decoder loss, add sensor fault */
  config.sensors[SENSOR_MAP].fault = FAULT_RANGE;
  ck_assert_int_eq(determine_next_cel_state(), CEL_SLOWBLINK);

  /* Lean in boost */
  config.sensors[SENSOR_MAP].fault = FAULT_NONE;
  config.decoder.valid = 1;
  config.sensors[SENSOR_MAP].value = 180;
  config.sensors[SENSOR_EGO].value = 1.1;
  ck_assert_int_eq(determine_next_cel_state(), CEL_FASTBLINK);
}
END_TEST

START_TEST(check_tasks_cel_pin_state) {

  /* with no cel, time is irrelevent */
  ck_assert_int_eq(determine_cel_pin_state(CEL_NONE, 0, 0), 0);

  /* With constant, time still irrelevent */
  ck_assert_int_eq(determine_cel_pin_state(CEL_CONSTANT, 0, 0), 1);

  /* Slowblink should be on at .75 s, off at 1.25 s */
  ck_assert_int_eq(determine_cel_pin_state(CEL_SLOWBLINK,
                                           time_from_us(10750000),
                                           time_from_us(10000000)),
                   1);
  ck_assert_int_eq(determine_cel_pin_state(CEL_SLOWBLINK,
                                           time_from_us(11250000),
                                           time_from_us(10000000)),
                   0);

  /* Fastblink should be on at 1.1 s, off at 1.3 s */
  ck_assert_int_eq(determine_cel_pin_state(CEL_FASTBLINK,
                                           time_from_us(11100000),
                                           time_from_us(10000000)),
                   1);
  ck_assert_int_eq(determine_cel_pin_state(CEL_FASTBLINK,
                                           time_from_us(11300000),
                                           time_from_us(10000000)),
                   0);
}
END_TEST

TCase *setup_tasks_tests() {
  TCase *tasks_tests = tcase_create("tasks");
  tcase_add_test(tasks_tests, check_tasks_handle_fuel_pump);
  tcase_add_test(tasks_tests, check_tasks_next_cel_state);
  tcase_add_test(tasks_tests, check_tasks_cel_pin_state);
  return tasks_tests;
}

#endif
