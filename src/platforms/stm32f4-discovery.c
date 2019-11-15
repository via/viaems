
#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/dwt.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/syscfg.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/usb/usbd.h>

#include "config.h"
#include "decoder.h"
#include "limits.h"
#include "platform.h"
#include "scheduler.h"
#include "sensors.h"
#include "stats.h"
#include "tasks.h"
#include "util.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <assert.h>
/* Hardware setup:
 *  Discovery board LEDs:
 *  LD3 - Orange - PD13
 *  LD4 - Green - PD12
 *  LD5 - Red - PD14
 *  LD6 - Blue - PD15
 *
 *  Fixed pin mapping:
 *  Test trigger out - A0 (Uses TIM5)
 *  Event timer TIM2 slaved off TIM8
 *  Trigger 0 - PB3 (TIM2_CH2)
 *  Trigger 1 - PB10 (TIM2_CH3)
 *  Trigger 2 - PB11 (TIM2_CH4)
 *
 *  CAN2:
 *    PB5, PB6
 *
 *  USB:
 *    PA9, PA11, PA12
 *
 *  Freq:
 *    TIM10 PB8
 *    TIM11 PB9
 *    TIM13 PA6
 *    TIM14 PA7
 *
 *  TLC2543 or ADC7888 on SPI2 (PB12-15) CS, SCK, MISO, MOSI
 *    - Uses TIM6 dma1 stream 1 chan 7 to trigger DMA at about 50 khz for 10
 *     inputs
 *    - RX uses SPI2 dma1 stream 3 chan 0
 *    - On end of RX dma, trigger interrupt
 *
 *  Configurable pin mapping:
 *  Scheduled Outputs:
 *   - 0-15 maps to port D, pins 0-15
 *  Freq sensor:
 *   - 1-4 maps to port A, pin 8-11 (TIM1 CH1-4)
 *  PWM Outputs
 *   - 0-PC6  1-PC7 2-PC8 3-PC9 TIM3 channel 1-4
 *  GPIO (Digital Sensor or Output)
 *   - 0-15 Maps to Port E
 *
 */

