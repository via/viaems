#include "config.h"
#include "decoder.h"
#include "platform.h"
#include "scheduler.h"
#include "sensors.h"

#include "stm32_sched_buffers.h"
#include "viaems.h"

__attribute__(( section(".dmadata"))) 
struct output_buffer output_buffers[2] = { 
  [0] = { .plan = {
    .schedulable_start = 0,
    .schedulable_end = NUM_SLOTS - 1,
  }},
  [1] = { .plan = { 
    .schedulable_start = NUM_SLOTS,
    .schedulable_end = NUM_SLOTS * 2 - 1,
  }},
};
static uint32_t current_buffer = 0;

uint32_t stm32_current_buffer() {
  return current_buffer;
}

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

/* Retire all stop/stop events that are in the time range of our "completed"
 * buffer and were previously submitted by setting them to "fired" and clearing
 * out the dma bits */
static void retire_output_buffer(struct output_buffer *buf) {
  for (int i = 0; i < buf->plan.n_events; i++) {
    struct schedule_entry *entry = buf->plan.schedule[i];

    timeval_t offset_from_start = entry->time - buf->plan.schedulable_start;
    platform_output_slot_unset(
      buf->slots, offset_from_start, entry->pin, entry->val);
    entry->state = SCHED_FIRED;
  }
}

/* Any scheduled start/stop event in the time range for the new buffer can be
 * "submitted" and the dma bits set */
static void populate_output_buffer(struct output_buffer *buf) {
  for (int i = 0; i < buf->plan.n_events; i++) {
    struct schedule_entry *entry = buf->plan.schedule[i];

    timeval_t offset_from_start = entry->time - buf->plan.schedulable_start;
    platform_output_slot_set(
      buf->slots, offset_from_start, entry->pin, entry->val);
    entry->state = SCHED_SUBMITTED;
  }
}

void set_gpio_port(uint32_t value);
void set_pwm(int output, float value);

void stm32_buffer_swap(struct viaems *viaems,
                       const struct engine_update *update) {
  struct output_buffer *buf = &output_buffers[current_buffer];
  current_buffer = (current_buffer + 1) % 2;

  retire_output_buffer(buf);

  buf->plan.schedulable_start += NUM_SLOTS * 2;
  buf->plan.schedulable_end += NUM_SLOTS * 2;
  buf->plan.n_events = 0;

  viaems_reschedule(viaems, update, &buf->plan);

  // TODO this is setting gpios planned for the future, when it could use the
  // in-flight one to be more precise
  set_gpio_port(buf->plan.gpio);

  for (int i = 0; i < MAX_PWM; i++) {
    set_pwm(i, buf->plan.pwm[i]);
  }

  populate_output_buffer(buf);
}
