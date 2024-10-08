#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <assert.h>

#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/dwt.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/iwdg.h>
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
#include "tasks.h"
#include "util.h"

#include "stm32_sched_buffers.h"

/* Hardware setup:
 *  Discovery board LEDs:
 *  LD3 - Orange - PD13
 *  LD4 - Green - PD12
 *  LD5 - Red - PD14
 *  LD6 - Blue - PD15
 *
 *  Fixed pin mapping:
 *  Primary time base is TIM2, slaved off TIM8
 *  Trigger 0 - PA0 (TIM2_CH1)
 *  Trigger 1 - PA1 (TIM2_CH2)
 *
 *  Test/Out Trigger 0 - PB10
 *  Test/Out Trigger 1 - PB11
 *
 *  Event Timer (TIM2_CH4)
 *
 *  CAN2:
 *    PB5, PB6
 *
 *  USB:
 *    PA9, PA11, PA12
 *
 *  TLC2543 or ADC7888 on SPI2 (PB12-15) CS, SCK, MISO, MOSI
 *    - Uses TIM7 dma1 stream 2 chan 1 to trigger DMA at about 50 khz for 10
 *      inputs
 *    - RX uses SPI2 dma1 stream 3 chan 0
 *    - On end of RX dma, trigger interrupt
 *
 *  Configurable pin mapping:
 *  Scheduled Outputs:
 *   - 0-15 maps to port D, pins 0-15
 *  PWM Outputs
 *   - 0-PC6  1-PC7 2-PC8 3-PC9 TIM3 channel 1-4
 *  GPIO (Digital Sensor or Output)
 *   - 0-15 Maps to Port E
 *
 *  nvic priorities:
 *    tim6 (test trigger) 0
 *    dma2s1 (buffer swap) 16
 *    tim2 (triggers, callbacks, scheduling, fueling calcs) 32
 *    dma1s3 (adc) 64
 *    otg (usb) 128
 *    systick 128
 */

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

static void platform_setup_tim2() {
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

  timer_set_oc_slow_mode(TIM2, TIM_OC4);
  timer_set_oc_mode(TIM2, TIM_OC4, TIM_OCM_FROZEN);

  /* Setup input captures for CH1-4 Triggers */
  gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, GPIO0);
  gpio_set_af(GPIOA, GPIO_AF1, GPIO0);
  gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, GPIO1);
  gpio_set_af(GPIOA, GPIO_AF1, GPIO1);

  timer_ic_set_input(TIM2, TIM_IC1, TIM_IC_IN_TI1);
  timer_ic_set_filter(TIM2, TIM_IC1, TIM_IC_CK_INT_N_2);
  timer_ic_set_polarity(
    TIM2, TIM_IC1, capture_edge_from_config(config.freq_inputs[0].edge));
  timer_ic_enable(TIM2, TIM_IC1);

  timer_ic_set_input(TIM2, TIM_IC2, TIM_IC_IN_TI2);
  timer_ic_set_filter(TIM2, TIM_IC2, TIM_IC_CK_INT_N_2);
  timer_ic_set_polarity(
    TIM2, TIM_IC2, capture_edge_from_config(config.freq_inputs[1].edge));
  timer_ic_enable(TIM2, TIM_IC2);

  timer_enable_counter(TIM2);
}

