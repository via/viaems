#include "gd32f4xx.h"
#include "platform.h"

void gd32f4xx_configure_sdcard(void) {

  const uint32_t spi_pins = GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
  gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, spi_pins);
  gpio_af_set(GPIOC, GPIO_AF_6, spi_pins);
  GPIO_OSPD(GPIOC) |= GPIO_OSPEED_SET(10, GPIO_OSPEED_MAX) |
                      GPIO_OSPEED_SET(11, GPIO_OSPEED_MAX) |
                      GPIO_OSPEED_SET(12, GPIO_OSPEED_MAX);

  gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_4);
  GPIO_OSPD(GPIOA) |= GPIO_OSPEED_SET(4, GPIO_OSPEED_MAX);

  sdcard_spi_chipselect(false);

  SPI_CTL0(SPI2) = SPI_PSC_128 |    /* APB2 (48 MHz) / 128 = 375 KHz */
                   SPI_CTL0_MSTMOD; /* Master Mode */

  SPI_CTL1(SPI2) = SPI_CTL1_NSSDRV | /* Manage NSS Output */
                   SPI_CTL1_DMAREN | /* Enable RX DMA */
                   SPI_CTL1_DMATEN;  /* Enable TX DMA */

  nvic_irq_enable(DMA0_Channel2_IRQn, 7, 0);
}

/* Use DMA0 CH2(0) for RX and DMA0 CH5(0) for TX */
void sdcard_spi_transaction(const uint8_t *tx, uint8_t *rx, size_t len) {

  DMA_CH2PADDR(DMA0) = (uint32_t)&SPI_DATA(SPI2);
  DMA_CH2M0ADDR(DMA0) = (uint32_t)&rx[0];
  DMA_CH2CNT(DMA0) = len;

  DMA_CH2CTL(DMA0) =
    DMA_PERIPH_0_SELECT | DMA_MEMORY_WIDTH_8BIT | DMA_PERIPH_WIDTH_8BIT |
    DMA_PERIPH_TO_MEMORY | DMA_CHXCTL_MNAGA |
    DMA_CHXCTL_FTFIE | /* Enable interrupt on buffer completion */
    DMA_CHXCTL_CHEN;

  DMA_CH5PADDR(DMA0) = (uint32_t)&SPI_DATA(SPI2);
  DMA_CH5M0ADDR(DMA0) = (uint32_t)&tx[0];
  DMA_CH5CNT(DMA0) = len;

  DMA_CH5CTL(DMA0) = DMA_PERIPH_0_SELECT | DMA_MEMORY_WIDTH_8BIT |
                     DMA_PERIPH_WIDTH_8BIT | DMA_MEMORY_TO_PERIPH |
                     DMA_CHXCTL_MNAGA | DMA_CHXCTL_CHEN;

  SPI_CTL0(SPI2) |= SPI_CTL0_SPIEN;

  /* Block until done */
  while (DMA_CH2CTL(DMA0) & DMA_CHXCTL_CHEN)
    ;

  SPI_CTL0(SPI2) &= ~SPI_CTL0_SPIEN; /* Disable DMA */

  /* Errata for gd32f450: manually clear completion flags */
  DMA_INTC1(DMA0) = (1 << 11) | (1 << 10);
}

void DMA0_Channel2_IRQHandler(void) {
  if (dma_interrupt_flag_get(DMA0, DMA_CH2, DMA_INT_FLAG_FTF) == SET) {
    dma_interrupt_flag_clear(DMA0, DMA_CH2, DMA_INT_FLAG_FTF);
  }
}

void sdcard_spi_chipselect(bool asserted) {
  if (asserted) {
    GPIO_OCTL(GPIOA) &= ~(1 << 4);
  } else {
    GPIO_OCTL(GPIOA) |= (1 << 4);
  }
}

void sdcard_spi_highspeed(bool speed) {
  if (speed) {
    SPI_CTL0(SPI2) =
      SPI_CTL0_MSTMOD | SPI_PSC_8; /* APB2 (48 MHz) / 4 = 5.25 MHz */
  } else {
    SPI_CTL0(SPI2) =
      SPI_CTL0_MSTMOD | SPI_PSC_128; /* APB2 (48 MHz) / 4 = 375 KHz */
  }
}
