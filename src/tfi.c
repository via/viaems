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
  {IGNITION_EVENT, 45, 15, {}, {}},
  {IGNITION_EVENT, 135, 15, {}, {}},
  {IGNITION_EVENT, 45, 11, {}, {}},
  {IGNITION_EVENT, 135, 11, {}, {}},
  {IGNITION_EVENT, 45, 10, {}, {}},
  {IGNITION_EVENT, 135, 10, {}, {}},
};

degrees_t adv = 25;

int main() {
  decoder_init(&d);
  platform_init(&d, NULL);
  while (1) {
    if (d.needs_decoding) {
      set_output(12, 1);
      set_output(13, 0);
      d.decode(&d);
      set_output(12, 0);
      /* Calculate Advance */
      /* Plan times for things */
      set_output(14, 1);
      if (d.valid) {
        gpio_toggle(GPIOD, GPIO13);
        if (!schedule_ignition_event(&events[0], &d, 0, 150)) {
          schedule_ignition_event(&events[1], &d, 0, 150);
        }
        gpio_toggle(GPIOD, GPIO13);
        if (!schedule_ignition_event(&events[2], &d, adv+5, 150)) {
          schedule_ignition_event(&events[3], &d, adv+5, 150);
        }
        gpio_toggle(GPIOD, GPIO13);
        if (!schedule_ignition_event(&events[4], &d, adv+10, 150)) {
          schedule_ignition_event(&events[5], &d, adv+10, 150);
        }
        gpio_toggle(GPIOD, GPIO13);
      }
      set_output(14, 0);
    }
  }

  return 0;
}
    
