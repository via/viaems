
/*
    Copyright (c) 2022, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from this
software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* This file is derived from the GD32F4xx Usb library, with modifications
  support the viaems usb use case */

#include <assert.h>
#include <stdatomic.h>
#include <string.h>

#include "gd32f4xx.h"
#include "platform.h"
#include "util.h"

#include "drv_usb_hw.h"
#include "drv_usbd_int.h"
#include "usb_cdc.h"
#include "usb_ch9_std.h"
#include "usbd_enum.h"

usb_core_driver cdc_acm;

#define USBD_VID 0x1209
#define USBD_PID 0x2041

struct cdc_state {
  _Atomic bool pending_write;
  _Atomic bool pending_read;
  uint8_t cmd[8];

  acm_line line_coding;
};

struct cdc_state state;

/* note:it should use the C99 standard when compiling the below codes */
/* USB standard device descriptor */
const usb_desc_dev cdc_dev_desc  =
{
    .header = 
     {
         .bLength          = USB_DEV_DESC_LEN, 
         .bDescriptorType  = USB_DESCTYPE_DEV,
     },
    .bcdUSB                = 0x0200U,
    .bDeviceClass          = USB_CLASS_CDC,
    .bDeviceSubClass       = 0x00U,
    .bDeviceProtocol       = 0x00U,
    .bMaxPacketSize0       = USB_FS_EP0_MAX_LEN,
    .idVendor              = USBD_VID,
    .idProduct             = USBD_PID,
    .bcdDevice             = 0x0100U,
    .iManufacturer         = STR_IDX_MFC,
    .iProduct              = STR_IDX_PRODUCT,
    .iSerialNumber         = STR_IDX_SERIAL,
    .bNumberConfigurations = USBD_CFG_MAX_NUM,
};

/* USB device configuration descriptor */
const usb_cdc_desc_config_set cdc_config_desc  = 
{
    .config = 
    {
        .header =
        {
            .bLength         = sizeof(usb_desc_config),
            .bDescriptorType = USB_DESCTYPE_CONFIG,
        },
        .wTotalLength         = USB_CDC_ACM_CONFIG_DESC_SIZE,
        .bNumInterfaces       = 0x02U,
        .bConfigurationValue  = 0x01U,
        .iConfiguration       = 0x00U,
        .bmAttributes         = 0x80U,
        .bMaxPower            = 0x32U
    },

    .cmd_itf = 
    {
        .header =
        {
            .bLength         = sizeof(usb_desc_itf),
            .bDescriptorType = USB_DESCTYPE_ITF
        },
        .bInterfaceNumber     = 0x00U,
        .bAlternateSetting    = 0x00U,
        .bNumEndpoints        = 0x01U,
        .bInterfaceClass      = USB_CLASS_CDC,
        .bInterfaceSubClass   = USB_CDC_SUBCLASS_ACM,
        .bInterfaceProtocol   = USB_CDC_PROTOCOL_AT,
        .iInterface           = 0x00U
    },

    .cdc_header =
    {
        .header =
        {
            .bLength         = sizeof(usb_desc_header_func),
            .bDescriptorType = USB_DESCTYPE_CS_INTERFACE
        },
        .bDescriptorSubtype  = 0x00U,
        .bcdCDC              = 0x0110U
    },

    .cdc_call_managment =
    {
        .header =
        {
            .bLength         = sizeof(usb_desc_call_managment_func),
            .bDescriptorType = USB_DESCTYPE_CS_INTERFACE
        },
        .bDescriptorSubtype  = 0x01U,
        .bmCapabilities      = 0x00U,
        .bDataInterface      = 0x01U
    },

    .cdc_acm =
    {
        .header =
        {
            .bLength         = sizeof(usb_desc_acm_func),
            .bDescriptorType = USB_DESCTYPE_CS_INTERFACE
        },
        .bDescriptorSubtype  = 0x02U,
        .bmCapabilities      = 0x02U,
    },

    .cdc_union =
    {
        .header =
        {
            .bLength         = sizeof(usb_desc_union_func),
            .bDescriptorType = USB_DESCTYPE_CS_INTERFACE
        },
        .bDescriptorSubtype  = 0x06U,
        .bMasterInterface    = 0x00U,
        .bSlaveInterface0    = 0x01U,
    },

    .cdc_cmd_endpoint =
    {
        .header =
        {
            .bLength         = sizeof(usb_desc_ep),
            .bDescriptorType = USB_DESCTYPE_EP,
        },
        .bEndpointAddress    = CDC_CMD_EP,
        .bmAttributes        = USB_EP_ATTR_INT,
        .wMaxPacketSize      = USB_CDC_CMD_PACKET_SIZE,
        .bInterval           = 0x0AU
    },

    .cdc_data_interface =
    {
        .header =
        {
            .bLength         = sizeof(usb_desc_itf),
            .bDescriptorType = USB_DESCTYPE_ITF,
        },
        .bInterfaceNumber    = 0x01U,
        .bAlternateSetting   = 0x00U,
        .bNumEndpoints       = 0x02U,
        .bInterfaceClass     = USB_CLASS_DATA,
        .bInterfaceSubClass  = 0x00U,
        .bInterfaceProtocol  = USB_CDC_PROTOCOL_NONE,
        .iInterface          = 0x00U
    },

    .cdc_out_endpoint =
    {
        .header =
        {
            .bLength         = sizeof(usb_desc_ep),
            .bDescriptorType = USB_DESCTYPE_EP,
        },
        .bEndpointAddress     = CDC_DATA_OUT_EP,
        .bmAttributes         = USB_EP_ATTR_BULK,
        .wMaxPacketSize       = USB_CDC_DATA_PACKET_SIZE,
        .bInterval            = 0x00U
    },

    .cdc_in_endpoint =
    {
        .header =
        {
            .bLength         = sizeof(usb_desc_ep),
            .bDescriptorType = USB_DESCTYPE_EP
        },
        .bEndpointAddress     = CDC_DATA_IN_EP,
        .bmAttributes         = USB_EP_ATTR_BULK,
        .wMaxPacketSize       = USB_CDC_DATA_PACKET_SIZE,
        .bInterval            = 0x00U
    }
};

