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

static _Atomic timeval_t curtime;

static _Atomic timeval_t event_timer_time = 0;
static _Atomic bool event_timer_enabled = false;
static _Atomic bool event_timer_pending = false;

static int event_logging_enabled = 1;
static uint16_t cur_outputs = 0;

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

uint64_t cycle_count() {
  struct timespec tp;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tp);
  return (uint64_t)tp.tv_sec * 1000000000 + tp.tv_nsec;
}

uint64_t cycles_to_ns(uint64_t cycles) {
  return cycles;
}

void schedule_event_timer(timeval_t t) {
  event_timer_pending = false;
  event_timer_time = t;
  event_timer_enabled = true;
  if (time_before_or_equal(t, current_time())) {
    event_timer_pending = true;
  }
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

bool interrupts_enabled() {
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

uint32_t platform_adc_samplerate(void) {
  return 1000;
}

uint32_t platform_knock_samplerate(void) {
  return 1000;
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
      decoder_update_scheduling(0, msg.time);
      break;
    case TRIGGER1:
      sprintf(output, "# TRIGGER1 %lu\n", (unsigned long)msg.time);
      write(STDERR_FILENO, output, strlen(output));
      decoder_update_scheduling(1, msg.time);
      break;
    case SCHEDULED_EVENT:
      if (event_timer_pending) {
        event_timer_pending = false;
        scheduler_callback_timer_execute();
      }
      break;
    default:
      break;
    }

  } while (1);
}

#define MAX_SLOTS 128
static struct sched_entry output_events[64];
static size_t num_output_events = 0;
static size_t next_output_event = 0;

static void clear_output_events() {
  num_output_events = 0;
  next_output_event = 0;
}

static void add_output_event(struct sched_entry *se) {
  output_events[num_output_events] = *se;
  num_output_events++;
}

static int output_event_compare(const void *_a, const void *_b) {
  const struct sched_entry *a = _a;
  const struct sched_entry *b = _b;

  if (a->time == b->time) {
    return 0;
  }
  return time_before(a->time, b->time) ? -1 : 1;
}

static void platform_buffer_swap() {

  disable_interrupts();
  clear_output_events();
  for (int i = 0; i < MAX_EVENTS; i++) {
    struct output_event *oev = &config.events[i];
    if (sched_entry_get_state(&oev->start) == SCHED_SUBMITTED) {
      sched_entry_set_state(&oev->start, SCHED_FIRED);
    }
    if (sched_entry_get_state(&oev->stop) == SCHED_SUBMITTED) {
      sched_entry_set_state(&oev->stop, SCHED_FIRED);
    }
    if (sched_entry_get_state(&oev->start) == SCHED_SCHEDULED &&
        time_in_range(
          oev->start.time, current_time(), current_time() + MAX_SLOTS - 1)) {
      add_output_event(&oev->start);
      sched_entry_set_state(&oev->start, SCHED_SUBMITTED);
    }
    if (sched_entry_get_state(&oev->stop) == SCHED_SCHEDULED &&
        time_in_range(
          oev->stop.time, current_time(), current_time() + MAX_SLOTS - 1)) {
      add_output_event(&oev->stop);
      sched_entry_set_state(&oev->stop, SCHED_SUBMITTED);
    }
  }
  enable_interrupts();
  /* Sort the outputs by ascending time */
  qsort(output_events,
        num_output_events,
        sizeof(struct sched_entry),
        output_event_compare);
}

timeval_t platform_output_earliest_schedulable_time() {
  /* Round down to nearest MAX_SLOTS and then go to next window */
  return current_time() / MAX_SLOTS * MAX_SLOTS + MAX_SLOTS;
}

static void do_output_slots() {
  /* Only take action on first slot time */
  if (curtime % MAX_SLOTS == 0) {
    platform_buffer_swap();
  }

  uint16_t old_outputs = cur_outputs;

  while ((next_output_event < num_output_events) &&
         (output_events[next_output_event].time == current_time())) {
    struct sched_entry s = output_events[next_output_event];
    next_output_event++;
    if (s.val) {
      cur_outputs |= (1 << s.pin);
    } else {
      cur_outputs &= ~(1 << s.pin);
    }
  }
  if (old_outputs != cur_outputs) {
    char output[64];
    sprintf(output, "# OUTPUTS %lu %2x\n", (long unsigned)curtime, cur_outputs);
    write(STDERR_FILENO, output, strlen(output));
    console_record_event((struct logged_event){
      .time = curtime,
      .value = cur_outputs,
      .type = EVENT_OUTPUT,
    });
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

    curtime += 1;
    do_output_slots();

    if ((curtime % 10000) == 0) {
      run_tasks();
    }

    if (event_timer_enabled && (event_timer_time == curtime)) {
      event_timer_pending = true;
    }
    if (event_timer_pending) {
      struct event event = { .type = SCHEDULED_EVENT };
      if (write(*interrupt_fd, &event, sizeof(event)) < 0) {
        perror("write");
        exit(2);
      }
    }

  } while (1);
}

static int interrupt_pipes[2];

void platform_benchmark_init() {}

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
}
