
#include "platform.h"
#include "gd32f4xx.h"

void gd32f4xx_spi_transaction(const uint8_t *tx, uint8_t *rx, size_t len);

void gd32f4xx_configure_spi_flash(void) {

  const uint32_t spi_pins = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
  gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, spi_pins);
  gpio_af_set(GPIOB, GPIO_AF_5, spi_pins);

  GPIO_OSPD(GPIOB) |= GPIO_OSPEED_SET(12, GPIO_OSPEED_MAX) |
                      GPIO_OSPEED_SET(13, GPIO_OSPEED_MAX) |
                      GPIO_OSPEED_SET(14, GPIO_OSPEED_MAX) |
                      GPIO_OSPEED_SET(15, GPIO_OSPEED_MAX);

  SPI_CTL0(SPI1) = SPI_PSC_8 | /* APB2 (48 MHz) / 8 = 6 MHz */
                   SPI_CTL0_MSTMOD; /* Master Mode */

  SPI_CTL1(SPI1) = SPI_CTL1_NSSDRV | /* Manage NSS Output */
                   SPI_CTL1_DMAREN | /* Enable RX DMA */
                   SPI_CTL1_DMATEN;  /* Enable TX DMA */

  nvic_irq_enable(DMA0_Channel3_IRQn, 7, 0);

}


/* Use DMA0 CH3(0) for RX and DMA0 CH4(0) for TX */
void flash_spi_transaction(const uint8_t *tx, uint8_t *rx, size_t len) {

  DMA_CH3PADDR(DMA0) = (uint32_t)&SPI_DATA(SPI1);
  DMA_CH3M0ADDR(DMA0) = (uint32_t)&rx[0];
  DMA_CH3CNT(DMA0) = len;

  DMA_CH3CTL(DMA0) =
    DMA_PERIPH_0_SELECT | DMA_MEMORY_WIDTH_8BIT | DMA_PERIPH_WIDTH_8BIT |
    DMA_PERIPH_TO_MEMORY | DMA_CHXCTL_MNAGA | 
    DMA_CHXCTL_FTFIE | /* Enable interrupt on buffer completion */
    DMA_CHXCTL_CHEN;

  DMA_CH4PADDR(DMA0) = (uint32_t)&SPI_DATA(SPI1);
  DMA_CH4M0ADDR(DMA0) = (uint32_t)&tx[0];
  DMA_CH4CNT(DMA0) = len;

  DMA_CH4CTL(DMA0) = DMA_PERIPH_0_SELECT | DMA_MEMORY_WIDTH_8BIT |
                     DMA_PERIPH_WIDTH_8BIT | DMA_MEMORY_TO_PERIPH |
                     DMA_CHXCTL_MNAGA | DMA_CHXCTL_CHEN;

  SPI_CTL0(SPI1) |= SPI_CTL0_SPIEN;

  /* Block until done */
  while (DMA_CH3CTL(DMA0) & DMA_CHXCTL_CHEN);

  SPI_CTL0(SPI1) &= ~SPI_CTL0_SPIEN; /* Disable DMA */

  /* Errata for gd32f450: manually clear completion flags */
  DMA_INTC1(DMA0) = (1 << 5) | (1 << 4);

}

void DMA0_Channel3_IRQHandler(void) {
  if (dma_interrupt_flag_get(DMA0, DMA_CH3, DMA_INT_FLAG_FTF) == SET) {
    dma_interrupt_flag_clear(DMA0, DMA_CH3, DMA_INT_FLAG_FTF);
  }
}
