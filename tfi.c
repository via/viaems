#include "platform.h"
#include "util.h"
#include "decoder.h"

struct decoder d;

struct ignition_event {
  timeval_t on;
  timeval_t off;
  int output_id;
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

void do_ignition_event(struct ignition_event *ig, timeval_t start, timeval_t stop) {
  char turn_on = time_in_range(ig->on, start, stop);
  char turn_off = time_in_range(ig->off, start, stop);

  if (turn_on && turn_off) {
    /* This should never happen unless something took way longer than normal */
    while(1); 
  }
  if (turn_on) {
    ig->output_val = 1;
    set_output(ig->output_id, 1);
  }
  if (turn_off) {
    ig->output_val = 0;
    set_output(ig->output_id, 0);
  }

}

int main() {

  platform_init();
  decoder_init(&d);
  timeval_t cur = current_time();
  timeval_t prev = cur;

  struct ignition_event ig1 = {0, 0, 1, 0};
  struct ignition_event tmp;

  while (1) {
    cur = current_time();
    if (d.needs_decoding) {
      d.decode(&d);
      /* Calculate Advance */
      /* Plan times for things */
      tmp.on = cur + time_from_rpm_diff(d.rpm, 20);
      tmp.off = cur + time_from_rpm_diff(d.rpm, 30);
      update_ignition_event(&ig1, &tmp);
    }
    if (decoder_valid(&d)) {
      do_ignition_event(&ig1, prev, cur);
    }
    prev = cur;
  }

  return 0;
}
    
