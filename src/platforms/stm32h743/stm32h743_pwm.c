#include "stm32h743xx.h"

void set_pwm(int pin, float val) {
  uint16_t arr = 0;
  switch(pin) {
    case 0:
      arr = TIM16->ARR;
      break;
    case 1:
      arr = TIM17->ARR;
      break;
  }

  uint16_t raw;
  if (val < 0.0f) {
    raw = 0;
  } else if (val >= 100.0f) {
    raw = arr;
  } else {
    raw = (val / 100.0f) * arr;
  }

  switch(pin) {
    case 0:
      TIM16->CCR1 = raw;
      break;
    case 1:
      TIM17->CCR1 = raw;
      break;
  }
}

void set_pwm_frequency(int pin, uint32_t frequency) {
  uint32_t prescaler = (200000000.0f / 65536.0f) / frequency;
  if (prescaler > 65535) {
    prescaler = 65535;
  }

  float real_freq = 200000000 / (float)(prescaler + 1) / 65536;
  float ratio = real_freq / frequency;
  uint16_t max_arr = ratio * 65535;

  switch (pin) {
    case 0:
      TIM16->PSC = (uint16_t)prescaler;
      TIM16->ARR = (uint16_t)max_arr;
      break;
    case 1:
      TIM17->PSC = (uint16_t)prescaler;
      TIM17->ARR = (uint16_t)max_arr;
      break;
  }
}

void platform_configure_pwm(void) {
  /* Configure PB8 and 9 as AF1 for TIM16/17 */
  GPIOB->MODER &= ~(GPIO_MODER_MODE8 | GPIO_MODER_MODE9);
  GPIOB->MODER |= GPIO_MODER_MODE8_1 | GPIO_MODER_MODE9_1; /* AF */
  GPIOB->AFR[1] |=
    _VAL2FLD(GPIO_AFRH_AFSEL8, 1) | _VAL2FLD(GPIO_AFRH_AFSEL9, 1); /* AF1 */

  TIM16->ARR = 65535;
  TIM16->CCMR1 = _VAL2FLD(TIM_CCMR1_OC1M, 6) | /* PWM mode 1 */
                 TIM_CCMR1_OC1PE; /* Preload enable */
  TIM16->CCER = TIM_CCER_CC1E; /* Enable CC1 */
  TIM16->CR1 = TIM_CR1_ARPE; /* ARR Preload */
  TIM16->EGR |= TIM_EGR_UG; /* Update to trigger preload */
  TIM16->BDTR = TIM_BDTR_MOE; /* Main output enable */
  TIM16->CR1 |= TIM_CR1_CEN; /* Enable clock */


  TIM17->ARR = 65535;
  TIM17->CCMR1 = _VAL2FLD(TIM_CCMR1_OC1M, 6) | /* PWM mode 1 */
                 TIM_CCMR1_OC1PE; /* Preload enable */
  TIM17->CCER = TIM_CCER_CC1E; /* Enable CC1 */
  TIM17->CR1 = TIM_CR1_ARPE; /* ARR Preload */
  TIM17->EGR |= TIM_EGR_UG; /* Update to trigger preload */
  TIM17->BDTR = TIM_BDTR_MOE; /* Main output enable */
  TIM17->CR1 |= TIM_CR1_CEN; /* Enable clock */

  /* TODO: Allow this to be runtime configurable */
  set_pwm_frequency(0, 30000);
  set_pwm(0, 0.0f);

  set_pwm_frequency(1, 30);
  set_pwm(1, 0.0f);
}


