#include <libusb-1.0/libusb.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <unistd.h>

static const uint16_t USB_VID = 0x1209;
static const uint16_t USB_PID = 0x2041;
static const uint8_t USB_IN_EP = 0x81;
static const uint8_t USB_OUT_EP = 0x01;

static struct libusb_device_handle *devh = NULL;

static void read_callback(struct libusb_transfer *xfer) {
  const uint8_t *rxbuf = xfer->buffer;
  const size_t length = xfer->actual_length;
  write(STDOUT_FILENO, rxbuf, length);

  libusb_fill_bulk_transfer(xfer,
                            devh,
                            USB_IN_EP,
                            xfer->buffer,
                            xfer->length,
                            read_callback,
                            NULL,
                            1000);
  libusb_submit_transfer(xfer);
}

static int tx_loop(void *ptr) {
  (void)ptr;

  uint8_t rxbuf[512];
  do {
    ssize_t len = read(STDIN_FILENO, rxbuf, sizeof(rxbuf));
    if (len == 0) {
      continue;
    }
    int actual_length;
    while (
      libusb_bulk_transfer(devh, USB_OUT_EP, rxbuf, len, &actual_length, 0) < 0)
      ;
  } while (true);
  return 0;
}

int main(void) {

  int rc = libusb_init(NULL);
  if (rc < 0) {
    perror("libusb_init");
    return EXIT_FAILURE;
  }

  devh = libusb_open_device_with_vid_pid(NULL, USB_VID, USB_PID);
  if (!devh) {
    perror("libusb_open_device_with_vid_pid");
    return EXIT_FAILURE;
  }

  for (int if_num = 0; if_num < 2; if_num++) {
    if (libusb_kernel_driver_active(devh, if_num)) {
      libusb_detach_kernel_driver(devh, if_num);
    }
    rc = libusb_claim_interface(devh, if_num);
    if (rc < 0) {
      perror("libusb_claim_device");
      return EXIT_FAILURE;
    }
  }
  /* Start configuring the device:
   * - set line state
   */
  const uint32_t ACM_CTRL_DTR = 0x01;
  const uint32_t ACM_CTRL_RTS = 0x02;
  rc = libusb_control_transfer(
    devh, 0x21, 0x22, ACM_CTRL_DTR | ACM_CTRL_RTS, 0, NULL, 0, 0);
  if (rc < 0) {
    perror("libusb_control_transfer");
    return EXIT_FAILURE;
  }

  /* - set line encoding: here 9600 8N1
   * 9600 = 0x2580 ~> 0x80, 0x25 in little endian
   */
  unsigned char encoding[] = { 0x80, 0x25, 0x00, 0x00, 0x00, 0x00, 0x08 };
  rc = libusb_control_transfer(
    devh, 0x21, 0x20, 0, 0, encoding, sizeof(encoding), 0);
  if (rc < 0) {
    perror("libusb_control_transfer");
    return EXIT_FAILURE;
  }

  struct {
    struct libusb_transfer *xfer;
    uint8_t buffer[16384];
  } transfers[4];

  for (int i = 0; i < 4; i++) {
    transfers[i].xfer = libusb_alloc_transfer(0);
    if (!transfers[i].xfer) {
      return EXIT_FAILURE;
    }
    libusb_fill_bulk_transfer(transfers[i].xfer,
                              devh,
                              0x81,
                              transfers[i].buffer,
                              sizeof(transfers[i].buffer),
                              read_callback,
                              NULL,
                              1000);

    int rc = libusb_submit_transfer(transfers[i].xfer);
    if (rc != 0) {
      return EXIT_FAILURE;
    }
  }

  thrd_t tx_thread;
  thrd_create(&tx_thread, tx_loop, NULL);

  while (true) {
    libusb_handle_events(NULL);
  }

  return EXIT_SUCCESS;
}
