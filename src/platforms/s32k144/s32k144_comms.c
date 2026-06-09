#include "s32k1xx.h"

struct tx_queue_entry {
  uint32_t control;
  uint8_t data[256];
} __attribute__((packed));

struct rx_queue_entry {
  uint8_t data[256];
  uint32_t control;
} __attribute__((packed));

static struct tx_queue_entry txqe;
static struct rx_queue_entry rxqe;

// Configure SPI1 to be a master at 12 Mhz.
// We send/receive SPI "frames" of 260 bytes (32 bit control + 256 byte data). 
// CS is kept active low during the frame, and set high between frames.
// Expects FIRCDIV2 (24 MHz) to be used as peripheral clock.
void configure_spi(void) {

  txqe.control = 0xAA5511AA;
  for (int i = 0; i < 256; i++) {
    txqe.data[i] = (i + 1);
  }

  // Configure DMA for TX
  *DMA_CR |= DMA_CR_EMLM | // Minor loop mapping
             DMA_CR_ERCA;  // Round Robin

  *DMA_TCD_SADDR(0) = (uint32_t)&txqe;
  *DMA_TCD_SOFF(0) = 4;
  *DMA_TCD_ATTR(0) = DMA_TCD_ATTR_SSIZE(2) | // 32 bit source
                     DMA_TCD_ATTR_DSIZE(2);  // 32 bit dest



  *DMA_TCD_NBYTES_MLOFFNO(0) = DMA_TCD_NBYTES_NBYTES(4);

//  *DMA_TCD_SLAST(0) = (uint32_t)(-800);

  *DMA_TCD_DADDR(0) = (uint32_t)LPSPI_TDR(1);
  *DMA_TCD_DOFF(0) = 0; // Increment 4 bytes to dest
  *DMA_TCD_CITER_ELINKNO(0) = (256 + 4) / 4;
//  *DMA_TCD_DLASTSGA(0) = (-8); // Minor loop offset ignored on last minor loop
//                               // so we still need a -8 offset
  *DMA_TCD_BITER_ELINKNO(0) = (256 + 4) / 4;

  *DMA_TCD_CSR(0) = (1u << 3);  // DREQ
  *DMA_ERQ = 1; // Enable CH0

  *DMAMUX_CHCFG(0) = DMAMUX_CHCFG_ENBL |
                    DMAMUX_CHCFG_SOURCE(17); // LPSPI1 TX

#if 0
  *DMAMUX_CHCFG(1) = DMAMUX_CHCFG_ENBL |
                    DMAMUX_CHCFG_SOURCE(16); // LPSPI1 RX
#endif

  *LPSPI_CFGR1(1) = 1; // MASTER MODE
  *LPSPI_CR(1) = 1; // MEN
  *LPSPI_DER(1) = 1; // TDDE
  *LPSPI_TCR(1) = (1u << 19) | // RX Mask (testing/temporary)
//                  (1u << 27) | // temp: 6 MHz
                  (1u << 22) | // Byte swap
                  (2079 << 0); // Frame size is 2080 bits (256 + 4 bytes)
}


void send_spi(void) {
  static uint32_t seq = 0;

  *LPSPI_TDR(1) = seq;

  seq += 1;
}
