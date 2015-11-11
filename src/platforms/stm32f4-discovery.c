#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/nvic.h>

#include "decoder.h"
#include "platform.h"
#include "factors.h"
#include "limits.h"

static struct decoder *decoder;

/* Hardware setup:
 *  LD3 - Orange - PD13
 *  LD4 - Green - PD12
 *  LD5 - Red - PD14
 *  LD6 - Blue - PD15
 *  B1 - User - PA0
 *
 *  T0 - Primary Trigger - PB0
 */

void platform_init(struct decoder *d, struct analog_inputs *a) {
  decoder = d;

  /* 168 Mhz clock */
  rcc_clock_setup_hse_3v3(&hse_8mhz_3v3[CLOCK_3V3_168MHZ]);

  /* Port D gpio clock */
  rcc_periph_clock_enable(RCC_GPIOD);
  rcc_periph_clock_enable(RCC_GPIOB);
  gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO12);
  gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);
  gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO14);
  gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO15);
  gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO0);

  /* Set up TIM2 as 32bit clock */
  rcc_periph_clock_enable(RCC_TIM2);
  timer_reset(TIM2);
  timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
  timer_set_period(TIM2, 0xFFFFFFFF);
  timer_set_prescaler(TIM2, 0);
  timer_disable_preload(TIM2);
  timer_continuous_mode(TIM2);
  timer_enable_counter(TIM2);

  /* Set up interrupt on B0 */
  rcc_periph_clock_enable(RCC_SYSCFG);
  nvic_enable_irq(NVIC_EXTI0_IRQ);
  exti_select_source(EXTI0, GPIOB);
  exti_set_trigger(EXTI0, EXTI_TRIGGER_FALLING);
  exti_enable_request(EXTI0);

}

void exti0_isr() {
  exti_reset_request(EXTI0);
  timeval_t cur = current_time();
  decoder->last_t0 = cur;
  decoder->needs_decoding = 1;
}

void enable_interrupts() {
}

void disable_interrupts() {
}

timeval_t current_time() {
  return timer_get_counter(TIM2);
}

void set_output(int output, char value) {
  if (value) {
    gpio_set(GPIOD, (1 << output));
  } else {
    gpio_clear(GPIOD, (1 << output));
  }
}
