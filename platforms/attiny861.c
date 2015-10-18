#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "decoder.h"
#include "platform.h"
#include "limits.h"

static unsigned int time_high = 0;
static struct decoder *decoder;

void platform_init(struct decoder *d) {

  decoder = d;
  /* Set up clocks*/
  /* Default CKSEL3:0 - 0010, 8 Mhz */
  /* Set CLKPCE high then immediately write CLKPS */
  CLKPR = _BV(CLKPCE);
  CLKPR = 0;

  /* Set up overflow interrupt */
  TCCR0A |= _BV(TCW0); /* 16-bit width */
  TCCR0B |= _BV(CS00); /* No prescaling */
  TIMSK |= _BV(TOIE0); /* Timer overflow interrupt */

  /* Set up INT0 interrupt */
  MCUCR |= _BV(ISC01); /* Falling Edge */
  GIMSK |= _BV(INT0);

  SREG |= _BV(SREG_I);
}

ISR(INT0_vect) {
  timeval_t cur = current_time();
  decoder->last_t0 = cur;
  decoder->needs_decoding = 1;
}

ISR(TIMER0_OVF_vect) {
  time_high++;
}

void enable_interrupts() {
  sei();
}

void disable_interrupts() {
  cli();
}

timeval_t current_time() {
  timeval_t cur = 0;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    cur = TCNT0L;
    cur |= (unsigned int)TCNT0H << 8;
    cur |= ((timeval_t)time_high << 16);
    /* If we've overflowed since we've had interrupts off,
     * and the timer bits are close to 0, we probably missed the
     * overflow */
    if (bit_is_set(TIFR, TOV0) && (cur & 0xFFFF) < 0x7FFF) {
      cur += ((timeval_t)1 << 16);
    }
  }
  return cur;
}

void set_output(int output, char value) {
}

