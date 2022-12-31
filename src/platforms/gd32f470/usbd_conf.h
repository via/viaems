#ifndef __USBD_CONF_H
#define __USBD_CONF_H

#include "usb_conf.h"
#define USBD_CFG_MAX_NUM 1
#define USBD_ITF_MAX_NUM 1

#define CDC_COM_INTERFACE 0

#define USB_STR_DESC_MAX_SIZE 255

#define CDC_DATA_IN_EP EP1_IN   /* EP1 for data IN */
#define CDC_DATA_OUT_EP EP3_OUT /* EP3 for data OUT */
#define CDC_CMD_EP EP2_IN       /* EP2 for CDC commands */

#define USB_STRING_COUNT 4

#define USB_CDC_CMD_PACKET_SIZE 8   /* Control Endpoint Packet size */
#define USB_CDC_DATA_PACKET_SIZE 64 /* Endpoint IN & OUT Packet size */

#endif /* __USBD_CONF_H */
