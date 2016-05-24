#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/cortex.h>

#include "decoder.h"
#include "platform.h"
#include "scheduler.h"
#include "limits.h"
#include "sensors.h"
#include "util.h"
#include "config.h"

static uint16_t adc_dma_buf[NUM_SENSORS];
static uint8_t adc_pins[NUM_SENSORS];
static char *usart_rx_dest;
static int usart_rx_end;

/* Hardware setup:
 *  Discovery board LEDs:
 *  LD3 - Orange - PD13
 *  LD4 - Green - PD12
 *  LD5 - Red - PD14
 *  LD6 - Blue - PD15
 *
 *  Fixed pin mapping:
 *  USART1_TX - PB6 (dma2 stream 7 chan 4)
 *  USART1_RX - PB7 (dma2 stream 5 chan 4)
 *  Test trigger out - A0 (Uses TIM5)
 *
 *  Configurable pin mapping:
 *  Triggers:
 *   - 0-4 maps to port B, pins 0-4
 *  ADC: 
 *   - 1-7 maps to port A, pins 1-7
 *  Outputs:
 *   - 0-15 maps to port D, pins 0-15
 *  Freq sensor:
 *   - 1 maps to port A, pin 8 (TIM1 CH1)
 *  PWM Outputs
 *   - 0-PC6  1-PC7 2-PC8 TIM3 channel 1-3
 *  GPIO (Digital Sensor or Output)
 *   - 0-7 maps to Port B 8-15
 */

static void platform_enable_trigger(int pin) {
  switch(pin) {
    case 0:
      gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO0);
      nvic_enable_irq(NVIC_EXTI0_IRQ);
      exti_select_source(EXTI0, GPIOB);
      exti_set_trigger(EXTI0, EXTI_TRIGGER_FALLING);
      exti_enable_request(EXTI0);
      break;
    case 1:
      gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO1);
      nvic_enable_irq(NVIC_EXTI1_IRQ);
      exti_select_source(EXTI1, GPIOB);
      exti_set_trigger(EXTI1, EXTI_TRIGGER_FALLING);
      exti_enable_request(EXTI1);
      break;
    case 2:
      gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO2);
      nvic_enable_irq(NVIC_EXTI2_IRQ);
      exti_select_source(EXTI2, GPIOB);
      exti_set_trigger(EXTI2, EXTI_TRIGGER_FALLING);
      exti_enable_request(EXTI2);
      break;
    case 3:
      gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO3);
      nvic_enable_irq(NVIC_EXTI3_IRQ);
      exti_select_source(EXTI3, GPIOB);
      exti_set_trigger(EXTI3, EXTI_TRIGGER_FALLING);
      exti_enable_request(EXTI3);
      break;
    case 4:
      gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO4);
      nvic_enable_irq(NVIC_EXTI4_IRQ);
      exti_select_source(EXTI4, GPIOB);
      exti_set_trigger(EXTI4, EXTI_TRIGGER_FALLING);
      exti_enable_request(EXTI4);
      break;
    default:
      break;
  }
}

static void platform_init_freqsensor(unsigned char pin) {
  uint32_t tim;
  switch(pin) {
    case 1:
      /* TIM1 CH1 */
      tim = TIM1;
      gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO8);
      gpio_set_af(GPIOA, GPIO_AF1, GPIO8);
      break;
  };
  timer_reset(tim);
  timer_set_mode(tim, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
  timer_set_period(tim, 0xFFFFFFFF);
  timer_disable_preload(tim);
  timer_continuous_mode(tim);
  /* Setup output compare registers */
  timer_disable_oc_output(tim, TIM_OC1);
  timer_disable_oc_output(tim, TIM_OC2);
  timer_disable_oc_output(tim, TIM_OC3);
  timer_disable_oc_output(tim, TIM_OC4);

  /* Set up compare */
  timer_ic_set_input(tim, TIM_IC1, TIM_IC_IN_TI1);
  timer_ic_set_filter(tim, TIM_IC1, TIM_IC_CK_INT_N_8);
  timer_ic_set_polarity(tim, TIM_IC1, TIM_IC_RISING);
  timer_set_prescaler(tim, 2*SENSOR_FREQ_DIVIDER); /* Prescale set to map up to 20kHz */
  timer_slave_set_mode(tim, TIM_SMCR_SMS_RM);
  timer_slave_set_trigger(tim, TIM_SMCR_TS_IT1FP1);
  timer_ic_enable(tim, TIM_IC1);

  timer_enable_counter(tim);
  timer_enable_irq(tim, TIM_DIER_CC1IE);

  switch(pin) {
    case 1:
      nvic_enable_irq(NVIC_TIM1_CC_IRQ);
      break;
  }
}

