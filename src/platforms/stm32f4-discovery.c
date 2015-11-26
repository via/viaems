#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/cortex.h>

#include "decoder.h"
#include "platform.h"
#include "scheduler.h"
#include "limits.h"
#include "adc.h"
#include "util.h"
#include "config.h"

static struct decoder *decoder;
static uint16_t adc_dma_buf[MAX_ADC_INPUTS];
static uint8_t adc_pins[MAX_ADC_INPUTS];
static char *usart_rx_dest;
static int usart_rx_end;

/* Hardware setup:
 *  LD3 - Orange - PD13
 *  LD4 - Green - PD12
 *  LD5 - Red - PD14
 *  LD6 - Blue - PD15
 *  BUT1 - User - PA0
 *
 *  Test trigger out - A0
 *
 *  ADC Pin 0-7 - A1-A8
 *
 *  T0 - Primary Trigger - PB3
 *  T1 - Secondary Trigger - PB4
 *  USART1_TX - PB6 (dma2 stream 7 chan 4)
 *  USART1_RX - PB7 (dma2 stream 5 chan 4)
 */

void platform_init(struct decoder *d) {
  decoder = d;

  /* 168 Mhz clock */
  rcc_clock_setup_hse_3v3(&hse_8mhz_3v3[CLOCK_3V3_168MHZ]);

  /* Port D gpio clock */
  rcc_periph_clock_enable(RCC_GPIOD);
  rcc_periph_clock_enable(RCC_GPIOB);
  /* IG1 out */
  gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO0);

  /*LEDs*/
  gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO12);
  gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);
  gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO14);
  gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO15);
  /* Trigger */
  gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO3);
  gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO4);

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

  /* Set up interrupt on B3 */
  rcc_periph_clock_enable(RCC_SYSCFG);
  nvic_enable_irq(NVIC_EXTI3_IRQ);
  exti_select_source(EXTI3, GPIOB);
  exti_set_trigger(EXTI3, EXTI_TRIGGER_FALLING);
  exti_enable_request(EXTI3);

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

  /* USART initialization */
/*	nvic_enable_irq(NVIC_USART1_IRQ); */

	gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6);
	gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO7);
	gpio_set_output_options(GPIOB, GPIO_OTYPE_OD, GPIO_OSPEED_25MHZ, GPIO7);

	/* Setup USART1 TX and RX pin as alternate function. */
	gpio_set_af(GPIOB, GPIO_AF7, GPIO6);
	gpio_set_af(GPIOB, GPIO_AF7, GPIO7);

	/* Setup USART2 parameters. */
  rcc_periph_clock_enable(RCC_USART1);
	usart_set_baudrate(USART1, config.console.baud);
	usart_set_databits(USART1, config.console.data_bits);
  switch (config.console.stop_bits) {
    case 1:
      usart_set_stopbits(USART1, USART_STOPBITS_1);
      break;
    default:
      while (1);
  }
	usart_set_mode(USART1, USART_MODE_TX_RX);
  switch (config.console.parity) {
    case 'N':
      usart_set_parity(USART1, USART_PARITY_NONE);
      break;
    case 'O':
      usart_set_parity(USART1, USART_PARITY_ODD);
      break;
    case 'E':
      usart_set_parity(USART1, USART_PARITY_EVEN);
      break;
  }
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);

	/* Finally enable the USART. */
	usart_enable(USART1);
/*  usart_rx_reset(); */
}

void usart_rx_reset() {
  usart_rx_dest = config.console.rxbuffer;
  usart_rx_end = 0;
  usart_enable_rx_interrupt(USART1);
}


void usart1_isr() {
  if (usart_rx_end) {
    /* Already have unprocessed line in buffer */
    return;
  }

  *usart_rx_dest = usart_recv(USART1);
  if (*usart_rx_dest == '\n') {
    usart_rx_end = 1;
  }

  usart_rx_dest++;
  if (!usart_rx_end && 
      usart_rx_dest == config.console.rxbuffer + CONSOLE_BUFFER_SIZE) {
    /* Buffer full */
    usart_rx_reset();
    return;
  }
}

int usart_tx_ready() {
  return usart_get_flag(USART1, USART_SR_TC);
}

int usart_rx_ready() {
  return usart_rx_end;
}

void usart_tx(char *buf, unsigned short len) {
  usart_enable_tx_dma(USART1);
  dma_stream_reset(DMA2, DMA_STREAM7);
  dma_set_priority(DMA2, DMA_STREAM7, DMA_SxCR_PL_LOW);
  dma_set_memory_size(DMA2, DMA_STREAM7, DMA_SxCR_MSIZE_8BIT);
  dma_set_peripheral_size(DMA2, DMA_STREAM7, DMA_SxCR_PSIZE_8BIT);
  dma_enable_memory_increment_mode(DMA2, DMA_STREAM7);
  dma_set_transfer_mode(DMA2, DMA_STREAM7, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
  dma_set_peripheral_address(DMA2, DMA_STREAM7, (uint32_t) &USART1_DR);
  dma_set_memory_address(DMA2, DMA_STREAM7, (uint32_t) buf);
  dma_set_number_of_data(DMA2, DMA_STREAM7, len);
  dma_enable_transfer_complete_interrupt(DMA2, DMA_STREAM7);
  dma_channel_select(DMA2, DMA_STREAM7, DMA_SxCR_CHSEL_4);
  dma_enable_stream(DMA2, DMA_STREAM7);

}

void adc_gather(void *_unused) {
  adc_disable_dma(ADC1);
  adc_enable_dma(ADC1);
  adc_clear_overrun_flag(ADC1);

  dma_stream_reset(DMA2, DMA_STREAM0);
  dma_set_priority(DMA2, DMA_STREAM0, DMA_SxCR_PL_HIGH);
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

void exti3_isr() {
  exti_reset_request(EXTI3);
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

void enable_test_trigger(trigger_type trig, unsigned int rpm) {
  if (trig != FORD_TFI) {
    return;
  }

  timeval_t t = time_from_rpm_diff(rpm, 45);

  /* Set up TIM5 as 32bit clock */
  rcc_periph_clock_enable(RCC_TIM5);
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO0);
	gpio_set_af(GPIOA, GPIO_AF2, GPIO0);
  timer_reset(TIM5);
  timer_disable_oc_output(TIM5, TIM_OC1);
  timer_set_mode(TIM5, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
  timer_set_period(TIM5, (unsigned int)t);
  timer_set_prescaler(TIM5, 0);
  timer_disable_preload(TIM5);
  timer_continuous_mode(TIM5);
  /* Setup output compare registers */
  timer_ic_set_input(TIM5,  TIM_IC1, TIM_IC_OUT);
	timer_disable_oc_clear(TIM5, TIM_OC1);
	timer_disable_oc_preload(TIM5, TIM_OC1);
	timer_set_oc_slow_mode(TIM5, TIM_OC1);
	timer_set_oc_mode(TIM5, TIM_OC1, TIM_OCM_TOGGLE);
  timer_set_oc_value(TIM5, TIM_OC1, t);
  timer_set_oc_polarity_high(TIM5, TIM_OC1);
  timer_enable_oc_output(TIM5, TIM_OC1);
  timer_enable_counter(TIM5);
}

