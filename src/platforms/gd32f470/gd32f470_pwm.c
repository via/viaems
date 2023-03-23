#include "platform.h"
#include "tasks.h"

#include "gd32f4xx.h"

/* Use TIMER2 as a fixed-frequency 4-output PWM timer.  This has the downside of
 * requiring all four PWM outputs to be the same frequency, but uses a
 * contiguous port-C block that isn't used elsewhere.  In the future we can also
 * use TIMER8 and TIMER11 to add more frequency selections.  Other
 * considerations were TIMER9, 10, and 12, but their outputs overlap with CAN0
 * and SPI.
 */

/* For now, only set up TIMER2 hardcoded to 30 Hz, and configure PC6-PC9 as the
 * four outputs */
void gd32f470_configure_pwm(void) {
  uint32_t pins = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9;
  gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, pins);
  gpio_af_set(GPIOC, GPIO_AF_2, pins);

  /* TIMER2 is 2x APB1's clock = 96 MHz, divide by 48 results in a period of 30
   * Hz when counting to 65536 */
  TIMER_CAR(TIMER2) = 65535;
  TIMER_PSC(TIMER2) = 48;

  /* Set all outputs to zero */
  TIMER_CH0CV(TIMER2) = 0;
  TIMER_CH1CV(TIMER2) = 0;
  TIMER_CH2CV(TIMER2) = 0;
  TIMER_CH3CV(TIMER2) = 0;

  /* All four outputs as PWM0 with shadow preload enabled */
  TIMER_CHCTL0(TIMER2) = TIMER_OC_MODE_PWM0 | TIMER_CHCTL0_CH0COMSEN |
                         (TIMER_OC_MODE_PWM0 << 8) | TIMER_CHCTL0_CH1COMSEN;
  TIMER_CHCTL1(TIMER2) = TIMER_OC_MODE_PWM0 | TIMER_CHCTL1_CH2COMSEN |
                         (TIMER_OC_MODE_PWM0 << 8) | TIMER_CHCTL1_CH3COMSEN;
  /* Enable output compares */
  TIMER_CHCTL2(TIMER2) = TIMER_CHCTL2_CH0EN | TIMER_CHCTL2_CH1EN |
                         TIMER_CHCTL2_CH2EN | TIMER_CHCTL2_CH3EN;

  TIMER_CTL0(TIMER2) = TIMER_CTL0_CEN;
}

void set_pwm(int output, float percent) {
  uint16_t duty = (percent / 100.0f) * 65535;
  switch (output) {
  case 1:
    TIMER_CH0CV(TIMER2) = duty;
    break;
  case 2:
    TIMER_CH1CV(TIMER2) = duty;
    break;
  case 3:
    TIMER_CH2CV(TIMER2) = duty;
    break;
  case 4:
    TIMER_CH3CV(TIMER2) = duty;
    break;
  }
}