static void platform_init_freqsensor() {
  timer_set_mode(TIM1, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
  timer_set_period(TIM1, 0xFFFFFFFF);
  timer_disable_preload(TIM1);
  timer_continuous_mode(TIM1);
  /* Setup output compare registers */
  timer_disable_oc_output(TIM1, TIM_OC1);
  timer_disable_oc_output(TIM1, TIM_OC2);
  timer_disable_oc_output(TIM1, TIM_OC3);
  timer_disable_oc_output(TIM1, TIM_OC4);

  /* Set up compare */
  timer_ic_set_input(TIM1, TIM_IC1, TIM_IC_IN_TI1);
  timer_ic_set_filter(TIM1, TIM_IC1, TIM_IC_CK_INT_N_2);
  timer_ic_set_polarity(TIM1, TIM_IC1, TIM_IC_RISING);
  timer_ic_enable(TIM1, TIM_IC1);

  timer_ic_set_input(TIM1, TIM_IC2, TIM_IC_IN_TI2);
  timer_ic_set_filter(TIM1, TIM_IC2, TIM_IC_CK_INT_N_2);
  timer_ic_set_polarity(TIM1, TIM_IC2, TIM_IC_RISING);
  timer_ic_enable(TIM1, TIM_IC2);

  timer_ic_set_input(TIM1, TIM_IC3, TIM_IC_IN_TI3);
  timer_ic_set_filter(TIM1, TIM_IC3, TIM_IC_CK_INT_N_2);
  timer_ic_set_polarity(TIM1, TIM_IC3, TIM_IC_RISING);
  timer_ic_enable(TIM1, TIM_IC3);

  timer_ic_set_input(TIM1, TIM_IC4, TIM_IC_IN_TI4);
  timer_ic_set_filter(TIM1, TIM_IC4, TIM_IC_CK_INT_N_2);
  timer_ic_set_polarity(TIM1, TIM_IC4, TIM_IC_RISING);
  timer_ic_enable(TIM1, TIM_IC4);

  timer_set_prescaler(
    TIM1, 2 * SENSOR_FREQ_DIVIDER); /* Prescale set to map up to 20kHz */

  timer_enable_counter(TIM1);
  timer_enable_irq(
    TIM1, TIM_DIER_CC1IE | TIM_DIER_CC2IE | TIM_DIER_CC3IE | TIM_DIER_CC4IE);

  nvic_enable_irq(NVIC_TIM1_CC_IRQ);
  nvic_set_priority(NVIC_TIM1_CC_IRQ, 64);
}

void tim1_cc_isr() {
  static struct {
    uint16_t value;
    uint32_t time;
  } prev[4] = { 0 };

  stats_increment_counter(STATS_INT_RATE);
  stats_increment_counter(STATS_INT_PWM_RATE);
  stats_start_timing(STATS_INT_TOTAL_TIME);

  /* TODO: Doesn't detect connection fault if the interrupt doesn't get called,
   * e.g. only if another freq input is still working will it detect another
   * faulting */
  for (int i = 0; i < NUM_SENSORS; ++i) {
    if ((config.sensors[i].source != SENSOR_FREQ)) {
      continue;
    }
    volatile uint32_t timer_flag;
    volatile uint32_t *timer_ccr;
    uint32_t pin = config.sensors[i].pin;
    switch (pin) {
    case 1:
      timer_flag = TIM_SR_CC1IF;
      timer_ccr = &TIM1_CCR1;
      break;
    case 2:
      timer_flag = TIM_SR_CC2IF;
      timer_ccr = &TIM1_CCR2;
      break;
    case 3:
      timer_flag = TIM_SR_CC3IF;
      timer_ccr = &TIM1_CCR3;
      break;
    case 4:
      timer_flag = TIM_SR_CC4IF;
      timer_ccr = &TIM1_CCR4;
      break;
    default:
      continue;
    }

    if (timer_get_flag(TIM1, timer_flag)) {
      uint16_t cur = *timer_ccr;
      config.sensors[i].raw_value = (uint16_t)(cur - prev[pin - 1].value);
      prev[pin - 1].value = cur;
      prev[pin - 1].time = current_time();
      sensors_process(SENSOR_FREQ);
      config.sensors[i].fault = FAULT_NONE;
      timer_clear_flag(TIM1, timer_flag);
    } else if ((current_time() - prev[pin - 1].time) > TICKRATE) {
      /* TODO fix */
      /* Been more than one second, fault */
      config.sensors[i].fault = FAULT_CONN;
    }
  }

  stats_finish_timing(STATS_INT_TOTAL_TIME);
}

static int capture_edge_from_config(trigger_edge e) {
  switch (e) {
  case RISING_EDGE:
    return TIM_IC_RISING;
  case FALLING_EDGE:
    return TIM_IC_FALLING;
  case BOTH_EDGES:
    return TIM_IC_BOTH;
  }
  return RISING_EDGE;
}

static void platform_init_eventtimer() {
  /* Set up TIM2 as 32bit clock that is slaved off TIM8*/

  timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
  timer_slave_set_mode(TIM2, TIM_SMCR_SMS_ECM1);
  timer_slave_set_trigger(TIM2, TIM_SMCR_TS_ITR1); /* TIM2 slaved off TIM8 */
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
  /* Setup input captures for CH2-4 Triggers */
  gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, GPIO3);
  gpio_set_af(GPIOB, GPIO_AF1, GPIO3);
  timer_ic_set_input(TIM2, TIM_IC2, TIM_IC_IN_TI2);
  timer_ic_set_filter(TIM2, TIM_IC2, TIM_IC_CK_INT_N_2);
  timer_ic_set_polarity(
    TIM2, TIM_IC2, capture_edge_from_config(config.decoder.t0_edge));
  timer_ic_enable(TIM2, TIM_IC2);

  gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, GPIO10);
  gpio_set_af(GPIOB, GPIO_AF1, GPIO10);
  timer_ic_set_input(TIM2, TIM_IC3, TIM_IC_IN_TI3);
  timer_ic_set_filter(TIM2, TIM_IC3, TIM_IC_CK_INT_N_2);
  timer_ic_set_polarity(
    TIM2, TIM_IC3, capture_edge_from_config(config.decoder.t1_edge));
  timer_ic_enable(TIM2, TIM_IC3);

  gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, GPIO11);
  gpio_set_af(GPIOB, GPIO_AF1, GPIO11);
  timer_ic_set_input(TIM2, TIM_IC4, TIM_IC_IN_TI4);
  timer_ic_set_filter(TIM2, TIM_IC4, TIM_IC_CK_INT_N_2);
  timer_ic_set_polarity(TIM2, TIM_IC4, TIM_IC_RISING);
  timer_ic_enable(TIM2, TIM_IC4);

  timer_enable_counter(TIM2);
  timer_enable_irq(TIM2, TIM_DIER_CC2IE);
  timer_enable_irq(TIM2, TIM_DIER_CC3IE);
  timer_enable_irq(TIM2, TIM_DIER_CC4IE);
  nvic_enable_irq(NVIC_TIM2_IRQ);
  nvic_set_priority(NVIC_TIM2_IRQ, 32);

  /* Set debug unit to stop the timer on halt */
  *((volatile uint32_t *)0xE0042008) |= 19; /*TIM2, TIM5, and TIM6 */
  *((volatile uint32_t *)0xE004200C) |= 2;  /* TIM8 stop */

  timer_set_mode(TIM8, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
  timer_set_period(TIM8, 41); /* 0.25 uS */
  timer_set_prescaler(TIM8, 0);
  timer_disable_preload(TIM8);
  timer_continuous_mode(TIM8);
  timer_disable_oc_output(TIM8, TIM_OC1);
  timer_disable_oc_output(TIM8, TIM_OC2);
  timer_disable_oc_output(TIM8, TIM_OC3);
  timer_disable_oc_output(TIM8, TIM_OC4);
  timer_set_master_mode(TIM8, TIM_CR2_MMS_UPDATE);
  timer_enable_counter(TIM8);
  timer_enable_update_event(TIM8);
  timer_update_on_overflow(TIM8);
  timer_set_dma_on_update_event(TIM8);
  TIM8_DIER |= TIM_DIER_UDE; /* Enable update dma */
}

static uint32_t output_buffer_len;
/* Give two buffers of len size, dma will start reading from buf0 and double
 * buffer between buf0 and buf0.
 * Returns the time that buf0 starts at
 */
