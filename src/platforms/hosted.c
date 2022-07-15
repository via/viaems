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

#include <netinet/ip.h>
#include <sys/socket.h>

#include "config.h"
#include "console.h"
#include "platform.h"
#include "scheduler.h"
#include "stats.h"

static _Atomic timeval_t curtime;

static _Atomic timeval_t eventtimer_time;
static _Atomic uint32_t eventtimer_enable = 0;
static int event_logging_enabled = 1;
static uint32_t test_trigger_rpm = 1000;
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

static int console_socket_bind_fd = -1;
static int console_socket_fd = -1;

timeval_t last_tx = 0;
size_t console_write(const void *buf, size_t len) {
  if (console_socket_fd < 0) {
    return len;
    /* Write to nowhere */
  }
  struct timespec wait = {
    .tv_nsec = 100000,
  };
  nanosleep(&wait, NULL);
  ssize_t written = -1;
  while ((written = write(console_socket_fd, buf, len)) < 0)
    ;
  if (written > 0) {
    last_tx = curtime;
    return written;
  }
  return 0;
}

size_t console_read(void *buf, size_t len) {
  
  /* If we don't currently have an active socket, try to accept */
  if (console_socket_fd < 0) {
    console_socket_fd = accept(console_socket_bind_fd, NULL, NULL);
    if (console_socket_fd < 0) {
      return 0;
    }
    /* Set nonblock */
    fcntl(console_socket_fd, F_SETFL, fcntl(console_socket_fd, F_GETFL, 0) | O_NONBLOCK);

  }

  int s = len > 64 ? 64 : len;
  ssize_t res = read(console_socket_fd, buf, s);
  if (res < 0) {
    return 0;
  }
  return (size_t)res;
}

void platform_load_config() {}

void platform_save_config() {}

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

  timeval_t time_between_teeth = time_from_rpm_diff(test_trigger_rpm, 10);
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
    if (trigger == 30 && camsync) {
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

static void platform_buffer_swap(timeval_t new_start, timeval_t new_stop) {

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
          oev->start.time, new_start, new_stop)) {
      add_output_event(&oev->start);
      sched_entry_set_state(&oev->start, SCHED_SUBMITTED);
    }
    if (sched_entry_get_state(&oev->stop) == SCHED_SCHEDULED &&
        time_in_range(
          oev->stop.time, new_start, new_stop)) {
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

static void report_output_event(int fd, timeval_t time, uint16_t outputs) {
  char output[64];
  sprintf(output, "# OUTPUTS %lu %2x\n", (long unsigned)time, outputs);
  write(fd, output, strlen(output));
  console_record_event((struct logged_event){
      .time = time,
      .value = outputs,
      .type = EVENT_OUTPUT,
      });
}

static void do_output_slots(timeval_t from, timeval_t to) {
  /* Only take action on first slot time */
  platform_buffer_swap(from, to);

  for (unsigned int i = 0; i < num_output_events; i++) {
    struct sched_entry *s = &output_events[i];
    /* Update outputs */
    if (s->val) {
      cur_outputs |= (1 << s->pin);
    } else {
      cur_outputs &= ~(1 << s->pin);
    }

    if ((i != num_output_events - 1) &&
        (s->time == (s + 1)->time)) {
      /* This isn't the last event, and the next event is the same time,
       * skip in reporting */
       continue;
    }

    report_output_event(STDERR_FILENO, s->time, cur_outputs);
  }
}

/* Thread that busywaits to increment the primary timebase.  This loop is also
 * responsible for triggering event timer, buffer swap, and trigger events
 */
void *platform_timebase_thread(void *_interrupt_fd) {
  int *interrupt_fd = (int *)_interrupt_fd;
  timeval_t last_tasks_run = 0;
  struct timespec current_time;
  struct timespec tick_increment = {
    .tv_nsec = 32000,
  };

  if (clock_gettime(CLOCK_MONOTONIC, &current_time)) {
    perror("clock_gettime");
  }

  do {
    struct timespec next_tick = add_times(current_time, tick_increment);
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_tick, NULL);
    current_time = next_tick;

    /* Process anything that needed to happen between curtime and curtime+128 */
    timeval_t before = curtime;
    timeval_t after = curtime + 128; /* 32 uS */
    curtime = after;

    do_output_slots(after, after + MAX_SLOTS - 1);

    if (curtime - last_tasks_run > time_from_us(10000)) {
      run_tasks();
      last_tasks_run = curtime;
    }
    do_test_trigger(*interrupt_fd);

    if (eventtimer_enable && time_in_range(eventtimer_time, before, after)) {
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

static void start_interrupt_thread(int *interrupt_fd) {
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
                     interrupt_fd)) {
    perror("pthread_create");
    /* Try without realtime sched */
    if (pthread_create(
          &interrupts, NULL, platform_interrupt_thread, interrupt_fd)) {
      perror("pthread_create");
      exit(EXIT_FAILURE);
    }
  }
}

static void start_timebase_thread(int *interrupt_fd) {
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
                     interrupt_fd)) {
    perror("pthread_create");
    /* Try without realtime sched */
    if (pthread_create(
          &timebase, NULL, platform_timebase_thread, interrupt_fd)) {
      perror("pthread_create");
      exit(EXIT_FAILURE);
    }
  }
}

static int start_console_listener(int port, bool wait) {
  (void) wait;
  int sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);

  struct sockaddr_in bindaddr = {
    .sin_family = AF_INET,
    .sin_port = htons(port),
    .sin_addr = {
      .s_addr = htonl(INADDR_ANY),
    },
  };

  bind(sock, &bindaddr, sizeof(bindaddr));
  listen(sock, 10);

  return sock;
}


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

  console_socket_bind_fd = start_console_listener(1234, false);

  pipe(interrupt_pipes);
  start_timebase_thread(&interrupt_pipes[1]);
  start_interrupt_thread(&interrupt_pipes[0]);

  sensors_process(SENSOR_ADC);
  sensors_process(SENSOR_FREQ);
  sensors_process(SENSOR_CONST);
}
