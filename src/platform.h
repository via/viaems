#ifndef _PLATFORM_H
#define _PLATFORM_H
#include <stddef.h>
#include <stdint.h>

#ifndef NULL
#define NULL 0
#endif

struct decoder;
#ifdef TIMER64
typedef uint64_t timeval_t;
#else
typedef uint32_t timeval_t;
#endif
typedef float degrees_t;
/* timeval_t is gauranteed to be 32 bits */

timeval_t current_time(void);
uint64_t cycle_count(void);
uint64_t cycles_to_ns(uint64_t cycles);

void set_event_timer(timeval_t);
timeval_t get_event_timer(void);
void disable_event_timer(void);

/* Clear any pending interrupt */
void clear_event_timer(void);

/* Cause event timer handler to trigger in interrupt context */
void pend_event_timer(void);

void platform_init();
/* Benchmark init is minimum necessary to use platform for benchmark */
void platform_benchmark_init(void);

void disable_interrupts(void);
void enable_interrupts(void);
int interrupts_enabled(void);

void set_output(int output, char value);
int get_output(int output);
void set_gpio(int output, char value);
int get_gpio(int output);
void set_pwm(int output, float percent);

size_t console_read(void *buf, size_t max);
size_t console_write(const void *buf, size_t count);

void platform_load_config(void);
void platform_save_config(void);

void platform_enable_event_logging(void);
void platform_disable_event_logging(void);
void platform_reset_into_bootloader(void);

void set_test_trigger_rpm(uint32_t rpm);
uint32_t get_test_trigger_rpm(void);

/* Returns the earliest time that may still be scheduled.  This can only change
 * when buffers are swapped, so it is safe to use this value to schedule events
 * if interrupts are disabled */
timeval_t platform_output_earliest_schedulable_time(void);

#ifdef UNITTEST
#include <check.h>

void check_platform_reset(void);
void set_current_time(timeval_t);
#endif

#endif
