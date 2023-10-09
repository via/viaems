#include "controllers.h"
#include "config.h"
#include "decoder.h"
#include "platform.h"
#include "util.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

static StaticTask_t engine_task;
static TaskHandle_t engine_task_handle;
static StackType_t engine_task_stack[configMINIMAL_STACK_SIZE];

#define DECODE_QUEUE_SIZE 1
static StaticQueue_t decode_queue;
static struct trigger_event decode_queue_storage[DECODE_QUEUE_SIZE];
QueueHandle_t decode_queue_handle = NULL;

#define ADC_QUEUE_SIZE 1
static StaticQueue_t adc_queue;
static struct adc_update adc_queue_storage[ADC_QUEUE_SIZE];
QueueHandle_t adc_queue_handle = NULL;

#define MAIN_LOOP_TRIGGER_EVENT 1
#define MAIN_LOOP_RAW_ADC 2
#define MAIN_LOOP_RESCHEDULE 4

void publish_trigger_event(const struct trigger_event *ev) {
  if (xQueueSendFromISR(decode_queue_handle, &ev, NULL)) {
  }

  itm_debug("Pt\n");
  xTaskNotify(engine_task_handle, MAIN_LOOP_TRIGGER_EVENT, eSetBits);
}

void publish_raw_adc(const struct adc_update *ev) {
  if (xQueueSendFromISR(adc_queue_handle, &ev, NULL)) {
  }

  itm_debug("Pa\n");
  xTaskNotify(engine_task_handle, MAIN_LOOP_RAW_ADC, eSetBits);
}

void publish_reschedule() {
  itm_debug("Pr\n");
  xTaskNotify(engine_task_handle, MAIN_LOOP_RESCHEDULE, eSetBits);
}

static void engine_loop(void *unused) {

  while (true) {
    uint32_t val = 0;
    if (!xTaskNotifyWait(0, ULONG_MAX, &val, portMAX_DELAY)) {
      abort();
    }

   itm_debug("Le\n");

    bool is_new_cycle = false;
    bool is_new_sensors = false;

    if (val & MAIN_LOOP_TRIGGER_EVENT) {
      struct trigger_event ev;
      if (!xQueueReceive(decode_queue_handle, &ev, 0)) {
        abort();
      }
      itm_debug("Rt\n");

      decoder_decode(&ev);
      /* TODO: determine if new engine cycle */
      is_new_cycle = true;
    }

    if (val & MAIN_LOOP_RAW_ADC) {
      struct adc_update ev;
      if (!xQueueReceive(adc_queue_handle, &ev, 0)) {
        abort();
      }
      itm_debug("Ra\n");
      sensor_update_adc(&ev);
      is_new_sensors = true;
    }

    if (is_new_sensors || is_new_cycle) {
      calculate_ignition();
      calculate_fueling();
    }

    for (unsigned int e = 0; e < MAX_EVENTS; ++e) {
      schedule_event(&config.events[e]);
    }

   itm_debug("Fe\n");
  }
}


TaskHandle_t console_task_handle;
static StaticTask_t console_task;
static StackType_t console_task_stack[512] = { 0 };

#define CONSOLE_LOOP_TRIGGER_EVENT 1
#define CONSOLE_LOOP_RAW_ADC 2
#define CONSOLE_LOOP_RESCHEDULE 4
#define CONSOLE_LOOP_RX_READY 8
#define CONSOLE_LOOP_TX_READY 16

static void console_loop(void *_unused) {
  (void)_unused;

  while (true) {
    bool tx_ready = false;
    bool rx_ready = false;

    uint32_t val = 0;
    if (!xTaskNotifyWait(0, ULONG_MAX, &val, portMAX_DELAY)) {
      abort();
    }
    if (val & CONSOLE_LOOP_RX_READY) {
      rx_ready = true;
    }
    if (val & CONSOLE_LOOP_TX_READY) {
      tx_ready = true;
    }

#if 0
    if (rx_ready && tx_ready) {
      transmit(process_command());
      rx_ready = false;
      tx_ready = false;
    } else if (tx_ready) {
      if (console_loop_trigger_event_has_next()) {
        transmit(render_trigger_event(console_loop_trigger_event_next()));
        tx_ready = false;
      }
    }
#endif
    console_process();
  }
}

TaskHandle_t sim_task_handle;
static StaticTask_t sim_task;
static StackType_t sim_task_stack[configMINIMAL_STACK_SIZE] = { 0 };
static void sim_loop(void *_unused) {
  (void)_unused;
  while (true) {
    xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
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

TaskHandle_t tasks_task_handle;
static StaticTask_t tasks_task;
static StackType_t tasks_task_stack[configMINIMAL_STACK_SIZE] = { 0 };
void tasks_loop() {
  struct timer_callback timer = {
    .task = tasks_task_handle,
    .time = current_time(),
  };

  while (true) {
    schedule_callback(&timer, timer.time + time_from_us(10000));
    if (!xTaskNotifyWait(0, 0, NULL, pdMS_TO_TICKS(100))) {
      /* TODO: We didn't get a callback within 100 ms, something bad happened */
    }

    handle_fuel_pump();
    handle_boost_control();
    handle_idle_control();
    handle_check_engine_light();
  }
}

void start_controllers(void) {
  sim_task_handle = xTaskCreateStatic(sim_loop, "sim", configMINIMAL_STACK_SIZE, NULL, 5, sim_task_stack, &sim_task);

  decode_queue_handle = xQueueCreateStatic(DECODE_QUEUE_SIZE, sizeof(struct trigger_event), (uint8_t *)decode_queue_storage, &decode_queue);
  vQueueAddToRegistry(decode_queue_handle, "decode");

  adc_queue_handle = xQueueCreateStatic(ADC_QUEUE_SIZE, sizeof(struct adc_update), (uint8_t *)adc_queue_storage, &adc_queue);
  vQueueAddToRegistry(adc_queue_handle, "adc");

  engine_task_handle = xTaskCreateStatic(engine_loop, "engine", configMINIMAL_STACK_SIZE, NULL, 4, engine_task_stack, &engine_task);

  tasks_task_handle = xTaskCreateStatic(tasks_loop, "tasks", configMINIMAL_STACK_SIZE, NULL, 2, tasks_task_stack, &tasks_task);
  console_task_handle = xTaskCreateStatic(console_loop, "console", 512, NULL, 1, console_task_stack, &console_task);

  set_test_trigger_rpm(5000);

  vTaskStartScheduler();
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
