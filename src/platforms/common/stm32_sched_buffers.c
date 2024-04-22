#include "config.h"
#include "decoder.h"
#include "platform.h"

#include "stm32_sched_buffers.h"

__attribute__((
  section(".dmadata"))) struct output_buffer output_buffers[2] = { 
  { 0 },
  { .first_time = NUM_SLOTS },
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
timeval_t platform_output_earliest_schedulable_time() {
  return output_buffers[current_buffer].first_time + NUM_SLOTS * 2;
}

/* Retire all stop/stop events that are in the time range of our "completed"
 * buffer and were previously submitted by setting them to "fired" and clearing
 * out the dma bits.
 * Returns true if any sched_entrys were retired */
void platform_retire_schedule(timeval_t start, timeval_t duration) {
  struct output_buffer *buf = &output_buffers[current_buffer];
  assert(buf->first_time == start);
  assert(duration == NUM_SLOTS);
  
  timeval_t offset_from_start;
  for (int i = 0; i < MAX_EVENTS; i++) {
    struct output_event *oev = &config.events[i];

    offset_from_start = oev->start.time - buf->first_time;
    if (oev->start.state == SCHED_SUBMITTED &&
        offset_from_start < NUM_SLOTS) {
      platform_output_slot_unset(
        buf->slots, offset_from_start, oev->start.pin, oev->start.val);
      oev->start.state = SCHED_FIRED;
    }

    offset_from_start = oev->stop.time - buf->first_time;
    if (oev->stop.state == SCHED_SUBMITTED &&
        offset_from_start < NUM_SLOTS) {
      platform_output_slot_unset(
        buf->slots, offset_from_start, oev->stop.pin, oev->stop.val);
      oev->stop.state = SCHED_FIRED;
    }
  }
}

/* Any scheduled start/stop event in the time range for the new buffer can be
 * "submitted" and the dma bits set */
void platform_submit_schedule(timeval_t start, timeval_t duration) {
  struct output_buffer *buf = &output_buffers[current_buffer];
  buf->first_time += 2 * NUM_SLOTS;
  assert(buf->first_time == start);

  timeval_t offset_from_start;
  for (int i = 0; i < MAX_EVENTS; i++) {
    struct output_event *oev = &config.events[i];
    offset_from_start = oev->start.time - buf->first_time;
    if (oev->start.state == SCHED_SCHEDULED &&
        offset_from_start < NUM_SLOTS) {
      platform_output_slot_set(
        buf->slots, offset_from_start, oev->start.pin, oev->start.val);
      oev->start.state = SCHED_SUBMITTED;
    }

    offset_from_start = oev->stop.time - buf->first_time;
    if (oev->stop.state == SCHED_SCHEDULED &&
        offset_from_start < NUM_SLOTS) {
      platform_output_slot_set(
        buf->slots, offset_from_start, oev->stop.pin, oev->stop.val);
      oev->stop.state = SCHED_SUBMITTED;
    }
  }

  current_buffer = (current_buffer == 0) ? 1 : 0;
}

bool stm32_buffer_swap(void) {
  struct output_buffer *buf = &output_buffers[current_buffer];
  struct engine_pump_update u = {
    .retire_start = buf->first_time,
    .retire_duration = NUM_SLOTS,
    .plan_start = buf->first_time + 2 * NUM_SLOTS,
    .plan_duration = NUM_SLOTS,
  };

  trigger_engine_pump(&u);
}
