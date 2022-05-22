#include "device.h"
#include "sensors.h"
#include "config.h"

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
  TIM15->CR1 = TIM15_CR1_URS_VAL(1); /* overflow generates DMA */

  TIM15->ARR = 833; /* APB2 timer clock is 200 MHz, divide to get approx 240 kHz */

  TIM15->DIER = TIM15_DIER_UDE; /* DMA on update */
  TIM15->CR1 |= TIM15_CR1_CEN; /* Enable clock */
}

static void setup_spi1(void) {
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
#define SPI_INPUT(X) (0x8600 | ((X) << 11)) /* No scan */
/* Query the 16 inputs in a pattern that causes input 0 and 1 to get sampled at
 * 4x the rate of the rest, over a total of 24 spi commands.  The first command
 * sets the setup register. The order allows the last command to be a NOP, and
 * thus sample results don't cross buffer boundaries 
 * */
static const uint16_t max11632_transmit_sequence[NUM_SPI_TX] = {
    SPI_SETUP,     SPI_INPUT(2),  SPI_INPUT(3),  SPI_INPUT(0), SPI_INPUT(1), SPI_INPUT(4),  
    SPI_INPUT(5),  SPI_INPUT(6),  SPI_INPUT(7),  SPI_INPUT(0), SPI_INPUT(1), SPI_INPUT(8),  
    SPI_INPUT(9),  SPI_INPUT(10), SPI_INPUT(11), SPI_INPUT(0), SPI_INPUT(1), SPI_INPUT(12), 
    SPI_INPUT(13), SPI_INPUT(14), SPI_INPUT(15), SPI_INPUT(0), SPI_INPUT(1), 0x0000,        
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

static uint16_t read_raw_from_position(const uint16_t *values, int position) {
  uint16_t first = values[position];
  uint16_t second = values[position + 1];
  return ((first & 0x0F) << 8) | (second >> 8);
}

static uint16_t read_adc_pin(const uint16_t *values, int pin) {
  switch (pin) {
    case 2:
      return read_raw_from_position(values, 1);
    case 3:
      return read_raw_from_position(values, 2);
    case 4:
      return read_raw_from_position(values, 5);
    case 5:
      return read_raw_from_position(values, 6);
    case 6:
      return read_raw_from_position(values, 7);
    case 7:
      return read_raw_from_position(values, 8);
    case 8:
      return read_raw_from_position(values, 11);
    case 9:
      return read_raw_from_position(values, 12);
    case 10:
      return read_raw_from_position(values, 13);
    case 11:
      return read_raw_from_position(values, 14);
    case 12:
      return read_raw_from_position(values, 17);
    case 13:
      return read_raw_from_position(values, 18);
    case 14:
      return read_raw_from_position(values, 19);
    case 15:
      return read_raw_from_position(values, 20);
    default:
      return 0;
  }
}

void dma_str2_isr(void) {
  if ((DMA1->LISR & DMA1_LISR_TCIF2) == DMA1_LISR_TCIF2) {
    DMA1->LIFCR = DMA1_LIFCR_CTCIF2;
    const uint16_t *sequence = ((DMA1->S2CR & DMA1_S2CR_CT) == 0) ?
        spi_rx_buffer[1] : /* If current target is 0, read from 1 */
        spi_rx_buffer[0];

    for (int i = 0; i < NUM_SENSORS; ++i) {
      if (config.sensors[i].source == SENSOR_ADC) {
        config.sensors[i].raw_value = read_adc_pin(sequence, config.sensors[i].pin);
      }
    }
  }
  sensors_process(SENSOR_ADC);
}

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
               DMA1_S2CR_DIR_VAL(0) | /* Direction is periph(SPI) to memory */
               DMA1_S2CR_TCIE | /* Interrupt when each transfer complete */
               DMA1_S2CR_DMEIE; /* Interrupt on DMA error */

  /* Configure DMAMUX */
  DMAMUX1->C2CR &= ~(DMAMUX1_C2CR_DMAREQ_ID);
  DMAMUX1->C2CR |= DMAMUX1_C2CR_DMAREQ_ID_VAL(37); /* SPI1 Rx Update */

  nvic_enable_irq(DMA_STR2_IRQ);
  nvic_set_priority(DMA_STR2_IRQ, 4); 
  DMA1->S2CR |= DMA1_S2CR_EN; /* Enable */
}

void platform_configure_sensors(void) {
  setup_spi1();
  setup_tim15();
  setup_spi1_rx_dma();
  setup_spi1_tx_dma();
}
