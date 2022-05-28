#include "stm32h743xx.h"

#include <stdio.h>
#include <stdbool.h>

enum setup_request_direction {
  SETUP_HOST_TO_DEVICE = 0,
  SETUP_DEVICE_TO_HOST = 1,
};

enum setup_request_type {
  SETUP_TYPE_STANDARD = 0,
  SETUP_TYPE_CLASS = 1,
  SETUP_TYPE_VENDOR = 2,
};

enum setup_request_recipient {
  SETUP_RECIP_DEVICE = 0,
  SETUP_RECIP_INTERFACE = 1,
  SETUP_RECIP_ENDPOINT = 2,
  SETUP_RECIP_OTHER = 3,
};

struct setup_packet {
  uint8_t request_type;
  uint8_t request;
  uint16_t value;
  uint16_t index;
  uint16_t length;
};

enum setup_request {
  SETUP_GET_STATUS = 0,
  SETUP_CLEAR_FEATURE = 1,
  SETUP_SET_FEATURE = 3,
  SETUP_SET_ADDRESS = 5,
  SETUP_GET_DESCRIPTOR = 6,
  SETUP_SET_DESCRIPTOR = 7,
  SETUP_GET_CONFIGURATION = 8,
  SETUP_SET_CONFIGURATION = 9,
  SETUP_GET_INTERFACE = 10,
  SETUP_SET_INTERFACE = 11,
  SETUP_SYNCH_FRAME = 12,
};

enum setup_request_recipient parse_setup_recipient(uint8_t req) {
  return req & 0x1F;
}

enum setup_request_type parse_setup_type(uint8_t req) {
  return (req >> 5) & 3;
}

enum setup_request_direction parse_setup_direction(uint8_t req) {
  return (req >> 7) & 1;
}

enum descriptor_type {
  DESC_DEVICE = 1,
  DESC_CONFIGURATION = 2,
  DESC_STRING = 3,
  DESC_INTERFACE = 4,
  DESC_ENDPOINT = 5,
};

struct device_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t bcdUSB;
	uint8_t bDeviceClass;
	uint8_t bDeviceSubClass;
	uint8_t bDeviceProtocol;
	uint8_t bMaxPacketSize0;
	uint16_t idVendor;
	uint16_t idProduct;
	uint16_t bcdDevice;
	uint8_t iManufacturer;
	uint8_t iProduct;
	uint8_t iSerialNumber;
	uint8_t bNumConfigurations;
};
#define DEVICE_DESCRIPTOR_SIZE 18

