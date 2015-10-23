#include "platform.h"
#include "util.h"
#include "decoder.h"

struct decoder d;

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
    while(1); 
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
  platform_init(&d);
  timeval_t cur = current_time();
  timeval_t prev = cur;
  degrees_t adv = 15;

  struct ignition_event ig1 = {
    .on = 0, 
    .off = 0, 
    .output_id = 0, 
    .output_val = 0
  };
  struct ignition_event tmp;

  while (1) {
    set_output(1, 1);
    if (d.needs_decoding) {
      set_output(1, 0);
      set_output(2, 1);
      set_output(3, 1);
      d.decode(&d);
      set_output(2, 0);
      /* Calculate Advance */
      /* Plan times for things */
      tmp.on = d.last_trigger_time + time_from_rpm_diff(d.rpm, 45 - adv);
      tmp.off = tmp.on + 16000; /* 1 ms pulse */
      set_output(3, 0);
      update_ignition_event(&ig1, &tmp);
    }
    cur = current_time();
    do_ignition_event(&ig1, prev, cur, decoder_valid(&d));
    prev = cur;
  }

  return 0;
}
    
