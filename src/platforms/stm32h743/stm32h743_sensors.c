#include "device.h"


/* Configure TIM15 as a trigger source for DMA to/from the SPI */
static void setup_tim15(void) {
  RCC->APB2ENR |= RCC_APB2ENR_TIM15EN;

  TIM15->CR1 = TIM15_CR1_URS_VAL(1); /* overflow generates DMA */

  TIM15->ARR = 1428; /* APB2 timer clock is 200 MHz, divide to get approx 140 kHz */

  TIM15->DIER = TIM15_DIER_UDE; /* Interrupt and DMA on update */
  TIM15->CR1 |= TIM15_CR1_CEN; /* Enable clock */
}

void setup_spi1(void) {
  RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

  /* A5, A6, A7 set to AF5 */
  GPIOA->MODER &= ~(GPIOA_MODER_MODE5 | GPIOA_MODER_MODE6 | GPIOA_MODER_MODE7);
  GPIOA->MODER |= GPIOA_MODER_MODE5_VAL(2) | GPIOA_MODER_MODE6_VAL(2) | GPIOA_MODER_MODE7_VAL(2);
  GPIOA->AFRL |= GPIOA_AFRL_AFSEL5_VAL(5) | GPIOA_AFRL_AFSEL6_VAL(5) | GPIOA_AFRL_AFSEL7_VAL(5);

  SPI1->CFG1 = SPI1_CFG1_MBR_VAL(0) | /* Clk / 2 = 4 MHz*/
               SPI1_CFG1_DSIZE_VAL(15); /* 16 bit transfers */
  SPI1->CFG2 = SPI1_CFG2_CPHA_VAL(0) | /* First clock transition is first capture edge */
               SPI1_CFG2_MASTER |
               SPI1_CFG2_SSOE |
               SPI1_CFG2_SSM; /* Use software slave management, and then
               explicitly set SSI high to operate as a single master */

  SPI1->CR1 = SPI1_CR1_SSI;
  SPI1->CR1 |= SPI1_CR1_SPE;
  SPI1->CR1 |= SPI1_CR1_CSTART;

  while(1) {
    SPI1->TXDR = 0x5A;
  }
}