void platform_setup_tim8() {
  /* Set up TIM8 to cause an update event at 4 MHz */
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

static void platform_init_eventtimer() {

  platform_setup_tim2();

  /* Only enable interrupts for trigger/sync inputs */
  if (config.freq_inputs[0].type == TRIGGER) {
    timer_enable_irq(TIM2, TIM_DIER_CC1IE);
  }
  if (config.freq_inputs[1].type == TRIGGER) {
    timer_enable_irq(TIM2, TIM_DIER_CC2IE);
  }

  nvic_enable_irq(NVIC_TIM2_IRQ);
  nvic_set_priority(NVIC_TIM2_IRQ, 32);

  /* Set debug unit to stop the timer on halt */
  *((volatile uint32_t *)0xE0042008) |= 29; /*TIM2, TIM5, and TIM7 and */
  *((volatile uint32_t *)0xE004200C) |= 2;  /* TIM8 stop */

  platform_setup_tim8();
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

void set_pwm(int output, float percent) {
  if (percent < 0.0f) {
    percent = 0.0f;
  } else if (percent > 100.0f) {
    percent = 100.0f;
  }
  int ival = (percent / 100.0f) * 65535;
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
  for (i = 0; i < MAX_EVENTS; ++i) {
    if (config.events[i].inverted && config.events[i].type != DISABLED_EVENT) {
      set_output(config.events[i].pin, 1);
    }
  }
  gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, 0xFFFF & ~GPIO5);
  gpio_set_output_options(
    GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, 0xFFFF & ~GPIO5);
  gpio_mode_setup(GPIOE, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, 0xFF);
  gpio_set_output_options(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, 0xFF);

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
  dma_set_memory_address(DMA2, DMA_STREAM1, (uint32_t)output_buffers[0].slots);
  dma_set_memory_address_1(
    DMA2, DMA_STREAM1, (uint32_t)output_buffers[1].slots);
  dma_set_number_of_data(DMA2, DMA_STREAM1, NUM_SLOTS);
  dma_channel_select(DMA2, DMA_STREAM1, DMA_SxCR_CHSEL_7);
  dma_enable_direct_mode(DMA2, DMA_STREAM1);
  dma_enable_transfer_complete_interrupt(DMA2, DMA_STREAM1);

  timer_disable_counter(TIM8);
  dma_enable_stream(DMA2, DMA_STREAM1);
  timer_enable_counter(TIM8);

  nvic_enable_irq(NVIC_DMA2_STREAM1_IRQ);
  nvic_set_priority(NVIC_DMA2_STREAM1_IRQ, 16);
}

/* We use TIM7 to control the sample rate.  It is set up to trigger a DMA event
 * on counter update to TX on SPI2.  When the full 16 bits is transmitted and
 * the SPI RX buffer is filled, the RX DMA event will fill, and populate
 * spi_rx_raw_adc.
 */

#ifdef SPI_AD7888
#include "ad7888_adc.h"
#define SPI_FREQ_DIVIDER SPI_CR1_BAUDRATE_FPCLK_DIV_32 /* 1.3125 MHz */
#define SPI_SAMPLE_RATE 70000
#define ADC_SAMPLE_RATE 5000
#elif SPI_TLV2553
#include "tlv2553_adc.h"
#define SPI_FREQ_DIVIDER SPI_CR1_BAUDRATE_FPCLK_DIV_4 /* 10.5 MHz */
#define SPI_SAMPLE_RATE 150000
#define ADC_SAMPLE_RATE 5000
#define KNOCK_SAMPLE_RATE 50000
#else
#error No ADC specified!
#endif

static volatile uint16_t spi_rx_raw_adc[2][NUM_SPI_TX] = { 0 };

static void platform_init_spi_adc() {
  /* Configure SPI output */
  gpio_mode_setup(
    GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO13 | GPIO14 | GPIO15);
  gpio_set_af(GPIOB, GPIO_AF5, GPIO13 | GPIO14 | GPIO15);
  gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO12);
  gpio_set_output_options(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO12);
  gpio_clear(GPIOB, GPIO12);
  spi_disable(SPI2);
  spi_reset(SPI2);
  spi_enable_software_slave_management(SPI2);
  spi_set_nss_high(SPI2);
  spi_init_master(SPI2,
                  SPI_FREQ_DIVIDER,
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
  dma_enable_double_buffer_mode(DMA1, DMA_STREAM3);
  dma_set_memory_address(DMA1, DMA_STREAM3, (uint32_t)spi_rx_raw_adc[0]);
  dma_set_memory_address_1(DMA1, DMA_STREAM3, (uint32_t)spi_rx_raw_adc[1]);
  dma_set_number_of_data(DMA1, DMA_STREAM3, NUM_SPI_TX);
  dma_channel_select(DMA1, DMA_STREAM3, DMA_SxCR_CHSEL_0);
  dma_enable_direct_mode(DMA1, DMA_STREAM3);
  dma_enable_transfer_complete_interrupt(DMA1, DMA_STREAM3);
  dma_enable_stream(DMA1, DMA_STREAM3);

  nvic_enable_irq(NVIC_DMA1_STREAM3_IRQ);
  nvic_set_priority(NVIC_DMA1_STREAM3_IRQ, 64);

  /* Configure TIM7 to drive DMA for SPI */
  timer_set_mode(TIM7, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
  const uint32_t period = (84000000 / SPI_SAMPLE_RATE) - 1;
  timer_set_period(TIM7, period);
  timer_disable_preload(TIM7);
  timer_continuous_mode(TIM7);
  /* Setup output compare registers */
  timer_disable_oc_output(TIM7, TIM_OC1);
  timer_disable_oc_output(TIM7, TIM_OC2);
  timer_disable_oc_output(TIM7, TIM_OC3);
  timer_disable_oc_output(TIM7, TIM_OC4);

  timer_set_prescaler(TIM7, 0);

  /* Set up DMA for SPI TX */
  dma_stream_reset(DMA1, DMA_STREAM2);
  dma_set_priority(DMA1, DMA_STREAM2, DMA_SxCR_PL_LOW);
  dma_set_memory_size(DMA1, DMA_STREAM2, DMA_SxCR_MSIZE_16BIT);
  dma_set_peripheral_size(DMA1, DMA_STREAM2, DMA_SxCR_PSIZE_16BIT);
  dma_enable_circular_mode(DMA1, DMA_STREAM2);
  dma_enable_memory_increment_mode(DMA1, DMA_STREAM2);
  dma_set_transfer_mode(DMA1, DMA_STREAM2, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
  dma_set_peripheral_address(DMA1, DMA_STREAM2, (uint32_t)&SPI2_DR);
  dma_set_memory_address(DMA1, DMA_STREAM2, (uint32_t)adc_transmit_sequence);
  dma_set_number_of_data(DMA1, DMA_STREAM2, NUM_SPI_TX);
  dma_channel_select(DMA1, DMA_STREAM2, DMA_SxCR_CHSEL_1);
  dma_enable_direct_mode(DMA1, DMA_STREAM2);
  dma_enable_stream(DMA1, DMA_STREAM2);

  timer_enable_counter(TIM7);
  timer_enable_update_event(TIM7);
  timer_update_on_overflow(TIM7);
  timer_set_dma_on_update_event(TIM7);
  TIM7_DIER |= TIM_DIER_UDE; /* Enable update dma */
}

/* Sensor sampling complete */
void dma1_stream3_isr(void) {
  if (!dma_get_interrupt_flag(DMA1, DMA_STREAM3, DMA_TCIF)) {
    return;
  }
  dma_clear_interrupt_flags(DMA1, DMA_STREAM3, DMA_TCIF);

  const uint16_t *sequence = (dma_get_target(DMA1, DMA_STREAM3) == 0)
                               ? (const uint16_t *)spi_rx_raw_adc[1]
                               : (const uint16_t *)spi_rx_raw_adc[0];

  struct adc_update update = {
    .time = current_time(),
    .valid = adc_response_is_valid(sequence),
  };
  for (int i = 0; i < MAX_ADC_PINS; ++i) {
    uint32_t adc_value = read_adc_pin(sequence, i);
    /* 5 volt ADC inputs */
    float raw_value = (float)(5 * adc_value) / 4096.0f;
    update.values[i] = raw_value;
  }
  sensor_update_adc(&update);
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

static const char *const usb_strings[] = {
  "https://github.com/via/viaems/",
  "ViaEMS console",
  "0",
};

void otg_fs_isr() {
  usbd_poll(usbd_dev);
}

void platform_init_usb() {

  gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO11 | GPIO12);
  gpio_set_af(GPIOA, GPIO_AF10, GPIO11 | GPIO12);

  usbd_dev = usbd_init(&otgfs_usb_driver,
                       &dev,
                       &usb_config,
                       usb_strings,
                       3,
                       usbd_control_buffer,
                       sizeof(usbd_control_buffer));

  usbd_register_set_config_callback(usbd_dev, cdcacm_set_config);
  nvic_enable_irq(NVIC_OTG_FS_IRQ);
  nvic_set_priority(NVIC_OTG_FS_IRQ, 128);
}

static void setup_task_handler() {
  /* Set up systick for 100 hz */
  systick_set_reload(1680000);
  systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
  systick_counter_enable();
  nvic_set_priority(NVIC_SYSTICK_IRQ, 128);
  systick_interrupt_enable();

  /* Setup IWDG to reset if we pass 30 mS without an interrupt */
  iwdg_set_period_ms(30);
  iwdg_start();
}

void platform_benchmark_init() {
  rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);
  rcc_wait_for_osc_ready(RCC_HSE);
  rcc_periph_clock_enable(RCC_SYSCFG);
  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_GPIOE);
  rcc_periph_clock_enable(RCC_OTGFS);
  gpio_mode_setup(GPIOE, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, 0xFFFF);
  dwt_enable_cycle_counter();
  platform_init_usb();
}

void platform_init(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

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
  rcc_periph_clock_enable(RCC_TIM7);
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
  platform_init_usb();

  dwt_enable_cycle_counter();

  setup_task_handler();
}

void sys_tick_handler(void) {
  run_tasks();
  iwdg_reset();
}

static void platform_disable_periphs() {
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

  rcc_periph_reset_pulse(RST_TIM7);
  rcc_periph_clock_disable(RCC_TIM7);

  rcc_periph_reset_pulse(RST_TIM8);
  rcc_periph_clock_disable(RCC_TIM8);

  rcc_periph_reset_pulse(RST_SPI2);
  rcc_periph_clock_disable(RCC_SPI2);

  rcc_periph_reset_pulse(RST_OTGFS);
  rcc_periph_clock_disable(RCC_OTGFS);
}

#define BOOTLOADER_ADDR 0x1fff0000
void platform_reset_into_bootloader() {
  handle_emergency_shutdown();

  platform_disable_periphs();

  /* 168 Mhz clock */
  rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_168MHZ]);

  __asm__ volatile("msr msp, %0" ::"g"(*(volatile uint32_t *)BOOTLOADER_ADDR));
  (*(void (**)())(BOOTLOADER_ADDR + 4))();
  while (1)
    ;
}

