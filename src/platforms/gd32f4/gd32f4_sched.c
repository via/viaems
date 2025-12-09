#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "config.h"
#include "decoder.h"
#include "gd32f4xx.h"
#include "platform.h"
#include "sensors.h"
#include "sim.h"
#include "util.h"
#include "viaems.h"

#include "stm32_sched_buffers.h"

/* The primary outputs are driven by the DMA copying from a circular
 * double-buffer to the GPIO's BSRR register.  TIMER7 is configured to generate
 * an update even at 4 MHz, and that update event triggers the DMA.  The same
 * update event also triggers TIMER1, which keeps track of the current time.
 *
 * Each DMA buffer represents 32 uS of time (128 time points at 4 MHz), and each
 * 32 bit word in the DMA buffer is directly written to BSRR.  The high 16 bits
 * of this word cause the GPIO for that bit to go high, the low 16 bits cause it
 * to go low.
 *
 * The DMA buffers are updated and modified only in the DMA ISR, which is
 * triggered once the DMA has finished the buffer and is currently executing on
 * the other buffer. At this time, we iterate through all events that would have
 * been in the time range of the completed buffer and set them to FIRED.  We
 * then enumerate all events that are scheduled for the next time range and set
 * the bits appropriately.
 *
 * Since TIMER1 is the main time base and also has input captures, we use the
 * first two capture units as the trigger inputs.  These inputs trigger a
 * capture interrupt, the captured time is used to directly call the decoder,
 * which optionally may do fuel/ignition calculations and reschedule events.
 */

extern struct viaems gd32f4_viaems;

static void setup_timer7(void) {
  TIMER_CTL1(TIMER7) =
    TIMER_TRI_OUT_SRC_UPDATE; /* Master Mode: TRGO on counter update */
  TIMER_CAR(TIMER7) =
    47; /* Period of (47+1) with 192 MHz Clock produces 4 MHz */
  TIMER_DMAINTEN(TIMER7) = TIMER_DMAINTEN_UPDEN; /* Enable DMA event on update*/
  TIMER_CTL0(TIMER7) = TIMER_CTL0_CEN;           /* Start */

  DBG_CTL2 |= DBG_CTL2_TIMER7_HOLD;
}

static void configure_trigger_inputs(void) {
  trigger_edge cc0_edge = default_config.trigger_inputs[0].edge;
  trigger_edge cc1_edge = default_config.trigger_inputs[1].edge;
  trigger_edge cc2_edge = default_config.trigger_inputs[2].edge;

  bool cc0_en = (default_config.trigger_inputs[0].type != NONE);
  bool cc1_en = (default_config.trigger_inputs[1].type != NONE);
  bool cc2_en = (default_config.trigger_inputs[2].type != NONE);

  bool cc0p = (cc0_edge == FALLING_EDGE) || (cc0_edge == BOTH_EDGES);
  bool cc0np = (cc0_edge == BOTH_EDGES);

  bool cc1p = (cc1_edge == FALLING_EDGE) || (cc1_edge == BOTH_EDGES);
  bool cc1np = (cc1_edge == BOTH_EDGES);

  bool cc2p = (cc2_edge == FALLING_EDGE) || (cc2_edge == BOTH_EDGES);
  bool cc2np = (cc2_edge == BOTH_EDGES);

  /* Ensure CH0, CH1, and CH2 are off */
  TIMER_CHCTL2(TIMER1) &=
    ~(TIMER_CHCTL2_CH0EN | TIMER_CHCTL2_CH1EN | TIMER_CHCTL2_CH2EN);

  const uint32_t input_filter = 0x0; /* sample at CK_DTS, which is the 4 MHz
                                        clock provided by TIMER7.  The only
                                        options are up to 8 samples at 240 MHz,
                                        or a minimum of 6 samples at CK_DTS / 2,
                                        which introduces a 3 uS delay. */

  /* Configure 2 input captures for trigger inputs */
  TIMER_CHCTL0(TIMER1) =
    input_filter | 0x1 |         /* CH0MS Mode 01 -- CH0 input */
    ((input_filter | 0x1) << 8); /* CH1MS Mode 01 -- CH1 input */

  /* Enable third input for frequency input */
  TIMER_CHCTL1(TIMER1) = input_filter | 0x1; /* CH2MS Mode 01 -- CH2 input */

  TIMER_CHCTL2(TIMER1) =
    (cc0p ? TIMER_CHCTL2_CH0P : 0) | (cc0np ? TIMER_CHCTL2_CH0NP : 0) |
    (cc1p ? TIMER_CHCTL2_CH1P : 0) | (cc1np ? TIMER_CHCTL2_CH2NP : 0) |
    (cc2p ? TIMER_CHCTL2_CH2P : 0) | (cc2np ? TIMER_CHCTL2_CH2NP : 0) |
    (cc0_en ? TIMER_CHCTL2_CH0EN : 0) | (cc1_en ? TIMER_CHCTL2_CH1EN : 0) |
    (cc2_en ? TIMER_CHCTL2_CH2EN : 0) | TIMER_CHCTL2_CH3EN;
}

