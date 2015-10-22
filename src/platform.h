#ifndef _PLATFORM_H
#define _PLATFORM_H
#include <stdint.h>

#define TICKRATE 20000000

struct decoder;
typedef uint32_t timeval_t;
typedef uint16_t degrees_t;
/* timeval_t is gauranteed to be 32 bits */

timeval_t current_time();
void platform_init(struct decoder *);

void disable_interrupts();
void enable_interrupts();

void set_output(int output, char value);

#endif
