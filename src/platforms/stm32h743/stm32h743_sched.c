#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "config.h"
#include "decoder.h"
#include "platform.h"
#include "sensors.h"
#include "stm32h743xx.h"
#include "util.h"

/* The primary outputs are driven by the DMA copying from a circular
 * double-buffer to the GPIO's BSRR register.  TIM8 is configured to generate an
 * update even at 4 MHz, and that update event triggers the DMA.  The same
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
  TIM8->CR1 = TIM_CR1_URS; /* overflow generates DMA */

  TIM8->CR2 = TIM_CR2_MMS_1; /* TRGO on update */

  TIM8->ARR =
    49; /* APB2 timer clock is 200 MHz, divide by (49+1) to get 4 MHz */

  TIM8->DIER = TIM_DIER_UDE; /* Interrupt and DMA on update */
  TIM8->CR1 |= TIM_CR1_CEN;  /* Enable clock */
}

timeval_t current_time() {
  return TIM2->CNT;
}

void TIM2_IRQHandler(void) {
  bool cc1_fired = false;
  bool cc2_fired = false;
  timeval_t cc1;
  timeval_t cc2;

  if ((TIM2->SR & TIM_SR_CC1IF) == TIM_SR_CC1IF) {
    /* TI1 capture */
    cc1_fired = true;
    cc1 = TIM2->CCR1;
  }

  if ((TIM2->SR & TIM_SR_CC2IF) == TIM_SR_CC2IF) {
    /* TI1 capture */
    cc2_fired = true;
    cc2 = TIM2->CCR2;
  }

  if (((TIM2->SR & TIM_SR_CC1OF) == TIM_SR_CC1OF) ||
      ((TIM2->SR & TIM_SR_CC2OF) == TIM_SR_CC2OF)) {
    TIM2->SR &= ~(TIM_SR_CC1OF | TIM_SR_CC2OF);
    decoder_desync(DECODER_OVERFLOW);
    return;
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

  if ((TIM2->SR & TIM_SR_CC4IF) == TIM_SR_CC4IF) {
    /* TI1 capture */
    TIM2->SR &= ~TIM_SR_CC4IF;
    scheduler_callback_timer_execute();
  }
}

void set_event_timer(timeval_t t) {
  TIM2->CCR4 = t;
  TIM2->DIER |= TIM_DIER_CC4IE;
}

timeval_t get_event_timer() {
  return TIM2->CCR4;
}

void clear_event_timer() {
  TIM2->SR &= ~TIM_SR_CC4IF;
}

void disable_event_timer() {
  TIM2->DIER &= ~TIM_DIER_CC4IE;
  clear_event_timer();
}

static void setup_tim2(void) {
  TIM2->CR1 = TIM_CR1_UDIS;               /* Upcounting, no update event */
  TIM2->SMCR = TIM_SMCR_TS_0 |            /* Trigger ITR1 is TIM8's TRGO */
               _VAL2FLD(TIM_SMCR_SMS, 7); /* External Clock Mode 1 */
  TIM2->ARR = 0xFFFFFFFF;                 /* Count through whole 32bits */

  /* Set up input triggers */
  TIM2->CCMR1 = TIM_CCMR1_IC1F_0 | /* CK_INT, N=2 filter */
                TIM_CCMR1_CC1S_0 | /* IC1 */
                TIM_CCMR1_IC2F_0 | /* CK_INT, N=2 filter */
                TIM_CCMR1_CC2S_0;  /* IC2 */
  TIM2->CCMR2 = TIM_CCMR2_CC4S_0 | /* CC4 Output */
                TIM_CCMR2_OC4M_0;

  trigger_edge cc1_edge = config.freq_inputs[0].edge;
  trigger_edge cc2_edge = config.freq_inputs[1].edge;

  int cc1p = (cc1_edge == FALLING_EDGE) || (cc1_edge == BOTH_EDGES);
  int cc1np = (cc1_edge == BOTH_EDGES);
  int cc2p = (cc2_edge == FALLING_EDGE) || (cc2_edge == BOTH_EDGES);
  int cc2np = (cc2_edge == BOTH_EDGES);

  TIM2->CCER = _VAL2FLD(TIM_CCER_CC1P, cc1p) | _VAL2FLD(TIM_CCER_CC1NP, cc1np) |
               _VAL2FLD(TIM_CCER_CC2P, cc2p) |
               _VAL2FLD(TIM_CCER_CC2NP, cc2np) | /* Edge selection */
               TIM_CCER_CC1E | TIM_CCER_CC2E |   /* Enable CC1/CC2 */
               TIM_CCER_CC4E;                    /* Enable CC4 (Output) */

  /*Enable interrupts for CC1/CC2 if input is TRIGGER */
  TIM2->DIER = (config.freq_inputs[0].type == TRIGGER ? TIM_DIER_CC1IE : 0) |
               (config.freq_inputs[1].type == TRIGGER ? TIM_DIER_CC2IE : 0);

  NVIC_SetPriority(TIM2_IRQn, 
    NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 1, 0));
  NVIC_EnableIRQ(TIM2_IRQn);

  /* A0 and A1 as trigger inputs */
  GPIOA->MODER &= ~(GPIO_MODER_MODE0 | GPIO_MODER_MODE1);
  GPIOA->MODER |=
    GPIO_MODER_MODE0_1 | GPIO_MODER_MODE1_1; /* A0/A1 in AF mode */
  GPIOA->AFR[0] |= _VAL2FLD(GPIO_AFRL_AFSEL0, 1) |
                   _VAL2FLD(GPIO_AFRL_AFSEL1, 1); /* AF1 (TIM2)*/

  TIM2->CR1 |= TIM_CR1_CEN; /* Enable clock */
}