timeval_t init_output_thread(uint32_t *buf0, uint32_t *buf1, uint32_t len) {
  timeval_t start;

  output_buffer_len = len;
  /* dma2 stream 1, channel 7*/
  dma_stream_reset(DMA2, DMA_STREAM1);
  dma_set_priority(DMA2, DMA_STREAM1, DMA_SxCR_PL_HIGH);
  dma_enable_double_buffer_mode(DMA2, DMA_STREAM1);
  dma_set_memory_size(DMA2, DMA_STREAM1, DMA_SxCR_MSIZE_32BIT);
  dma_set_peripheral_size(DMA2, DMA_STREAM1, DMA_SxCR_PSIZE_32BIT);
  dma_enable_memory_increment_mode(DMA2, DMA_STREAM1);
  dma_set_transfer_mode(DMA2, DMA_STREAM1, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
  dma_enable_circular_mode(DMA2, DMA_STREAM1);
  dma_set_peripheral_address(DMA2, DMA_STREAM1, (uint32_t)&GPIOD_BSRR);
  dma_set_memory_address(DMA2, DMA_STREAM1, (uint32_t)buf0);
  dma_set_memory_address_1(DMA2, DMA_STREAM1, (uint32_t)buf1);
  dma_set_number_of_data(DMA2, DMA_STREAM1, len);
  dma_channel_select(DMA2, DMA_STREAM1, DMA_SxCR_CHSEL_7);
  dma_enable_direct_mode(DMA2, DMA_STREAM1);
  dma_enable_transfer_complete_interrupt(DMA2, DMA_STREAM1);

  timer_disable_counter(TIM8);
  dma_enable_stream(DMA2, DMA_STREAM1);
  start = timer_get_counter(TIM2);
  timer_enable_counter(TIM8);

  nvic_enable_irq(NVIC_DMA2_STREAM1_IRQ);
  nvic_set_priority(NVIC_DMA2_STREAM1_IRQ, 16);

  return start;
}

/* Returns 0 if buf0 is active */
int current_output_buffer() {
  return dma_get_target(DMA2, DMA_STREAM1);
}

/* Returns the current slot that DMA is pointing at.
 * Note that the slot returned is the one queued to output next, but is already
 * in the FIFO so is considered 'done'
 */
int current_output_slot() {
  return output_buffer_len - DMA2_S1NDTR;
}

static void platform_init_pwm() {

  gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6);
  gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO7);
  gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO8);
  gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9);
  gpio_set_af(GPIOC, GPIO_AF2, GPIO6);
  gpio_set_af(GPIOC, GPIO_AF2, GPIO7);
  gpio_set_af(GPIOC, GPIO_AF2, GPIO8);
  gpio_set_af(GPIOC, GPIO_AF2, GPIO9);

  timer_disable_oc_output(TIM3, TIM_OC1);
  timer_set_mode(TIM3, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
  /* 30ish Hz */
  timer_set_period(TIM3, 65535);
  timer_set_prescaler(TIM3, 32);
  timer_enable_preload(TIM3);
  timer_continuous_mode(TIM3);
  /* Setup output compare registers */
  timer_ic_set_input(TIM3, TIM_IC1, TIM_IC_OUT);
  timer_disable_oc_clear(TIM3, TIM_OC1);
  timer_enable_oc_preload(TIM3, TIM_OC1);
  timer_set_oc_slow_mode(TIM3, TIM_OC1);
  timer_set_oc_mode(TIM3, TIM_OC1, TIM_OCM_PWM1);
  timer_set_oc_value(TIM3, TIM_OC1, 0);
  timer_set_oc_polarity_high(TIM3, TIM_OC1);
  timer_enable_oc_output(TIM3, TIM_OC1);

  timer_ic_set_input(TIM3, TIM_IC2, TIM_IC_OUT);
  timer_disable_oc_clear(TIM3, TIM_OC2);
  timer_enable_oc_preload(TIM3, TIM_OC2);
  timer_set_oc_slow_mode(TIM3, TIM_OC2);
  timer_set_oc_mode(TIM3, TIM_OC2, TIM_OCM_PWM1);
  timer_set_oc_value(TIM3, TIM_OC2, 0);
  timer_set_oc_polarity_high(TIM3, TIM_OC2);
  timer_enable_oc_output(TIM3, TIM_OC2);

  timer_ic_set_input(TIM3, TIM_IC3, TIM_IC_OUT);
  timer_disable_oc_clear(TIM3, TIM_OC3);
  timer_enable_oc_preload(TIM3, TIM_OC3);
  timer_set_oc_slow_mode(TIM3, TIM_OC3);
  timer_set_oc_mode(TIM3, TIM_OC3, TIM_OCM_PWM1);
  timer_set_oc_value(TIM3, TIM_OC3, 0);
  timer_set_oc_polarity_high(TIM3, TIM_OC3);
  timer_enable_oc_output(TIM3, TIM_OC3);

  timer_ic_set_input(TIM3, TIM_IC4, TIM_IC_OUT);
  timer_disable_oc_clear(TIM3, TIM_OC4);
  timer_enable_oc_preload(TIM3, TIM_OC4);
  timer_set_oc_slow_mode(TIM3, TIM_OC4);
  timer_set_oc_mode(TIM3, TIM_OC4, TIM_OCM_PWM1);
  timer_set_oc_value(TIM3, TIM_OC4, 0);
  timer_set_oc_polarity_high(TIM3, TIM_OC4);
  timer_enable_oc_output(TIM3, TIM_OC4);
  timer_enable_counter(TIM3);
}

void set_pwm(int output, float value) {
  int ival = value * 65535;
  switch (output) {
  case 1:
    return timer_set_oc_value(TIM3, TIM_OC1, ival);
  case 2:
    return timer_set_oc_value(TIM3, TIM_OC2, ival);
  case 3:
    return timer_set_oc_value(TIM3, TIM_OC3, ival);
  case 4:
    return timer_set_oc_value(TIM3, TIM_OC4, ival);
  }
}

static void platform_init_scheduled_outputs() {
  gpio_clear(GPIOD, 0xFFFF);
  unsigned int i;
  for (i = 0; i < config.num_events; ++i) {
    if (config.events[i].inverted && config.events[i].type) {
      set_output(config.events[i].pin, 1);
    }
  }
  gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, 0xFFFF & ~GPIO5);
  gpio_set_output_options(
    GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, 0xFFFF & ~GPIO5);
  gpio_mode_setup(GPIOE, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, 0xFF);
  gpio_set_output_options(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, 0xFF);
}

void platform_enable_event_logging() {

  nvic_enable_irq(NVIC_EXTI0_IRQ);
  nvic_enable_irq(NVIC_EXTI1_IRQ);
  nvic_enable_irq(NVIC_EXTI2_IRQ);
  nvic_enable_irq(NVIC_EXTI3_IRQ);
  nvic_enable_irq(NVIC_EXTI4_IRQ);
  nvic_enable_irq(NVIC_EXTI9_5_IRQ);
  nvic_enable_irq(NVIC_EXTI15_10_IRQ);

  exti_select_source(0xFF, GPIOD);
  exti_set_trigger(0xFF, EXTI_TRIGGER_BOTH);
  exti_enable_request(0xFF);
}

