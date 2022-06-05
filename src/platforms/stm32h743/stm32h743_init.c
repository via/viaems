#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/usb/usbd.h>

#include "platform.h"

void platform_enable_event_logging() {}

void platform_disable_event_logging() {}

void platform_reset_into_bootloader() {}

void set_pwm(int pin, float val) {
  (void)pin;
  (void)val;
}

void disable_interrupts() {
}

void enable_interrupts() {
}

timeval_t current_time() {
  return 0;
}

uint64_t cycle_count() {
  return 0;
}

uint64_t cycles_to_ns(uint64_t s) { return 0; }

timeval_t platform_output_earliest_schedulable_time() {
}


void set_event_timer(timeval_t t) {
}

timeval_t get_event_timer() {
  return 0;
}

void clear_event_timer() {
}

void disable_event_timer() {
}

int interrupts_enabled() {
  return 0;
}

void set_output(int output, char value) {
}

int get_output(int output) {
}

void set_gpio(int output, char value) {
}

void adc_gather(void *_adc) {
  (void)_adc;
}

void set_test_trigger_rpm(uint32_t rpm) {
  (void)rpm;
}

uint32_t get_test_trigger_rpm() {
  return 0;
}

void platform_save_config() {}

extern unsigned _configdata_loadaddr, _sconfigdata, _econfigdata;
void platform_load_config() {
  volatile unsigned *src, *dest;
  for (src = &_configdata_loadaddr, dest = &_sconfigdata; dest < &_econfigdata;
       src++, dest++) {
    *dest = *src;
  }
}

void platform_benchmark_init() {}

static void setup_clocks() {
  struct rcc_pll_config pll_config = {
    .sysclock_source = RCC_PLL,
    .pll_source = RCC_PLLCKSELR_PLLSRC_HSE,
    .hse_frequency = 8000000,
    .pll1 = {
      .divm = 1,
      .divn = 50,
      .divp = 1,
    },
    .pll3 = {
      .divm = 1,
      .divn = 6,
      .divp = 5,
      .divq = 1,
    },
    .core_pre = RCC_D1CFGR_D1CPRE_BYP,
    .hpre = RCC_D1CFGR_D1HPRE_DIV2,
    .ppre1 = RCC_D2CFGR_D2PPRE_DIV2, 
    .ppre2 = RCC_D2CFGR_D2PPRE_DIV2,
    .ppre3 = RCC_D1CFGR_D1PPRE_DIV2,
    .ppre4 = RCC_D3CFGR_D3PPRE_DIV2,
    .flash_waitstates = 2,
    .voltage_scale = PWR_VOS_SCALE_1,
    .power_mode = PWR_SYS_SCU_LDO,
  };
  rcc_clock_setup_pll(&pll_config);
  rcc_set_usb_clksel(RCC_D2CCIP2R_USBSEL_PLL3Q);

  PWR_CR3 |= PWR_CR3_USB33DEN;
}

static void configure_usart1() {
  usart_enable_fifos(USART1);
  usart_set_baudrate(USART1, 115200);
  usart_set_databits(USART1, 8);
  usart_set_stopbits(USART1, USART_STOPBITS_1);
  usart_set_mode(USART1, USART_MODE_TX);
  usart_enable(USART1);

  gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6 | GPIO7);
  gpio_set_af(GPIOB, GPIO_AF7, GPIO6 | GPIO7);
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

size_t console_write(const void *ptr, size_t max) {
  const char *c = ptr;
  nvic_disable_irq(NVIC_OTG_FS_IRQ);
  __asm__("dsb");
  __asm__("isb");
  size_t rem = max > 64 ? 64 : max;
  rem = usbd_ep_write_packet(usbd_dev, 0x82, ptr, rem);
  nvic_enable_irq(NVIC_OTG_FS_IRQ);
  __asm__("dsb");
  __asm__("isb");
  return rem;
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

void otg_fs_isr() {
  usbd_poll(usbd_dev);
}

static void platform_init_usb() {

  gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO11 | GPIO12);
  gpio_set_af(GPIOA, GPIO_AF10, GPIO11 | GPIO12);

  usbd_dev = usbd_init(&stm32h743_usb_driver,
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

void platform_init() {
  /* TODO: Icache */

  setup_clocks();

  rcc_periph_clock_enable(RCC_SYSCFG);
  rcc_periph_clock_enable(RCC_USART1);
  rcc_periph_clock_enable(RCC_GPIOB);
  rcc_periph_clock_enable(RCC_GPIOC);

  configure_usart1();
  platform_init_usb();
}
