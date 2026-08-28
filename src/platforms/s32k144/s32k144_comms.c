#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include <stdio.h>

#include "s32k1xx.h"

#include "spsc.h"

struct queue_entry {
  uint32_t control;
  uint8_t data[256];
} __attribute__((packed));

#define CONTROL_LEN(X) (((X) & 0xFF) << 24)  // Quantity of valid bytes - 1
#define CONTROL_TXVALID (1u << 9)            // Contents of this frame is valid
#define CONTROL_RXVALID (1u << 8)            // Contents received during this frame is accepted
#define CONTROL_PARITY (1u << 0)             // Parity bit for control word

static struct queue_entry null_rx_entry;
static struct queue_entry null_tx_entry;

static struct queue_entry txqes[4];
static struct queue_entry rxqes[4];

struct spsc_queue tx_queue = { .size = 4 };
struct spsc_queue rx_queue = { .size = 4 };

static void update_frame(void) {

  static struct queue_entry *txqe = &null_tx_entry;
  static struct queue_entry *rxqe = &null_tx_entry;

  bool slave_received = false;

  // If RX control word is valid, record if the slave received our last transmission, 
  // and push our RX payload
  if (__builtin_parity(rxqe->control) == 1) {
    slave_received = (rxqe->control & CONTROL_RXVALID) > 0;
    bool slave_sent = (rxqe->control & CONTROL_TXVALID) > 0;
    if (slave_sent && (rxqe != &null_rx_entry)) {
      spsc_push(&rx_queue);
      rxqe = &null_rx_entry;
    }
  }

  // Set up our next RX buffer
  if (rxqe == &null_rx_entry) {
    int32_t rx_idx = spsc_allocate(&rx_queue);
    if (rx_idx >= 0) {
      rxqe = &rxqes[rx_idx];
    }
  }

  // If we recorded that the slave received our last transmission, free it from the queue
  if ((txqe != &null_tx_entry) && slave_received) {
    spsc_release(&tx_queue);
    txqe = &null_tx_entry;
  }

  if (txqe == &null_tx_entry) {
    // Do we have a frame to transmit?
    int32_t tx_idx = spsc_next(&tx_queue);
    if (tx_idx >= 0) {
      txqe = &txqes[tx_idx];
    }
  }

  txqe->control &= CONTROL_LEN(255); // Reset all fields other than length

  if (txqe != &null_tx_entry) {
    txqe->control |= CONTROL_TXVALID;
  }

  if (rxqe != &null_rx_entry) {
    txqe->control |= CONTROL_RXVALID;
  }

  if (__builtin_parity(txqe->control) == 0) {
    txqe->control |= CONTROL_PARITY;
  }

  *DMA_TCD_SADDR(0) = (uint32_t)txqe;
  *DMA_TCD_DADDR(1) = (uint32_t)rxqe;
}