void platform_disable_event_logging() {
  nvic_disable_irq(NVIC_EXTI0_IRQ);
  nvic_disable_irq(NVIC_EXTI1_IRQ);
  nvic_disable_irq(NVIC_EXTI2_IRQ);
  nvic_disable_irq(NVIC_EXTI3_IRQ);
  nvic_disable_irq(NVIC_EXTI4_IRQ);
  nvic_disable_irq(NVIC_EXTI9_5_IRQ);
  nvic_disable_irq(NVIC_EXTI15_10_IRQ);
}

static void show_scheduled_outputs() {
  uint32_t flag_changes = exti_get_flag_status(0xFF);
  if (flag_changes == 0) {
    while(1);
  }
  console_record_event((struct logged_event){
    .type = EVENT_OUTPUT,
    .time = current_time(),
    .value = gpio_port_read(GPIOD),
  });
  exti_reset_request(flag_changes);
  __asm__("dsb");
  __asm__("isb");
}

void exti0_isr() {
  show_scheduled_outputs();
}
void exti1_isr() {
  show_scheduled_outputs();
}
void exti2_isr() {
  show_scheduled_outputs();
}
void exti3_isr() {
  show_scheduled_outputs();
}
void exti9_5_isr() {
  show_scheduled_outputs();
}
void exti15_10_isr() {
  show_scheduled_outputs();
}

/* We use TIM6 to control the sample rate.  It is set up to trigger a DMA event
 * on counter update to TX on SPI2.  When the full 16 bits is transmitted and
 * the SPI RX buffer is filled, the RX DMA event will fill, and populate
 * spi_rx_raw_adc.
 *
 * When configured for the TLC2543, currently using 10 inputs, spi_tx_list
 * contains the 16bit data words to trigger reads on AIN0-AIN10 and vref/2 on
 * the TLC2543.  The AD7888 has no self checkable channels, and is configured
 * for 8 inputs. DMA is set up such that each channel is sampled in order.  DMA
 * RX is set up accordingly, but note that because the ADC returns the previous
 * sample result each time, command 1 in spi_tx_list corresponds to response 2
 * in spi_rx_raw_adc, and so forth.
 *
 * Currently sample rate is about 50 khz, with a SPI bus frequency of 1.3ish MHz
 *
 * Each call to adc_gather reconfigures TX DMA, resets and starts TIM6, and
 * lowers CS Once all receives are complete, RX dma completes, notifies
 * completion, and raises CS.
 */
#ifdef SPI_TLC2543
#define SPI_WRITE_COUNT 13
#else
#define SPI_WRITE_COUNT 9
#endif
static volatile uint16_t spi_rx_raw_adc[SPI_WRITE_COUNT] = { 0 };

static void platform_init_spi_adc() {
  /* Configure SPI output */
  gpio_mode_setup(
    GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO13 | GPIO14 | GPIO15);
  gpio_set_af(GPIOB, GPIO_AF5, GPIO13 | GPIO14 | GPIO15);
  gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO12);
  gpio_set_output_options(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO12);
  gpio_set(GPIOB, GPIO12);
  spi_disable(SPI2);
  spi_reset(SPI2);
  spi_enable_software_slave_management(SPI2);
  spi_set_nss_high(SPI2);
  spi_init_master(SPI2,
                  SPI_CR1_BAUDRATE_FPCLK_DIV_32,
                  SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
                  SPI_CR1_CPHA_CLK_TRANSITION_1,
                  SPI_CR1_DFF_16BIT,
                  SPI_CR1_MSBFIRST);

  spi_enable_rx_dma(SPI2);
  spi_enable(SPI2);

  /* Configure DMA for SPI RX*/
  dma_enable_stream(DMA1, DMA_STREAM3);
  dma_stream_reset(DMA1, DMA_STREAM3);
  dma_set_priority(DMA1, DMA_STREAM3, DMA_SxCR_PL_LOW);
  dma_set_memory_size(DMA1, DMA_STREAM3, DMA_SxCR_MSIZE_16BIT);
  dma_set_peripheral_size(DMA1, DMA_STREAM3, DMA_SxCR_PSIZE_16BIT);
  dma_enable_memory_increment_mode(DMA1, DMA_STREAM3);
  dma_set_transfer_mode(DMA1, DMA_STREAM3, DMA_SxCR_DIR_PERIPHERAL_TO_MEM);
  dma_set_peripheral_address(DMA1, DMA_STREAM3, (uint32_t)&SPI2_DR);
  dma_enable_circular_mode(DMA1, DMA_STREAM3);
  dma_set_memory_address(DMA1, DMA_STREAM3, (uint32_t)spi_rx_raw_adc);
  dma_set_number_of_data(DMA1, DMA_STREAM3, SPI_WRITE_COUNT);
  dma_channel_select(DMA1, DMA_STREAM3, DMA_SxCR_CHSEL_0);
  dma_enable_direct_mode(DMA1, DMA_STREAM3);
  dma_enable_transfer_complete_interrupt(DMA1, DMA_STREAM3);
  dma_enable_stream(DMA1, DMA_STREAM3);

  nvic_enable_irq(NVIC_DMA1_STREAM3_IRQ);
  nvic_set_priority(NVIC_DMA1_STREAM3_IRQ, 16);

  /* Configure TIM6 to drive DMA for SPI */
  timer_set_mode(TIM6, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
  timer_set_period(TIM6, 1800); /* Approx 50 khz sampling rate */
  timer_disable_preload(TIM6);
  timer_continuous_mode(TIM6);
  /* Setup output compare registers */
  timer_disable_oc_output(TIM6, TIM_OC1);
  timer_disable_oc_output(TIM6, TIM_OC2);
  timer_disable_oc_output(TIM6, TIM_OC3);
  timer_disable_oc_output(TIM6, TIM_OC4);

  timer_set_prescaler(TIM6, 0);
}

