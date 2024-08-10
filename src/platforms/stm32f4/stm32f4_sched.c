#include "config.h"
#include "decoder.h"
#include "platform.h"
#include "scheduler.h"
#include "stm32f427xx.h"
#include "util.h"

#include "stm32_sched_buffers.h"

/* The primary outputs are driven by the DMA copying from a circular
 * double-buffer to the GPIO's BSRR register.  TIM8 is configured to generate
 * an update even at 4 MHz, and that update event triggers the DMA.  The same
 * update event also triggers TIM2, which keeps track of the current time.
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
 * Since TIM2 is the main time base and also has input captures, we use the
 * first two capture units as the trigger inputs.  These inputs trigger a
 * capture interrupt, the captured time is used to directly call the decoder,
 * which optionally may do fuel/ignition calculations and reschedule events.
 */

static void setup_tim8(void) {
  TIM8->CR2 = _VAL2FLD(TIM_CR2_MMS, 2); /* Master mode: TRGO on update */
  TIM8->ARR = 41; /* Period of (41+1) with 168 MHz Clock produces 4 MHz */
  TIM8->DIER = TIM_DIER_UDE; /* Enable DMA event on update*/
  TIM8->CR1 = TIM_CR1_CEN;   /* Start counter */

  DBGMCU->APB2FZ |= DBGMCU_APB2_FZ_DBG_TIM8_STOP; /* Stop timer in debug hold */
}

static void configure_trigger_inputs(void) {
  trigger_edge cc1_edge = config.freq_inputs[0].edge;
  trigger_edge cc2_edge = config.freq_inputs[1].edge;
  bool cc1_en = (config.freq_inputs[0].type == TRIGGER);
  bool cc2_en = (config.freq_inputs[1].type == TRIGGER);

  bool cc1p = (cc1_edge == FALLING_EDGE) || (cc1_edge == BOTH_EDGES);
  bool cc1np = (cc1_edge == BOTH_EDGES);
  bool cc2p = (cc2_edge == FALLING_EDGE) || (cc2_edge == BOTH_EDGES);
  bool cc2np = (cc2_edge == BOTH_EDGES);

  /* Ensure CC1 and CC2 are disabled */
  TIM2->CCER &= ~(TIM_CCER_CC1E | TIM_CCER_CC2E);

  /* Configure 2 input captures for trigger inputs */
  uint32_t trigger_filter = 0x0; /* No filtering */
  TIM2->CCMR1 = _VAL2FLD(TIM_CCMR1_IC1F, trigger_filter) |
                _VAL2FLD(TIM_CCMR1_CC1S, 0x1) | /* CC1 uses TI1 */
                _VAL2FLD(TIM_CCMR1_IC2F, trigger_filter) |
                _VAL2FLD(TIM_CCMR1_CC2S, 0x1); /* CC2 uses TI2 */
  TIM2->CCER = (cc1p ? TIM_CCER_CC1P : 0) | (cc1np ? TIM_CCER_CC2NP : 0) |
               (cc2p ? TIM_CCER_CC2P : 0) | (cc2np ? TIM_CCER_CC2NP : 0) |
               TIM_CCER_CC4E | /* Setup output compare for callbacks */
               (cc1_en ? TIM_CCER_CC1E : 0) | (cc2_en ? TIM_CCER_CC2E : 0);
}

static void setup_tim2(void) {

  TIM2->SMCR = _VAL2FLD(TIM_SMCR_SMS, 7) | /* External clock mode 1 */
               _VAL2FLD(TIM_SMCR_TS, 1);   /* ITR1 (TRGO from TIM8) */
  TIM2->ARR = 0xffffffff;

  configure_trigger_inputs();

  NVIC_SetPriority(TIM2_IRQn, 3);
  NVIC_EnableIRQ(TIM2_IRQn);

  TIM2->DIER = TIM_DIER_CC1IE | TIM_DIER_CC2IE; /* Enable trigger input capture
                                                   interrupts */
  TIM2->CR1 |= TIM_CR1_CEN;

  DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_TIM2_STOP;
}

timeval_t current_time() {
  return TIM2->CNT;
}

