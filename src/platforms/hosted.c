#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "console.h"
#include "platform.h"
#include "scheduler.h"
#include "stats.h"

/* A platform that runs on a hosted OS, preferably one that supports posix
 * timers.  Sends console to stdout/stdin
 *
 * Outputs:
 * [TIME] OUTPUTS [PORT-D VALUE]
 * [TIME] STOPPED
 *
 * Inputs:
 * TRIGGER1
 * TRIGGER2
 * ADC [PIN] [VALUE]
 * RUN [TIME]
 */

_Atomic static timeval_t curtime;

_Atomic static timeval_t eventtimer_time;
_Atomic static uint32_t eventtimer_enable = 0;
int event_logging_enabled = 1;
uint32_t test_trigger_rpm = 0;
timeval_t test_trigger_last = 0;

struct slot {
  uint16_t on_mask;
  uint16_t off_mask;
} __attribute__((packed)) * output_slots[2];
size_t max_slots;
size_t cur_slot = 0;
size_t cur_buffer = 0;
sigset_t smask;

void platform_enable_event_logging() {
  event_logging_enabled = 1;
}

void platform_disable_event_logging() {
  event_logging_enabled = 0;
}

void platform_reset_into_bootloader() {}

uint16_t cur_outputs = 0;

timeval_t current_time() {
  return curtime;
}

timeval_t cycle_count() {

  struct timespec tp;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tp);
  return (timeval_t)(tp.tv_sec * 1000000000 + tp.tv_nsec);
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

static ucontext_t *sig_context = NULL;
static void signal_handler_entered(struct ucontext_t *context) {
  sig_context = context;
}
static void signal_handler_exited() {
  sig_context = NULL;
}

static int interrupts_disabled = 0;
int disable_interrupts() {
  int old = interrupts_disabled;
  if (sig_context) {
    sigaddset(&sig_context->uc_sigmask, SIGVTALRM);
  } else {
    sigset_t sm;
    sigemptyset(&sm);
    sigaddset(&sm, SIGVTALRM);
    sigprocmask(SIG_BLOCK, &sm, NULL);
  }
  interrupts_disabled = 1;
  stats_start_timing(STATS_INT_DISABLE_TIME);
  return !old;
}

void enable_interrupts() {
  interrupts_disabled = 0;
  stats_finish_timing(STATS_INT_DISABLE_TIME);
  if (sig_context) {
    sigdelset(&sig_context->uc_sigmask, SIGVTALRM);
  } else {
    sigset_t sm;
    sigemptyset(&sm);
    sigaddset(&sm, SIGVTALRM);
    sigprocmask(SIG_UNBLOCK, &sm, NULL);
  }
}

