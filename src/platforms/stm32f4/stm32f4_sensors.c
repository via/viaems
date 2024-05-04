#include "config.h"
#include "sensors.h"

#include "stm32f427xx.h"

#ifdef SPI_TLV2553
#include "tlv2553_adc.h"
#define SPI_FREQ_DIVIDER 0x2 /* Prescaler = 8, 10.5 MHz */
#define SPI_SAMPLE_RATE 150000
#define ADC_SAMPLE_RATE 5000
#define KNOCK_SAMPLE_RATE 50000
#elif SPI_AD7888
#include "ad7888_adc.h"
#define SPI_FREQ_DIVIDER 0x5 /* Prescaler 64,  1.3125 MHz */
#define SPI_SAMPLE_RATE 70000
#define ADC_SAMPLE_RATE 5000
#define KNOCK_SAMPLE_RATE -1 /* No knock inputs */
#else
#error No ADC specified!
#endif

/* Configure TIM1 to cycle at the SPI_SAMPLE_RATE.  It uses compare outputs to
 * manage CS, and on update it'll trigger DMA to send a SPI word.  DMA is also
 * configured to receive the resulting SPI response and place it into a double
 * circular buffer.  After each full set of SPI commands, an interrupt is fired,
 * and ADC processing is triggered */

static void setup_tim1(void) {
  GPIOA->MODER |= _VAL2FLD(GPIO_MODER_MODE8, 2);       /* Pin 8 AF */
  GPIOA->AFR[1] |= _VAL2FLD(GPIO_AFRH_AFSEL8, 1);      /* AF1 */
  GPIOA->OSPEEDR |= _VAL2FLD(GPIO_OSPEEDR_OSPEED8, 1); /* 25 MHz */

  /* TIM1 is on APB2's CK_TIMER, driven at 168 MHz (2x APB2's clock) */
  const uint32_t period = 168000000 / SPI_SAMPLE_RATE;
  TIM1->ARR = period - 1;

  TIM1->DIER = TIM_DIER_UDE;

  /* Configure TIMER0's CH0 as a PWM output to act as CS.  On each cycle start
     low and after the duty% is reached, go high */
  TIM1->CCR1 = 0.9f * period;
  TIM1->CCMR1 = _VAL2FLD(TIM_CCMR1_OC1M, 0x7); /* PWM Mode 2 */
  TIM1->CCER = TIM_CCER_CC1E;                  /* Enable compare output 1 */
  TIM1->BDTR = TIM_BDTR_MOE | TIM_BDTR_OSSR;   /* Enable outputs */
  TIM1->CR1 = TIM_CR1_CEN;                     /* Start timer */

  DBGMCU->APB2FZ |= DBGMCU_APB2_FZ_DBG_TIM1_STOP;
}

static void setup_spi1(void) {

  /* Set pins 5, 6, 7 to AF mode */
  GPIOA->MODER |= _VAL2FLD(GPIO_MODER_MODE5, 2) |
                  _VAL2FLD(GPIO_MODER_MODE6, 2) | _VAL2FLD(GPIO_MODER_MODE7, 2);
  /* Set pins 5, 6, 7 to AF 5, SPI1 */
  GPIOA->AFR[0] |= _VAL2FLD(GPIO_AFRL_AFSEL5, 5) |
                   _VAL2FLD(GPIO_AFRL_AFSEL6, 5) |
                   _VAL2FLD(GPIO_AFRL_AFSEL7, 5);

  /* Set pins 5, 6, 7 to Speed 1, 25 MHz max */
  GPIOA->OSPEEDR |= _VAL2FLD(GPIO_OSPEEDR_OSPEED5, 1) |
                    _VAL2FLD(GPIO_OSPEEDR_OSPEED6, 1) |
                    _VAL2FLD(GPIO_OSPEEDR_OSPEED7, 1);

  SPI1->CR1 = _VAL2FLD(SPI_CR1_DFF, 1) | /* 16 bit frame */
              SPI_CR1_MSTR |             /* Master mode */
              _VAL2FLD(SPI_CR1_BR, SPI_FREQ_DIVIDER);

  SPI1->CR2 = SPI_CR2_SSOE |   /* Always master */
              SPI_CR2_RXDMAEN; /* RX DMA */

  SPI1->CR1 |= SPI_CR1_SPE; /* Start SPI */
}

