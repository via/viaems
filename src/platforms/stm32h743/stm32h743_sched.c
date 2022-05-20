#include <stdlib.h>
#include <stdbool.h>

#include "platform.h"
#include "decoder.h"
#include "config.h"
#include "sensors.h"
#include "util.h"
#include "device.h"

static void setup_tim8(void) {
  RCC->APB2ENR |= RCC_APB2ENR_TIM8EN;

  TIM8->CR1 = TIM8_CR1_DIR_VAL(0) | /* Upcounting */
              TIM8_CR1_URS_VAL(1); /* overflow generates DMA */
              /* Leave clock disabled until done */

  TIM8->CR2 = TIM8_CR2_MMS_VAL(2); /* TRGO on update */

  TIM8->ARR = 49; /* APB2 timer clock is 200 MHz, divide by (49+1) to get 4 MHz */

  TIM8->DIER = TIM8_DIER_UDE; /* Interrupt and DMA on update */
  TIM8->CR1 |= TIM8_CR1_CEN; /* Enable clock */
}

timeval_t current_time() {
  return TIM2->CNT;
}

void tim2_isr(void) {
  bool cc1_fired = false;
  bool cc2_fired = false;
  timeval_t cc1;
  timeval_t cc2;

  if ((TIM2->SR & TIM2_SR_CC1IF) == TIM2_SR_CC1IF) {
    /* TI1 capture */
    cc1_fired = true;
    cc1 = TIM2->CCR1;
  }

  if ((TIM2->SR & TIM2_SR_CC2IF) == TIM2_SR_CC2IF) {
    /* TI1 capture */
    cc2_fired = true;
    cc2 = TIM2->CCR2;
  }

  if (((TIM2->SR & TIM2_SR_CC1OF) == TIM2_SR_CC1OF) ||
      ((TIM2->SR & TIM2_SR_CC2OF) == TIM2_SR_CC2OF)) {
    TIM2->SR &= ~(TIM2_SR_CC1OF | TIM2_SR_CC2OF);
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

  if ((TIM2->SR & TIM2_SR_CC4IF) == TIM2_SR_CC4IF) {
    /* TI1 capture */
    TIM2->SR &= ~TIM2_SR_CC4IF;
    scheduler_callback_timer_execute();
  }

}

void set_event_timer(timeval_t t) {
  TIM2->CCR4 = t;
   TIM2->DIER |= TIM2_DIER_CC4IE;
}

timeval_t get_event_timer() {
  return TIM2->CCR4;
}

void clear_event_timer() {
  TIM2->SR &= ~TIM2_SR_CC4IF;
}

void disable_event_timer() {
  TIM2->DIER &= ~TIM2_DIER_CC4IE;
  clear_event_timer();
}

static void setup_tim2(void) {
  RCC->APB1LENR |= RCC_APB1LENR_TIM2EN;
  TIM2->CR1 = TIM2_CR1_UDIS; /* Upcounting, no update event */
  TIM2->SMCR = TIM2_SMCR_TS_VAL(1) | /* Trigger ITR1 is TIM8's TRGO */
               TIM2_SMCR_SMS_VAL(7); /* External Clock Mode 1 */
  TIM2->ARR = 0xFFFFFFFF; /* Count through whole 32bits */

  /* Set up input triggers */
  TIM2->CCMR1_Input = TIM2_CCMR1_Input_IC1F_VAL(1) | /* CK_INT, N=2 filter */
                      TIM2_CCMR1_Input_CC1S_VAL(1) | /* IC1 */
                      TIM2_CCMR1_Input_IC2F_VAL(1) | /* CK_INT, N=2 filter */
                      TIM2_CCMR1_Input_CC2S_VAL(1);  /* IC2 */
  TIM2->CCMR2_Output = TIM2_CCMR2_Output_CC4S_VAL(0) | /* CC4 Output */
                       TIM2_CCMR2_Output_OC4M_VAL(0);

  trigger_edge cc1_edge = config.freq_inputs[0].edge;
  trigger_edge cc2_edge = config.freq_inputs[1].edge;

  int cc1p = (cc1_edge == FALLING_EDGE) || (cc1_edge == BOTH_EDGES);
  int cc1np = (cc1_edge == BOTH_EDGES);
  int cc2p = (cc2_edge == FALLING_EDGE) || (cc2_edge == BOTH_EDGES);
  int cc2np = (cc2_edge == BOTH_EDGES);

  TIM2->CCER = TIM2_CCER_CC1P_VAL(cc1p) | TIM2_CCER_CC1NP_VAL(cc1np) |
               TIM2_CCER_CC2P_VAL(cc2p) | TIM2_CCER_CC2NP_VAL(cc2np) | /* Edge selection */
               TIM2_CCER_CC1E | TIM2_CCER_CC2E | /* Enable CC1/CC2 */
               TIM2_CCER_CC4E; /* Enable CC4 (Output) */


  /*Enable interrupts for CC1/CC2 if input is TRIGGER */
  TIM2->DIER = (config.freq_inputs[0].type == TRIGGER ? TIM2_DIER_CC1IE : 0) |
               (config.freq_inputs[1].type == TRIGGER ? TIM2_DIER_CC2IE : 0);
  NVIC->IPR7 = (1 << 4); /* TODO write a helper */
  nvic_enable_irq(TIM2_IRQ); 

  /* A0 and A1 as trigger inputs */
  GPIOA->MODER &= ~(GPIOA_MODER_MODE0 | GPIOA_MODER_MODE1);
  GPIOA->MODER |= GPIOA_MODER_MODE0_VAL(2) | GPIOA_MODER_MODE1_VAL(2); /* A0/A1 in AF mode */
  GPIOA->AFRL |= GPIOA_AFRL_AFSEL0_VAL(1) | GPIOA_AFRL_AFSEL1_VAL(1); /* AF1 (TIM2)*/
  
  TIM2->CR1 |= TIM2_CR1_CEN; /* Enable clock */
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

static __attribute__((section(".dmadata"))) struct output_buffer output_buffers[2] = { 0 };
static int current_buffer = 0;

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

void dma_str0_isr(void) {
  if ((DMA1->LISR & DMA1_LISR_TCIF0) == DMA1_LISR_TCIF0) {
    DMA1->LIFCR = DMA1_LIFCR_CTCIF0;
    platform_buffer_swap();


    if ((DMA1->S0CR & DMA1_S0CR_CT) != DMA1_S0CR_CT_VAL(current_buffer)) { 
      /* We have overflowed or gone out of sync, abort immediately */
      abort();
    }
  }
  if ((DMA1->LISR & DMA1_LISR_DMEIF0) == DMA1_LISR_DMEIF0) {
    while (1);
  }
  if ((DMA1->LISR & DMA1_LISR_DMEIF0) == DMA1_LISR_DMEIF0) {
    while (1);
  }
}

static void setup_scheduled_outputs(void) {
  RCC->AHB4ENR |= RCC_AHB4ENR_GPIODEN; /*Enable GPIOD Clock */
  RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;

  /* While still in hi-z, set outputs that are active-low */
  for (int i = 0; i < MAX_EVENTS; ++i) {
    if (config.events[i].inverted && config.events[i].type != DISABLED_EVENT) {
      GPIOD->ODR |= (1 << config.events[i].pin);
    }
  }

  GPIOD->MODER = 0x55555555; /* All outputs MODER = 0x01 */
  GPIOD->OSPEEDR = 0xFFFFFFFF; /* All outputs High Speed */

  /* Configure DMA1 Stream 0 */
  DMA1->S0CR = 0; /* Reset */
  DMA1->S0PAR = (uint32_t)&GPIOD->BSRR; /* Peripheral address is GPIOD BSRR */
  DMA1->S0M0AR = (uint32_t)&output_buffers[0].slots; /* Memory address 0 and 1 */
  DMA1->S0M1AR = (uint32_t)&output_buffers[1].slots; 
  DMA1->S0NDTR = NUM_SLOTS;

  DMA1->S0CR = DMA1_S0CR_DBM | /* Double Buffer */
               DMA1_S0CR_PL_VAL(3) | /* High Priority */
               DMA1_S0CR_MSIZE_VAL(2) | /* 32 bit memory size */
               DMA1_S0CR_PSIZE_VAL(2) | /* 32 bit peripheral size */
               DMA1_S0CR_MINC | /* Memory increment after each transfer */
               DMA1_S0CR_DIR_VAL(1) | /* Direction is memory to peripheral */
               DMA1_S0CR_TCIE | /* Interrupt when each transfer complete */
               DMA1_S0CR_DMEIE; /* Interrupt on DMA error */

  /* Configure DMAMUX */
  DMAMUX1->C0CR &= ~(DMAMUX1_C0CR_DMAREQ_ID);
  DMAMUX1->C0CR |= DMAMUX1_C0CR_DMAREQ_ID_VAL(51); /* TIM8 Update */

  nvic_enable_irq(DMA_STR0_IRQ);
  DMA1->S0CR |= DMA1_S0CR_EN; /* Enable */
}

void platform_init_scheduler() {

  /* Set debug unit to stop the timer on halt */
  *((volatile uint32_t *)0xE0042008) |= 29; /*TIM2, TIM5, and TIM7 and */
  *((volatile uint32_t *)0xE004200C) |= 2;  /* TIM8 stop */

  setup_tim2();
  setup_tim8();
  setup_scheduled_outputs();
}