uint64_t cycle_count() {
  return dwt_read_cycle_counter();
}

uint64_t cycles_to_ns(uint64_t cycles) {
  return cycles * 1000 / 168;
}

/* This is now the lowest priority interrupt, with buffer swapping and sensor
 * reading interrupts being higher priority.  Keep scheduled callbacks quick to
 * keep scheduling delay low.  Currently nothing takes more than 15 uS
 * (rescheduling individual events).  All fueling calculations and scheduling is
 * now done in the ISR for trigger updates.
 */
void tim2_isr() {

  bool cc1_fired = false;
  bool cc2_fired = false;
  timeval_t cc1;
  timeval_t cc2;

  if (timer_get_flag(TIM2, TIM_SR_CC1IF)) {
    cc1_fired = true;
    cc1 = TIM2_CCR1;
    timer_clear_flag(TIM2, TIM_SR_CC1IF);
  }

  if (timer_get_flag(TIM2, TIM_SR_CC2IF)) {
    cc2_fired = true;
    cc2 = TIM2_CCR2;
    timer_clear_flag(TIM2, TIM_SR_CC2IF);
  }

  /* Did we miss a capture event? */
  if (timer_get_flag(TIM2, TIM_SR_CC1OF) ||
      timer_get_flag(TIM2, TIM_SR_CC2OF)) {
    timer_clear_flag(TIM2, TIM_SR_CC1OF);
    timer_clear_flag(TIM2, TIM_SR_CC2OF);
    decoder_desync(DECODER_OVERFLOW);
    return;
  }

  if (cc1_fired && cc2_fired && time_before(cc2, cc1)) {
    decoder_update_scheduling(1, cc2);
    decoder_update_scheduling(0, cc1);
  } else {
    if (cc1_fired) {
      decoder_update_scheduling(0, cc1);
    }
    if (cc2_fired) {
      decoder_update_scheduling(1, cc2);
    }
  }

  if (timer_get_flag(TIM2, TIM_SR_CC4IF)) {
    timer_clear_flag(TIM2, TIM_SR_CC4IF);
    scheduler_callback_timer_execute();
  }
}
void dma2_stream1_isr(void) {
  if (dma_get_interrupt_flag(DMA2, DMA_STREAM1, DMA_TCIF)) {
    dma_clear_interrupt_flags(DMA2, DMA_STREAM1, DMA_TCIF);
    stm32_buffer_swap();

    if (stm32_current_buffer() != dma_get_target(DMA2, DMA_STREAM1)) {
      /* We have overflowed or gone out of sync, abort immediately */
      abort();
    }
  }
}

