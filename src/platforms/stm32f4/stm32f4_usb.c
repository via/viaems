#include <string.h>

#include "platform.h"
#include "stm32f4xx.h"
#include "usb.h"
#include "usb_cdc.h"

#define CDC_EP0_SIZE 0x08
#define CDC_RXD_EP 0x01
#define CDC_TXD_EP 0x81
#define CDC_DATA_SZ 0x40
#define CDC_NTF_EP 0x82
#define CDC_NTF_SZ 0x08

struct cdc_config {
  struct usb_config_descriptor config;
  struct usb_iad_descriptor comm_iad;
  struct usb_interface_descriptor comm;
  struct usb_cdc_header_desc cdc_hdr;
  struct usb_cdc_call_mgmt_desc cdc_mgmt;
  struct usb_cdc_acm_desc cdc_acm;
  struct usb_cdc_union_desc cdc_union;
  struct usb_endpoint_descriptor comm_ep;
  struct usb_interface_descriptor data;
  struct usb_endpoint_descriptor data_eprx;
  struct usb_endpoint_descriptor data_eptx;
} __attribute__((packed));

/* Device descriptor */
static const struct usb_device_descriptor device_desc = {
  .bLength = sizeof(struct usb_device_descriptor),
  .bDescriptorType = USB_DTYPE_DEVICE,
  .bcdUSB = VERSION_BCD(2, 0, 0),
  .bDeviceClass = USB_CLASS_IAD,
  .bDeviceSubClass = USB_SUBCLASS_IAD,
  .bDeviceProtocol = USB_PROTO_IAD,
  .bMaxPacketSize0 = CDC_EP0_SIZE,
  .idVendor = 0x1209,
  .idProduct = 0x2041,
  .bcdDevice = VERSION_BCD(1, 0, 0),
  .iManufacturer = 1,
  .iProduct = 2,
  .iSerialNumber = INTSERIALNO_DESCRIPTOR,
  .bNumConfigurations = 1,
};

/* Device configuration descriptor */
static const struct cdc_config config_desc = {
    .config = {
        .bLength                = sizeof(struct usb_config_descriptor),
        .bDescriptorType        = USB_DTYPE_CONFIGURATION,
        .wTotalLength           = sizeof(struct cdc_config),
        .bNumInterfaces         = 2,
        .bConfigurationValue    = 1,
        .iConfiguration         = NO_DESCRIPTOR,
        .bmAttributes           = USB_CFG_ATTR_RESERVED | USB_CFG_ATTR_SELFPOWERED,
        .bMaxPower              = USB_CFG_POWER_MA(100),
    },
    .comm_iad = {
        .bLength = sizeof(struct usb_iad_descriptor),
        .bDescriptorType        = USB_DTYPE_INTERFASEASSOC,
        .bFirstInterface        = 0,
        .bInterfaceCount        = 2,
        .bFunctionClass         = USB_CLASS_CDC,
        .bFunctionSubClass      = USB_CDC_SUBCLASS_ACM,
        .bFunctionProtocol      = USB_PROTO_NONE,
        .iFunction              = NO_DESCRIPTOR,
    },
    .comm = {
        .bLength                = sizeof(struct usb_interface_descriptor),
        .bDescriptorType        = USB_DTYPE_INTERFACE,
        .bInterfaceNumber       = 0,
        .bAlternateSetting      = 0,
        .bNumEndpoints          = 1,
        .bInterfaceClass        = USB_CLASS_CDC,
        .bInterfaceSubClass     = USB_CDC_SUBCLASS_ACM,
        .bInterfaceProtocol     = USB_PROTO_NONE,
        .iInterface             = NO_DESCRIPTOR,
    },
    .cdc_hdr = {
        .bFunctionLength        = sizeof(struct usb_cdc_header_desc),
        .bDescriptorType        = USB_DTYPE_CS_INTERFACE,
        .bDescriptorSubType     = USB_DTYPE_CDC_HEADER,
        .bcdCDC                 = VERSION_BCD(1,1,0),
    },
    .cdc_mgmt = {
        .bFunctionLength        = sizeof(struct usb_cdc_call_mgmt_desc),
        .bDescriptorType        = USB_DTYPE_CS_INTERFACE,
        .bDescriptorSubType     = USB_DTYPE_CDC_CALL_MANAGEMENT,
        .bmCapabilities         = 0,
        .bDataInterface         = 1,

    },
    .cdc_acm = {
        .bFunctionLength        = sizeof(struct usb_cdc_acm_desc),
        .bDescriptorType        = USB_DTYPE_CS_INTERFACE,
        .bDescriptorSubType     = USB_DTYPE_CDC_ACM,
        .bmCapabilities         = 0,
    },
    .cdc_union = {
        .bFunctionLength        = sizeof(struct usb_cdc_union_desc),
        .bDescriptorType        = USB_DTYPE_CS_INTERFACE,
        .bDescriptorSubType     = USB_DTYPE_CDC_UNION,
        .bMasterInterface0      = 0,
        .bSlaveInterface0       = 1,
    },
    .comm_ep = {
        .bLength                = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType        = USB_DTYPE_ENDPOINT,
        .bEndpointAddress       = CDC_NTF_EP,
        .bmAttributes           = USB_EPTYPE_INTERRUPT,
        .wMaxPacketSize         = CDC_NTF_SZ,
        .bInterval              = 0xFF,
    },
    .data = {
        .bLength                = sizeof(struct usb_interface_descriptor),
        .bDescriptorType        = USB_DTYPE_INTERFACE,
        .bInterfaceNumber       = 1,
        .bAlternateSetting      = 0,
        .bNumEndpoints          = 2,
        .bInterfaceClass        = USB_CLASS_CDC_DATA,
        .bInterfaceSubClass     = USB_SUBCLASS_NONE,
        .bInterfaceProtocol     = USB_PROTO_NONE,
        .iInterface             = NO_DESCRIPTOR,
    },
    .data_eprx = {
        .bLength                = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType        = USB_DTYPE_ENDPOINT,
        .bEndpointAddress       = CDC_RXD_EP,
        .bmAttributes           = USB_EPTYPE_BULK,
        .wMaxPacketSize         = CDC_DATA_SZ,
        .bInterval              = 0x01,
    },
    .data_eptx = {
        .bLength                = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType        = USB_DTYPE_ENDPOINT,
        .bEndpointAddress       = CDC_TXD_EP,
        .bmAttributes           = USB_EPTYPE_BULK,
        .wMaxPacketSize         = CDC_DATA_SZ,
        .bInterval              = 0x01,
    },
};

