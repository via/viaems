#include "config.h"
#include "sensors.h"

#include "gd32f4xx.h"

#ifdef SPI_MAX11632
#include "max11632_adc.h"
#define SPI_FREQ_DIVIDER SPI_PSC_32 /* 3.75 MHz */
#define SPI_SAMPLE_RATE 192000
#elif SPI_TLV2553
#include "tlv2553_adc.h"
#define SPI_FREQ_DIVIDER SPI_PSC_16 /* 7.5 MHz */
#define SPI_SAMPLE_RATE 150000
#else
#error No ADC specified!
#endif


/* Query a MAX11632 ADC on SPI0 at a fixed sampling rate of 192 KHz and a SPI
 * clock of 3.75 MHz.  SPI0 is on APB2 with a clock frequency of 120 MHz.
 * Prescale down to 3.75 (divide by 32) to keep under the maximum clock
 * frequency of the MAX11632.
 */

/* Configure TIMER0 to update at 192 KHz and generate a DMA request. We use
 * TIMER0's output compare functionality to drive CS high and low once per
 * sample. */

static void setup_timer0(void) {
  gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_8);
  gpio_af_set(GPIOA, GPIO_AF_1, GPIO_PIN_8);

  /* TIMER0 is on APB2's CK_TIMER, driven at 240 MHz (2x APB2's clock). Divide
   * by 1250 */
  const uint32_t period = 240000000 / SPI_SAMPLE_RATE;
  TIMER_CAR(TIMER0) = period - 1;

  TIMER_DMAINTEN(TIMER0) = TIMER_DMAINTEN_UPDEN;

  /* Configure TIMER0's CH0 as a PWM output to act as CS.  On each cycle start
     low and after the duty% is reached, go high */
  TIMER_CH0CV(TIMER0) = 0.9f * period;
  TIMER_CHCTL0(TIMER0) = TIMER_OC_MODE_PWM1 | TIMER_CHCTL0_CH0COMSEN;
  TIMER_CHCTL2(TIMER0) = TIMER_CHCTL2_CH0EN;
  TIMER_CCHP(TIMER0) = TIMER_CCHP_POEN | TIMER_CCHP_ROS;

  TIMER_CTL0(TIMER0) = TIMER_CTL0_CEN;

  DBG_CTL1 |= DBG_CTL2_TIMER0_HOLD;
}

/* Configure SPI0 for a SPI clock of 3.75 MHz in master mode with hardware NSS
 */
static void setup_spi0(void) {

  const uint32_t spi_pins = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
  gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, spi_pins);
  gpio_af_set(GPIOA, GPIO_AF_5, spi_pins);

  GPIO_OSPD(GPIOA) |= GPIO_OSPEED_SET(5, GPIO_OSPEED_25MHZ) |
                      GPIO_OSPEED_SET(6, GPIO_OSPEED_25MHZ) |
                      GPIO_OSPEED_SET(7, GPIO_OSPEED_25MHZ) |
                      GPIO_OSPEED_SET(8, GPIO_OSPEED_25MHZ);

  SPI_CTL0(SPI0) = SPI_CTL0_FF16 |  /* 16 bit data format */
                   SPI_FREQ_DIVIDER |     /* Divide to 3.75 MHz */
                   SPI_CTL0_MSTMOD; /* Master Mode */

  SPI_CTL1(SPI0) = SPI_CTL1_NSSDRV | /* Manage NSS Output */
                   SPI_CTL1_DMAREN;  /* Enable RX DMA */

  SPI_CTL0(SPI0) |= SPI_CTL0_SPIEN; /* Enable */
}

/* Use TIMER0 to trigger the DMA to write SPI0.  Use circular mode to transmit
 * the SPI commands.  TIMER0_UP can be driven with DMA1, Channel 5 Periph 6. */
void setup_spi0_tx_dma(void) {
  DMA_CH5PADDR(DMA1) = (uint32_t)&SPI_DATA(SPI0);
  DMA_CH5M0ADDR(DMA1) = (uint32_t)&adc_transmit_sequence[0];
  DMA_CH5CNT(DMA1) = NUM_SPI_TX;

  DMA_CH5CTL(DMA1) = DMA_PERIPH_6_SELECT | DMA_MEMORY_WIDTH_16BIT |
                     DMA_PERIPH_WIDTH_16BIT | DMA_MEMORY_TO_PERIPH |
                     DMA_CHXCTL_MNAGA | DMA_CHXCTL_CMEN | DMA_CHXCTL_CHEN;
}

static volatile uint16_t __attribute__((aligned(4)))
__attribute__((section(".dmadata"))) spi_rx_buffer[2][NUM_SPI_TX] = { 0 };

/* DMA is driven by SPI's RX and transfers the SPI result into a double buffer
 * of results.  The interrupt is also used for the ADC calculations */
void setup_spi0_rx_dma(void) {

  /* Enable interrupt for dma completion */
  nvic_irq_enable(DMA1_Channel0_IRQn, 3, 0);

  DMA_CH0PADDR(DMA1) = (uint32_t)&SPI_DATA(SPI0);
  DMA_CH0M0ADDR(DMA1) = (uint32_t)&spi_rx_buffer[0][0];
  DMA_CH0M1ADDR(DMA1) = (uint32_t)&spi_rx_buffer[1][0];
  DMA_CH0CNT(DMA1) = NUM_SPI_TX;

  DMA_CH0CTL(DMA1) =
    DMA_PERIPH_3_SELECT | DMA_MEMORY_WIDTH_16BIT | DMA_PERIPH_WIDTH_16BIT |
    DMA_PERIPH_TO_MEMORY | DMA_CHXCTL_MNAGA | DMA_CHXCTL_SBMEN |
    DMA_CHXCTL_FTFIE | /* Enable interrupt on buffer completion */
    DMA_CHXCTL_CHEN;
}

void DMA1_Channel0_IRQHandler(void) {
  if (dma_interrupt_flag_get(DMA1, DMA_CH0, DMA_INT_FLAG_FTF) == SET) {
    dma_interrupt_flag_clear(DMA1, DMA_CH0, DMA_INT_FLAG_FTF);
    const uint16_t *sequence = (dma_using_memory_get(DMA1, DMA_CH0) == 0)
                                 ? (const uint16_t *)spi_rx_buffer[1]
                                 : (const uint16_t *)spi_rx_buffer[0];
    bool fault = !adc_response_is_valid(sequence);
    for (int i = 0; i < NUM_SENSORS; ++i) {
      if (config.sensors[i].source == SENSOR_ADC) {
        config.sensors[i].fault = fault ? FAULT_CONN : FAULT_NONE;
        config.sensors[i].raw_value =
          read_adc_pin(sequence, config.sensors[i].pin);
      }
    }
    sensors_process(SENSOR_ADC);
  }
}

void gd32f470_configure_adc(void) {
  setup_spi0();
  setup_spi0_tx_dma();
  setup_spi0_rx_dma();
  setup_timer0();
}