volatile int interrupt_disables = 0;
void enable_interrupts() {
  interrupt_disables -= 1;
  assert(interrupt_disables >= 0);
  cm_enable_interrupts();
}

/* Returns current enabled status prior to call */
void disable_interrupts() {
  cm_disable_interrupts();
  interrupt_disables += 1;
}

bool interrupts_enabled() {
  return !cm_is_masked_interrupts();
}

void schedule_event_timer(timeval_t time) {
  assert(!interrupts_enabled());
  /* Clear out any pending interrupt */
  timer_disable_irq(TIM2, TIM_DIER_CC4IE);
  timer_clear_flag(TIM2, TIM_SR_CC4IF);

  /* Set timer and enable interrupt */
  timer_set_oc_value(TIM2, TIM_OC4, time);
  timer_enable_irq(TIM2, TIM_DIER_CC4IE);

  /* If time is prior to now, we may have missed it, set the interrupt pending
   * just in case */
  if (time_before_or_equal(time, current_time())) {
    timer_generate_event(TIM2, TIM_EGR_CC4G);
  }
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

  handle_emergency_shutdown();
  platform_disable_periphs();
  /* Flash erase takes longer than our watchdog */
  iwdg_set_period_ms(5000);

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
  reset_handler();
}

size_t console_read(void *buf, size_t max) {
  disable_interrupts();
  size_t amt = usb_rx_len > max ? max : usb_rx_len;
  memcpy(buf, usb_rx_buf, amt);
  memmove(usb_rx_buf, usb_rx_buf + amt, usb_rx_len - amt);
  usb_rx_len -= amt;
  enable_interrupts();
  return amt;
}