// Configure SPI1 to be a master at 12 Mhz.
// We send/receive SPI "frames" of 260 bytes (32 bit control + 256 byte data). 
// CS is kept active low during the frame, and set high between frames.
// Expects SPLLDIV2 (40 MHz) to be used as peripheral clock.
void configure_spi(void) {

  update_frame();

  // Configure DMA for TX
  *DMA_CR |= DMA_CR_EMLM | // Minor loop mapping
             DMA_CR_ERCA;  // Round Robin

  *DMA_TCD_SADDR(0) = (uint32_t)&null_tx_entry;
  *DMA_TCD_SOFF(0) = 4;
  *DMA_TCD_ATTR(0) = DMA_TCD_ATTR_SSIZE(2) | // 32 bit source
                     DMA_TCD_ATTR_DSIZE(2);  // 32 bit dest


  *DMA_TCD_NBYTES_MLOFFNO(0) = DMA_TCD_NBYTES_NBYTES(4);

  *DMA_TCD_SLAST(0) = 0;
  *DMA_TCD_DLASTSGA(0) = 0;

  *DMA_TCD_DADDR(0) = (uint32_t)LPSPI_TDR(1);
  *DMA_TCD_DOFF(0) = 0;
  *DMA_TCD_CITER_ELINKNO(0) = (256 + 4) / 4;
  *DMA_TCD_BITER_ELINKNO(0) = (256 + 4) / 4;

  *DMA_TCD_CSR(0) = (1u << 3);  // DREQ

  *DMAMUX_CHCFG(0) = DMAMUX_CHCFG_ENBL |
                    DMAMUX_CHCFG_SOURCE(17); // LPSPI1 TX


  *DMA_TCD_SADDR(1) = (uint32_t)LPSPI_RDR(1);
  *DMA_TCD_SOFF(1) = 0;
  *DMA_TCD_ATTR(1) = DMA_TCD_ATTR_SSIZE(2) | // 32 bit source
                     DMA_TCD_ATTR_DSIZE(2);  // 32 bit dest


  *DMA_TCD_NBYTES_MLOFFNO(1) = DMA_TCD_NBYTES_NBYTES(4);

  *DMA_TCD_SLAST(1) = 0;
  *DMA_TCD_DLASTSGA(1) = 0;

  *DMA_TCD_DADDR(1) = (uint32_t)&null_rx_entry;
  *DMA_TCD_DOFF(1) = 4; // Increment 4 bytes to dest
  *DMA_TCD_CITER_ELINKNO(1) = (256 + 4) / 4;
  *DMA_TCD_BITER_ELINKNO(1) = (256 + 4) / 4;

  *DMA_TCD_CSR(1) = (1u << 1) | // INTMAJOR
                    (1u << 3);  // DREQ

  *DMAMUX_CHCFG(1) = DMAMUX_CHCFG_ENBL |
                    DMAMUX_CHCFG_SOURCE(16); // LPSPI1 RX

  *NVIC_ISER((1/32)) = (1 << (1 & 0x1F)); // Enable NVIC 0 interrupt (DMA Ch0)

  *LPSPI_CFGR1(1) = 1; // MASTER MODE
  *LPSPI_CR(1) = 1; // MEN
  *LPSPI_DER(1) = 3; // TDDE and RDDE
  *LPSPI_TCR(1) = (1u << 30) | // CPHA
                  (1u << 27) | // 10 MHz
                  (1u << 22) | // Byte swap
                  (2079 << 0); // Frame size is 2080 bits (256 + 4 bytes)
  *LPSPI_CCR(1) = LPSPI_CCR_DBT(255) | LPSPI_CCR_PCSSCK(16) | LPSPI_CCR_SCKPCS(16);

  *DMA_ERQ = 3;
}


void DMA1_IRQHandler(void) {
  *DMA_INT = 2;
  update_frame();
  *DMA_ERQ = 3; // Enable CH1 and CH2
}


size_t platform_read(uint8_t *buffer, size_t max) {
  static int32_t cur_read_idx = -1;
  static uint32_t cur_pos = 0;

  uint32_t req_remaining = max;

  while (req_remaining > 0) {
    if (cur_read_idx < 0) {
      cur_read_idx = spsc_next(&rx_queue);
      cur_pos = 0;
      
      if (cur_read_idx < 0) {
        return (max - req_remaining);
      }
    }


    uint32_t len = (rxqes[cur_read_idx].control >> 24) + 1;
    uint32_t remaining = len - cur_pos;

    size_t read_amt = req_remaining > remaining ? remaining : req_remaining;

    size_t write_pos = max - req_remaining;
    memcpy(buffer + write_pos, rxqes[cur_read_idx].data + cur_pos, read_amt);

    remaining -= read_amt;
    req_remaining -= read_amt;

    if (remaining == 0) {
      spsc_release(&rx_queue);
      cur_read_idx = -1;
    } else {
      cur_pos += read_amt;
    }
  } 


  return (max - req_remaining);
}

size_t platform_write(const uint8_t *buffer, size_t length) {
  static int32_t cur_idx;
  static uint32_t cur_pos;

  if (length == 0) {
    return 0;
  }

  int32_t idx = spsc_allocate(&tx_queue);
  if (idx < 0) {
    return 0;
  }
  size_t amt = length > 256 ? 256 : length;

  memcpy(txqes[idx].data, buffer, amt);
  txqes[idx].control = CONTROL_LEN(amt - 1);

  spsc_push(&tx_queue);
  return amt;
}

