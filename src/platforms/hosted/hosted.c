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
#include "decoder.h"
#include "platform.h"
#include "scheduler.h"
#include "sensors.h"
#include "sim.h"
#include "util.h"
#include "viaems.h"

static timeval_t curtime = 0;

static timeval_t sim_wakeup_time = 0;
static bool sim_wakeup_enabled = false;
static uint16_t cur_outputs = 0;

static struct viaems hosted_viaems;

/* Disable leak detection in asan. There are several convenience allocations,
 * but they should be single ones for the lifetime of the program */
const char *__asan_default_options(void) {
  return "detect_leaks=0";
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

void write_string(const char *s) {
  puts(s);
}

uint64_t cycles_to_ns(uint64_t cycles) {
  return cycles;
}

void set_sim_wakeup(timeval_t t) {
  sim_wakeup_time = t;
  sim_wakeup_enabled = true;
}

void set_gpio(uint16_t new_gpios, timeval_t when) {
  static uint16_t gpios = 0;
  if (gpios != new_gpios) {
    console_record_event((struct logged_event){
      .time = when,
      .value = new_gpios,
      .type = EVENT_GPIO,
    });
  }

  gpios = new_gpios;
}

size_t console_write(const void *buf, size_t len) {
  ssize_t written = -1;
  while ((written = write(STDOUT_FILENO, buf, len)) < 0)
    ;
  if (written > 0) {
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

struct config *platform_load_config() {
  return &default_config;
}

void platform_save_config() {}

uint32_t platform_adc_samplerate(void) {
  return 5000;
}

uint32_t platform_knock_samplerate(void) {
  return 5000;
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

static int schedule_entry_compare(const void *_a, const void *_b) {
  const struct schedule_entry *const *a = _a;
  const struct schedule_entry *const *b = _b;

  if ((*a)->time == (*b)->time) {
    return 0;
  }
  return time_before((*a)->time, (*b)->time) ? -1 : 1;
}

static void retire_plan(struct platform_plan *plan) {
  for (int i = 0; i < plan->n_events; i++) {
    plan->schedule[i]->state = SCHED_FIRED;
  }
}

static void report_output_event(timeval_t time, uint16_t outputs) {
  console_record_event((struct logged_event){
    .time = time,
    .value = outputs,
    .type = EVENT_OUTPUT,
  });
}

static void execute_plan(struct platform_plan *plan) {
  qsort(plan->schedule,
        plan->n_events,
        sizeof(struct schedule_entry *),
        schedule_entry_compare);
  for (int i = 0; i < plan->n_events; i++) {
    plan->schedule[i]->state = SCHED_SUBMITTED;
  }

  for (int i = 0; i < plan->n_events; i++) {
    const struct schedule_entry *s = plan->schedule[i];
    /* Update outputs */
    if (s->val) {
      cur_outputs |= (1 << s->pin);
    } else {
      cur_outputs &= ~(1 << s->pin);
    }

    if ((i != plan->n_events - 1) && (s->time == (s + 1)->time)) {
      /* This isn't the last event, and the next event is the same time,
       * skip in reporting */
      continue;
    }

    report_output_event(s->time, cur_outputs);
  }

  set_gpio(plan->gpio, plan->schedulable_start);
}

static struct adc_update current_adc = { 0 };
static void handle_replay_events(timeval_t until_time);

/* Thread that busywaits to increment the primary timebase.  This loop is also
 * responsible for triggering event timer, buffer swap, and trigger events
 */

void *platform_timebase_thread(void *_ptr) {
  (void)_ptr;

  /* Each iteration will jump forward 200 uS, and first:
   *  - handle any sim wakeups between now and +200 uS
   *  - handle any replay events between now and +200 uS
   *  - run the main engine rescheduling
   *  - process the list of events provided
   *  - actually wait 200 uS of real time
   */

  static struct platform_plan plan = { 0 };
  struct timespec current_time;
  struct timespec tick_increment = {
    .tv_nsec = 200000,
  };

  if (clock_gettime(CLOCK_MONOTONIC, &current_time)) {
    perror("clock_gettime");
  }

  do {
    timeval_t after = curtime + 800; /* 200 uS */

    if (sim_wakeup_enabled && time_before(sim_wakeup_time, after)) {
      sim_wakeup_callback(&hosted_viaems.decoder);
      sim_wakeup_enabled = false;
    }

    handle_replay_events(after);

    struct engine_update update = { .current_time = after };
    update.position = decoder_get_engine_position(&hosted_viaems.decoder);
    sensor_update_adc(&hosted_viaems.sensors, &update.position, &current_adc);

    update.sensors = sensors_get_values(&hosted_viaems.sensors);

    retire_plan(&plan);

    plan = (struct platform_plan){
      .schedulable_start = after,
      .schedulable_end = after + 800 - 1,
    };

    viaems_reschedule(&hosted_viaems, &update, &plan);

    execute_plan(&plan);

    struct timespec next_tick = add_times(current_time, tick_increment);
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_tick, NULL);
    current_time = next_tick;
    curtime = after;

  } while (true);
}

struct hosted_args {
  const char *read_config_file;
  const char *write_config_file;
  const char *read_replay_file;
  bool benchmark_mode;
};

static void parse_args(struct hosted_args *args, int argc, char *argv[]) {
  *args = (struct hosted_args){ 0 };
  int opt;
  while ((opt = getopt(argc, argv, "c:o:bi:")) != -1) {
    switch (opt) {
    case 'b':
      args->benchmark_mode = true;
      break;
    case 'c':
      args->read_config_file = strdup(optarg);
      break;
    case 'o':
      args->write_config_file = strdup(optarg);
      break;
    case 'i':
      args->read_replay_file = strdup(optarg);
      break;
    default:
      fprintf(
        stderr,
        "usage: viaems [-c config] [-o outconfig] [-b] [-i replayfile]\n");
      exit(EXIT_FAILURE);
    }
  }
}

struct replay_event {
  timeval_t time;
  enum {
    NO_EVENT,
    TRIGGER_EVENT,
    ADC_EVENT,
    END_EVENT,
  } type;

  uint32_t trigger;
  struct adc_update adc;
};

static FILE *replay_file = NULL;

static struct replay_event read_next_replay_event(timeval_t last_time) {
  /* Fetch next line */
  static char *linebuf = NULL;
  static size_t linebuf_size = 0;

  if (getline(&linebuf, &linebuf_size, replay_file) < 0) {
    if (linebuf != NULL) {
      free(linebuf);
    }
    exit(EXIT_SUCCESS);
  }

  switch (linebuf[0]) {
  case 't': {
    int delay;
    int trigger;
    sscanf(linebuf, "t %d %d", &delay, &trigger);

    return (struct replay_event){
      .time = last_time + delay,
      .type = TRIGGER_EVENT,
      .trigger = trigger,
    };
  }
  case 'e': {
    int delay;
    sscanf(linebuf, "e %d", &delay);

    return (struct replay_event){
      .time = last_time + delay,
      .type = END_EVENT,
    };
  }
  case 'a': {
    int delay;
    struct replay_event ev;
    sscanf(linebuf,
           "a %d %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
           &delay,
           &ev.adc.values[0],
           &ev.adc.values[1],
           &ev.adc.values[2],
           &ev.adc.values[3],
           &ev.adc.values[4],
           &ev.adc.values[5],
           &ev.adc.values[6],
           &ev.adc.values[7],
           &ev.adc.values[8],
           &ev.adc.values[9],
           &ev.adc.values[10],
           &ev.adc.values[11],
           &ev.adc.values[12],
           &ev.adc.values[13],
           &ev.adc.values[14],
           &ev.adc.values[15]);

    /* Schedule an end event */
    ev.adc.valid = true;
    ev.adc.time = last_time + delay;
    ev.time = last_time + delay;
    ev.type = ADC_EVENT;
    return ev;
  }
  default:
    fprintf(stderr, "Invalid replay command: %c\n", linebuf[0]);
    exit(EXIT_SUCCESS);
  }
}

static void handle_replay_events(timeval_t until_time) {

  static struct replay_event current_event = { 0 };

  if (!replay_file) {
    return;
  }
  if (current_event.type == NO_EVENT) {
    current_event = read_next_replay_event(0);
  }

  while (time_before(current_event.time, until_time)) {
    switch (current_event.type) {
    case END_EVENT:
      exit(EXIT_SUCCESS);
      break;
    case TRIGGER_EVENT:
      decoder_update(&hosted_viaems.decoder,
                     &(struct trigger_event){
                       .time = current_event.time,
                       .type = current_event.trigger == 0 ? TRIGGER : SYNC });
      break;
    case NO_EVENT:
      break;
    case ADC_EVENT:
      current_adc = current_event.adc;
      break;
    }

    current_event = read_next_replay_event(current_event.time);
  }
}

int main(int argc, char *argv[]) {
  struct hosted_args args;
  parse_args(&args, argc, argv);

  if (args.benchmark_mode) {
    int start_benchmarks(void);
    start_benchmarks();
    return 0;
  }

  struct config *config = platform_load_config();
  viaems_init(&hosted_viaems, config);

  // Make sure sensors are initialized before console starts
  sensor_update_adc(
    &hosted_viaems.sensors, &(struct engine_position){ 0 }, &current_adc);

  if (args.read_replay_file) {
    replay_file = fopen(args.read_replay_file, "r");
  }

  /* Set stdin nonblock */
  fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL, 0) | O_NONBLOCK);

  pthread_t timebase;
  if (pthread_create(&timebase, NULL, platform_timebase_thread, NULL)) {
    perror("pthread_create");
    exit(EXIT_FAILURE);
  }

  while (true) {
    viaems_idle(&hosted_viaems, current_time());
  }

  return 0;
}
