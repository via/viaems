#include <assert.h>
#include <errno.h>
#include <fcntl.h>           /* For O_* constants */
#include <mqueue.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>        /* For mode constants */
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "console.h"
#include "platform.h"
#include "scheduler.h"
#include "stats.h"

_Atomic static timeval_t curtime;

_Atomic static timeval_t eventtimer_time;
_Atomic static uint32_t eventtimer_enable = 0;
int event_logging_enabled = 1;
uint32_t test_trigger_rpm = 0;
timeval_t test_trigger_last = 0;
_Atomic int platform_decode_time = 0;
_Atomic int platform_needs_decode = 0;
_Atomic int platform_needs_event_callback = 0;

struct slot {
  uint16_t on_mask;
  uint16_t off_mask;
} __attribute__((packed)) *output_slots[2];
size_t max_slots;
_Atomic size_t cur_slot = 0;
_Atomic size_t cur_buffer = 0;

void platform_enable_event_logging() {
  event_logging_enabled = 1;
}

void platform_disable_event_logging() {
  event_logging_enabled = 0;
}

void platform_reset_into_bootloader() {}

_Atomic static uint16_t cur_outputs = 0;

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
  platform_needs_event_callback = 0;
}

void disable_event_timer() {
  eventtimer_enable = 0;
}

/* Used to lock the interrupt thread */
pthread_mutex_t interrupt_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

/* Used to determine when we've recursively unlocked */
pthread_mutex_t interrupt_count_mutex = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP;
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

void set_pwm(int output, float value) {}

void adc_gather() {}

timeval_t last_tx = 0;
size_t console_write(const void *buf, size_t len) {
  if (curtime - last_tx < 50) {
    return 0;
  }
  struct timespec wait = {
    .tv_nsec = 20000,
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

#if 0
static void hosted_platform_timer(int sig, siginfo_t *info, void *ucontext) {
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
      struct decoder_event ev = { .t0 = 1, .time = curtime };
      decoder_update_scheduling(&ev, 1);
      trigger_count++;

      if ((config.decoder.type == TOYOTA_24_1_CAS) &&
          (trigger_count >= config.decoder.num_triggers)) {
        trigger_count = 0;
        struct decoder_event ev = { .t1 = 1, .time = curtime };
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
}
#endif

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
  } while ((current.tv_sec < until.tv_sec) ||
           ((current.tv_sec == until.tv_sec) &&
            (current.tv_nsec < until.tv_nsec)));
}


struct interrupt {
  enum {
    TRIGGER0,
    TRIGGER0_W_TIME,
    TRIGGER1,
    TRIGGER1_W_TIME,
    EVENT,
  } type;
  timeval_t trigger_time;
};

static void do_test_trigger(mqd_t queue) {
  static int trigger = 0;

  struct interrupt ev = { .type = TRIGGER0_W_TIME, .trigger_time = curtime};
  if (mq_send(queue, (const char *)&ev, sizeof(ev), 0) < 0) {
    perror("mq_send");
    exit(3);
  }

  trigger++;
  if (trigger == 24) {
    struct interrupt ev = { .type = TRIGGER1_W_TIME, .trigger_time = curtime};
    if (mq_send(queue, (const char *)&ev, sizeof(ev), 0) < 0) {
      perror("mq_send");
      exit(4);
    }
    trigger = 0;
  }
}

const char *interrupt_queue_path = "/viaems_interrupt_queue";

void *platform_interrupt_thread(void *_unused) {
  struct sched_param sp = (struct sched_param){
    .sched_priority = 1,
  };

  if (sched_setscheduler(0, SCHED_RR, &sp)) {
    perror("sched_setscheduler");
  }

  struct mq_attr mq_attrs = {
    .mq_msgsize = sizeof(struct interrupt),
    .mq_maxmsg = 8,
  };

  mqd_t q = mq_open(interrupt_queue_path, O_RDONLY | O_CREAT, S_IWUSR | S_IRUSR, &mq_attrs);
  if (q == -1) {
    perror("mq_open");
    exit(1);
  }

  do {
    struct interrupt msg;

    size_t rsize = mq_receive(q, (char *)&msg, sizeof(msg), 0);
    if (rsize < 0) {
      if (errno == EINTR) {
        continue;
      }
      perror("mq_receive");
      exit(1);
    }


    switch (msg.type) {
      case TRIGGER0:
        decoder_update_scheduling(&(struct decoder_event){.t0 = 1, .time = curtime}, 1);
        break;
      case TRIGGER0_W_TIME:
        decoder_update_scheduling(&(struct decoder_event){.t0 = 1, .time = msg.trigger_time}, 1);
        break;
      case TRIGGER1:
        decoder_update_scheduling(&(struct decoder_event){.t1 = 1, .time = curtime}, 1);
        break;
      case TRIGGER1_W_TIME:
        decoder_update_scheduling(&(struct decoder_event){.t1 = 1, .time = msg.trigger_time}, 1);
        break;
      case EVENT:
        scheduler_callback_timer_execute();
        break;
       
    }

  } while(1);

}


static void do_output_slots() {
  if (!output_slots[0]) {
    return;
  }

  cur_slot++;
  if (cur_slot == max_slots) {
    cur_buffer = (cur_buffer + 1) % 2;
    cur_slot = 0;
    scheduler_buffer_swap();
  }
  
  uint16_t old_outputs = cur_outputs;
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


}
/* Thread that busywaits to increment the primary timebase.  This loop is also
 * responsible for triggering event timer, buffer swap, and trigger events
 */

void *platform_timebase_thread(void *_unused) {
  struct sched_param sp = (struct sched_param){
    .sched_priority = 1,
  };

  if (sched_setscheduler(0, SCHED_RR, &sp)) {
    perror("sched_setscheduler");
  }
  struct mq_attr mq_attrs = {
    .mq_msgsize = sizeof(struct interrupt),
    .mq_maxmsg = 8,
  };
  mqd_t q = mq_open(interrupt_queue_path, O_WRONLY | O_CREAT, S_IWUSR | S_IRUSR, &mq_attrs);
  if (q == -1) {
    perror("mq_open");
  }

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
//    nanosleep(&tick_increment, NULL);
    current_time = next_tick;
    curtime += 1;


    do_output_slots();

    if ((curtime % 5000) == 0) {
      do_test_trigger(q);
    }
    
    if (eventtimer_enable && (eventtimer_time + 1 == curtime)) {
      struct interrupt event = {.type = EVENT};
      if (mq_send(q, (const char *)&event, sizeof(event), 0) < 0) {
        perror("mq_send");
        exit(2);
      }
    }

  
  } while(1);

}

void platform_init() {


  pthread_t timebase;
  pthread_create(&timebase, NULL, platform_timebase_thread, NULL);

  pthread_t interrupts;
  pthread_create(&interrupts, NULL, platform_interrupt_thread, NULL);

  sensors_process(SENSOR_ADC);
  sensors_process(SENSOR_FREQ);
  sensors_process(SENSOR_CONST);


}