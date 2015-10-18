#include "platform.h"
#include "util.h"
#include "decoder.h"

struct decoder d;

int main() {

  platform_init();
  decoder_init(&d);
  timeval_t cur = current_time();
  timeval_t prev = cur;
  timeval_t ig_on = 0;
  timeval_t ig_off = 0;

  while (1) {
    cur = current_time();
    if (d.needs_decoding) {
      d.decode(&d);
      /* Calculate Advance */
      /* Plan times for things */
      ig_on = cur + time_from_rpm_diff(d.rpm, 20);
      ig_off = cur + time_from_rpm_diff(d.rpm, 30);
    }
    if (decoder_valid(&d)) {
      if (time_in_range(ig_on, prev, cur)) {
        set_output(1, 1);
      }
      if (time_in_range(ig_off, prev, cur)) {
        set_output(1, 0);
      }
    }
    prev = cur;
  }

  return 0;
}
    
