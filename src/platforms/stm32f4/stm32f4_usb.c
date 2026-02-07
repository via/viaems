#include <string.h>

#include "platform.h"
#include "stm32f4xx.h"
#include "tusb.h"

#define USB_VID 0x1209
#define USB_PID 0x2041
#define USB_BCD 0x0200

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
static tusb_desc_device_t const desc_device = {
  .bLength = sizeof(tusb_desc_device_t),
  .bDescriptorType = TUSB_DESC_DEVICE,
  .bcdUSB = USB_BCD,

  // Use Interface Association Descriptor (IAD) for CDC
  // As required by USB Specs IAD's subclass must be common class (2) and
  // protocol must be IAD (1)
  .bDeviceClass = TUSB_CLASS_MISC,
  .bDeviceSubClass = MISC_SUBCLASS_COMMON,
  .bDeviceProtocol = MISC_PROTOCOL_IAD,
  .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

  .idVendor = USB_VID,
  .idProduct = USB_PID,
  .bcdDevice = 0x0100,

  .iManufacturer = 0x01,
  .iProduct = 0x02,
  .iSerialNumber = 0x03,

  .bNumConfigurations = 0x01
};

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const *tud_descriptor_device_cb(void) {
  return (uint8_t const *)&desc_device;
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+
enum { ITF_NUM_CDC_0 = 0, ITF_NUM_CDC_0_DATA, ITF_NUM_TOTAL };

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + CFG_TUD_CDC * TUD_CDC_DESC_LEN)

#define EPNUM_CDC_0_OUT 0x01
#define EPNUM_CDC_0_IN 0x81
#define EPNUM_CDC_0_NOTIF 0x82

static uint8_t const desc_fs_configuration[] = {
  // Config number, interface count, string index, total length, attribute,
  // power in mA
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),

  // 1st CDC: Interface number, string index, EP notification address and size,
  // EP data address (out, in) and size.
  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_0,
                     4,
                     EPNUM_CDC_0_NOTIF,
                     16,
                     EPNUM_CDC_0_OUT,
                     EPNUM_CDC_0_IN,
                     64),

};

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
  (void)index; // for multiple configurations

  return desc_fs_configuration;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// String Descriptor Index
enum {
  STRID_LANGID = 0,
  STRID_MANUFACTURER,
  STRID_PRODUCT,
  STRID_SERIAL,
};

// array of pointer to string descriptors
static char const *string_desc_arr[] = {
  (const char[]){ 0x09, 0x04 }, // 0: is supported language is English (0x0409)
  "ViaEMS",                     // 1: Manufacturer
  "https://github.com/via/viaems", // 2: Product
  "000",                           // 3: Serials will use unique ID if possible
  "ViaEMS Console",                // 4: CDC Interface
};

static uint16_t _desc_str[32 + 1];

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long
// enough for transfer to complete
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void)langid;
  size_t chr_count;

  switch (index) {
  case STRID_LANGID:
    memcpy(&_desc_str[1], string_desc_arr[0], 2);
    chr_count = 1;
    break;

  default:
    // Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
    // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

    if (!(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0]))) {
      return NULL;
    }

    const char *str = string_desc_arr[index];

    // Cap at max char
    chr_count = strlen(str);
    size_t const max_count =
      sizeof(_desc_str) / sizeof(_desc_str[0]) - 1; // -1 for string type
    if (chr_count > max_count) {
      chr_count = max_count;
    }

    // Convert ASCII string into UTF-16
    for (size_t i = 0; i < chr_count; i++) {
      _desc_str[1 + i] = str[i];
    }
    break;
  }

  // first byte is length (including header), second byte is string type
  _desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
  return _desc_str;
}

size_t platform_read(uint8_t *ptr, size_t max) {
  NVIC_DisableIRQ(OTG_FS_IRQn);
  size_t result = 0;
  if (tud_cdc_ready()) {
    result = tud_cdc_read(ptr, max);
  }
  NVIC_EnableIRQ(OTG_FS_IRQn);
  return result;
}

size_t platform_write(const uint8_t *ptr, size_t max) {
  if (!tud_cdc_connected()) {
    return max;
  }
  const uint8_t *s = ptr;
  size_t remaining = max;
  while (remaining > 0) {
    NVIC_DisableIRQ(OTG_FS_IRQn);
    uint32_t written = tud_cdc_write(s, remaining);
    NVIC_EnableIRQ(OTG_FS_IRQn);
    s += written;
    remaining -= written;
  }
  tud_cdc_write_flush();
  return max;
}

/* Use usb to send text from newlib printf */
int __attribute__((externally_visible)) _write(int fd,
                                               const char *buf,
                                               size_t count) {
  (void)fd;
  size_t pos = 0;
  while (pos < count) {
    pos += platform_write(buf + pos, count - pos);
  }
  return count;
}

uint32_t SystemCoreClock = 168000000;
void OTG_FS_IRQHandler(void) {
  tusb_int_handler(0, true);
  tud_task();
}

void stm32f4_configure_usb(void) {
  RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;

  GPIOA->MODER &= ~(GPIO_MODER_MODE11 | GPIO_MODER_MODE12);
  GPIOA->MODER |= _VAL2FLD(GPIO_MODER_MODE11, 2) |
                  _VAL2FLD(GPIO_MODER_MODE12, 2); /* A11/A12 in AF mode */
  GPIOA->OSPEEDR |=
    GPIO_OSPEEDR_OSPEED11 | GPIO_OSPEEDR_OSPEED12; /* High speed */
  GPIOA->AFR[1] |= _VAL2FLD(GPIO_AFRH_AFSEL11, 10) |
                   _VAL2FLD(GPIO_AFRH_AFSEL12, 10); /* AF10 (USB FS)*/

  NVIC_SetPriority(OTG_FS_IRQn, 15);
  NVIC_EnableIRQ(OTG_FS_IRQn);

  USB_OTG_FS->GCCFG |= USB_OTG_GCCFG_NOVBUSSENS;

  tusb_rhport_init_t dev_init = {
    .role = TUSB_ROLE_DEVICE,
    .speed = TUSB_SPEED_AUTO,
  };
  tusb_init(0, &dev_init);
}