static void setup_timer1(void) {

  /* Enable the trigger/freq input GPIOs */
  gpio_mode_set(GPIOA,
                GPIO_MODE_AF,
                GPIO_PUPD_PULLUP,
                GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);
  gpio_af_set(GPIOA, GPIO_AF_1, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);

  /* Timer is driven by ITI1, the output of TIMER7 */
  TIMER_SMCFG(TIMER1) = TIMER_SLAVE_MODE_EXTERNAL0 | TIMER_SMCFG_TRGSEL_ITI1;
  TIMER_CAR(TIMER1) = 0xffffffff;

  configure_trigger_inputs();

  nvic_irq_enable(TIMER1_IRQn, 2, 0);
  TIMER_DMAINTEN(TIMER1) =
    TIMER_DMAINTEN_CH0IE | TIMER_DMAINTEN_CH1IE | TIMER_DMAINTEN_CH2IE;

  TIMER_CTL0(TIMER1) |= TIMER_CTL0_CEN;

  DBG_CTL1 |= DBG_CTL1_TIMER1_HOLD;
}

timeval_t current_time() {
  return TIMER_CNT(TIMER1);
}

struct gd32f4_timebase_freq_pin {
  int pin;
  timeval_t last_time;
};

static struct gd32f4_timebase_freq_pin tim1_ch2_freq = {
  .pin = 2,
  .last_time = 0,
};

void TIMER1_IRQHandler(void) {
  bool cc0_fired = false;
  bool cc1_fired = false;
  timeval_t cc0;
  timeval_t cc1;
  const trigger_type cc0_type = default_config.trigger_inputs[0].type;
  const trigger_type cc1_type = default_config.trigger_inputs[1].type;

  if (TIMER_INTF(TIMER1) & TIMER_INTF_CH0IF) {
    cc0 = TIMER_CH0CV(TIMER1);
    cc0_fired = true;
  }

  if (TIMER_INTF(TIMER1) & TIMER_INTF_CH1IF) {
    cc1 = TIMER_CH1CV(TIMER1);
    cc1_fired = true;
  }

  if (cc0_fired && cc1_fired && time_before(cc1, cc0)) {
    decoder_update(&gd32f4_viaems.decoder,
                   &(struct trigger_event){ .time = cc1, .type = cc1_type });
    decoder_update(&gd32f4_viaems.decoder,
                   &(struct trigger_event){ .time = cc0, .type = cc0_type });
  } else {
    if (cc0_fired) {
      decoder_update(&gd32f4_viaems.decoder,
                     &(struct trigger_event){ .time = cc0, .type = cc0_type });
    }
    if (cc1_fired) {
      decoder_update(&gd32f4_viaems.decoder,
                     &(struct trigger_event){ .time = cc1, .type = cc1_type });
    }
  }

  /* Handle FREQ input */
  if (TIMER_INTF(TIMER1) & TIMER_INTF_CH2IF) {
    timeval_t freqtime = TIMER_CH2CV(TIMER1);
    timeval_t period = freqtime - tim1_ch2_freq.last_time;
    tim1_ch2_freq.last_time = freqtime;
    bool valid = (period != 0);
    struct freq_update update = {
      .time = freqtime,
      .valid = valid,
      .pin = tim1_ch2_freq.pin,
      .frequency = valid ? ((float)TICKRATE / period) : 0,
      .pulsewidth = 0,
    };
    struct engine_position pos =
      decoder_get_engine_position(&gd32f4_viaems.decoder);
    sensor_update_freq(&gd32f4_viaems.sensors, &pos, &update);
  }

  if (TIMER_INTF(TIMER1) & TIMER_INTF_CH3IF) {
    TIMER_INTF(TIMER1) = ~TIMER_INTF_CH3IF;
    sim_wakeup_callback(&gd32f4_viaems.decoder);
  }
}