int interrupts_enabled() {
  return !interrupts_disabled;
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

void set_pwm(int output, float value) {}

void adc_gather() {}

timeval_t last_tx = 0;
size_t console_write(const void *buf, size_t len) {
  if (curtime - last_tx < 200) {
    return 0;
  }
  ssize_t written = -1;
  while ((written = write(STDOUT_FILENO, buf, len)) < 0)
    ;
  if (written > 0) {
    last_tx = curtime;
    return written;
  }
  return 0;
}

char rx_buffer[128];
int rx_amt = 0;
size_t console_read(void *buf, size_t len) {
  if (rx_amt == 0) {
    return 0;
  }

  size_t amt = len < rx_amt ? len : rx_amt;
  memcpy(buf, rx_buffer, amt);
  rx_amt -= amt;
  return amt;
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

void set_test_trigger_rpm(unsigned int rpm) {
  test_trigger_rpm = rpm;
}

static void hosted_platform_timer(int sig, siginfo_t *info, void *ucontext) {
  /* - Increase "time"
   * - Trigger appropriate interrupts
   *     timer event
   *     input event
   *     dma buffer swap
   *
   * - Set outputs from buffer
   */

  signal_handler_entered(ucontext);
  stats_increment_counter(STATS_INT_RATE);
  stats_start_timing(STATS_INT_TOTAL_TIME);

  if (!output_slots[0]) {
    /* Not initialized */
    signal_handler_exited();
    return;
  }

  int failing = 0; // (current_time() % 1000000) > 500000;
  if (current_time() % 1000 == 0) {
    // test_trigger_rpm += 1;
    if (test_trigger_rpm > 9000) {
      test_trigger_rpm = 800;
    }
  }
  if (test_trigger_rpm && !failing) {
    timeval_t time_between = time_from_rpm_diff(test_trigger_rpm, 30);
    static uint32_t trigger_count = 0;
    if (curtime >= test_trigger_last + time_between) {
      test_trigger_last = curtime;
      struct decoder_event ev = { .trigger = 0, .time = curtime };
      decoder_update_scheduling(&ev, 1);
      trigger_count++;

      if ((config.decoder.type == TOYOTA_24_1_CAS) &&
          (trigger_count >= config.decoder.num_triggers)) {
        trigger_count = 0;
        struct decoder_event ev = { .trigger = 1, .time = curtime };
        decoder_update_scheduling(&ev, 1);
      }
    }
  }

  static uint16_t old_outputs = 0;
  curtime++;
  cur_slot++;
  if (cur_slot == max_slots) {
    cur_buffer = (cur_buffer + 1) % 2;
    cur_slot = 0;
    stats_start_timing(STATS_INT_BUFFERSWAP_TIME);
    scheduler_buffer_swap();
    stats_finish_timing(STATS_INT_BUFFERSWAP_TIME);
  }

  cur_outputs |= output_slots[cur_buffer][cur_slot].on_mask;
  cur_outputs &= ~output_slots[cur_buffer][cur_slot].off_mask;

  if (cur_outputs != old_outputs) {
    console_record_event((struct logged_event){
      .time = curtime,
      .value = cur_outputs,
      .type = EVENT_OUTPUT,
    });
    old_outputs = cur_outputs;
  }

  /* poll for command input */
  struct pollfd pfds[] = {
    { .fd = STDIN_FILENO, .events = POLLIN },
  };
  if (!rx_amt && poll(pfds, 1, 0)) {
    ssize_t r = read(STDIN_FILENO, rx_buffer, 127);
    if (r > 0) {
      rx_amt = r;
    }
  }

  if (eventtimer_enable && (eventtimer_time + 1 == curtime)) {
    scheduler_callback_timer_execute();
  }

  sensors_process(SENSOR_ADC);
  sensors_process(SENSOR_FREQ);

  stats_finish_timing(STATS_INT_TOTAL_TIME);
  signal_handler_exited();
}

void platform_init() {

  /* Set up signal handling */
  sigemptyset(&smask);
  struct sigaction sa = {
    .sa_sigaction = hosted_platform_timer,
    .sa_mask = smask,
  };
  sigaction(SIGVTALRM, &sa, NULL);

#ifdef SUPPORTS_POSIX_TIMERS
  struct sigevent sev = {
    .sigev_notify = SIGEV_SIGNAL,
    .sigev_signo = SIGVTALRM,
  };

  timer_t timer;
  if (timer_create(CLOCK_MONOTONIC, &sev, &timer) == -1) {
    perror("timer_create");
  }
  struct itimerspec interval = {
    .it_interval = {
      .tv_sec = 0,
      .tv_nsec = 10000,
    },
    .it_value = {
      .tv_sec = 0,
      .tv_nsec = 10000,
    },
  };
  timer_settime(timer, 0, &interval, NULL);
#else
  /* Use interval timer */
  struct itimerval t = {
    .it_interval =
      (struct timeval){
        .tv_sec = 0,
        .tv_usec = 1,
      },
    .it_value =
      (struct timeval){
        .tv_sec = 0,
        .tv_usec = 1,
      },
  };
  setitimer(ITIMER_VIRTUAL, &t, NULL);
#endif
  stats_init(1000000000);

  /* Set stdin nonblock */
  fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL, 0) | O_NONBLOCK);
}
