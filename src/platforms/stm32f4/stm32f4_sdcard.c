#include "stm32f427xx.h"
#include "platform.h"

void stm32f4xx_configure_sdcard(void) {

  /* Set up SPI3 AF on pins:
   * C10 - SCK
   * C11 - MISO
   * C12 - MOSI
   * Set up as regular GPIO:
   * A4 - CS
   */

  /* Set GPIOC 10-12 as AF6 */
  GPIOC->MODER |= _VAL2FLD(GPIO_MODER_MODE10, 2) |
                  _VAL2FLD(GPIO_MODER_MODE11, 2) |
                  _VAL2FLD(GPIO_MODER_MODE12, 2);

  GPIOC->AFR[1] |= _VAL2FLD(GPIO_AFRH_AFSEL10, 6) |
                   _VAL2FLD(GPIO_AFRH_AFSEL11, 6) |
                   _VAL2FLD(GPIO_AFRH_AFSEL12, 6);

  /* High speed */
  GPIOC->OSPEEDR |= _VAL2FLD(GPIO_OSPEEDR_OSPEED10, 3) |
                    _VAL2FLD(GPIO_OSPEEDR_OSPEED11, 3) |
                    _VAL2FLD(GPIO_OSPEEDR_OSPEED12, 3);

  GPIOC->PUPDR |= _VAL2FLD(GPIO_PUPDR_PUPD11, 1); // MISO Pull up

  // GPIO A4 as standard high speed gpio 
  GPIOA->OSPEEDR |= _VAL2FLD(GPIO_OSPEEDR_OSPEED4, 3);

  SPI3->CR1 = _FLD2VAL(SPI_CR1_BR, 6) | // Divide by 128
              SPI_CR1_MSTR;

  SPI3->CR2 = SPI_CR2_SSOE;

  SPI3->CR1 |= SPI_CR1_SPE;
}

uint8_t sdcard_spi_single(uint8_t tx) {
  SPI3->DR = tx;
  while (SPI3->SR & SPI_SR_BSY);
  return SPI3->DR;
}


#if 0
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
#endif
}

/* Use DMA0 CH2(0) for RX and DMA0 CH5(0) for TX */
static void sdcard_spi_transaction_dma(const uint8_t *tx, uint8_t *rx, size_t len) {

#if 0
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

  set_gpio(8, 1);
  SPI_CTL0(SPI2) |= SPI_CTL0_SPIEN;

  /* Block until done */
  while (DMA_CH2CTL(DMA0) & DMA_CHXCTL_CHEN)
    ;
  while (SPI_STAT(SPI2) & SPI_STAT_TRANS);

  SPI_CTL0(SPI2) &= ~SPI_CTL0_SPIEN; /* Disable DMA */
  set_gpio(8, 0);

  /* Errata for gd32f450: manually clear completion flags */
  DMA_INTC0(DMA0) = (1 << 21) | (1 << 20);
  DMA_INTC1(DMA0) = (1 << 11) | (1 << 10);
#endif
}

#if 0
void DMA0_Channel2_IRQHandler(void) {
  if (dma_interrupt_flag_get(DMA0, DMA_CH2, DMA_INT_FLAG_FTF) == SET) {
    dma_interrupt_flag_clear(DMA0, DMA_CH2, DMA_INT_FLAG_FTF);
  }
}
#endif

void sdcard_spi_chipselect(bool asserted) {
  if (asserted) {
    GPIOA->BSRR = (1u << (4 + 16));
  } else {
    GPIOA->BSRR = (1u << 4);
  }
}

void sdcard_spi_highspeed(bool speed) {
  if (speed) {
    SPI3->CR1 = (SPI3->CR1 & ~SPI_CR1_BR_Msk) | _FLD2VAL(SPI_CR1_BR, 1); // Div by 4
  } else {
    SPI3->CR1 = (SPI3->CR1 & ~SPI_CR1_BR_Msk) | _FLD2VAL(SPI_CR1_BR, 6); // Div by 128
  }
}