void tim1_cc_isr() {
  timeval_t t = TIM1_CCR1;
  timer_clear_flag(TIM1, TIM_SR_CC1IF);
  for (int i = 0; i < NUM_SENSORS; ++i) {
    if ((config.sensors[i].method == SENSOR_FREQ) &&
        (config.sensors[i].pin == 1)) {
      config.sensors[i].raw_value = t;
    }
  }
  sensor_freq_new_data();
}

static void platform_init_eventtimer() {
  /* Set up TIM2 as 32bit clock */
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
}

static void platform_init_outputs() {
  /* IG1 out */
  gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO0);
  gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO1);
  gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO2);
}

static void platform_init_adc() {
  /* Set up DMA for the ADC */
  nvic_enable_irq(NVIC_DMA2_STREAM0_IRQ);

  /* Set up ADC */
  for (int i = 0; i < NUM_SENSORS; ++i) {
    if (config.sensors[i].method == SENSOR_ADC) {
      adc_pins[i] = config.sensors[i].pin;
      gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, (1<<adc_pins[i]));
    }
  }
  adc_off(ADC1);
  adc_enable_scan_mode(ADC1);
  adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_15CYC);
  adc_power_on(ADC1);
}

static void platform_init_usart() {
  /* USART initialization */
  nvic_enable_irq(NVIC_USART1_IRQ);

  gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6);
  gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO7);
  gpio_set_output_options(GPIOB, GPIO_OTYPE_OD, GPIO_OSPEED_25MHZ, GPIO7);

  /* Setup USART1 TX and RX pin as alternate function. */
  gpio_set_af(GPIOB, GPIO_AF7, GPIO6);
  gpio_set_af(GPIOB, GPIO_AF7, GPIO7);

  /* Setup USART2 parameters. */
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
  usart_rx_reset();
}

void platform_init() {

  /* 168 Mhz clock */
  rcc_clock_setup_hse_3v3(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);

  /* Enable clocks for subsystems */
  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_GPIOB);
  rcc_periph_clock_enable(RCC_GPIOC);
  rcc_periph_clock_enable(RCC_GPIOD);
  rcc_periph_clock_enable(RCC_SYSCFG);
  rcc_periph_clock_enable(RCC_ADC1);
  rcc_periph_clock_enable(RCC_USART1);
  rcc_periph_clock_enable(RCC_DMA2);
  rcc_periph_clock_enable(RCC_TIM1);
  rcc_periph_clock_enable(RCC_TIM2);
  rcc_periph_clock_enable(RCC_TIM3);

  if (config.decoder.t0_pin >= 0) {
    platform_enable_trigger(config.decoder.t0_pin);
  }
  if (config.decoder.t1_pin >= 0) {
    platform_enable_trigger(config.decoder.t1_pin);
  }

  platform_init_eventtimer();
  platform_init_adc();
  platform_init_outputs();
  platform_init_usart();

  for (int i = 0; i < NUM_SENSORS; ++i) {
    if (config.sensors[i].method == SENSOR_FREQ) {
      platform_init_freqsensor(config.sensors[i].pin);
    }
  }
}