size_t console_write(const void *buf, size_t count) {
  size_t rem = count > 64 ? 64 : count;
  /* https://github.com/libopencm3/libopencm3/issues/531
   * We can't let the usb irq be called while writing */
  nvic_disable_irq(NVIC_OTG_FS_IRQ);
  __asm__("dsb");
  __asm__("isb");
  rem = usbd_ep_write_packet(usbd_dev, 0x82, buf, rem);
  nvic_enable_irq(NVIC_OTG_FS_IRQ);
  __asm__("dsb");
  __asm__("isb");
  return rem;
}

/* Use usb to send text from newlib printf */
ssize_t __attribute__((externally_visible))
_write(int fd, const char *buf, size_t count) {
  (void)fd;
  size_t pos = 0;
  while (pos < count) {
    pos += console_write(buf + pos, count - pos);
    usbd_poll(usbd_dev);
  }
  return count;
}

void __attribute__((externally_visible)) _exit(int status) {
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

uint32_t platform_adc_samplerate(void) {
#ifdef ADC_SAMPLE_RATE
  return ADC_SAMPLE_RATE;
#else
  return 0;
#endif
}
uint32_t platform_knock_samplerate(void) {
#ifdef KNOCK_SAMPLE_RATE
  return KNOCK_SAMPLE_RATE;
#else
  return 1000;
#endif
}