static uint8_t usbd_control_buffer[128];
static usbd_device *usbd_dev;

/* Most of the following is copied from libopencm3-examples */
static enum usbd_request_return_codes cdcacm_control_request(
  usbd_device *usbd_dev,
  struct usb_setup_data *req,
  uint8_t **buf,
  uint16_t *len,
  void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req)) {
  (void)complete;
  (void)buf;
  (void)usbd_dev;

  switch (req->bRequest) {
  case USB_CDC_REQ_SET_CONTROL_LINE_STATE: {
    /*
     * This Linux cdc_acm driver requires this to be implemented
     * even though it's optional in the CDC spec, and we don't
     * advertise it in the ACM functional descriptor.
     */
    return USBD_REQ_HANDLED;
  }
  case USB_CDC_REQ_SET_LINE_CODING:
    if (*len < sizeof(struct usb_cdc_line_coding)) {
      return USBD_REQ_NOTSUPP;
    }

    return USBD_REQ_HANDLED;
  }
  return USBD_REQ_NOTSUPP;
}

#define USB_RX_BUF_LEN 1024
static char usb_rx_buf[USB_RX_BUF_LEN];
static volatile size_t usb_rx_len = 0;
static void cdcacm_data_rx_cb(usbd_device *usbd_dev, uint8_t ep) {
  (void)ep;
  usb_rx_len += usbd_ep_read_packet(
    usbd_dev, 0x01, usb_rx_buf + usb_rx_len, USB_RX_BUF_LEN - usb_rx_len);
}

static void cdcacm_set_config(usbd_device *usbd_dev, uint16_t wValue) {
  (void)wValue;

  usbd_ep_setup(usbd_dev, 0x01, USB_ENDPOINT_ATTR_BULK, 64, cdcacm_data_rx_cb);
  usbd_ep_setup(usbd_dev, 0x82, USB_ENDPOINT_ATTR_BULK, 64, NULL);
  usbd_ep_setup(usbd_dev, 0x83, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);

  usbd_register_control_callback(usbd_dev,
                                 USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
                                 USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
                                 cdcacm_control_request);
}

static const struct usb_device_descriptor dev = {
  .bLength = USB_DT_DEVICE_SIZE,
  .bDescriptorType = USB_DT_DEVICE,
  .bcdUSB = 0x0200,
  .bDeviceClass = USB_CLASS_CDC,
  .bDeviceSubClass = 0,
  .bDeviceProtocol = 0,
  .bMaxPacketSize0 = 64,
  .idVendor = 0x0483,
  .idProduct = 0x5740,
  .bcdDevice = 0x0200,
  .iManufacturer = 1,
  .iProduct = 2,
  .iSerialNumber = 3,
  .bNumConfigurations = 1,
};

static const struct usb_endpoint_descriptor comm_endp[] = { {
  .bLength = USB_DT_ENDPOINT_SIZE,
  .bDescriptorType = USB_DT_ENDPOINT,
  .bEndpointAddress = 0x83,
  .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
  .wMaxPacketSize = 16,
  .bInterval = 255,
} };

static const struct usb_endpoint_descriptor data_endp[] = {
  {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = 0x01,
    .bmAttributes = USB_ENDPOINT_ATTR_BULK,
    .wMaxPacketSize = 64,
    .bInterval = 1,
  },
  {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = 0x82,
    .bmAttributes = USB_ENDPOINT_ATTR_BULK,
    .wMaxPacketSize = 64,
    .bInterval = 1,
  }
};

static const struct {
  struct usb_cdc_header_descriptor header;
  struct usb_cdc_call_management_descriptor call_mgmt;
  struct usb_cdc_acm_descriptor acm;
  struct usb_cdc_union_descriptor cdc_union;
} __attribute__((packed)) cdcacm_functional_descriptors = {
  .header = {
    .bFunctionLength = sizeof(struct usb_cdc_header_descriptor),
    .bDescriptorType = CS_INTERFACE,
    .bDescriptorSubtype = USB_CDC_TYPE_HEADER,
    .bcdCDC = 0x0110,
  },
  .call_mgmt = {
    .bFunctionLength =
      sizeof(struct usb_cdc_call_management_descriptor),
    .bDescriptorType = CS_INTERFACE,
    .bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
    .bmCapabilities = 0,
    .bDataInterface = 1,
  },
  .acm = {
    .bFunctionLength = sizeof(struct usb_cdc_acm_descriptor),
    .bDescriptorType = CS_INTERFACE,
    .bDescriptorSubtype = USB_CDC_TYPE_ACM,
    .bmCapabilities = 0,
  },
  .cdc_union = {
    .bFunctionLength = sizeof(struct usb_cdc_union_descriptor),
    .bDescriptorType = CS_INTERFACE,
    .bDescriptorSubtype = USB_CDC_TYPE_UNION,
    .bControlInterface = 0,
    .bSubordinateInterface0 = 1,
   }
};

static const struct usb_interface_descriptor comm_iface[] = {
  { .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 0,
    .bAlternateSetting = 0,
    .bNumEndpoints = 1,
    .bInterfaceClass = USB_CLASS_CDC,
    .bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
    .bInterfaceProtocol = USB_CDC_PROTOCOL_AT,
    .iInterface = 0,

    .endpoint = comm_endp,

    .extra = &cdcacm_functional_descriptors,
    .extralen = sizeof(cdcacm_functional_descriptors) }
};

static const struct usb_interface_descriptor data_iface[] = { {
  .bLength = USB_DT_INTERFACE_SIZE,
  .bDescriptorType = USB_DT_INTERFACE,
  .bInterfaceNumber = 1,
  .bAlternateSetting = 0,
  .bNumEndpoints = 2,
  .bInterfaceClass = USB_CLASS_DATA,
  .bInterfaceSubClass = 0,
  .bInterfaceProtocol = 0,
  .iInterface = 0,

  .endpoint = data_endp,
} };

