#include "platform.h"
#include "util.h"
#include "decoder.h"
#include "scheduler.h"
#include <libopencm3/stm32/gpio.h>

struct decoder d;

/* For each event, a second one that is a full cycle further should be used to
 * allow for decoder/event timing conflicts.
 */
struct output_event events[] = {
  {IGNITION_EVENT, 0, 14, {}, {}},
  {IGNITION_EVENT, 90, 13, {}, {}},
  {IGNITION_EVENT, 180, 12, {}, {}},
  {IGNITION_EVENT, 270, 11, {}, {}},
  {IGNITION_EVENT, 360, 14, {}, {}},
  {IGNITION_EVENT, 450, 13, {}, {}},
  {IGNITION_EVENT, 540, 12, {}, {}},
  {IGNITION_EVENT, 630, 11, {}, {}},
};

int main() {
  decoder_init(&d);
  platform_init(&d, NULL);
  while (1) {
    if (d.needs_decoding) {
      d.decode(&d);

      if (d.valid) {
      set_output(15, 1);
        for (int e = 0; e < 8; ++e) {
          schedule_ignition_event(&events[e], &d, 15, 300);
        }
      }
      set_output(15, 0);
    }
  }

  return 0;
}
    
