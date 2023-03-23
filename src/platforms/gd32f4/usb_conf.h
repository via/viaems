#ifndef _USB_CONF_H
#define _USB_CONF_H

#include "gd32f4xx.h"
#define USB_FS_CORE

#define RX_FIFO_FS_SIZE 128
#define TX0_FIFO_FS_SIZE 64
#define TX1_FIFO_FS_SIZE 64
#define TX2_FIFO_FS_SIZE 64
#define TX3_FIFO_FS_SIZE 0

#define USBFS_SOF_OUTPUT 0
#define USBFS_LOW_POWER 0

#define USE_DEVICE_MODE

#define __ALIGN_BEGIN
#define __ALIGN_END

#endif