const char *manu_str = "https://github.com/via/viaems/";
const char *prod_str = "ViaEMS console";

usbd_device udev;
uint32_t ubuf[0x20];

#define USB_RX_BUF_LEN 1024
static char usb_rx_buf[USB_RX_BUF_LEN];
static _Atomic size_t usb_rx_len = 0;

static struct usb_cdc_line_coding cdc_line = {
  .dwDTERate = 115200,
  .bCharFormat = USB_CDC_1_STOP_BITS,
  .bParityType = USB_CDC_NO_PARITY,
  .bDataBits = 8,
};

struct stringdesc {
  uint8_t length;
  uint8_t type;
  uint16_t data[128];
} __attribute__((packed));
static struct stringdesc stringdesc;

static void populate_string_descriptor(struct stringdesc *dest,
                                       const char *str) {
  size_t len = strlen(str);
  dest->length = len * 2 + 2;
  dest->type = 0x03;

  for (size_t pos = 0; pos < len; pos++) {
    stringdesc.data[pos] = str[pos];
  }
}

static usbd_respond cdc_getdesc(usbd_ctlreq *req,
                                void **address,
                                uint16_t *length) {
  const uint8_t dtype = req->wValue >> 8;
  const uint8_t dnumber = req->wValue & 0xFF;
  const void *desc;
  uint16_t len = 0;
  switch (dtype) {
  case USB_DTYPE_DEVICE:
    desc = &device_desc;
    break;
  case USB_DTYPE_CONFIGURATION:
    desc = &config_desc;
    len = sizeof(config_desc);
    break;
  case USB_DTYPE_STRING:
    switch (dnumber) {
    case 0:
      stringdesc =
        (struct stringdesc){ .length = 4, .type = 0x03, .data = { 0x0409 } };
      break;
    case 1:
      populate_string_descriptor(&stringdesc, manu_str);
      break;
    case 2:
      populate_string_descriptor(&stringdesc, prod_str);
      break;
    default:
      return usbd_fail;
    }
    desc = &stringdesc;
    break;
  default:
    return usbd_fail;
  }
  if (len == 0) {
    len = ((struct usb_header_descriptor *)desc)->bLength;
  }
  *address = (void *)desc;
  *length = len;
  return usbd_ack;
}