/* Use TIM1 to trigger the DMA to write SPI1.  Use circular mode to transmit
 * the SPI commands. */
void setup_spi1_tx_dma(void) {

  /* Use DMA2 Stream 5 Channel 6 (TIM1_UP) to write to SPI1_DR */
  DMA2_Stream5->PAR = (uint32_t)&SPI1->DR;
  DMA2_Stream5->M0AR = (uint32_t)&adc_transmit_sequence[0];
  DMA2_Stream5->NDTR = NUM_SPI_TX;

  DMA2_Stream5->CR =
    _VAL2FLD(DMA_SxCR_CHSEL, 6) |   /* TIM1_UP */
    DMA_SxCR_CIRC |                 /* Circular mode */
    _VAL2FLD(DMA_SxCR_PL, 3) |      /* High priority */
    _VAL2FLD(DMA_SxCR_MSIZE, 0x1) | /* 16 bit memory size */
    _VAL2FLD(DMA_SxCR_PSIZE, 0x1) | /* 16 bit peripheral size */
    DMA_SxCR_MINC |                 /* Memory-increment */
    _VAL2FLD(DMA_SxCR_DIR, 0x1) |   /* Memory -> peripheral */
    DMA_SxCR_EN;                    /* Enable DMA */
}

static volatile uint16_t __attribute__((aligned(4)))
__attribute__((section(".dmadata"))) spi_rx_buffer[2][NUM_SPI_TX] = { 0 };

/* DMA is driven by SPI's RX and transfers the SPI result into a double buffer
 * of results.  The interrupt is also used for the ADC calculations */
void setup_spi1_rx_dma(void) {

  /* Enable interrupt for dma completion */
  NVIC_SetPriority(DMA2_Stream0_IRQn, 10);
  NVIC_EnableIRQ(DMA2_Stream0_IRQn);

  /* Use DMA2 Stream 0 Channel 3 (SPI1_RX) to read SPI_DR into the receive
   * buffer */
  DMA2_Stream0->PAR = (uint32_t)&SPI1->DR;
  DMA2_Stream0->M0AR = (uint32_t)&spi_rx_buffer[0][0];
  DMA2_Stream0->M1AR = (uint32_t)&spi_rx_buffer[1][0];
  DMA2_Stream0->NDTR = NUM_SPI_TX;

  DMA2_Stream0->CR =
    _VAL2FLD(DMA_SxCR_CHSEL, 3) |   /* SPI1_RX */
    DMA_SxCR_DBM |                  /* double buffer mode */
    _VAL2FLD(DMA_SxCR_PL, 2) |      /* medium priority */
    _VAL2FLD(DMA_SxCR_MSIZE, 0x1) | /* 16 bit memory size */
    _VAL2FLD(DMA_SxCR_PSIZE, 0x1) | /* 16 bit peripheral size */
    DMA_SxCR_MINC |                 /* Memory-increment */
    _VAL2FLD(DMA_SxCR_DIR, 0x0) |   /* peripheral -> memory*/
    DMA_SxCR_TCIE |                 /* completion interrupt enabled */
    DMA_SxCR_EN;                    /* Enable DMA */
}

void DMA2_Stream0_IRQHandler(void) {
  if (DMA2->LISR & DMA_LISR_TCIF0) {
    DMA2->LIFCR = DMA_LIFCR_CTCIF0;
    uint32_t dma_current_buffer = _FLD2VAL(DMA_SxCR_CT, DMA2_Stream0->CR);
    const uint16_t *sequence = (dma_current_buffer)
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
    set_gpio(1, 1);
    publish_raw_adc(&update);
    set_gpio(1, 0);
    process_knock_inputs(sequence);
  }
}