/* USB language ID Descriptor */
static const usb_desc_LANGID usbd_language_id_desc = 
{
    .header =
    {
        .bLength         = sizeof(usb_desc_LANGID),
        .bDescriptorType = USB_DESCTYPE_STR,
    },
    .wLANGID              = ENG_LANGID
};

/* USB manufacture string */
static usb_desc_str manufacturer_string;

/* USB product string */
static usb_desc_str product_string;

/* USBD serial string */
static usb_desc_str serial_string;

/* USB string descriptor set */
void *const usbd_cdc_strings[] = {
  [STR_IDX_LANGID] = (uint8_t *)&usbd_language_id_desc,
  [STR_IDX_MFC] = (uint8_t *)&manufacturer_string,
  [STR_IDX_PRODUCT] = (uint8_t *)&product_string,
  [STR_IDX_SERIAL] = (uint8_t *)&serial_string
};

usb_desc cdc_desc = { .dev_desc = (uint8_t *)&cdc_dev_desc,
                      .config_desc = (uint8_t *)&cdc_config_desc,
                      .strings = usbd_cdc_strings };

/* local function prototypes ('static') */
static uint8_t cdc_acm_init(usb_dev *udev, uint8_t config_index);
static uint8_t cdc_acm_deinit(usb_dev *udev, uint8_t config_index);
static uint8_t cdc_acm_req(usb_dev *udev, usb_req *req);
static uint8_t cdc_acm_ctlx_out(usb_dev *udev);
static uint8_t cdc_acm_in(usb_dev *udev, uint8_t ep_num);
static uint8_t cdc_acm_out(usb_dev *udev, uint8_t ep_num);

/* USB CDC device class callbacks structure */
usb_class_core cdc_class = { .command = NO_CMD,
                             .alter_set = 0U,

                             .init = cdc_acm_init,
                             .deinit = cdc_acm_deinit,
                             .req_proc = cdc_acm_req,
                             .ctlx_out = cdc_acm_ctlx_out,
                             .data_in = cdc_acm_in,
                             .data_out = cdc_acm_out };

