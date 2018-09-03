#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include <time.h>

#include "platform.h"
#include "scheduler.h"
#include "config.h"
#include "stats.h"

/* A platform that runs on a hosted OS, preferably one that supports posix 
 * timers.  Sends console to stdout, and creates a control socket on tfi.sock.
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

struct slot {
  uint16_t on_mask;
  uint16_t off_mask;
} __attribute__((packed)) *output_slots[2];
int control_socket;
size_t max_slots;
size_t cur_slot = 0;
size_t cur_buffer = 0;
sigset_t smask;

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

static int interrupts_disabled = 0;
void disable_interrupts() {
  sigset_t sm;
  sigemptyset(&sm);
  sigaddset(&sm, SIGVTALRM);
  sigprocmask(SIG_BLOCK, &sm, NULL);
  interrupts_disabled = 1;
  stats_start_timing(STATS_INT_DISABLE_TIME);
}

void enable_interrupts() {
  interrupts_disabled = 0;
  stats_finish_timing(STATS_INT_DISABLE_TIME);
  sigset_t sm;
  sigemptyset(&sm);
  sigaddset(&sm, SIGVTALRM);
  sigprocmask(SIG_UNBLOCK, &sm, NULL);
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
}

void set_pwm(int output, float value) {
}

void adc_gather() {
}

timeval_t last_tx = 0;
size_t console_write(const void *buf, size_t len) {
  if (curtime - last_tx < 200) {
    return 0;
  }
  size_t written = write(STDOUT_FILENO, buf, len);
  if (written) {
    last_tx = curtime;
  }
  return written;
}

char rx_buffer[128];
int rx_amt = 0;
size_t console_read(void *buf, size_t len) {
  if (rx_amt == 0) {
    return 0;
  }

  size_t amt = len < rx_amt ? len: rx_amt;
  memcpy(buf, rx_buffer, amt);
  rx_amt -= amt;
  return amt;
}

void platform_load_config() {
}

void platform_save_config() {
}

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

void enable_test_trigger(trigger_type t, unsigned int rpm) {
}

static void hosted_send_trigger() {
  config.decoder.last_t0 = curtime;
  config.decoder.needs_decoding_t0 = 1;
  decoder_update_scheduling();
}

static void hosted_platform_timer() {
  /* - Increase "time"
   * - Trigger appropriate interrupts
   *     timer event
   *     input event
   *     dma buffer swap
   *
   * - Set outputs from buffer
   */

  stats_increment_counter(STATS_INT_RATE);
  stats_start_timing(STATS_INT_TOTAL_TIME);

  if (!output_slots[0]) {
    /* Not initialized */
    return;
  }

  static uint16_t old_outputs = 0;
  static timeval_t run_until_time = 0;
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
    char buf[32];
    sprintf(buf, "%d OUTPUTS %2x\n", curtime, cur_outputs);
    write(control_socket, buf, strlen(buf));
    old_outputs = cur_outputs;
  }
  if (curtime >= run_until_time) {
    char buf[1024];
    char *bufpos = buf;

    sprintf(buf, "%d STOPPED\n", curtime);
    write(control_socket, buf, strlen(buf));

    size_t len = read(control_socket, buf, 1024);
    buf[len] = '\0';
    while (bufpos) {
      if (strncmp(bufpos, "RUN", 3) == 0) {
        run_until_time = atoi(&bufpos[4]);
      }
      if (strncmp(bufpos, "TRIGGER1", 8) == 0) {
        hosted_send_trigger();
      }
      if (strncmp(bufpos, "ADC", 3) == 0) {
        int sensor, value;
        sscanf(&bufpos[4], "%d %d", &sensor, &value);
        config.sensors[sensor].raw_value = value & 0xFFF;
        config.sensors[sensor].fault = FAULT_NONE;
        if (config.sensors[sensor].source == SENSOR_FREQ) {
          sensor_freq_new_data();
        }
        if (config.sensors[sensor].source == SENSOR_ADC) {
          sensor_adc_new_data();
        }
        sensors_process();
      }

      /* Multiple commands sent in one packet? */
      bufpos = strchr(bufpos, '\n');
      if (bufpos) {
        bufpos++;
      }
    }
  }

  /* poll for command input */
  struct pollfd pfds[] = {
    {.fd = STDIN_FILENO, .events = POLLIN},
  };
  if (!rx_amt && poll(pfds, 1, 0)) {
    size_t r = read(STDIN_FILENO, rx_buffer, 127);
    if (r) {
      rx_amt = r;
    }
  }

  if (eventtimer_enable && (eventtimer_time + 1 == curtime)) {
    scheduler_callback_timer_execute();
  }

  stats_finish_timing(STATS_INT_TOTAL_TIME);
}

static void bind_control_socket(const char *path) {
  struct sockaddr_un sockaddr;
  int control_server_socket = 0;

  control_server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
  if (control_server_socket < 0) {
    perror("socket");
    exit(1);
  }

  sockaddr.sun_family = AF_UNIX;
  strcpy(sockaddr.sun_path, "tfi.sock");
  if (bind(control_server_socket, (struct sockaddr *)&sockaddr, sizeof(sockaddr))) {
    perror("bind");
    exit(1);
  }

  if (listen(control_server_socket, 5)) {
    perror("listen");
    exit(1);
  }

  control_socket = accept(control_server_socket, 0, 0);
  if (control_socket == -1) {
    perror("accept");
    exit(1);
  }

}

void platform_init(int argc, char *argv[]) {

  unlink("tfi.sock");
  bind_control_socket("tfi.sock");

  /* Set up signal handling */
  sigemptyset(&smask);
  struct sigaction sa = {
    .sa_handler = hosted_platform_timer,
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
    .it_interval = (struct timeval) {
      .tv_sec = 0,
      .tv_usec = 1,
    },
    .it_value = (struct timeval) {
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
