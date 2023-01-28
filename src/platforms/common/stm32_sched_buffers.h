#ifndef _COMMON_STM32_SCHED_BUFFERS_H
#define _COMMON_STM32_SCHED_BUFFERS_H

#include "platform.h"
#include <stdint.h>
#define NUM_SLOTS 128

/* Implement STM32 and GD32 support for output buffer handling.  Currently this
 * includes the STM32F4, STM32H7, and GD32F4.
 */

/* GPIO BSRR is 16 bits of "on" mask and 16 bits of "off" mask */
struct output_slot {
  uint16_t on;
  uint16_t off;
} __attribute__((packed)) __attribute((aligned(4)));

struct output_buffer {
  timeval_t first_time; /* First time represented by the range */
  struct output_slot slots[NUM_SLOTS];
};

extern struct output_buffer output_buffers[2];

/* Perform a buffer swap for STM32-like hardware.  First find all sched_entryS
 * that have fired in the most recently completed buffer and mark them as fired.
 * Then find all the sched_entryS that are to be scheduled and set up the next
 * DMA buffer for them.
 */
bool stm32_buffer_swap(void);

/* Returns the current buffer being used for outputs */
uint32_t stm32_current_buffer(void);

#endif
