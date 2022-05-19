#include <stdbool.h>

#include "platform.h"
#include "decoder.h"
#include "util.h"
#include "device.h"

static void setup_tim8(void) {
  RCC->APB2ENR |= RCC_APB2ENR_TIM8EN;

  TIM8->CR1 = TIM8_CR1_DIR_VAL(0) | /* Upcounting */
              TIM8_CR1_URS_VAL(1); /* overflow generates DMA */
              /* Leave clock disabled until done */

  TIM8->CR2 = TIM8_CR2_MMS_VAL(2); /* TRGO on update */

  TIM8->ARR = 49; /* APB2 timer clock is 200 MHz, divide by (49+1) to get 4 MHz */

  TIM8->DIER = TIM8_DIER_UIE;
  TIM8->CR1 |= TIM8_CR1_CEN; /* Enable clock */
}

timeval_t current_time() {
  return TIM2->CNT;
}


void tim2_isr(void) {
  struct decoder_event evs[2];
  int n_events = 0;
  bool cc1_fired = false;
  bool cc2_fired = false;
  timeval_t cc1;
  timeval_t cc2;

  if ((TIM2->SR & TIM2_SR_CC1IF) == TIM2_SR_CC1IF) {
    /* TI1 capture */
    cc1_fired = true;
    cc1 = TIM2->CCR1;
    TIM2->SR &= ~TIM2_SR_CC1IF;
  }

  if ((TIM2->SR & TIM2_SR_CC2IF) == TIM2_SR_CC2IF) {
    /* TI1 capture */
    cc2_fired = true;
    cc2 = TIM2->CCR2;
    TIM2->SR &= ~TIM2_SR_CC2IF;
  }

  if (((TIM2->SR & TIM2_SR_CC1OF) == TIM2_SR_CC1OF) ||
      ((TIM2->SR & TIM2_SR_CC2OF) == TIM2_SR_CC2OF)) {
    TIM2->SR &= ~(TIM2_SR_CC1OF | TIM2_SR_CC2OF);
    decoder_desync(DECODER_OVERFLOW);
    return;
  }

  if (cc1_fired && cc2_fired && time_before(cc2, cc1)) {
    decoder_update_scheduling(1, cc2);
    decoder_update_scheduling(0, cc1);
  } else {
    if (cc1_fired) {
      decoder_update_scheduling(0, cc1);
    }
    if (cc2_fired) {
      decoder_update_scheduling(1, cc2);
    }
  }

}

static void setup_tim2(void) {
  RCC->APB1LENR |= RCC_APB1LENR_TIM2EN;
  TIM2->CR1 = TIM2_CR1_UDIS; /* Upcounting, no update event */
  TIM2->SMCR = TIM2_SMCR_TS_VAL(1) | /* Trigger ITR1 is TIM8's TRGO */
               TIM2_SMCR_SMS_VAL(7); /* External Clock Mode 1 */
  TIM2->ARR = 0xFFFFFFFF; /* Count through whole 32bits */

  /* Set up input triggers */
  TIM2->CCMR1_Input = TIM2_CCMR1_Input_IC1F_VAL(1) | /* CK_INT, N=2 filter */
                      TIM2_CCMR1_Input_CC1S_VAL(1) | /* IC1 */
                      TIM2_CCMR1_Input_IC2F_VAL(1) | /* CK_INT, N=2 filter */
                      TIM2_CCMR1_Input_CC2S_VAL(1);  /* IC2 */
  TIM2->CCER = TIM2_CCER_CC1P_VAL(0) | TIM2_CCER_CC1NP_VAL(0) | /* Rising edge */
               TIM2_CCER_CC2P_VAL(0) | TIM2_CCER_CC2NP_VAL(0) | /* Rising edge */
               TIM2_CCER_CC1E | TIM2_CCER_CC2E; /* Enable */

  /*Enable interrupts for CC1/CC2 */
  TIM2->DIER = TIM8_DIER_CC1IE | TIM8_DIER_CC2IE;
  nvic_enable_irq(TIM2_IRQ); 

  /* A0 and A1 as trigger inputs */
  GPIOA->MODER &= ~(GPIOA_MODER_MODE0 | GPIOA_MODER_MODE1);
  GPIOA->MODER |= GPIOA_MODER_MODE0_VAL(2) | GPIOA_MODER_MODE1_VAL(2); /* A0/A1 in AF mode */
  GPIOA->AFRL |= GPIOA_AFRL_AFSEL0_VAL(1) | GPIOA_AFRL_AFSEL1_VAL(1); /* AF1 (TIM2)*/
  
  TIM2->CR1 |= TIM2_CR1_CEN; /* Enable clock */
}

void platform_init_scheduler() {
  setup_tim2();
  setup_tim8();
}
