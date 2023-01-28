#include "config.h"
#include "decoder.h"
#include "platform.h"

#include "stm32_sched_buffers.h"

__attribute__((
  section(".dmadata"))) struct output_buffer output_buffers[2] = { 0 };
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
static bool retire_output_buffer(struct output_buffer *buf) {
  timeval_t offset_from_start;
  bool retired = false;
  for (int i = 0; i < MAX_EVENTS; i++) {
    struct output_event *oev = &config.events[i];

    offset_from_start = oev->start.time - buf->first_time;
    if (sched_entry_get_state(&oev->start) == SCHED_SUBMITTED &&
        offset_from_start < NUM_SLOTS) {
      platform_output_slot_unset(
        buf->slots, offset_from_start, oev->start.pin, oev->start.val);
      sched_entry_set_state(&oev->start, SCHED_FIRED);
      retired = true;
    }

    offset_from_start = oev->stop.time - buf->first_time;
    if (sched_entry_get_state(&oev->stop) == SCHED_SUBMITTED &&
        offset_from_start < NUM_SLOTS) {
      platform_output_slot_unset(
        buf->slots, offset_from_start, oev->stop.pin, oev->stop.val);
      sched_entry_set_state(&oev->stop, SCHED_FIRED);
      retired = true;
    }
  }
  return retired;
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

bool stm32_buffer_swap(void) {
  struct output_buffer *buf = &output_buffers[current_buffer];
  current_buffer = (current_buffer + 1) % 2;

  bool retired = retire_output_buffer(buf);

  buf->first_time = round_time_to_buffer_start(current_time()) + NUM_SLOTS;

  populate_output_buffer(buf);
  return retired;
}
