#ifndef _PLATFORM_H
#define _PLATFORM_H
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
typedef uint16_t degrees_t;
/* timeval_t is gauranteed to be 32 bits */

timeval_t current_time();

void set_event_timer(timeval_t);
timeval_t get_event_timer();
/* Clear any pending interrupt */
void clear_event_timer();
void disable_event_timer();

void platform_init();

void disable_interrupts();
void enable_interrupts();
int interrupts_enabled();

void set_output(int output, char value);
void adc_gather(void *);

int usart_tx_ready();
int usart_rx_ready();
void usart_rx_reset();
void usart_tx(char *, unsigned short);

#endif

