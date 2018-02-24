#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include "platform.h"
#include "scheduler.h"
#include "config.h"

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

void set_event_timer(timeval_t t) {
}

timeval_t get_event_timer() {
  return 0;
}

void clear_event_timer() {
}

void disable_event_timer() {
}

void disable_interrupts() {
  sigset_t sm;
  sigemptyset(&sm);
  sigaddset(&sm, SIGVTALRM);
  sigprocmask(SIG_BLOCK, &sm, NULL);
}

void enable_interrupts() {
  sigset_t sm;
  sigemptyset(&sm);
  sigaddset(&sm, SIGVTALRM);
  sigprocmask(SIG_UNBLOCK, &sm, NULL);
}

int interrupts_enabled() {
  return 0;
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
int usart_tx_ready() {
  return (curtime -  last_tx) > 200;
}

int usart_rx_ready() {
  return 0;
}

void usart_rx_reset() {
}

void usart_tx(char *str, unsigned short len) {
  write(STDOUT_FILENO, str, len);
  last_tx = curtime;
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

  static uint16_t old_outputs = 0;
  static timeval_t run_until_time = 0;
  curtime++;
  cur_slot++;
  if (cur_slot == max_slots) {
    cur_buffer = (cur_buffer + 1) % 2;
    cur_slot = 0;
    scheduler_buffer_swap();
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
        sensor_adc_new_data();
      }

      /* Multiple commands sent in one packet? */
      bufpos = strchr(bufpos, '\n');
      if (bufpos) {
        bufpos++;
      }
    }
  }

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


}
