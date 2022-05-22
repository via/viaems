#include "device.h"

/* Query a MAX11632's 16 inputs in a pattern described below that allows all
 * inputs to sample at 10 kHz, and 2 inputs at 40 Khz for knock detection.  This
 * is achieved with pattern of 24 total commands.  We sample at 240 kHz, which
 * is achievable with a 4.8 MHz SPI clock.
 *
 * TIM15 is configured to 240 kHz, and drives the DMA to move the commands to
 * the SPI TX register.  It is configured to accept a 9.6 MHz clock from the RCC
 * via PLL1Q, prescaled to 4.8 MHz.  Hardware management of the slave select is
 * used, allowing the CS to go high between commands.
 *
 * SPI RX drives a dma to a double buffered region, and the DMA's completion
 * drives the sensor conversion interrupt */

/* Configure TIM15 as a trigger source for DMA to/from the SPI */
static void setup_tim15(void) {
  RCC->APB2ENR |= RCC_APB2ENR_TIM15EN;

  TIM15->CR1 = TIM15_CR1_URS_VAL(1); /* overflow generates DMA */

  TIM15->ARR = 833; /* APB2 timer clock is 200 MHz, divide to get approx 240 kHz */

  TIM15->DIER = TIM15_DIER_UDE; /* DMA on update */
  TIM15->CR1 |= TIM15_CR1_CEN; /* Enable clock */
}

static void setup_spi1(void) {
  RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

  /* A4, A5, A6, A7 set to AF5 */
  GPIOA->MODER &= ~(GPIOA_MODER_MODE4 | 
                    GPIOA_MODER_MODE5 | 
                    GPIOA_MODER_MODE6 | 
                    GPIOA_MODER_MODE7);
  GPIOA->MODER |= GPIOA_MODER_MODE4_VAL(2) | 
                  GPIOA_MODER_MODE5_VAL(2) | 
                  GPIOA_MODER_MODE6_VAL(2) | 
                  GPIOA_MODER_MODE7_VAL(2);
  GPIOA->AFRL |= GPIOA_AFRL_AFSEL4_VAL(5) |
                 GPIOA_AFRL_AFSEL5_VAL(5) | 
                 GPIOA_AFRL_AFSEL6_VAL(5) | 
                 GPIOA_AFRL_AFSEL7_VAL(5);

  SPI1->CFG1 = SPI1_CFG1_MBR_VAL(0) | /* Clk / 2 = 4 MHz*/
               SPI1_CFG1_DSIZE_VAL(15) | /* 16 bit transfers */
               SPI1_CFG1_RXDMAEN; /* RX DMA */
  SPI1->CFG2 = SPI1_CFG2_CPHA_VAL(0) | /* First clock transition is first capture edge */
               SPI1_CFG2_MASTER |
               SPI1_CFG2_SSOE |
               SPI1_CFG2_SSOM;

  SPI1->CR1 = SPI1_CR1_SSI;
  SPI1->CR1 |= SPI1_CR1_SPE;
  SPI1->CR1 |= SPI1_CR1_CSTART;
}

#define NUM_SPI_TX 24
#define SPI_SETUP 0x7400 /* Clock mode 11, external reference */
#define SPI_INPUT(X) (0x86 | ((X) << 3)) /* No scan */
/* Query the 16 inputs in a pattern that causes input 0 and 1 to get sampled at
 * 4x the rate of the rest, over a total of 24 spi commands.  The first command
 * sets the setup register */
static const uint16_t max11632_transmit_sequence[NUM_SPI_TX] = {
    SPI_SETUP,     SPI_INPUT(2),  SPI_INPUT(3),  SPI_INPUT(4),  SPI_INPUT(0), SPI_INPUT(1),
    SPI_INPUT(5),  SPI_INPUT(6),  SPI_INPUT(7),  SPI_INPUT(8),  SPI_INPUT(0), SPI_INPUT(1),
    SPI_INPUT(9),  SPI_INPUT(10), SPI_INPUT(11), SPI_INPUT(12), SPI_INPUT(0), SPI_INPUT(1),
    SPI_INPUT(13), SPI_INPUT(14), SPI_INPUT(15), 0x0000,        SPI_INPUT(0), SPI_INPUT(1),
  };

static void setup_spi1_tx_dma(void) {
  /* TX DMA uses DMA1 stream 1 */
  DMA1->S1CR = 0;
  DMA1->S1PAR = (uint32_t)&SPI1->TXDR; /* Peripheral address is SPI Tx Data */
  DMA1->S1M0AR = (uint32_t)&max11632_transmit_sequence;
  DMA1->S1NDTR = NUM_SPI_TX;

  DMA1->S1CR = DMA1_S1CR_CIRC | /* Circular Transmit */
               DMA1_S1CR_PL_VAL(2) | /* Medium Priority */
               DMA1_S1CR_MSIZE_VAL(1) | /* 16 bit memory size */
               DMA1_S1CR_PSIZE_VAL(1) | /* 16 bit peripheral size */
               DMA1_S1CR_MINC | /* Memory increment after each transfer */
               DMA1_S1CR_DIR_VAL(1); /* Direction is memory to peripheral */

  /* Configure DMAMUX */
  DMAMUX1->C1CR &= ~(DMAMUX1_C1CR_DMAREQ_ID);
  DMAMUX1->C1CR |= DMAMUX1_C1CR_DMAREQ_ID_VAL(106); /* TIM15 Update */

  DMA1->S1CR |= DMA1_S1CR_EN; /* Enable */
}

static uint16_t __attribute__((aligned(4)))
                __attribute__((section(".dmadata"))) 
                spi_rx_buffer[2][NUM_SPI_TX];

static void setup_spi1_rx_dma(void) {
  /* TX DMA uses DMA1 stream 2 */
  DMA1->S2CR = 0;
  DMA1->S2PAR = (uint32_t)&SPI1->RXDR; /* Peripheral address is SPI Tx Data */
  DMA1->S2M0AR = (uint32_t)&spi_rx_buffer[0][0];
  DMA1->S2M1AR = (uint32_t)&spi_rx_buffer[1][0];
  DMA1->S2NDTR = NUM_SPI_TX;

  DMA1->S2CR = DMA1_S2CR_DBM | /* Circular Transmit */
               DMA1_S2CR_PL_VAL(2) | /* Medium Priority */
               DMA1_S2CR_MSIZE_VAL(1) | /* 16 bit memory size */
               DMA1_S2CR_PSIZE_VAL(1) | /* 16 bit peripheral size */
               DMA1_S2CR_MINC | /* Memory increment after each transfer */
               DMA1_S2CR_DIR_VAL(0); /* Direction is periph(SPI) to memory */

  /* Configure DMAMUX */
  DMAMUX1->C2CR &= ~(DMAMUX1_C2CR_DMAREQ_ID);
  DMAMUX1->C2CR |= DMAMUX1_C2CR_DMAREQ_ID_VAL(37); /* SPI1 Rx Update */

  DMA1->S2CR |= DMA1_S2CR_EN; /* Enable */
}

void configure_sensor_inputs(void) {
  setup_spi1();
  setup_tim15();
  setup_spi1_rx_dma();
  setup_spi1_tx_dma();
}