void schedule_event_timer(timeval_t time) {
  TIM2->DIER &= ~TIM_DIER_CC4IE; /* Disable CC4 interrupt */
  TIM2->SR = ~TIM_SR_CC4IF;      /* Clear any pending CC4 interrupt */

  TIM2->CCR4 = time;
  TIM2->DIER |= TIM_DIER_CC4IE; /* Re-enable CC4 with time set */

  /* If time is prior to now, we may have missed it, set the interrupt pending
   * just in case */
  if (time_before_or_equal(time, current_time())) {
    TIM2->EGR = TIM_EGR_CC4G;
  }
}

void TIM2_IRQHandler(void) {
  bool cc1_fired = false;
  bool cc2_fired = false;
  timeval_t cc1;
  timeval_t cc2;

  if (TIM2->SR & TIM_SR_CC1IF) {
    cc1 = TIM2->CCR1;
    cc1_fired = true;
  }

  if (TIM2->SR & TIM_SR_CC1IF) {
    cc2 = TIM2->CCR2;
    cc2_fired = true;
  }

  if (cc1_fired && cc2_fired && time_before(cc2, cc1)) {
    decoder_update_scheduling(1, cc2);
    decoder_update_scheduling(0, cc1);
  } else {
    if (cc1_fired) {
      decoder_update_scheduling(0, cc1);
    }
    if (cc2_fired) {
      decoder_update_scheduling(1, cc2);
    }
  }

  if (TIM2->SR & TIM_SR_CC4IF) {
    TIM2->SR = ~TIM_SR_CC4IF;
    scheduler_callback_timer_execute();
  }
}

extern void abort(void); /* TODO handle this better */

void DMA2_Stream1_IRQHandler(void) {
  if (DMA2->LISR & DMA_LISR_TCIF1) {
    DMA2->LIFCR = DMA_LIFCR_CTCIF1;
    stm32_buffer_swap();
  }

  /* If we overran, abort */
  uint32_t dma_current_buffer = _FLD2VAL(DMA_SxCR_CT, DMA2_Stream1->CR);
  if (dma_current_buffer != stm32_current_buffer()) {
    abort();
  }

  /* In the event of a transfer error, abort */
  if (DMA2->LISR & DMA_LISR_TEIF1) {
    abort();
  }
}

static void setup_scheduled_outputs(void) {

  /* While still in hi-z, set outputs that are active-low */
  for (int i = 0; i < MAX_EVENTS; ++i) {
    if (config.events[i].inverted && config.events[i].type != DISABLED_EVENT) {
      GPIOD->ODR |= (1 << config.events[i].pin);
    }
  }

  GPIOD->MODER = 0x55555555;   /* All of GPIOD is output */
  GPIOD->OSPEEDR = 0xffffffff; /* All GPIOD set to High speed*/

  /* Enable Interrupt on buffer swap at highest priority */
  NVIC_SetPriority(DMA2_Stream1_IRQn, 0);
  NVIC_EnableIRQ(DMA2_Stream1_IRQn);

  /* Use DMA2 Stream 1 Channel 7 to write to GPIOD's BSRR whenever TIMER8
   * updates */
  DMA2_Stream1->PAR = (uint32_t)&GPIOD->BSRR;
  DMA2_Stream1->M0AR = (uint32_t)&output_buffers[0].slots;
  DMA2_Stream1->M1AR = (uint32_t)&output_buffers[1].slots;
  DMA2_Stream1->NDTR = NUM_SLOTS;

  DMA2_Stream1->CR =
    _VAL2FLD(DMA_SxCR_CHSEL, 7) |   /* TIMER7 Update */
    DMA_SxCR_DBM |                  /* Switch-buffer mode */
    _VAL2FLD(DMA_SxCR_PL, 3) |      /* High priority */
    _VAL2FLD(DMA_SxCR_MSIZE, 0x2) | /* 32 bit memory size */
    _VAL2FLD(DMA_SxCR_PSIZE, 0x2) | /* 32 bit peripheral size */
    DMA_SxCR_MINC |                 /* Memory-increment */
    _VAL2FLD(DMA_SxCR_DIR, 0x1) |   /* Memory -> peripheral */
    DMA_SxCR_TCIE |                 /* Enable interrupt on buffer swap */
    DMA_SxCR_TEIE |                 /* Enable interrupt on transfer error */
    DMA_SxCR_EN;                    /* Enable DMA */
}

void stm32f4_configure_scheduler() {
  setup_scheduled_outputs();
  setup_tim8();
  setup_tim2();
}
