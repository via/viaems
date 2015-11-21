#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "decoder.h"
#include "platform.h"
#include "limits.h"

static unsigned int time_high = 0;
static struct decoder *decoder;
unsigned int *adc_mapping[] = {NULL, NULL, NULL, NULL};

FUSES = {
  .low = (FUSE_CKDIV8 & FUSE_SUT0 & FUSE_CKSEL3 & FUSE_CKSEL2 & FUSE_CKSEL1),
  .high = (FUSE_SPIEN & FUSE_BODLEVEL1 & FUSE_BODLEVEL0),
};

void platform_init(struct decoder *d, struct analog_inputs *a) {

  decoder = d;
  adc_mapping[0] = &a->IAT;
  adc_mapping[1] = &a->CLT;
  adc_mapping[2] = &a->MAP;
  /* Set up clocks*/
  /* CKSEL3:0 - 0001, 16 Mhz */
  /* Set CLKPCE high then immediately write CLKPS */
  CLKPR = _BV(CLKPCE);
  CLKPR = 0;

  /* Set up overflow interrupt */
  TCCR0A |= _BV(TCW0); /* 16-bit width */
  TCCR0B |= _BV(CS00); /* No prescaling */
  TIMSK |= _BV(TOIE0); /* Timer overflow interrupt */

  /* Set up INT0 interrupt */
  MCUCR |= _BV(ISC00); /* Both edges */
  GIMSK |= _BV(INT0);

  /* Setup output */
  PORTB = 0;
  DDRB |= _BV(DDB0);
  DDRB |= _BV(DDB1);
  DDRB |= _BV(DDB2);
  DDRB |= _BV(DDB3);

  /* ADC configuration */

  /* Enable ADC, Interrupt, divider 128 for 125kHz clock */
  ADCSRA = _BV(ADEN) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
  ADMUX = 0; /* Vcc ref, mux 0 */

  SREG |= _BV(SREG_I);

}

ISR(INT0_vect) {
  PORTB ^= _BV(PORTB1);
  /* If falling edge, its a trigger. Otherwise ADC */
  if (PINB & _BV(PINB6)) {
    /* Kick off ADC */
    ADMUX &= 0xE0;  /* MUX = 0 */
    ADCSRA |= _BV(ADSC);
  } else {
    timeval_t cur = current_time();
    decoder->last_t0 = cur;
    decoder->needs_decoding = 1;
  }
}

ISR(TIMER0_OVF_vect) {
  time_high++;
}

ISR(ADC_vect) {
  unsigned int val = ADCL;
  val |= ((ADCH & 0x3) << 8);
  unsigned char curmux = ADMUX & 0x1F;
  *adc_mapping[curmux] = val;
  curmux += 1;
  if (adc_mapping[curmux] != NULL) {
    ADMUX = (ADMUX & 0xE0) | curmux;
    ADCSRA |= _BV(ADSC);
  }
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
  unsigned char mask = 1 << output;
  if (value) {
    PORTB |= mask;
  } else {
    PORTB &= ~mask;
  }
}

