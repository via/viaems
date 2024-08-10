#include "platform.h"
#include "tasks.h"

#include "stm32f427xx.h"

/* Use TIM3 as a fixed-frequency 4-output PWM timer.  This has the downside of
 * requiring all four PWM outputs to be the same frequency, but uses a
 * contiguous port-C block that isn't used elsewhere.  In the future we can also
 * use TIMER9 and TIMER12 to add more frequency selections.  Other
 * considerations were TIMER10, 11, and 13, but their outputs overlap with CAN0
 * and SPI.
 */

/* For now, only set up TIM3 hardcoded to 30 Hz, and configure PC6-PC9 as the
 * four outputs */
void stm32f4_configure_pwm(void) {
  /* Set GPIOC 6-9 to AF */
  GPIOC->MODER |= _VAL2FLD(GPIO_MODER_MODE6, 2) |
                  _VAL2FLD(GPIO_MODER_MODE7, 2) |
                  _VAL2FLD(GPIO_MODER_MODE8, 2) | _VAL2FLD(GPIO_MODER_MODE9, 2);

  /* AF 2 */
  GPIOC->AFR[0] |=
    _VAL2FLD(GPIO_AFRL_AFSEL6, 2) | _VAL2FLD(GPIO_AFRL_AFSEL7, 2);
  GPIOC->AFR[1] |=
    _VAL2FLD(GPIO_AFRH_AFSEL8, 2) | _VAL2FLD(GPIO_AFRH_AFSEL9, 2); /* AF1 */

  TIM3->ARR = 65535;
  /* TIM3 is 2x APB1's clock = 84 MHz, Hz when counting to 65536 we want to
   * target 30 Hz */
  uint32_t period = 84000000 / 65536 / 30;
  TIM3->PSC = period - 1;

  TIM3->CCR1 = 0;
  TIM3->CCR2 = 0;
  TIM3->CCR3 = 0;
  TIM3->CCR4 = 0;

  /* Enable preload and PWM mode 1 for all outputs */
  TIM3->CCMR1 = _VAL2FLD(TIM_CCMR1_OC1M, 0x6) | TIM_CCMR1_OC1PE |
                _VAL2FLD(TIM_CCMR1_OC2M, 0x6) | TIM_CCMR1_OC2PE;
  TIM3->CCMR2 = _VAL2FLD(TIM_CCMR2_OC3M, 0x6) | TIM_CCMR2_OC3PE |
                _VAL2FLD(TIM_CCMR2_OC4M, 0x6) | TIM_CCMR2_OC4PE;

  /* Enable all outputs */
  TIM3->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E;

  TIM3->CR1 = TIM_CR1_CEN; /* Start timer */
}

void set_pwm(int output, float percent) {
  uint16_t duty = (percent / 100.0f) * 65535;
  switch (output) {
  case 1:
    TIM3->CCR1 = duty;
    break;
  case 2:
    TIM3->CCR2 = duty;
    break;
  case 3:
    TIM3->CCR3 = duty;
    break;
  case 4:
    TIM3->CCR4 = duty;
    break;
  }
}
