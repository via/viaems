#ifndef _PLATFORM_H
#define _PLATFORM_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_GPIOS 8
#define MAX_PWM 4
#define MAX_EVENTS 16

typedef uint32_t timeval_t;
typedef float degrees_t;

/* Forward declaration deps */
struct schedule_entry;
struct viaems;
struct config;

timeval_t current_time(void);
uint64_t cycle_count(void);
uint64_t cycles_to_ns(uint64_t cycles);

struct console_tx_message;

/* Attempt to start a new message. Return false is unable to create. */
bool platform_message_writer_new(struct console_tx_message *, size_t length);

/* Perform blocking write of data for a message. Returns false if unable to write. 
 * When all bytes have been written (as specified in the length parameter to 
 * platform_message_writer_new), the message is considered complete */
bool platform_message_writer_write(struct console_tx_message *, uint8_t *data, size_t length);

/* Indicate to the platform that the message will not be completed, and any
 * internal state can be discarded */
void platform_message_writer_abort(struct console_tx_message *);

struct console_rx_message;

/* If there is a pending message to receive, returns true and resets the console
 * message struct */
bool platform_message_reader_new(struct console_rx_message *);

/* Perform a blocking read of data from a message into data. 
 * Returns false if there is an error reading data or if the message is
 * exhausted, after which the console message may not be read from again.
 */
bool platform_message_reader_read(struct console_rx_message *, uint8_t *data, size_t length);

/* Indicate to the platform that the message will not be read from again, and
 * any further data in the message can be discarded */
void platform_message_reader_abort(struct console_rx_message *);

#ifndef PLATFORM_HAS_NATIVE_MESSAGING
size_t platform_read(uint8_t *buffer, size_t max);
size_t platform_write(const uint8_t *buffer, size_t length);
#endif

void platform_save_config(void);

void platform_reset_into_bootloader(void);

uint32_t platform_adc_samplerate(void);
uint32_t platform_knock_samplerate(void);

void set_sim_wakeup(timeval_t);

/* Data structure provided by the platform to the reschedule callback to
 * represent the period of time that the platform will work with next.
 *
 * The platform populates the schedulable start/end time range, the current
 * values for the gpio and pwm outputs, and a pointer to the viaems event
 * schedule provided to the platform on init.
 *
 * It is expected that the reschedule callback modifies the event schedulable,
 * gpio, and pwm values.
 */
struct platform_plan {
  timeval_t schedulable_start;
  timeval_t schedulable_end;

  int n_events;
  struct schedule_entry *schedule[MAX_EVENTS * 2];

  uint32_t gpio;
  float pwm[MAX_PWM];
};

static inline void plan_set_gpio(struct platform_plan *plan,
                                 int pin,
                                 bool value) {
  if ((pin >= 0) && (pin < MAX_GPIOS)) {
    if (value) {
      plan->gpio |= (1 << pin);
    } else {
      plan->gpio &= ~(1 << pin);
    }
  }
}

static inline void plan_set_pwm(struct platform_plan *plan,
                                int pin,
                                float value) {
  if ((pin >= 0) && (pin < MAX_PWM)) {
    plan->pwm[pin] = value;
  }
}

#ifdef UNITTEST
#include <check.h>

void check_platform_reset(void);
void set_current_time(timeval_t);
#endif

#endif