static usbd_respond cdc_control(usbd_device *dev,
                                usbd_ctlreq *req,
                                usbd_rqc_callback *callback) {
  (void)callback;
  if (((USB_REQ_RECIPIENT | USB_REQ_TYPE) & req->bmRequestType) ==
        (USB_REQ_INTERFACE | USB_REQ_CLASS) &&
      req->wIndex == 0) {
    switch (req->bRequest) {
    case USB_CDC_SET_CONTROL_LINE_STATE:
      return usbd_ack;
    case USB_CDC_SET_LINE_CODING:
      memcpy(&cdc_line, req->data, sizeof(cdc_line));
      return usbd_ack;
    case USB_CDC_GET_LINE_CODING:
      dev->status.data_ptr = &cdc_line;
      dev->status.data_count = sizeof(cdc_line);
      return usbd_ack;
    default:
      return usbd_fail;
    }
  }
  return usbd_fail;
}

static void cdc_rx(usbd_device *dev, uint8_t event, uint8_t ep) {
  (void)event;
  if (ep != CDC_RXD_EP) {
    return;
  }
  char buf[64];
  int ret = usbd_ep_read(dev, CDC_RXD_EP, buf, CDC_DATA_SZ);
  if (ret < 0) {
    return;
  }

  if (USB_RX_BUF_LEN - usb_rx_len < (unsigned)ret) {
    /* No space, just drop the packet */
  } else {
    memcpy(usb_rx_buf + usb_rx_len, buf, ret);
    usb_rx_len += ret;
  }
}

static usbd_respond cdc_setconf(usbd_device *dev, uint8_t cfg) {
  switch (cfg) {
  case 0:
    /* deconfiguring device */
    usbd_ep_deconfig(dev, CDC_NTF_EP);
    usbd_ep_deconfig(dev, CDC_TXD_EP);
    usbd_ep_deconfig(dev, CDC_RXD_EP);
    usbd_reg_endpoint(dev, CDC_RXD_EP, 0);
    return usbd_ack;
  case 1:
    /* configuring device */
    usbd_ep_config(
      dev, CDC_RXD_EP, USB_EPTYPE_BULK | USB_EPTYPE_DBLBUF, CDC_DATA_SZ);
    usbd_ep_config(
      dev, CDC_TXD_EP, USB_EPTYPE_BULK | USB_EPTYPE_DBLBUF, CDC_DATA_SZ);
    usbd_ep_config(dev, CDC_NTF_EP, USB_EPTYPE_INTERRUPT, CDC_NTF_SZ);
    usbd_reg_endpoint(dev, CDC_RXD_EP, cdc_rx);
    usbd_ep_write(dev, CDC_TXD_EP, 0, 0);
    return usbd_ack;
  default:
    return usbd_fail;
  }
}

static void cdc_init_usbd(void) {
  usbd_init(&udev, &usbd_hw, CDC_EP0_SIZE, ubuf, sizeof(ubuf));
  usbd_reg_config(&udev, cdc_setconf);
  usbd_reg_control(&udev, cdc_control);
  usbd_reg_descr(&udev, cdc_getdesc);
}

void _OTG_FS_IRQHandler(void) {
  usbd_poll(&udev);
}

size_t console_read(void *ptr, size_t max) {
  disable_interrupts();
  size_t amt = usb_rx_len > max ? max : usb_rx_len;
  memcpy(ptr, usb_rx_buf, amt);
  memmove(usb_rx_buf, usb_rx_buf + amt, usb_rx_len - amt);
  usb_rx_len -= amt;
  enable_interrupts();
  return amt;
}

size_t console_write(const void *ptr, size_t max) {
  int amt = max;
  if (amt > CDC_DATA_SZ) {
    amt = CDC_DATA_SZ;
  }

  disable_interrupts();
  int written = usbd_ep_write(&udev, CDC_TXD_EP, (void *)ptr, amt);
  enable_interrupts();
  if (written < 0) {
    return 0;
  }
  return written;
}

void stm32f4_configure_usb(void) {
  GPIOA->MODER &= ~(GPIO_MODER_MODE11 | GPIO_MODER_MODE12);
  GPIOA->MODER |= _VAL2FLD(GPIO_MODER_MODE11, 2) |
                  _VAL2FLD(GPIO_MODER_MODE12, 2); /* A11/A12 in AF mode */
  GPIOA->OSPEEDR |=
    GPIO_OSPEEDR_OSPEED11 | GPIO_OSPEEDR_OSPEED12; /* High speed */
  GPIOA->AFR[1] |= _VAL2FLD(GPIO_AFRH_AFSEL11, 10) |
                   _VAL2FLD(GPIO_AFRH_AFSEL12, 10); /* AF10 (USB FS)*/

  cdc_init_usbd();
  NVIC_SetPriority(OTG_FS_IRQn, 15);
  NVIC_EnableIRQ(OTG_FS_IRQn);
  usbd_enable(&udev, true);
  usbd_connect(&udev, true);
}