#define NUM_SLOTS 128
/* GPIO BSRR is 16 bits of "on" mask and 16 bits of "off" mask */
struct output_slot {
  uint16_t on;
  uint16_t off;
} __attribute__((packed)) __attribute((aligned(4)));

struct output_buffer {
  timeval_t first_time; /* First time represented by the range */
  struct output_slot slots[NUM_SLOTS];
};

static __attribute__((
  section(".dmadata"))) struct output_buffer output_buffers[2] = { 0 };
static uint32_t current_buffer = 0;

static void platform_output_slot_unset(struct output_slot *slots,
                                       uint32_t index,
                                       uint32_t pin,
                                       bool value) {
  if (value) {
    slots[index].on &= ~(1 << pin);
  } else {
    slots[index].off &= ~(1 << pin);
  }
}

static void platform_output_slot_set(struct output_slot *slots,
                                     uint32_t index,
                                     uint32_t pin,
                                     bool value) {
  if (value) {
    slots[index].on |= (1 << pin);
  } else {
    slots[index].off |= (1 << pin);
  }
}

/* Return the first time that is gauranteed to be changable.  We use a circular
 * pair of buffers: We can't change the current buffer, and its likely the next
 * buffer's time range is already submitted, so use the time after that.
 */
timeval_t platform_output_earliest_schedulable_time() {
  return output_buffers[current_buffer].first_time + NUM_SLOTS * 2;
}

/* Retire all stop/stop events that are in the time range of our "completed"
 * buffer and were previously submitted by setting them to "fired" and clearing
 * out the dma bits */
static void retire_output_buffer(struct output_buffer *buf) {
  timeval_t offset_from_start;
  for (int i = 0; i < MAX_EVENTS; i++) {
    struct output_event *oev = &config.events[i];

    offset_from_start = oev->start.time - buf->first_time;
    if (sched_entry_get_state(&oev->start) == SCHED_SUBMITTED &&
        offset_from_start < NUM_SLOTS) {
      platform_output_slot_unset(
        buf->slots, offset_from_start, oev->start.pin, oev->start.val);
      sched_entry_set_state(&oev->start, SCHED_FIRED);
    }

    offset_from_start = oev->stop.time - buf->first_time;
    if (sched_entry_get_state(&oev->stop) == SCHED_SUBMITTED &&
        offset_from_start < NUM_SLOTS) {
      platform_output_slot_unset(
        buf->slots, offset_from_start, oev->stop.pin, oev->stop.val);
      sched_entry_set_state(&oev->stop, SCHED_FIRED);
    }
  }
}

/* Any scheduled start/stop event in the time range for the new buffer can be
 * "submitted" and the dma bits set */