static const struct usb_interface ifaces[] = { {
                                                 .num_altsetting = 1,
                                                 .altsetting = comm_iface,
                                               },
                                               {
                                                 .num_altsetting = 1,
                                                 .altsetting = data_iface,
                                               } };

static const struct usb_config_descriptor usb_config = {
  .bLength = USB_DT_CONFIGURATION_SIZE,
  .bDescriptorType = USB_DT_CONFIGURATION,
  .wTotalLength = 0,
  .bNumInterfaces = 2,
  .bConfigurationValue = 1,
  .iConfiguration = 0,
  .bmAttributes = 0x80,
  .bMaxPower = 0x32,

  .interface = ifaces,
};

static const char *usb_strings[] = {
  "https://github.com/via/viaems/",
  "ViaEMS console",
  "0",
};

void otg_fs_isr() {
  stats_increment_counter(STATS_INT_RATE);
  stats_increment_counter(STATS_USB_INTERRUPT_RATE);
  stats_start_timing(STATS_USB_INTERRUPT_TIME);
  stats_start_timing(STATS_INT_TOTAL_TIME);
  usbd_poll(usbd_dev);
  stats_finish_timing(STATS_USB_INTERRUPT_TIME);
  stats_finish_timing(STATS_INT_TOTAL_TIME);
}

void platform_init_usb() {

  gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9 | GPIO11 | GPIO12);
  gpio_set_af(GPIOA, GPIO_AF10, GPIO9 | GPIO11 | GPIO12);

  usbd_dev = usbd_init(&otgfs_usb_driver,
                       &dev,
                       &usb_config,
                       usb_strings,
                       3,
                       usbd_control_buffer,
                       sizeof(usbd_control_buffer));

  usbd_register_set_config_callback(usbd_dev, cdcacm_set_config);
  nvic_enable_irq(NVIC_OTG_FS_IRQ);
}

void platform_init() {

  /* 168 Mhz clock */
  rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);

  /* Enable clocks for subsystems */
  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_GPIOB);
  rcc_periph_clock_enable(RCC_GPIOC);
  rcc_periph_clock_enable(RCC_GPIOD);
  rcc_periph_clock_enable(RCC_GPIOE);
  rcc_periph_clock_enable(RCC_SYSCFG);
  rcc_periph_clock_enable(RCC_DMA1);
  rcc_periph_clock_enable(RCC_DMA2);
  rcc_periph_clock_enable(RCC_TIM1);
  rcc_periph_clock_enable(RCC_TIM2);
  rcc_periph_clock_enable(RCC_TIM3);
  rcc_periph_clock_enable(RCC_TIM6);
  rcc_periph_clock_enable(RCC_TIM8);
  rcc_periph_clock_enable(RCC_SPI2);
  rcc_periph_clock_enable(RCC_OTGFS);

  /* Wait for clock to spin up */
  rcc_wait_for_osc_ready(RCC_HSE);

  scb_set_priority_grouping(SCB_AIRCR_PRIGROUP_GROUP16_NOSUB);
  platform_init_scheduled_outputs();
  platform_init_eventtimer();
  platform_init_spi_adc();
  platform_init_pwm();
  platform_init_freqsensor();
  platform_init_usb();

  for (int i = 0; i < NUM_SENSORS; ++i) {
    if ((config.sensors[i].source == SENSOR_FREQ) &&
        (config.sensors[i].pin >= 1) && (config.sensors[i].pin <= 4)) {
      /* Pin 1-4 maps to GPIO8-11 */
      gpio_mode_setup(GPIOA,
                      GPIO_MODE_AF,
                      GPIO_PUPD_PULLDOWN,
                      GPIO8 << (config.sensors[i].pin - 1));
      gpio_set_af(GPIOA, GPIO_AF1, GPIO8 << (config.sensors[i].pin - 1));
    }
    if (config.sensors[i].source == SENSOR_DIGITAL) {
      gpio_mode_setup(GPIOE,
                      GPIO_MODE_INPUT,
                      GPIO_PUPD_PULLDOWN,
                      (1 << config.sensors[i].pin));
    }
  }
  dwt_enable_cycle_counter();
  stats_init(168000000);
}

void platform_reset_into_bootloader() {
  handle_emergency_shutdown();

  rcc_periph_reset_pulse(RST_OTGFS);

  /* Shut down subsystems */
  rcc_periph_reset_pulse(RST_GPIOA);
  rcc_periph_clock_disable(RCC_GPIOA);

  rcc_periph_reset_pulse(RST_GPIOB);
  rcc_periph_clock_disable(RCC_GPIOB);

  rcc_periph_reset_pulse(RST_GPIOC);
  rcc_periph_clock_disable(RCC_GPIOC);

  rcc_periph_reset_pulse(RST_GPIOD);
  rcc_periph_clock_disable(RCC_GPIOD);

  rcc_periph_reset_pulse(RST_GPIOE);
  rcc_periph_clock_disable(RCC_GPIOE);

  rcc_periph_reset_pulse(RST_SYSCFG);
  rcc_periph_clock_disable(RCC_SYSCFG);

  rcc_periph_reset_pulse(RST_DMA1);
  rcc_periph_clock_disable(RCC_DMA1);

  rcc_periph_reset_pulse(RST_DMA2);
  rcc_periph_clock_disable(RCC_DMA2);

  rcc_periph_reset_pulse(RST_TIM1);
  rcc_periph_clock_disable(RCC_TIM1);

  rcc_periph_reset_pulse(RST_TIM2);
  rcc_periph_clock_disable(RCC_TIM2);

  rcc_periph_reset_pulse(RST_TIM3);
  rcc_periph_clock_disable(RCC_TIM3);

  rcc_periph_reset_pulse(RST_TIM6);
  rcc_periph_clock_disable(RCC_TIM6);

  rcc_periph_reset_pulse(RST_TIM8);
  rcc_periph_clock_disable(RCC_TIM8);

  rcc_periph_reset_pulse(RST_SPI2);
  rcc_periph_clock_disable(RCC_SPI2);

  rcc_periph_reset_pulse(RST_OTGFS);
  rcc_periph_clock_disable(RCC_OTGFS);

  /* 168 Mhz clock */
  rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_168MHZ]);

  __asm__ volatile("msr msp, %0" ::"g"(*(volatile uint32_t *)0x20001000));
  (*(void (**)())(0x1fff0004))();
  while (1)
    ;
}