struct device_descriptor d = {
  .bLength = 18,
  .bDescriptorType = DESC_DEVICE,
  .bcdUSB = 0x0200,
  .bDeviceClass = 0xFF,
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

void render_device_descriptor(uint8_t *out, const struct device_descriptor *d) {
  *out++ = d->bLength;
  *out++ = d->bDescriptorType;

  *out++ = d->bcdUSB & 0xFF;
  *out++ = (d->bcdUSB >> 8) & 0xFF;

  *out++ = d->bDeviceClass;
  *out++ = d->bDeviceSubClass;
  *out++ = d->bDeviceProtocol;
  *out++ = d->bMaxPacketSize0;

  *out++ = d->idVendor & 0xFF;
  *out++ = (d->idVendor >> 8) & 0xFF;

  *out++ = d->idProduct & 0xFF;
  *out++ = (d->idProduct >> 8) & 0xFF;

  *out++ = d->iManufacturer;
  *out++ = d->iProduct;
  *out++ = d->iSerialNumber;
  *out++ = d->bNumConfigurations;
}

static USB_OTG_GlobalTypeDef *const OTG  = (void*)(USB_OTG_FS_PERIPH_BASE + USB_OTG_GLOBAL_BASE);
static USB_OTG_DeviceTypeDef *const OTGD = (void*)(USB_OTG_FS_PERIPH_BASE + USB_OTG_DEVICE_BASE);

static inline uint32_t *EPFIFO(uint32_t ep) {
    return (uint32_t*)(USB_OTG_FS_PERIPH_BASE + USB_OTG_FIFO_BASE + (ep << 12));
}

static inline USB_OTG_INEndpointTypeDef *EPIN(uint32_t ep) {
    return (void*)(USB_OTG_FS_PERIPH_BASE + USB_OTG_IN_ENDPOINT_BASE + (ep << 5));
}

static inline USB_OTG_OUTEndpointTypeDef *EPOUT(uint32_t ep) {
    return (void*)(USB_OTG_FS_PERIPH_BASE + USB_OTG_OUT_ENDPOINT_BASE + (ep << 5));
}


static void transmit_ep0(uint8_t *bytes, int length) {
  EPIN(0)->DIEPTSIZ = 
    _VAL2FLD(USB_OTG_DIEPTSIZ_PKTCNT, 1) |
    _VAL2FLD(USB_OTG_DIEPTSIZ_XFRSIZ, length);
  EPIN(0)->DIEPCTL |= 
    USB_OTG_DIEPCTL_CNAK |
    USB_OTG_DIEPCTL_EPENA;


  volatile uint32_t *tx_fifo = EPFIFO(0);
  int pos = 0;
  while (length > 0) {
    /* TODO actually do the right thing */
    *tx_fifo = *((uint32_t *)&bytes[pos]);
    pos += 4;
    length -= 4;
  }
}


static void setup_get_descriptor(uint8_t type, uint8_t index, uint16_t length) {
  printf("get_descriptor: type=%x, index=%x, length=%x\r\n", type,index, length);
  switch(type) {
    case DESC_DEVICE: {
      uint8_t buf[DEVICE_DESCRIPTOR_SIZE];
      render_device_descriptor(buf, &d);
      transmit_ep0(buf, 18);
    }
  }
}

static void parse_setup_packet(uint8_t *rawpkt) {
  struct setup_packet pkt = {
    .request_type = rawpkt[0],
    .request = rawpkt[1],
    .value = (rawpkt[3] << 8) | rawpkt[2],
    .index = (rawpkt[5] << 8) | rawpkt[4],
    .length = (rawpkt[7] << 8) | rawpkt[6],
  };
  switch (pkt.request) {
    case SETUP_GET_DESCRIPTOR:     
      return setup_get_descriptor(pkt.value >> 8, pkt.value & 0xFF, pkt.length);
  }
}


static void handle_reset() {
  printf("handle_reset()\r\n");
  /* 1) SNAK for all OUT endpoints */
//  OTG2_HS_DEVICE->OTG_HS_DOEPCTL0 |= OTG2_HS_DEVICE_OTG_HS_DOEPCTL0_SNAK;

  /* 2) Unmask interrupts */
  OTGD->DAINTMSK |= _VAL2FLD(USB_OTG_DAINTMSK_OEPM, 1) |
                    _VAL2FLD(USB_OTG_DAINTMSK_IEPM, 1);

  OTGD->DOEPMSK |= USB_OTG_DOEPMSK_STUPM |
                   USB_OTG_DOEPMSK_XFRCM;

  OTGD->DIEPMSK |= USB_OTG_DIEPMSK_TOM |
                   USB_OTG_DIEPMSK_XFRCM;

  /* Program FIFO size */
  OTG->GRXFSIZ = 128; /* 512 bytes for RX FIFO */
  OTG->DIEPTXF0_HNPTXFSIZ =
    _VAL2FLD(USB_OTG_TX0FD,16) | /* 64 byte depth */
    _VAL2FLD(USB_OTG_TX0FSA, 512);

  EPOUT(0)->DOEPTSIZ |= _VAL2FLD(USB_OTG_DOEPTSIZ_STUPCNT, 3);

  OTG->GINTMSK |= USB_OTG_GINTMSK_IEPINT |
                  USB_OTG_GINTMSK_OEPINT;

}

uint32_t enumeration_result;
static void handle_enumeration(void) {
  printf("handle_enumeration()\r\n");
  enumeration_result = OTGD->DSTS;
  EPIN(0)->DIEPCTL |= _VAL2FLD(USB_OTG_DIEPCTL_MPSIZ, 64);
}

static void read_fifo_to_bytes(uint8_t *buf, int count) {
  volatile uint32_t *rx_fifo = (uint32_t*)(0x40080000 + 0x1000);
  uint32_t word;
  while (count > 0) {
    word = *rx_fifo;
    int bytes_to_copy = count > 4 ? 4 : count;
    *buf++ = (word >> 0) & 0xFF;
    if (bytes_to_copy > 1) {
      *buf++ = (word >> 8) & 0xFF;
    }
    if (bytes_to_copy > 2) {
      *buf++ = (word >> 16) & 0xFF;
    }
    if (bytes_to_copy > 3) {
      *buf++ = (word >> 24) & 0xFF;
    }
    count -= bytes_to_copy;
  }
}


static void handle_rx(void) {
  uint32_t status = OTG->GRXSTSP;
  uint32_t length = (status >> 4) & 0x7FF;
  printf("handle_rx() len=%d, status=%x\r\n", length, status);
  switch(_FLD2VAL(USB_OTG_GRXSTSP_PKTSTS, status)) {
    case 0x6: {
      /* Setup Received */
      uint8_t pkt[8];
      read_fifo_to_bytes(pkt, 8);
      parse_setup_packet(pkt);
    }
  }
}

static void handle_otgint() {
  OTG->GOTGINT = OTG->GOTGINT;
}

void otg_fs_isr(void) {
  uint32_t usb_status = OTG->GINTSTS;
  uint32_t otg_status = OTG->GOTGINT;

  printf("%lu: gintsts: %x  gotgint: %x\r\n", current_time(), usb_status, otg_status);

  if (OTG->GINTSTS & USB_OTG_GINTSTS_USBRST) {
    handle_reset();
    OTG->GINTSTS = USB_OTG_GINTSTS_USBRST;
  }

  if (OTG->GINTSTS & USB_OTG_GINTSTS_OEPINT) {
    uint32_t ints = EPOUT(0)->DOEPINT;
    printf("got oepint, doepint0=%x\r\n", ints);
    EPOUT(0)->DOEPINT = ints;
  }

  if (OTG->GINTSTS & USB_OTG_GINTSTS_IEPINT) {
    uint32_t ints = EPIN(0)->DIEPINT;
    printf("got iepint, diepint0=%x\r\n", ints);
    EPIN(0)->DIEPINT = ints;
  }

  if (OTG->GINTSTS & USB_OTG_GINTSTS_ENUMDNE) {
    handle_enumeration();
    OTG->GINTSTS = USB_OTG_GINTSTS_ENUMDNE;
  }

  if (OTG->GINTSTS & USB_OTG_GINTSTS_RXFLVL) {
    handle_rx();
    OTG->GINTSTS = USB_OTG_GINTSTS_RXFLVL;
  }

  if (OTG->GINTSTS & USB_OTG_GINTSTS_PTXFE) {
  }

  if (OTG->GINTSTS & USB_OTG_GINTSTS_OTGINT) {
    handle_otgint();
  }

  if (OTG->GINTSTS & USB_OTG_GINTSTS_MMIS) {
    printf("mode mismatch!\n");
    OTG->GINTSTS = USB_OTG_GINTSTS_MMIS;
  }

  if (OTG->GINTSTS & USB_OTG_GINTSTS_ESUSP) {
    printf("end suspend!\n");
    OTG->GINTSTS = USB_OTG_GINTSTS_ESUSP;
  }

  if (OTG->GINTSTS & USB_OTG_GINTSTS_USBSUSP) {
    printf("start suspend!\n");
    OTG->GINTSTS = USB_OTG_GINTSTS_USBSUSP;
  }

  if (OTG->GINTSTS & USB_OTG_GINTSTS_SOF) {
    printf("sof!\n");
    OTG->GINTSTS = USB_OTG_GINTSTS_SOF;
  }
}


static void usb_core_init(void) {
  OTG->GINTSTS |= OTG->GINTSTS; /* Clear all */
  /* Enable global interrupts and interrupt on empty FIFOs*/
  OTG->GAHBCFG = USB_OTG_GAHBCFG_GINT |
                                   USB_OTG_GAHBCFG_PTXFELVL;

  OTG->GUSBCFG = USB_OTG_GUSBCFG_FDMOD |
                                   _VAL2FLD(USB_OTG_GUSBCFG_TRDT, 6) |
                                   USB_OTG_GUSBCFG_PHYSEL;

  OTG->GINTMSK = USB_OTG_GINTMSK_OTGINT |
                                   USB_OTG_GINTMSK_RXFLVLM |
                                   USB_OTG_GINTMSK_MMISM;
}

static void usb_periph_init(void) {
  /* Set to full speed with internal PHY */
  OTGD->DCFG = _VAL2FLD(USB_OTG_DCFG_DSPD, 3);

  OTG->GINTMSK |= USB_OTG_GINTMSK_USBRST |
                  USB_OTG_GINTMSK_ENUMDNEM |
                  USB_OTG_GINTMSK_ESUSPM |
                  USB_OTG_GINTMSK_USBSUSPM;

  OTG->GCCFG = USB_OTG_GCCFG_VBDEN |
               USB_OTG_GCCFG_PWRDWN;

//  OTG->OTG_HS_GOTGCTL |= 0xC;

  /* Clear soft disconnect */
  OTGD->DCTL &= ~USB_OTG_DCTL_SDIS;

                                
}

static void configure_usb_gpios() {
  /* A10,A11 DM/DP */
  GPIOA->MODER &= ~(GPIO_MODER_MODE11 | GPIO_MODER_MODE12);
  GPIOA->MODER |= _VAL2FLD(GPIO_MODER_MODE11, 2) | _VAL2FLD(GPIO_MODER_MODE12, 2); /* A11/A12 in AF mode */
  GPIOA->OSPEEDR = GPIO_OSPEEDR_OSPEED11 | GPIO_OSPEEDR_OSPEED12; /* High speed */
  GPIOA->AFR[1] |= _VAL2FLD(GPIO_AFRH_AFSEL11, 10) | _VAL2FLD(GPIO_AFRH_AFSEL12, 10); /* AF10 (USB FS)*/
}

void platform_configure_usb(void) {
  printf("Initialize USB\r\n");
  NVIC_EnableIRQ(OTG_FS_IRQn);
  configure_usb_gpios();
  usb_core_init();
  usb_periph_init();
}
