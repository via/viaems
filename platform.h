#ifndef _PLATFORM_H
#define _PLATFORM_H

#define TICKRATE 20000000

typedef unsigned long timeval_t;
typedef unsigned int degrees_t;
/* timeval_t is gauranteed to be 32 bits */

timeval_t current_time();
void platform_init();

void disable_interrupts();
void enable_interrupts();

void set_output(int output, char value);

#endif
