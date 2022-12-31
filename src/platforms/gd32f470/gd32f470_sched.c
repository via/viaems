#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "config.h"
#include "decoder.h"
#include "gd32f4xx.h"
#include "platform.h"
#include "sensors.h"
#include "util.h"

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

static void setup_timer7(void) {
  TIMER_CTL1(TIMER7) =
    TIMER_TRI_OUT_SRC_UPDATE; /* Master Mode: TRGO on counter update */
  TIMER_CAR(TIMER7) =
    59; /* Period of (59+1) with 240 MHz Clock produces 4 MHz */
  TIMER_DMAINTEN(TIMER7) = TIMER_DMAINTEN_UPDEN; /* Enable DMA event on update*/
  TIMER_CTL0(TIMER7) = TIMER_CTL0_CEN;           /* Start */

  DBG_CTL2 |= DBG_CTL2_TIMER7_HOLD;
}

static void configure_trigger_inputs(void) {
  trigger_edge cc0_edge = config.freq_inputs[0].edge;
  trigger_edge cc1_edge = config.freq_inputs[1].edge;
  bool cc0_en = (config.freq_inputs[0].type == TRIGGER);
  bool cc1_en = (config.freq_inputs[1].type == TRIGGER);

  bool cc0p = (cc0_edge == FALLING_EDGE) || (cc0_edge == BOTH_EDGES);
  bool cc0np = (cc0_edge == BOTH_EDGES);
  bool cc1p = (cc1_edge == FALLING_EDGE) || (cc1_edge == BOTH_EDGES);
  bool cc1np = (cc1_edge == BOTH_EDGES);

  /* Ensure CH0 and CH1 are off */
  TIMER_CHCTL2(TIMER1) &= ~(TIMER_CHCTL2_CH0EN | TIMER_CHCTL2_CH1EN);

  /* Configure 2 input captures for trigger inputs */
  TIMER_CHCTL0(TIMER1) = TIMER_CHCTL0_CH0CAPFLT | /* DTS/32 filter */
                         0x1 | /* CH0MS Mode 01 -- CH0 input */
                         TIMER_CHCTL0_CH1CAPFLT | /* DTS/32 filter */
                         (0x1 << 8); /* CH1MS Mode 01 -- CH1 input */

  TIMER_CHCTL2(TIMER1) =
    (cc0p ? TIMER_CHCTL2_CH0P : 0) | (cc0np ? TIMER_CHCTL2_CH0NP : 0) |
    (cc1p ? TIMER_CHCTL2_CH1P : 0) | (cc1np ? TIMER_CHCTL2_CH2NP : 0) |
    TIMER_CHCTL2_CH3EN | /* Setup output compare for callbacks */
    (cc0_en ? TIMER_CHCTL2_CH0EN : 0) | (cc1_en ? TIMER_CHCTL2_CH1EN : 0);
}

static void setup_timer1(void) {

  /* Enable the trigger input GPIOs */
  gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_0 | GPIO_PIN_1);
  gpio_af_set(GPIOA, GPIO_AF_1, GPIO_PIN_0 | GPIO_PIN_1);

  /* Timer is driven by ITI1, the output of TIMER7 */
  TIMER_SMCFG(TIMER1) = TIMER_SLAVE_MODE_EXTERNAL0 | TIMER_SMCFG_TRGSEL_ITI1;
  TIMER_CAR(TIMER1) = 0xffffffff;

  configure_trigger_inputs();

  nvic_irq_enable(TIMER1_IRQn, 1, 0);
  TIMER_DMAINTEN(TIMER1) = TIMER_DMAINTEN_CH0IE | TIMER_DMAINTEN_CH1IE;

  TIMER_CTL0(TIMER1) |= TIMER_CTL0_CEN;

  DBG_CTL1 |= DBG_CTL1_TIMER1_HOLD;
}

timeval_t current_time() {
  return TIMER_CNT(TIMER1);
}

void TIMER1_IRQHandler(void) {
  bool cc0_fired = false;
  bool cc1_fired = false;
  timeval_t cc0;
  timeval_t cc1;

  if (TIMER_INTF(TIMER1) & TIMER_INTF_CH0IF) {
    TIMER_INTF(TIMER1) &= ~TIMER_INTF_CH0IF;
    cc0_fired = true;
    cc0 = TIMER_CH0CV(TIMER1);
  }

  if (TIMER_INTF(TIMER1) & TIMER_INTF_CH1IF) {
    TIMER_INTF(TIMER1) &= ~TIMER_INTF_CH1IF;
    cc1_fired = true;
    cc1 = TIMER_CH1CV(TIMER1);
  }

  if (cc0_fired && cc1_fired && time_before(cc1, cc0)) {
    decoder_update_scheduling(1, cc1);
    decoder_update_scheduling(0, cc0);
  } else {
    if (cc0_fired) {
      decoder_update_scheduling(0, cc0);
    }
    if (cc1_fired) {
      decoder_update_scheduling(1, cc1);
    }
  }

  if (TIMER_INTF(TIMER1) & TIMER_INTF_CH3IF) {
    TIMER_INTF(TIMER1) &= ~TIMER_INTF_CH3IF;
    scheduler_callback_timer_execute();
  }
}

void schedule_event_timer(timeval_t time) {

  TIMER_DMAINTEN(TIMER1) &= ~TIMER_DMAINTEN_CH3IE;
  TIMER_INTF(TIMER1) &= ~TIMER_INTF_CH3IF;

  TIMER_CH3CV(TIMER1) = time;
  TIMER_DMAINTEN(TIMER1) |= TIMER_DMAINTEN_CH3IE;

  /* If time is prior to now, we may have missed it, set the interrupt pending
   * just in case */
  if (time_before_or_equal(time, current_time())) {
    TIMER_SWEVG(TIMER1) = TIMER_SWEVG_CH3G;
  }
}

void DMA1_Channel1_IRQHandler(void) {
  if (dma_interrupt_flag_get(DMA1, DMA_CH1, DMA_INT_FLAG_FTF) == SET) {
    dma_interrupt_flag_clear(DMA1, DMA_CH1, DMA_INT_FLAG_FTF);
    stm32_buffer_swap();
  }

  /* If we overran, abort */
  if (dma_using_memory_get(DMA1, DMA_CH1) != stm32_current_buffer()) {
    abort();
  }

  /* In the event of a transfer error, abort */
  if (dma_interrupt_flag_get(DMA1, DMA_CH1, DMA_INT_FLAG_TAE)) {
    abort();
  }
}

static void setup_scheduled_outputs(void) {

  /* While still in hi-z, set outputs that are active-low */
  for (int i = 0; i < MAX_EVENTS; ++i) {
    if (config.events[i].inverted && config.events[i].type != DISABLED_EVENT) {
      GPIO_OCTL(GPIOD) |= (1 << config.events[i].pin);
    }
  }

  GPIO_CTL(GPIOD) = 0x55555555;  /* All of GPIOD is output */
  GPIO_OSPD(GPIOD) = 0xffffffff; /* All GPIOD set to High speed*/

  /* Enable Interrupt on buffer swap at highest priority */
  nvic_irq_enable(DMA1_Channel1_IRQn, 0, 0);

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

void gd32f470_configure_scheduler() {
  setup_scheduled_outputs();
  setup_timer1();
  setup_timer7();
}