uint32_t platform_adc_samplerate(void) {
  return ADC_SAMPLE_RATE;
}

uint32_t platform_knock_samplerate(void) {
  return KNOCK_SAMPLE_RATE;
}

/* Configure TIM9's CH1 and CH1 to provide both frequency and pulsewidth
 * on a single input, CH2 (PA3). Fire interrupt on update */
static void setup_freq_pw_input(void) {
  GPIOA->MODER |= _VAL2FLD(GPIO_MODER_MODE3, 2);  /* GPIO Mode AF */
  GPIOA->AFR[0] |= _VAL2FLD(GPIO_AFRL_AFSEL3, 3); /* AF3 (TIM9) */

  /* Only have update interrupt fire on overflow, so we can have a timeout */
  TIM9->CR1 = TIM_CR1_URS;

  /* 168 MHz APB2 Timer clock divided by 2563 gives approximagely 1 Hz for
   * full 65536 wraparound */
  TIM9->PSC = 2563;
  TIM9->ARR = 65535;
  /* Interrupt on rising edge and overflow */
  TIM9->DIER = TIM_DIER_UIE | TIM_DIER_CC2IE;

  /* Restart counter on input 2 to compare channel 2 */
  /* Restart can only happen on input 1's output to capture 1 (which is not
   * connected) or input 2's connection to capture 2, so we need that to be the
   * rising edge for active high */
  TIM9->SMCR = _VAL2FLD(TIM_SMCR_SMS, 4) | /* Slave Mode Restart */
               _VAL2FLD(TIM_SMCR_TS, 6);   /* Trigger: TI2FP2 */

  /* Configure both channels to capture the same input. Use highest filtering
   * option available: 8x at fDTS_CK/32 */
  TIM9->CCMR1 = _VAL2FLD(TIM_CCMR1_CC2S, 1) | TIM_CCMR1_IC2F |
                _VAL2FLD(TIM_CCMR1_CC1S, 2) | TIM_CCMR1_IC1F;
  TIM9->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E;

  bool active_high = (config.freq_inputs[3].edge == RISING_EDGE);
  if (active_high) {
    TIM9->CCER |= TIM_CCER_CC1P;
  } else {
    TIM9->CCER |= TIM_CCER_CC2P;
  }

  NVIC_EnableIRQ(TIM1_BRK_TIM9_IRQn);
  NVIC_SetPriority(TIM1_BRK_TIM9_IRQn, 11);
  TIM9->CR1 |= TIM_CR1_CEN;

  DBGMCU->APB2FZ |= DBGMCU_APB2_FZ_DBG_TIM9_STOP;
}

void TIM1_BRK_TIM9_IRQHandler(void) {
  if (TIM9->SR & TIM_SR_CC2IF) {
    uint32_t pulsewidth = TIM9->CCR1;
    uint32_t period = TIM9->CCR2;
    bool valid = (period != 0);
    struct freq_update update = {
      .time = current_time(),
      .valid = valid,
      .pin = 3,
      .frequency = valid ? (65536.0f / (float)period) : 0,
      .pulsewidth = (float)pulsewidth / 65536.0f,
    };
    sensor_update_freq(&update);
  }

  if (TIM9->SR & TIM_SR_UIF) {
    TIM9->SR = ~TIM_SR_UIF;
    struct freq_update update = {
      .time = current_time(),
      .valid = false,
      .pin = 3,
    };
    sensor_update_freq(&update);
  }
}

void stm32f4_configure_adc(void) {
  setup_spi1();
  setup_spi1_tx_dma();
  setup_spi1_rx_dma();
  setup_tim1();

  setup_freq_pw_input();

  knock_configure(&config.knock_inputs[0]);
  knock_configure(&config.knock_inputs[1]);
}
