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

size_t console_read(void *buf, size_t max);
size_t console_write(const void *buf, size_t count);

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
