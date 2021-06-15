#include <assert.h>
#include <errno.h>
#include <fcntl.h> /* For O_* constants */
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> /* For mode constants */
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "console.h"
#include "platform.h"
#include "scheduler.h"
#include "stats.h"

static _Atomic timeval_t curtime;

static _Atomic timeval_t eventtimer_time;
static _Atomic uint32_t eventtimer_enable = 0;
static int event_logging_enabled = 1;
static uint32_t test_trigger_rpm = 0;
static _Atomic uint16_t cur_outputs = 0;

struct slot {
  uint16_t on_mask;
  uint16_t off_mask;
} __attribute__((packed)) * output_slots[2] = { 0 };
static size_t max_slots;
static _Atomic size_t cur_slot = 0;
static _Atomic size_t cur_buffer = 0;

void platform_enable_event_logging() {
  event_logging_enabled = 1;
}

void platform_disable_event_logging() {
  event_logging_enabled = 0;
}

void platform_reset_into_bootloader() {}

timeval_t current_time() {
  return curtime;
}

timeval_t cycle_count() {

  struct timespec tp;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tp);
  return (timeval_t)((uint64_t)tp.tv_sec * 1000000000 + tp.tv_nsec);
}

void set_event_timer(timeval_t t) {
  eventtimer_time = t;
  eventtimer_enable = 1;
}

timeval_t get_event_timer() {
  return eventtimer_time;
}

void clear_event_timer() {
  eventtimer_enable = 0;
}

void disable_event_timer() {
  eventtimer_enable = 0;
}

/* Used to lock the interrupt thread */
static pthread_mutex_t interrupt_mutex;

/* Used to determine when we've recursively unlocked */
static pthread_mutex_t interrupt_count_mutex;
_Atomic int interrupt_disables = 0;

void disable_interrupts() {
  if (pthread_mutex_lock(&interrupt_mutex)) {
    abort();
  }
  interrupt_disables += 1;
}

void enable_interrupts() {
  interrupt_disables -= 1;

  if (interrupt_disables < 0) {
    abort();
  }

  if (pthread_mutex_unlock(&interrupt_mutex)) {
    abort();
  }
}

int interrupts_enabled() {
  return (interrupt_disables == 0);
}

void set_output(int output, char value) {
  if (value) {
    cur_outputs |= (1 << output);
  } else {
    cur_outputs &= ~(1 << output);
  }
}

void set_gpio(int output, char value) {
  static uint16_t gpios = 0;
  uint16_t old_gpios = gpios;

  if (value) {
    gpios |= (1 << output);
  } else {
    gpios &= ~(1 << output);
  }

  if (old_gpios != gpios) {
    console_record_event((struct logged_event){
      .time = current_time(),
      .value = gpios,
      .type = EVENT_GPIO,
    });
  }
}

void set_pwm(int output, float value) {
  (void)output;
  (void)value;
}

void adc_gather() {}

timeval_t last_tx = 0;
size_t console_write(const void *buf, size_t len) {
  if (curtime - last_tx < 50) {
    return 0;
  }
  struct timespec wait = {
    .tv_nsec = 100000,
  };
  nanosleep(&wait, NULL);
  ssize_t written = -1;
  while ((written = write(STDOUT_FILENO, buf, len)) < 0)
    ;
  if (written > 0) {
    last_tx = curtime;
    return written;
  }
  return 0;
}

size_t console_read(void *buf, size_t len) {
  int s = len > 64 ? 64 : len;
  ssize_t res = read(STDIN_FILENO, buf, s);
  if (res < 0) {
    return 0;
  }
  return (size_t)res;
}

void platform_load_config() {}

void platform_save_config() {}

timeval_t init_output_thread(uint32_t *buf0, uint32_t *buf1, uint32_t len) {
  output_slots[0] = (struct slot *)buf0;
  output_slots[1] = (struct slot *)buf1;
  max_slots = len;
  return 0;
}

int current_output_buffer() {
  return cur_buffer;
}

int current_output_slot() {
  return cur_slot;
}

void set_test_trigger_rpm(uint32_t rpm) {
  test_trigger_rpm = rpm;
}

