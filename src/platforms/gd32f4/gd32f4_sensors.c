#include "config.h"
#include "sensors.h"

#include "gd32f4xx.h"

#ifdef SPI_TLV2553
#include "tlv2553_adc.h"
#define SPI_FREQ_DIVIDER SPI_PSC_8 /* 12 MHz */
#define SPI_SAMPLE_RATE 150000
#define ADC_SAMPLE_RATE 5000
#define KNOCK_SAMPLE_RATE 50000
#else
#error No ADC specified!
#endif

static void setup_timer0(void) {
  gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_8);
  gpio_af_set(GPIOA, GPIO_AF_1, GPIO_PIN_8);

  /* TIMER0 is on APB2's CK_TIMER, driven at 192 MHz (2x APB2's clock) */
  const uint32_t period = 192000000 / SPI_SAMPLE_RATE;
  TIMER_CAR(TIMER0) = period - 1;

  TIMER_DMAINTEN(TIMER0) = TIMER_DMAINTEN_UPDEN;

  /* Configure TIMER0's CH0 as a PWM output to act as CS.  On each cycle start
     low and after the duty% is reached, go high */
  TIMER_CH0CV(TIMER0) = 0.9f * period;
  TIMER_CHCTL0(TIMER0) = TIMER_OC_MODE_PWM1 | TIMER_CHCTL0_CH0COMSEN;
  TIMER_CHCTL2(TIMER0) = TIMER_CHCTL2_CH0EN;
  TIMER_CCHP(TIMER0) = TIMER_CCHP_POEN | TIMER_CCHP_ROS;

  TIMER_CTL0(TIMER0) = TIMER_CTL0_CEN;

  DBG_CTL2 |= DBG_CTL2_TIMER0_HOLD;
}

static void setup_spi0(void) {

  const uint32_t spi_pins = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
  gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, spi_pins);
  gpio_af_set(GPIOA, GPIO_AF_5, spi_pins);

  GPIO_OSPD(GPIOA) |= GPIO_OSPEED_SET(5, GPIO_OSPEED_25MHZ) |
                      GPIO_OSPEED_SET(6, GPIO_OSPEED_25MHZ) |
                      GPIO_OSPEED_SET(7, GPIO_OSPEED_25MHZ) |
                      GPIO_OSPEED_SET(8, GPIO_OSPEED_25MHZ);

  SPI_CTL0(SPI0) = SPI_CTL0_FF16 |                     /* 16 bit data format */
                   SPI_FREQ_DIVIDER | SPI_CTL0_MSTMOD; /* Master Mode */

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
    struct adc_update update = {
      .time = current_time(),
      .valid = adc_response_is_valid(sequence),
    };

    for (int i = 0; i < MAX_ADC_PINS; ++i) {
      uint32_t adc_value = read_adc_pin(sequence, i);
      /* 5 volt ADC inputs */
      float raw_value = (float)(5 * adc_value) / 4096.0f;
      update.values[i] = raw_value;
    }
    sensor_update_adc(&update);
    process_knock_inputs(sequence);
  }
}

uint32_t platform_adc_samplerate(void) {
  return ADC_SAMPLE_RATE;
}

uint32_t platform_knock_samplerate(void) {
  return KNOCK_SAMPLE_RATE;
}

/* Configure TIMER8's CH0 and CH1 to provide both frequency and pulsewidth
 * on a single input, CH1 (PA3). Fire interrupt on update */
static void setup_freq_pw_input(void) {

  /* Enable the trigger/freq input GPIOs */
  gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_3);
  gpio_af_set(GPIOA, GPIO_AF_3, GPIO_PIN_3);

  /* Only have update interrupt fire on overflow, so we can have a timeout */
  TIMER_CTL0(TIMER8) = TIMER_CTL0_UPS;
  /* 192 MHz APB2 Timer clock divided by 2930 gives approximagely 1 Hz for
   * full 65536 wraparound */
  TIMER_PSC(TIMER8) = 2930;
  TIMER_CAR(TIMER8) = 65535;
  /* Interrupt on rising edge and overflow */
  TIMER_DMAINTEN(TIMER8) = TIMER_DMAINTEN_UPIE | TIMER_DMAINTEN_CH1IE;

  /* Restart counter on input 1 to compare channel 1 */
  /* Restart can only happen on input 0's output to capture 0 (which is not
   * connected) or input 1's connection to capture 1, so we need that to be the
   * rising edge for active high */
  TIMER_SMCFG(TIMER8) = TIMER_SLAVE_MODE_RESTART | TIMER_SMCFG_TRGSEL_CI1FE1;

  /* Configure both channels to capture the same input. Use highest filtering
   * option available: 8x at fDTS_CK/32 */
  TIMER_CHCTL0(TIMER8) =
    TIMER_CHCTL0_CH0CAPFLT | 0x2 | TIMER_CHCTL0_CH1CAPFLT | (0x01 << 8);
  TIMER_CHCTL2(TIMER8) = TIMER_CHCTL2_CH0EN | TIMER_CHCTL2_CH1EN;

  bool active_high = (config.freq_inputs[3].edge == RISING_EDGE);
  if (active_high) {
    TIMER_CHCTL2(TIMER8) |= TIMER_CHCTL2_CH0P; /* CH0 captures falling edge */
  } else {
    TIMER_CHCTL2(TIMER8) |= TIMER_CHCTL2_CH1P; /* CH1 captures falling edge */
  }

  nvic_irq_enable(TIMER0_BRK_TIMER8_IRQn, 14, 0);
  TIMER_CTL0(TIMER8) |= TIMER_CTL0_CEN;

  DBG_CTL2 |= DBG_CTL2_TIMER8_HOLD;
}

void TIMER0_BRK_TIMER8_IRQHandler(void) {
  if (TIMER_INTF(TIMER8) & TIMER_INTF_CH1IF) {
    uint32_t pulsewidth = TIMER_CH0CV(TIMER8);
    uint32_t period = TIMER_CH1CV(TIMER8);
    bool valid = (period != 0);
    struct freq_update update = {
      .time = current_time(),
      .valid = valid,
      .pin = 3,
      .frequency = valid ? (65536.0f / (float)period) : 0,
      .pulsewidth = (float)pulsewidth / 65536.0f,
    };
    sensor_update_freq(&update);
    /* TODO: handle successive 0-period results by disabling the input
     * completely to protect the main loop */
  }

  if (TIMER_INTF(TIMER8) & TIMER_INTF_UPIF) {
    TIMER_INTF(TIMER8) = ~TIMER_INTF_UPIF;
    struct freq_update update = {
      .time = current_time(),
      .valid = false,
      .pin = 3,
    };
    sensor_update_freq(&update);
  }
}

void gd32f4xx_configure_adc(void) {
  setup_spi0();
  setup_spi0_tx_dma();
  setup_spi0_rx_dma();
  setup_timer0();
  setup_freq_pw_input();

  knock_configure(&config.knock_inputs[0]);
  knock_configure(&config.knock_inputs[1]);
}
