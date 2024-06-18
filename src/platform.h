#ifndef _PLATFORM_H
#define _PLATFORM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint32_t timeval_t;
/* timeval_t is gauranteed to be 32 bits */

/* Move to platform-specific header defines */
uint64_t cycles_to_ns(uint64_t cycles);

/* Set event timer to fire at given time. Timer will immediately be scheduled to
 * fire if the time has already passed -- the callback is gauranteed to be
 * called. This is done by:
 * 1) Disable any pending timer event
 * 2) Set the event timer and enable it
 * 3) Check if the event is now in the past, and if so set it pending
 */

void platform_init();
/* Benchmark init is minimum necessary to use platform for benchmark */
void platform_benchmark_init(void);

/* Move to platform-specific header defines */
uint32_t platform_adc_samplerate(void);
uint32_t platform_knock_samplerate(void);

#ifdef UNITTEST
#include <check.h>

void check_platform_reset(void);
void set_current_time(timeval_t);
#endif

#endif