uint32_t cycle_count() {
  return dwt_read_cycle_counter();
}

static volatile int adc_gather_in_progress = 0;
void adc_gather() {
#ifdef SPI_TLC2543
  static uint16_t spi_tx_list[] = {
    0x0C00, 0x1C00, 0x2C00, 0x3C00, 0x4C00, 0x5C00, 0x6C00,
    0x7C00, 0x8C00, 0x9C00, 0xAC00, 0xBC00, /* Check value (Vref+ + Vref-) / 2
                                             */
    0xBC00, /* Duplicated to actually get previous read */
  };
#else
  static uint16_t spi_tx_list[] = {
    0x0400, 0x0C00, 0x1400, 0x1C00, 0x2400, 0x2C00, 0x3400, 0x3C00, 0x3C00,
  };
#endif

  if (adc_gather_in_progress) {
    return; /* Don't start another until RX completes */
  }
  adc_gather_in_progress = 1;

  timer_set_counter(TIM6, 0);
  /* CS low */
  gpio_clear(GPIOB, GPIO12);

  /* Set up DMA for SPI TX */
  dma_stream_reset(DMA1, DMA_STREAM1);
  dma_set_priority(DMA1, DMA_STREAM1, DMA_SxCR_PL_LOW);
  dma_set_memory_size(DMA1, DMA_STREAM1, DMA_SxCR_MSIZE_16BIT);
  dma_set_peripheral_size(DMA1, DMA_STREAM1, DMA_SxCR_PSIZE_16BIT);
  dma_enable_memory_increment_mode(DMA1, DMA_STREAM1);
  dma_set_transfer_mode(DMA1, DMA_STREAM1, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
  dma_set_peripheral_address(DMA1, DMA_STREAM1, (uint32_t)&SPI2_DR);
  dma_set_memory_address(DMA1, DMA_STREAM1, (uint32_t)spi_tx_list);
  dma_set_number_of_data(
    DMA1, DMA_STREAM1, sizeof(spi_tx_list) / sizeof(spi_tx_list[0]));
  dma_channel_select(DMA1, DMA_STREAM1, DMA_SxCR_CHSEL_7);
  dma_enable_direct_mode(DMA1, DMA_STREAM1);
  dma_enable_stream(DMA1, DMA_STREAM1);

  timer_enable_counter(TIM6);
  timer_enable_update_event(TIM6);
  timer_update_on_overflow(TIM6);
  timer_set_dma_on_update_event(TIM6);
  TIM6_DIER |= TIM_DIER_UDE; /* Enable update dma */
}

void dma1_stream3_isr(void) {
  if (!dma_get_interrupt_flag(DMA1, DMA_STREAM3, DMA_TCIF)) {
    return;
  }
  dma_clear_interrupt_flags(DMA1, DMA_STREAM3, DMA_TCIF);

  /* CS high */
  gpio_set(GPIOB, GPIO12);
  timer_disable_counter(TIM6);

  int fault = 0;
#ifdef SPI_TLC2543
  if (((spi_rx_raw_adc[12] >> 4) > (2048 + 10)) ||
      ((spi_rx_raw_adc[12] >> 4) < (2048 - 10))) {
    fault = 1; /* Check value is vref/2 */
  }
#endif

  for (int i = 0; i < NUM_SENSORS; ++i) {
    if (config.sensors[i].source == SENSOR_ADC) {
      int pin = (config.sensors[i].pin + 1) % SPI_WRITE_COUNT;
      config.sensors[i].fault = fault ? FAULT_CONN : FAULT_NONE;
      uint16_t adc_value = spi_rx_raw_adc[pin];
#ifdef SPI_TLC2543
      adc_value >>= 4; /* 12 bit value is left justified */
#endif
      config.sensors[i].raw_value = adc_value;
    }
  }

  sensors_process(SENSOR_ADC);
  adc_gather_in_progress = 0;
}

/* This is now the lowest priority interrupt, with buffer swapping and sensor
 * reading interrupts being higher priority.  Keep scheduled callbacks quick to
 * keep scheduling delay low.  Currently nothing takes more than 15 uS
 * (rescheduling individual events).  All fueling calculations and scheduling is
 * now done in the ISR for trigger updates.  Overflows aren't currently handled,
 * but will be when we switch to DMA of trigger time readings, which will allow
 * graceful overflow (at the expensive of non-critical routines not running,
 * e.g. console).
 */
void tim2_isr() {
  stats_increment_counter(STATS_INT_RATE);
  stats_increment_counter(STATS_INT_EVENTTIMER_RATE);
  stats_start_timing(STATS_INT_TOTAL_TIME);
  if (timer_get_flag(TIM2, TIM_SR_CC2IF) &&
      timer_get_flag(TIM2, TIM_SR_CC3IF)) {
    timer_clear_flag(TIM2, TIM_SR_CC2IF);
    timer_clear_flag(TIM2, TIM_SR_CC3IF);
    if (time_in_range(TIM2_CCR2, TIM2_CCR3, current_time())) {
      struct decoder_event ev[] = {
        { .t1 = 1, .time = TIM2_CCR3 },
        { .t0 = 1, .time = TIM2_CCR2 },
      };
      decoder_update_scheduling(ev, 2);
    } else {
      struct decoder_event ev[] = {
        { .t0 = 1, .time = TIM2_CCR2 },
        { .t1 = 1, .time = TIM2_CCR3 },
      };
      decoder_update_scheduling(ev, 2);
    }
  } else {
    if (timer_get_flag(TIM2, TIM_SR_CC2IF)) {
      timer_clear_flag(TIM2, TIM_SR_CC2IF);
      struct decoder_event ev = { .t0 = 1, .time = TIM2_CCR2 };
      decoder_update_scheduling(&ev, 1);
    }
    if (timer_get_flag(TIM2, TIM_SR_CC3IF)) {
      timer_clear_flag(TIM2, TIM_SR_CC3IF);
      struct decoder_event ev = { .t1 = 1, .time = TIM2_CCR3 };
      decoder_update_scheduling(&ev, 1);
    }
  }
  if (timer_get_flag(TIM2, TIM_SR_CC1IF)) {
    timer_clear_flag(TIM2, TIM_SR_CC1IF);
    scheduler_callback_timer_execute();
  }
  stats_finish_timing(STATS_INT_TOTAL_TIME);
}

void dma2_stream1_isr(void) {
  stats_increment_counter(STATS_INT_RATE);
  stats_increment_counter(STATS_INT_BUFFERSWAP_RATE);
  stats_start_timing(STATS_INT_TOTAL_TIME);
  if (dma_get_interrupt_flag(DMA2, DMA_STREAM1, DMA_TCIF)) {
    dma_clear_interrupt_flags(DMA2, DMA_STREAM1, DMA_TCIF);
    stats_start_timing(STATS_INT_BUFFERSWAP_TIME);
    scheduler_buffer_swap();
    stats_finish_timing(STATS_INT_BUFFERSWAP_TIME);
  }
  stats_finish_timing(STATS_INT_TOTAL_TIME);
}

void enable_interrupts() {
  stats_finish_timing(STATS_INT_DISABLE_TIME);
  cm_enable_interrupts();
}

/* Returns current enabled status prior to call */
int disable_interrupts() {
  int ret = interrupts_enabled();
  cm_disable_interrupts();
  stats_start_timing(STATS_INT_DISABLE_TIME);
  return ret;
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

int get_output(int output) {
  return gpio_get(GPIOD, (1 << output)) != 0;
}

void set_output(int output, char value) {
  if (value) {
    gpio_set(GPIOD, (1 << output));
  } else {
    gpio_clear(GPIOD, (1 << output));
  }
}

int get_gpio(int output) {
  return gpio_get(GPIOE, (1 << output)) != 0;
}

void set_gpio(int output, char value) {
  if (value) {
    gpio_set(GPIOE, (1 << output));
  } else {
    gpio_clear(GPIOE, (1 << output));
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
  timer_disable_oc_output(TIM5, TIM_OC1);
  timer_set_mode(TIM5, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
  timer_set_period(TIM5, (unsigned int)t);
  timer_set_prescaler(TIM5, 20); /* Primary clock is still 168 Mhz */
  timer_disable_preload(TIM5);
  timer_continuous_mode(TIM5);
  /* Setup output compare registers */
  timer_ic_set_input(TIM5, TIM_IC1, TIM_IC_OUT);
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
  for (src = &_configdata_loadaddr, dest = &_sconfigdata; dest < &_econfigdata;
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
  conf_bytes = ((char *)&_econfigdata - (char *)&_sconfigdata);
  n_sectors = (conf_bytes + 16385) / 16386;

  for (int sector = 1; sector < 1 + n_sectors; sector++) {
    flash_erase_sector(sector, 2);
  }
  for (dest = &_configdata_loadaddr, src = &_sconfigdata; src < &_econfigdata;
       src++, dest++) {
    flash_program_word((uint32_t)dest, *src);
  }

  flash_lock();
}

size_t console_read(void *buf, size_t max) {
  disable_interrupts();
  stats_start_timing(STATS_CONSOLE_READ_TIME);
  size_t amt = usb_rx_len > max ? max : usb_rx_len;
  memcpy(buf, usb_rx_buf, amt);
  memmove(usb_rx_buf, usb_rx_buf + amt, usb_rx_len - amt);
  usb_rx_len -= amt;
  stats_finish_timing(STATS_CONSOLE_READ_TIME);
  enable_interrupts();
  return amt;
}

size_t console_write(const void *buf, size_t count) {
  size_t rem = count > 64 ? 64 : count;
  /* https://github.com/libopencm3/libopencm3/issues/531
   * We can't let the usb irq be called while writing */
  nvic_disable_irq(NVIC_OTG_FS_IRQ);
  rem = usbd_ep_write_packet(usbd_dev, 0x82, buf, rem);
  nvic_enable_irq(NVIC_OTG_FS_IRQ);
  return rem;
}

/* This should only ever be used in an emergency */
ssize_t _write(int fd, const void *buf, size_t count) {
  (void)fd;

  while (count > 0) {
    size_t written = console_write(buf, count);
    if (written == 0) {
      return 0;
    }
    buf += written;
    count -= written;
  }
  return count;
}

void _exit(int status) {
  (void)status;

  handle_emergency_shutdown();
  while (1)
    ;
}

/* TODO: implement graceful shutdown of outputs on fault */
#define STACK_CHK_GUARD 0xe2dee396
uintptr_t __attribute__((externally_visible)) __stack_chk_guard =
  STACK_CHK_GUARD;

__attribute__((noreturn)) __attribute__((externally_visible)) void
__stack_chk_fail(void) {
  while (1)
    ;
}

void platform_freeze_timers() {
  timer_disable_counter(TIM8);
}

void platform_unfreeze_timers() {
  timer_enable_counter(TIM8);
}
