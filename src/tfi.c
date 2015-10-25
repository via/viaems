#include "platform.h"
#include "util.h"
#include "decoder.h"
#include "factors.h"

struct decoder d;
struct analog_inputs inputs;

struct ignition_event {
  timeval_t on;
  timeval_t off;
  char output_id;
  char output_val;
};

void update_ignition_event(struct ignition_event *dst, 
                           struct ignition_event *src) {

  /* Special action required if the event is ongoing. For now just don't
   * change it.
   */
  if (dst->output_val) {
    return;
  }
  dst->on = src->on;
  dst->off = src->off;
}

void do_ignition_event(struct ignition_event *ig, timeval_t start, 
                       timeval_t stop, char valid) {
  char turn_on = time_in_range(ig->on, start, stop);
  char turn_off = time_in_range(ig->off, start, stop);

  if (turn_on && turn_off) {
    /* This should never happen unless something took way longer than normal */
    ig->output_val = 0;
    set_output(ig->output_id, 0);
    return;
  }
  if (turn_on && valid) {
    ig->output_val = 1;
    set_output(ig->output_id, 1);
  }
  if (turn_off) {
    ig->output_val = 0;
    set_output(ig->output_id, 0);
  }

}

int main() {

  decoder_init(&d);
  degrees_t adv = 18;
  struct ignition_event ig1 = {
    .on = 0, 
    .off = 0, 
    .output_id = 0, 
    .output_val = 0
  };
  struct ignition_event tmp = {0, 0, 0, 0};
  platform_init(&d, &inputs);
  timeval_t cur = current_time();
  timeval_t prev = cur;

  while (1) {
    if (d.needs_decoding) {
      set_output(2, 1);
      set_output(3, 1);
      d.decode(&d);
      set_output(2, 0);
      /* Calculate Advance */
      /* Plan times for things */
      if (d.rpm < 2500) {
        adv = d.rpm >> 7; /* 0-2500 rpms is 0-20 deg */
      } else {
        adv = 20;
      }
      tmp.on = d.last_trigger_time + time_from_rpm_diff(d.rpm, 45 - adv);
      tmp.off = tmp.on + 10000;
      set_output(3, 0);
      update_ignition_event(&ig1, &tmp);
    }
    cur = current_time();
    do_ignition_event(&ig1, prev, cur, decoder_valid(&d));
    prev = cur;
  }

  return 0;
}
    