void set_sim_wakeup(timeval_t time) {

  TIMER_DMAINTEN(TIMER1) &= ~TIMER_DMAINTEN_CH3IE;
  TIMER_INTF(TIMER1) = ~TIMER_INTF_CH3IF;

  TIMER_CH3CV(TIMER1) = time;
  TIMER_DMAINTEN(TIMER1) |= TIMER_DMAINTEN_CH3IE;

  /* If time is prior to now, we may have missed it, set the interrupt pending
   * just in case */
  if (time_before_or_equal(time, current_time())) {
    TIMER_SWEVG(TIMER1) = TIMER_SWEVG_CH3G;
  }
}

void reset_watchdog(void);

void DMA1_Channel1_IRQHandler(void) {
  static struct engine_update update = { 0 };
  if (dma_interrupt_flag_get(DMA1, DMA_CH1, DMA_INT_FLAG_FTF) == SET) {
    dma_interrupt_flag_clear(DMA1, DMA_CH1, DMA_INT_FLAG_FTF);

    update.sensors = sensors_get_values(&gd32f4_viaems.sensors);
    update.current_time =
      output_buffers[stm32_current_buffer()].plan.schedulable_start;

    __disable_irq();
    update.position = decoder_get_engine_position(&gd32f4_viaems.decoder);
    __enable_irq();

    stm32_buffer_swap(&gd32f4_viaems, &update);
  }

  /* If we overran, abort */
  if (dma_using_memory_get(DMA1, DMA_CH1) != stm32_current_buffer()) {
    abort();
  }

  /* In the event of a transfer error, abort */
  if (dma_interrupt_flag_get(DMA1, DMA_CH1, DMA_INT_FLAG_TAE)) {
    abort();
  }

  reset_watchdog();
}

static void setup_scheduled_outputs(void) {

  /* While still in hi-z, set outputs that are active-low */
  for (int i = 0; i < MAX_EVENTS; ++i) {
    if (default_config.outputs[i].inverted &&
        default_config.outputs[i].type != DISABLED_EVENT) {
      GPIO_OCTL(GPIOD) |= (1 << default_config.outputs[i].pin);
    }
  }

  GPIO_CTL(GPIOD) = 0x55555555;  /* All of GPIOD is output */
  GPIO_OSPD(GPIOD) = 0xffffffff; /* All GPIOD set to High speed*/

  nvic_irq_enable(DMA1_Channel1_IRQn, 2, 0);

  /* Use DMA1's Channel 1 to write to GPIOD's OCTL whenever TIMER7 updates */
  DMA_CH1PADDR(DMA1) = (uint32_t)&GPIO_BOP(GPIOD);
  DMA_CH1M0ADDR(DMA1) = (uint32_t)&output_buffers[0].slots;
  DMA_CH1M1ADDR(DMA1) = (uint32_t)&output_buffers[1].slots;
  DMA_CH1CNT(DMA1) = NUM_SLOTS;

  DMA_CH1CTL(DMA1) = DMA_PERIPH_7_SELECT | /* TIMER7 Update */
                     DMA_CHXCTL_SBMEN |    /* Switch-buffer mode */
                     DMA_PRIORITY_HIGH | DMA_MEMORY_WIDTH_32BIT |
                     DMA_PERIPH_WIDTH_32BIT |
                     DMA_CHXCTL_MNAGA | /* Memory-increment */
                     DMA_MEMORY_TO_PERIPH |
                     DMA_CHXCTL_FTFIE | /* Enable interrupt on buffer swap */
                     DMA_CHXCTL_TAEIE | /* Enable interrupt on transfer error */
                     DMA_CHXCTL_CHEN;   /* Enable DMA */
}

void gd32f4xx_configure_scheduler() {
  setup_scheduled_outputs();
  setup_timer1();
  setup_timer7();
}
