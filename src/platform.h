#ifndef _PLATFORM_H
#define _PLATFORM_H
#include <stdbool.h>
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

/* Set event timer to fire at given time. Timer will immediately be scheduled to
 * fire if the time has already passed -- the callback is gauranteed to be
 * called. This is done by:
 * 1) Disable any pending timer event
 * 2) Set the event timer and enable it
 * 3) Check if the event is now in the past, and if so set it pending
 */
void schedule_event_timer(timeval_t);

/* Clear any pending timer */
void clear_event_timer(void);

void platform_init(int argc, char *argv[]);
/* Benchmark init is minimum necessary to use platform for benchmark */
void platform_benchmark_init(void);

void disable_interrupts(void);
void enable_interrupts(void);
bool interrupts_enabled(void);

void set_output(int output, char value);
int get_output(int output);
void set_gpio(int output, char value);
int get_gpio(int output);
void set_pwm(int output, float percent);

/* Non-blocking read from stream console */
size_t platform_stream_read(size_t size, uint8_t buffer[size]);

/* Non-blocking write to stream console */
size_t platform_stream_write(size_t size, const uint8_t buffer[size]);

void platform_load_config(void);
void platform_save_config(void);

void platform_reset_into_bootloader(void);

uint32_t platform_adc_samplerate(void);
uint32_t platform_knock_samplerate(void);

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