uint32_t get_test_trigger_rpm() {
  return test_trigger_rpm;
}

static struct timespec add_times(struct timespec a, struct timespec b) {
  struct timespec ret = a;
  ret.tv_nsec += b.tv_nsec;
  ret.tv_sec += b.tv_sec;
  if (ret.tv_nsec > 999999999) {
    ret.tv_nsec -= 1000000000;
    ret.tv_sec += 1;
  }
  return ret;
}

void clock_nanosleep_busywait(struct timespec until) {
  struct timespec current;

  do {
    clock_gettime(CLOCK_MONOTONIC, &current);
  } while (
    (current.tv_sec < until.tv_sec) ||
    ((current.tv_sec == until.tv_sec) && (current.tv_nsec < until.tv_nsec)));
}

typedef enum event_type {
  TRIGGER0,
  TRIGGER1,
  OUTPUT_CHANGED,
  SCHEDULED_EVENT,
} event_type;

struct event {
  event_type type;
  timeval_t time;
  uint16_t values;
};

static void do_test_trigger(int interrupt_fd) {
  static bool camsync = true;
  static timeval_t last_trigger_time = 0;
  static int trigger = 0;
  static int cycle = 0;

  if (!test_trigger_rpm) {
    return;
  }

  timeval_t time_between_teeth = time_from_rpm_diff(test_trigger_rpm, 30);
  timeval_t curtime = current_time();
  if (time_diff(curtime, last_trigger_time) < time_between_teeth) {
    return;
  }
  last_trigger_time = curtime;

  trigger++;
  if (trigger == 36) {
    trigger = 0;
    camsync = !camsync;
    cycle += 1;
  } else {
    if (trigger == 30 && camsync && cycle < 8) {
      struct event ev = { .type = TRIGGER1, .time = curtime };
      if (write(interrupt_fd, &ev, sizeof(ev)) < 0) {
        perror("write");
        exit(3);
      }
    }
    struct event ev = { .type = TRIGGER0, .time = curtime };
    if (write(interrupt_fd, &ev, sizeof(ev)) < 0) {
      perror("write");
      exit(3);
    }
  }
}

void *platform_interrupt_thread(void *_interrupt_fd) {
  int *interrupt_fd = (int *)_interrupt_fd;

  do {
    struct event msg;

    ssize_t rsize = read(*interrupt_fd, &msg, sizeof(msg));
    if (rsize < 0) {
      if (errno == EINTR) {
        continue;
      }
      perror("read");
      exit(1);
    }

    char output[64];
    switch (msg.type) {
    case TRIGGER0:
      sprintf(output, "# TRIGGER0 %lu\n", (unsigned long)msg.time);
      write(STDERR_FILENO, output, strlen(output));
      decoder_update_scheduling(
        &(struct decoder_event){ .trigger = 0, .time = msg.time }, 1);
      break;
    case TRIGGER1:
      sprintf(output, "# TRIGGER1 %lu\n", (unsigned long)msg.time);
      write(STDERR_FILENO, output, strlen(output));
      decoder_update_scheduling(
        &(struct decoder_event){ .trigger = 1, .time = msg.time }, 1);
      break;
    case SCHEDULED_EVENT:
      if (eventtimer_enable) {
        scheduler_callback_timer_execute();
      }
      break;
    default:
      break;
    }

  } while (1);
}

static void do_output_slots() {
  static uint16_t old_outputs = 0;
  static uint16_t old_fifo_on_mask = 0;
  static uint16_t old_fifo_off_mask = 0;

  if (!output_slots[0]) {
    return;
  }

  cur_slot++;
  if (cur_slot == max_slots) {
    cur_buffer = (cur_buffer + 1) % 2;
    cur_slot = 0;
    scheduler_buffer_swap();
  }

  int read_slot = (cur_slot + 1) % max_slots;
  int read_buffer = (read_slot == 0) ? !cur_buffer : cur_buffer;

  cur_outputs |= old_fifo_on_mask;
  cur_outputs &= ~old_fifo_off_mask;

  old_fifo_on_mask = output_slots[read_buffer][read_slot].on_mask;
  old_fifo_off_mask = output_slots[read_buffer][read_slot].off_mask;

  if (cur_outputs != old_outputs) {
    char output[64];
    sprintf(output, "# OUTPUTS %lu %2x\n", (long unsigned)curtime, cur_outputs);
    write(STDERR_FILENO, output, strlen(output));
    console_record_event((struct logged_event){
      .time = curtime,
      .value = cur_outputs,
      .type = EVENT_OUTPUT,
    });
    old_outputs = cur_outputs;
  }
}
/* Thread that busywaits to increment the primary timebase.  This loop is also
 * responsible for triggering event timer, buffer swap, and trigger events
 */