static void populate_output_buffer(struct output_buffer *buf) {
  timeval_t offset_from_start;
  for (int i = 0; i < MAX_EVENTS; i++) {
    struct output_event *oev = &config.events[i];
    offset_from_start = oev->start.time - buf->first_time;
    if (sched_entry_get_state(&oev->start) == SCHED_SCHEDULED &&
        offset_from_start < NUM_SLOTS) {
      platform_output_slot_set(
        buf->slots, offset_from_start, oev->start.pin, oev->start.val);
      sched_entry_set_state(&oev->start, SCHED_SUBMITTED);
    }
    offset_from_start = oev->stop.time - buf->first_time;
    if (sched_entry_get_state(&oev->stop) == SCHED_SCHEDULED &&
        offset_from_start < NUM_SLOTS) {
      platform_output_slot_set(
        buf->slots, offset_from_start, oev->stop.pin, oev->stop.val);
      sched_entry_set_state(&oev->stop, SCHED_SUBMITTED);
    }
  }
}

static timeval_t round_time_to_buffer_start(timeval_t time) {
  timeval_t time_since_buffer_start = time % NUM_SLOTS;
  return time - time_since_buffer_start;
}

static void platform_buffer_swap() {
  struct output_buffer *buf = &output_buffers[current_buffer];
  current_buffer = (current_buffer + 1) % 2;

  retire_output_buffer(buf);

  buf->first_time = round_time_to_buffer_start(current_time()) + NUM_SLOTS;

  populate_output_buffer(buf);
}

void DMA1_Stream0_IRQHandler(void) {
  if ((DMA1->LISR & DMA_LISR_TCIF0) == DMA_LISR_TCIF0) {
    DMA1->LIFCR = DMA_LIFCR_CTCIF0;
    platform_buffer_swap();

    if (_FLD2VAL(DMA_SxCR_CT, DMA1_Stream0->CR) != current_buffer) {
      /* We have overflowed or gone out of sync, abort immediately */
      abort();
    }
  }
  if ((DMA1->LISR & DMA_LISR_DMEIF0) == DMA_LISR_DMEIF0) {
    while (1)
      ;
  }
}

static void setup_scheduled_outputs(void) {
  /* While still in hi-z, set outputs that are active-low */
  for (int i = 0; i < MAX_EVENTS; ++i) {
    if (config.events[i].inverted && config.events[i].type != DISABLED_EVENT) {
      GPIOD->ODR |= (1 << config.events[i].pin);
    }
  }

  GPIOD->MODER = 0x55555555;   /* All outputs MODER = 0x01 */
  GPIOD->OSPEEDR = 0xFFFFFFFF; /* All outputs High Speed */

  /* Configure DMA1 Stream 0 */
  DMA1_Stream0->CR = 0; /* Reset */
  DMA1_Stream0->PAR =
    (uint32_t)&GPIOD->BSRR; /* Peripheral address is GPIOD BSRR */
  DMA1_Stream0->M0AR =
    (uint32_t)&output_buffers[0].slots; /* Memory address 0 and 1 */
  DMA1_Stream0->M1AR = (uint32_t)&output_buffers[1].slots;
  DMA1_Stream0->NDTR = NUM_SLOTS;

  DMA1_Stream0->CR = DMA_SxCR_DBM |                /* Double Buffer */
                     _VAL2FLD(DMA_SxCR_PL, 3) |    /* High Priority */
                     _VAL2FLD(DMA_SxCR_MSIZE, 2) | /* 32 bit memory size */
                     _VAL2FLD(DMA_SxCR_PSIZE, 2) | /* 32 bit peripheral size */
                     DMA_SxCR_MINC |  /* Memory increment after each transfer */
                     DMA_SxCR_DIR_0 | /* Direction is memory to peripheral */
                     DMA_SxCR_TCIE | /* Interrupt when each transfer complete */
                     DMA_SxCR_DMEIE; /* Interrupt on DMA error */

  /* Configure DMAMUX */
  DMAMUX1_Channel0->CCR &= ~DMAMUX_CxCR_DMAREQ_ID;
  DMAMUX1_Channel0->CCR |=
    _VAL2FLD(DMAMUX_CxCR_DMAREQ_ID, 51); /* TIM8 Update */

  NVIC_EnableIRQ(DMA1_Stream0_IRQn);
  NVIC_SetPriority(DMA1_Stream0_IRQn, 
    NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
  DMA1_Stream0->CR |= DMA_SxCR_EN; /* Enable */
}

void platform_configure_scheduler() {
  setup_tim2();
  setup_tim8();
  setup_scheduled_outputs();
}
