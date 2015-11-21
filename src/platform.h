#ifndef _PLATFORM_H
#define _PLATFORM_H
#include <stdint.h>

#ifndef NULL
#define NULL 0
#endif

struct decoder;
struct analog_inputs;
typedef uint32_t timeval_t;
typedef uint16_t degrees_t;
/* timeval_t is gauranteed to be 32 bits */

timeval_t current_time();

void set_event_timer(timeval_t);
timeval_t get_event_timer();
/* Clear any pending interrupt */
void clear_event_timer();
void disable_event_timer();

void platform_init(struct decoder *, struct analog_inputs *);

void disable_interrupts();
void enable_interrupts();

void set_output(int output, char value);
void adc_gather(void *);

#endif
