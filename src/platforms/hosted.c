#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/timerfd.h>
#include <signal.h>
#include <poll.h>
#include <time.h>
#include <errno.h>

#include "decoder.h"
#include "platform.h"
#include "scheduler.h"
#include "limits.h"
#include "sensors.h"
#include "util.h"
#include "config.h"

/* "Hardware" setup:
 * stdout is the console
 * event_in is an input file containing events:
 *   - all times relative to start of run
 *  "time trigger1"
 *  "time map 2048"
 * event_out is a file containing times/events for triggers and outputs
 *  "time trigger1"
 *  "time output 4 on"
 */

static timer_t systimer;
static sigset_t smask;
static timeval_t ev_timer = 0;
static timeval_t last_tx = 0;

static int data_pipe;

static void event_signal(int s __attribute((unused))) {
  scheduler_execute();
}

static void primary_trigger(int s __attribute((unused))) {
  char buf[64];
  config.decoder.last_t0 = current_time();
  config.decoder.needs_decoding = 1;
  sprintf(buf, "%lu trigger1\n", config.decoder.last_t0);
  write(data_pipe, buf, strlen(buf));
}

static inline timeval_t timespec_to_ticks(struct timespec *t) {
  return (t->tv_sec * (timeval_t)1e9 + t->tv_nsec);
}

static inline struct timespec ticks_to_timespec(timeval_t t) {
  struct timespec ts = {0};
  ts.tv_sec = t / (timeval_t)1e9;
  ts.tv_nsec = t % (timeval_t)1e9;
  return ts;
}

timeval_t current_time() {
  struct timespec t;
  if (clock_gettime(CLOCK_MONOTONIC, &t)) {
    perror("clock_gettime");
    exit(EXIT_FAILURE);
  }
  return timespec_to_ticks(&t);
}

void set_event_timer(timeval_t t) {

  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  struct itimerspec ev = {
    .it_value = ticks_to_timespec(t),
  };
  timer_settime(systimer, TIMER_ABSTIME, &ev, NULL); 
  ev_timer = t;

}

timeval_t get_event_timer() {
  return ev_timer;
}

void clear_event_timer() {
  sigset_t ss;
  int i;
  sigpending(&ss); 
  while (sigismember(&ss, SIGALRM)) {
    sigemptyset(&ss);
    sigaddset(&ss, SIGALRM);
    sigwait(&ss, &i);
    sigpending(&ss); 
  }
}

void disable_event_timer() {
  struct itimerspec ev = {{0}};
  timer_settime(systimer, 0, &ev, NULL);
  clear_event_timer();
}

static void interface_settimer(int tfd, timeval_t start, timeval_t time) {
  struct timespec t;
  t = ticks_to_timespec(start + time);
  struct itimerspec ev = {
    .it_value = t,
  };
  timerfd_settime(tfd, TFD_TIMER_ABSTIME, &ev, NULL);
}
  

static void interface(pid_t ppid) {
  struct timespec t;
  int timer_fd;
  int res;
  FILE *ev_out_fd, *ev_in_fd;
  timeval_t start;
  unsigned long time;
  char cmdline[128];
  char command[16];
  char sensor[16];
  float val;
  uint64_t events;

  sigprocmask(SIG_BLOCK, &smask, 0);
  timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);

  ev_in_fd = fopen("event_in", "r");
  ev_out_fd = fopen("event_out", "w");

  fgets(cmdline, 128, ev_in_fd);
  res = sscanf(cmdline, "%lu %16s %16s %f\n", 
    &time, command, sensor, &val);
  clock_gettime(CLOCK_MONOTONIC, &t);
  start = timespec_to_ticks(&t);
  interface_settimer(timer_fd, start, time);

  struct pollfd fds[] = {
    { .fd = timer_fd, .events = POLLIN},
    { .fd = data_pipe, .events = POLLIN},
  };
  while (1) {
    if (poll(fds, 2, -1) > 0) {
      if (fds[0].revents & POLLIN) {
        read(timer_fd, &events, sizeof(uint64_t));
        /* Timer fired, send signal and move on */
        kill(ppid, SIGUSR1);
        if (!fgets(cmdline, 128, ev_in_fd)) {
          exit(EXIT_SUCCESS);
	}
	sscanf(cmdline, "%lu %16s %16s %f\n", 
          &time, command, sensor, &val);
	interface_settimer(timer_fd, start, time);
      } 
      if (fds[1].revents & POLLIN) {
        /* Event from ecu, write to log */
        res = read(data_pipe, cmdline, 127);
        cmdline[res] = '\0';
        fputs(cmdline, ev_out_fd);
      }
    }
  }
}


void platform_init() {

  /* Set up systimer as the event timer */
  int ret = timer_create(CLOCK_MONOTONIC, NULL, &systimer);
  if (ret) {
    perror("create_timer");
    exit(EXIT_FAILURE);
  }

  /* Global mask used for "interrupts" */
  sigemptyset(&smask);
  sigaddset(&smask, SIGALRM);
  sigaddset(&smask, SIGUSR1);
  sigaddset(&smask, SIGUSR2);

  /* Set up event timer signal handler */
  struct sigaction sa = {
    .sa_handler = event_signal,
    .sa_mask = smask,
  };
  sigaction(SIGALRM, &sa, NULL);

  /* Set up t0 and t1 for SIGUSR1 and SIGUSR2 */
  sa.sa_handler = primary_trigger;
  sigaction(SIGUSR1, &sa, NULL);

  /* Set up the channel for event information */
  int pipes[2];
  pipe(pipes);
  
  if(!fork()) {
    data_pipe = pipes[0];
    close(pipes[1]);
    interface(getppid());
  } else {
    close(pipes[0]);
    data_pipe = pipes[1];
  }
  

}

void disable_interrupts() {
  sigprocmask(SIG_BLOCK, &smask, NULL);
}

void enable_interrupts() {
  sigprocmask(SIG_UNBLOCK, &smask, NULL);
}

int interrupts_enabled() {
  sigset_t ss;
  sigprocmask(SIG_BLOCK, NULL, &ss);
  return !sigismember(&ss, SIGALRM);
}

void set_output(int output, char value) {
  char buf[64];
  int res;
  res = sprintf(buf, "%lu output %d %d\n", current_time(), output, value);
  write(data_pipe, buf, res);
}

void adc_gather(void *_unused) {

}

int usart_tx_ready() {
  return (current_time() > last_tx + 100000000);
}

int usart_rx_ready() {
  return 0;
}

void usart_rx_reset() {

}

void usart_tx(char *str, unsigned short len) {
  last_tx = current_time();
  printf("%s", str);
}

void enable_test_trigger(trigger_type trig, unsigned int rpm) {

}
