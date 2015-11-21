#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/cortex.h>

#include "decoder.h"
#include "platform.h"
#include "scheduler.h"
#include "limits.h"
#include "adc.h"
#include "config.h"

static struct decoder *decoder;
static uint16_t adc_dma_buf[MAX_ADC_INPUTS];
static uint8_t adc_pins[MAX_ADC_INPUTS];

/* Hardware setup:
 *  LD3 - Orange - PD13
 *  LD4 - Green - PD12
 *  LD5 - Red - PD14
 *  LD6 - Blue - PD15
 *  BUT1 - User - PA0
 *
 *  ADC Pin 0-7 - A1-A8
 *
 *  T0 - Primary Trigger - PB0
 *  T1 - Primary Trigger - PB1
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

  /* Set up DMA for the ADC */
  rcc_periph_clock_enable(RCC_DMA2);
  nvic_enable_irq(NVIC_DMA2_STREAM0_IRQ);

  /* Set up ADC */
  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_ADC1);
  for (int i = 0; i < MAX_ADC_INPUTS; ++i) {
    if (config.adc[i].pin) {
      adc_pins[i] = config.adc[i].pin;
      gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, (1<<adc_pins[i]));
    }
  }
  adc_off(ADC1);
  adc_enable_scan_mode(ADC1);
  adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_15CYC);
  adc_power_on(ADC1);
}

void adc_gather(void *_unused) {
  set_output(15, 1);
  adc_disable_dma(ADC1);
  adc_enable_dma(ADC1);
  adc_clear_overrun_flag(ADC1);

  dma_stream_reset(DMA2, DMA_STREAM0);
  dma_set_priority(DMA2, DMA_STREAM0, DMA_SxCR_PL_LOW);
  dma_set_memory_size(DMA2, DMA_STREAM0, DMA_SxCR_MSIZE_16BIT);
  dma_set_peripheral_size(DMA2, DMA_STREAM0, DMA_SxCR_PSIZE_16BIT);
  dma_enable_memory_increment_mode(DMA2, DMA_STREAM0);
  dma_set_transfer_mode(DMA2, DMA_STREAM0, DMA_SxCR_DIR_PERIPHERAL_TO_MEM);
  dma_set_peripheral_address(DMA2, DMA_STREAM0, (uint32_t) &ADC1_DR);
  dma_set_memory_address(DMA2, DMA_STREAM0, (uint32_t) adc_dma_buf);
  dma_set_number_of_data(DMA2, DMA_STREAM0, MAX_ADC_INPUTS);
  dma_enable_transfer_complete_interrupt(DMA2, DMA_STREAM0);
  dma_channel_select(DMA2, DMA_STREAM0, DMA_SxCR_CHSEL_0);
  dma_enable_stream(DMA2, DMA_STREAM0);

  adc_set_regular_sequence(ADC1, MAX_ADC_INPUTS, adc_pins);
  adc_start_conversion_regular(ADC1);
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

void dma2_stream0_isr(void) {
 if (dma_get_interrupt_flag(DMA2, DMA_STREAM0, DMA_TCIF)) {
   dma_clear_interrupt_flags(DMA2, DMA_STREAM0, DMA_TCIF);
   set_output(15, 0);
   for (int i = 0; i < MAX_ADC_INPUTS; ++i) {
     config.adc[i].raw_value = adc_dma_buf[i];
   }
   adc_notify();
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
