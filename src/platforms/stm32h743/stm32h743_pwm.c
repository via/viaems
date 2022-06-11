#include "stm32h743xx.h"

/* Configure TIM13 and TIM14 as PWM outputs with independently configurable
 * clock rates */



void set_pwm(int pin, float val) {
  uint16_t raw;
  if (val < 0.0f) {
    raw = 0;
  } else if (val >= 1.0f) {
    raw = 65535;
  } else {
    raw = (val / 100.0f) * 65535;
  }
  if (pin == 0) {
    TIM16->CCR1 = raw;
  }
}

void platform_configure_pwm(void) {
  /* Configure PB8 and 9 as AF1 for TIM16/17 */
  GPIOB->MODER &= ~(GPIO_MODER_MODE8 | GPIO_MODER_MODE9);
  GPIOB->MODER |= GPIO_MODER_MODE8_1 | GPIO_MODER_MODE9_1; /* AF */
  GPIOB->AFR[1] |=
    _VAL2FLD(GPIO_AFRH_AFSEL8, 1) | _VAL2FLD(GPIO_AFRH_AFSEL9, 1); /* AF1 */

  TIM16->PSC = 100; /* Full 64k represents 30 Hz */
  TIM16->ARR = 65535;

  TIM16->CCMR1 = _VAL2FLD(TIM_CCMR1_OC1M, 6) | /* PWM mode 1 */
                 TIM_CCMR1_OC1PE; /* Preload enable */

  TIM16->CCER = TIM_CCER_CC1E; /* Enable CC1 */
  TIM16->CR1 = TIM_CR1_ARPE; /* ARR Preload */
  TIM16->EGR |= TIM_EGR_UG; /* Update to trigger preload */
  TIM16->BDTR = TIM_BDTR_MOE; /* Main output enable */
  TIM16->CR1 |= TIM_CR1_CEN; /* Enable clock */
}


