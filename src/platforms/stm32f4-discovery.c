#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/cortex.h>

#include "decoder.h"
#include "platform.h"
#include "factors.h"
#include "scheduler.h"
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
  gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO10);
  gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO11);
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
  /* Setup output compare registers */
  timer_disable_oc_output(TIM2, TIM_OC1);
  timer_disable_oc_output(TIM2, TIM_OC2);
  timer_disable_oc_output(TIM2, TIM_OC3);
  timer_disable_oc_output(TIM2, TIM_OC4);
	timer_disable_oc_clear(TIM2, TIM_OC1);
	timer_disable_oc_preload(TIM2, TIM_OC1);
	timer_set_oc_slow_mode(TIM2, TIM_OC1);
	timer_set_oc_mode(TIM2, TIM_OC1, TIM_OCM_FROZEN);
  timer_enable_counter(TIM2);
	nvic_enable_irq(NVIC_TIM2_IRQ);

  /* Set up interrupt on B0 */
  rcc_periph_clock_enable(RCC_SYSCFG);
  nvic_enable_irq(NVIC_EXTI0_IRQ);
  exti_select_source(EXTI0, GPIOB);
  exti_set_trigger(EXTI0, EXTI_TRIGGER_FALLING);
  exti_enable_request(EXTI0);

}

void exti0_isr() {
  exti_reset_request(EXTI0);

  decoder->last_t0 = current_time();
  decoder->needs_decoding = 1;
}

void tim2_isr() {
  if (timer_get_flag(TIM2, TIM_SR_CC1IF)) {
    timer_clear_flag(TIM2, TIM_SR_CC1IF);
    scheduler_execute();
  }
}

void enable_interrupts() {
  cm_enable_interrupts();
}

void disable_interrupts() {
  cm_disable_interrupts();
}

void set_event_timer(timeval_t t) {
	timer_set_oc_value(TIM2, TIM_OC1, t);
	timer_enable_irq(TIM2, TIM_DIER_CC1IE);
}

timeval_t get_event_timer() {
  return TIM2_CCR1;
}

void clear_event_timer() {
  timer_clear_flag(TIM2, TIM_SR_CC1IF);
}

void disable_event_timer() {
	timer_disable_irq(TIM2, TIM_DIER_CC1IE);
  timer_clear_flag(TIM2, TIM_SR_CC1IF);
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

/* Initiate ADC gathering via DMA */
void adc_gather(void *_unused) {
}