/*!
    \brief      initialize the CDC ACM device
    \param[in]  udev: pointer to USB device instance
    \param[in]  config_index: configuration index
    \param[out] none
    \retval     USB device operation status
*/
static uint8_t cdc_acm_init(usb_dev *udev, uint8_t config_index) {
  (void)config_index;
  /* initialize the data Tx endpoint */
  usbd_ep_setup(udev, &(cdc_config_desc.cdc_in_endpoint));

  /* initialize the data Rx endpoint */
  usbd_ep_setup(udev, &(cdc_config_desc.cdc_out_endpoint));

  /* initialize the command Tx endpoint */
  usbd_ep_setup(udev, &(cdc_config_desc.cdc_cmd_endpoint));

  /* initialize CDC handler structure */
  state.pending_read = false;
  state.pending_write = false;

  state.line_coding = (acm_line){ .dwDTERate = 115200U,
                                  .bCharFormat = 0U,
                                  .bParityType = 0U,
                                  .bDataBits = 0x08U };

  return USBD_OK;
}

/*!
    \brief      deinitialize the CDC ACM device
    \param[in]  udev: pointer to USB device instance
    \param[in]  config_index: configuration index
    \param[out] none
    \retval     USB device operation status
*/
static uint8_t cdc_acm_deinit(usb_dev *udev, uint8_t config_index) {
  (void)config_index;

  /* deinitialize the data Tx/Rx endpoint */
  usbd_ep_clear(udev, CDC_DATA_IN_EP);
  usbd_ep_clear(udev, CDC_DATA_OUT_EP);

  /* deinitialize the command Tx endpoint */
  usbd_ep_clear(udev, CDC_CMD_EP);

  return USBD_OK;
}

/*!
    \brief      handle the CDC ACM class-specific requests
    \param[in]  udev: pointer to USB device instance
    \param[in]  req: device class-specific request
    \param[out] none
    \retval     USB device operation status
*/
static uint8_t cdc_acm_req(usb_dev *udev, usb_req *req) {
  usb_transc *transc = NULL;

  switch (req->bRequest) {
  case SEND_ENCAPSULATED_COMMAND:
    /* no operation for this driver */
    break;

  case GET_ENCAPSULATED_RESPONSE:
    /* no operation for this driver */
    break;

  case SET_COMM_FEATURE:
    /* no operation for this driver */
    break;

  case GET_COMM_FEATURE:
    /* no operation for this driver */
    break;

  case CLEAR_COMM_FEATURE:
    /* no operation for this driver */
    break;

  case SET_LINE_CODING:
    transc = &udev->dev.transc_out[0];

    /* set the value of the current command to be processed */
    udev->dev.class_core->alter_set = req->bRequest;

    /* enable EP0 prepare to receive command data packet */
    transc->remain_len = req->wLength;
    transc->xfer_buf = state.cmd;
    break;

  case GET_LINE_CODING:
    transc = &udev->dev.transc_in[0];

    state.cmd[0] = (uint8_t)(state.line_coding.dwDTERate);
    state.cmd[1] = (uint8_t)(state.line_coding.dwDTERate >> 8);
    state.cmd[2] = (uint8_t)(state.line_coding.dwDTERate >> 16);
    state.cmd[3] = (uint8_t)(state.line_coding.dwDTERate >> 24);
    state.cmd[4] = state.line_coding.bCharFormat;
    state.cmd[5] = state.line_coding.bParityType;
    state.cmd[6] = state.line_coding.bDataBits;

    transc->xfer_buf = state.cmd;
    transc->remain_len = 7U;
    break;

  case SET_CONTROL_LINE_STATE:
    /* no operation for this driver */
    break;

  case SEND_BREAK:
    /* no operation for this driver */
    break;

  default:
    break;
  }

  return USBD_OK;
}

/*!
    \brief      command data received on control endpoint
    \param[in]  udev: pointer to USB device instance
    \param[out] none
    \retval     USB device operation status
*/
static uint8_t cdc_acm_ctlx_out(usb_dev *udev) {

  if (udev->dev.class_core->alter_set != NO_CMD) {
    /* process the command data */
    state.line_coding.dwDTERate =
      (uint32_t)((uint32_t)state.cmd[0] | ((uint32_t)state.cmd[1] << 8U) |
                 ((uint32_t)state.cmd[2] << 16U) |
                 ((uint32_t)state.cmd[3] << 24U));

    state.line_coding.bCharFormat = state.cmd[4];
    state.line_coding.bParityType = state.cmd[5];
    state.line_coding.bDataBits = state.cmd[6];

    udev->dev.class_core->alter_set = NO_CMD;
  }

  return USBD_OK;
}