void *platform_timebase_thread(void *_interrupt_fd) {
  int *interrupt_fd = (int *)_interrupt_fd;
  struct timespec current_time;
  struct timespec tick_increment = {
    .tv_nsec = 250,
  };

  if (clock_gettime(CLOCK_MONOTONIC, &current_time)) {
    perror("clock_gettime");
  }

  do {
    struct timespec next_tick = add_times(current_time, tick_increment);
    clock_nanosleep_busywait(next_tick);
    current_time = next_tick;

    if (!output_slots[0]) {
      continue;
    }

    curtime += 1;

    do_output_slots();

    if ((curtime % 10000) == 0) {
      run_tasks();
    }
    do_test_trigger(*interrupt_fd);

    if (eventtimer_enable && (eventtimer_time == curtime)) {
      struct event event = { .type = SCHEDULED_EVENT };
      if (write(*interrupt_fd, &event, sizeof(event)) < 0) {
        perror("write");
        exit(2);
      }
    }

  } while (1);
}

static int interrupt_pipes[2];

void platform_init() {

  /* Initalize mutexes */
  pthread_mutexattr_t im_attr;
  pthread_mutexattr_init(&im_attr);
  pthread_mutexattr_settype(&im_attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&interrupt_mutex, &im_attr);

  pthread_mutexattr_t imc_attr;
  pthread_mutexattr_init(&imc_attr);
  pthread_mutexattr_settype(&imc_attr, PTHREAD_MUTEX_ERRORCHECK);
  pthread_mutex_init(&interrupt_count_mutex, &imc_attr);

  /* Set stdin nonblock */
  fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL, 0) | O_NONBLOCK);

  pipe(interrupt_pipes);

  pthread_t timebase;
  pthread_attr_t timebase_attr;
  pthread_attr_init(&timebase_attr);
  pthread_attr_setinheritsched(&timebase_attr, PTHREAD_EXPLICIT_SCHED);
  pthread_attr_setschedpolicy(&timebase_attr, SCHED_RR);
  struct sched_param timebase_param = {
    .sched_priority = 1,
  };
  pthread_attr_setschedparam(&timebase_attr, &timebase_param);
  if (pthread_create(&timebase,
                     &timebase_attr,
                     platform_timebase_thread,
                     &interrupt_pipes[1])) {
    perror("pthread_create");
    /* Try without realtime sched */
    if (pthread_create(
          &timebase, NULL, platform_timebase_thread, &interrupt_pipes[1])) {
      perror("pthread_create");
      exit(EXIT_FAILURE);
    }
  }

  pthread_t interrupts;
  pthread_attr_t interrupts_attr;
  pthread_attr_init(&interrupts_attr);
  pthread_attr_setinheritsched(&interrupts_attr, PTHREAD_EXPLICIT_SCHED);
  pthread_attr_setschedpolicy(&interrupts_attr, SCHED_RR);
  struct sched_param interrupts_param = {
    .sched_priority = 2,
  };
  pthread_attr_setschedparam(&interrupts_attr, &interrupts_param);
  if (pthread_create(&interrupts,
                     &interrupts_attr,
                     platform_interrupt_thread,
                     &interrupt_pipes[0])) {
    perror("pthread_create");
    /* Try without realtime sched */
    if (pthread_create(
          &interrupts, NULL, platform_interrupt_thread, &interrupt_pipes[0])) {
      perror("pthread_create");
      exit(EXIT_FAILURE);
    }
  }

  sensors_process(SENSOR_ADC);
  sensors_process(SENSOR_FREQ);
  sensors_process(SENSOR_CONST);
}
