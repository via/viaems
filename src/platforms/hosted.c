#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
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
 * stdin is event list with this format:
 * time (integer microseconds) trigger
 * time (integer microseconds) sensor map 3000
 * time (integer microseconds) sensor iat 58
 *
 * options:
 *  -l logfile -- send console output to logfile
 *  -o outputsfile -- send updates of output status to outputsfile
 */

static timer_t systimer;
static sigset_t smask;
static timeval_t ev_timer = 0;
static timeval_t last_tx = 0;

static void event_signal(int s __attribute((unused))) {
  scheduler_execute();
}

static void primary_trigger(int s __attribute((unused))) {
  config.decoder.last_t0 = current_time();
  config.decoder.needs_decoding = 1;
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
  str[len] = '\0';
  printf("%s", str);
}

void enable_test_trigger(trigger_type trig, unsigned int rpm) {

}