/*!
    \brief      handle CDC ACM data in
    \param[in]  udev: pointer to USB device instance
    \param[in]  ep_num: endpoint identifier
    \param[out] none
    \retval     USB device operation status
*/
static uint8_t cdc_acm_in(usb_dev *udev, uint8_t ep_num) {
  usb_transc *transc = &udev->dev.transc_in[EP_ID(ep_num)];

  if ((0U == transc->xfer_len % transc->max_len) && (0U != transc->xfer_len)) {
    usbd_ep_send(udev, ep_num, NULL, 0U);
  } else {
    state.pending_write = false;
  }

  return USBD_OK;
}

static uint8_t rxbuffer[64];
static uint32_t rx_len = 0;
/*!
    \brief      handle CDC ACM data out
    \param[in]  udev: pointer to USB device instance
    \param[in]  ep_num: endpoint identifier
    \param[out] none
    \retval     USB device operation status
*/
static uint8_t cdc_acm_out(usb_dev *udev, uint8_t ep_num) {
  rx_len = ((usb_core_driver *)udev)->dev.transc_out[ep_num].xfer_count;
  state.pending_read = true;
  return USBD_OK;
}

size_t platform_stream_read(size_t max, uint8_t ptr[max]) {

  if (state.pending_read) {
    disable_interrupts();
    assert(rx_len <= max);
    memcpy(ptr, rxbuffer, rx_len);
    state.pending_read = false;
    enable_interrupts();
    return rx_len;
  }

  if (max >= sizeof(rxbuffer)) {
    usbd_ep_recev(&cdc_acm, CDC_DATA_OUT_EP, rxbuffer, sizeof(rxbuffer));
  }
  return 0;
}

size_t platform_stream_write(size_t max, const uint8_t ptr[max]) {
  if (state.pending_write) {
    return 0;
  }

  disable_interrupts();
  state.pending_write = true;
  usbd_ep_send(&cdc_acm, CDC_DATA_IN_EP, ptr, max);
  enable_interrupts();

  return max;
}

void usb_udelay(uint32_t d) {
  timeval_t before = current_time();
  while ((current_time() - before) < time_from_us(d))
    ;
}

void usb_mdelay(uint32_t d) {
  usb_udelay(d * 1000);
}

void USBFS_IRQHandler(void) {
  usbd_isr(&cdc_acm);
}

static void usb_initialize_string(usb_desc_str *desc, const char *str) {
  desc->header.bDescriptorType = USB_DESCTYPE_STR;
  size_t len = 0;
  while (str[len] != 0) {
    desc->unicode_string[len] = str[len];
    len++;
  }
  desc->header.bLength = USB_STRING_LEN(len);
}

void gd32f4xx_console_init() {
  rcu_pll48m_clock_config(RCU_PLL48MSRC_PLLQ);
  rcu_ck48m_clock_config(RCU_CK48MSRC_PLL48M);

  /* USBFS_DM(PA11) and USBFS_DP(PA12) GPIO pin configuration */
  gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_11 | GPIO_PIN_12);
  gpio_output_options_set(
    GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_11 | GPIO_PIN_12);

  gpio_af_set(GPIOA, GPIO_AF_10, GPIO_PIN_11 | GPIO_PIN_12);

  nvic_irq_enable(USBFS_IRQn, 4, 0);

  usb_initialize_string(cdc_desc.strings[STR_IDX_MFC],
                        "https://github.com/via/viaems");
  usb_initialize_string(cdc_desc.strings[STR_IDX_PRODUCT], "ViaEMS Console");
  usb_initialize_string(cdc_desc.strings[STR_IDX_SERIAL], "0");

  usbd_init(&cdc_acm, USB_CORE_ENUM_FS, &cdc_desc, &cdc_class);
}