void usart_rx_reset() {
  usart_rx_dest = config.console.rxbuffer;
  usart_rx_end = 0;
  usart_enable_rx_interrupt(USART1);
}


void usart1_isr() {
  if (usart_rx_end) {
    /* Already have unprocessed line in buffer */
    (void)usart_recv(USART1);
    return;
  }

  *usart_rx_dest = usart_recv(USART1);
  if (*usart_rx_dest == '\r') {
    *(usart_rx_dest + 1) = '\0';
    usart_rx_end = 1;
  }

  usart_rx_dest++;
  if (!usart_rx_end && 
      usart_rx_dest == config.console.rxbuffer + CONSOLE_BUFFER_SIZE - 1) {
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

void adc_gather(void *_unused __attribute((unused))) {
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
  dma_set_number_of_data(DMA2, DMA_STREAM0, NUM_SENSORS);
  dma_enable_transfer_complete_interrupt(DMA2, DMA_STREAM0);
  dma_channel_select(DMA2, DMA_STREAM0, DMA_SxCR_CHSEL_0);
  dma_enable_stream(DMA2, DMA_STREAM0);

  adc_set_regular_sequence(ADC1, NUM_SENSORS, adc_pins);
  adc_start_conversion_regular(ADC1);
}

static inline void handle_trigger(unsigned char pin, timeval_t t) {
  if (config.decoder.t0_pin == pin) {
    config.decoder.last_t0 = t;
    config.decoder.needs_decoding_t0 = 1;
  }
  if (config.decoder.t1_pin == pin) {
    config.decoder.last_t1 = t;
    config.decoder.needs_decoding_t1 = 1;
  }
}


/* External interrupt handlers */
void exti0_isr() {
  timeval_t t = current_time();
  exti_reset_request(EXTI0);
  handle_trigger(0, t);
}

void exti1_isr() {
  timeval_t t = current_time();
  exti_reset_request(EXTI1);
  handle_trigger(1, t);
}

void exti2_isr() {
  timeval_t t = current_time();
  exti_reset_request(EXTI2);
  handle_trigger(2, t);
}

void exti3_isr() {
  timeval_t t = current_time();
  exti_reset_request(EXTI3);
  handle_trigger(3, t);
}

void exti4_isr() {
  timeval_t t = current_time();
  exti_reset_request(EXTI4);
  handle_trigger(4, t);
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
   for (int i = 0; i < NUM_SENSORS; ++i) {
     if (config.sensors[i].method == SENSOR_ADC) {
       config.sensors[i].raw_value = (uint32_t)adc_dma_buf[i];
     }
   }
   sensor_adc_new_data();
 }
}

void enable_interrupts() {
  cm_enable_interrupts();
  gpio_clear(GPIOD, GPIO2);
}

void disable_interrupts() {
  cm_disable_interrupts();
  gpio_set(GPIOD, GPIO2);
}

int interrupts_enabled() {
  return !cm_is_masked_interrupts();
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

extern unsigned _configdata_loadaddr, _sconfigdata, _econfigdata;
void platform_load_config() {
  volatile unsigned *src, *dest;
  for (src = &_configdata_loadaddr, dest = &_sconfigdata;
      dest < &_econfigdata;
      src++, dest++) {
    *dest = *src;
  }
}

void platform_save_config() {
  volatile unsigned *src, *dest;
  int n_sectors, conf_bytes;
  flash_unlock();

  /* configrom is sectors 1 through 3, each 16k,
   * compute how many we need to erase */
  conf_bytes = ((char*)&_econfigdata - (char*)&_sconfigdata);
  n_sectors = (conf_bytes + 16385) / 16386;

  flash_erase_sector(n_sectors, 2);
  for (dest = &_configdata_loadaddr, src = &_sconfigdata;
      src < &_econfigdata;
      src++, dest++) {
    flash_program_word((uint32_t)dest, *src);
  }

  flash_lock();
}
