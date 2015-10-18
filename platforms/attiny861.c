#include "platform.h"
#include "limits.h"

static unsigned int time_high = 0;

void disable_interrupts() {
}

void enable_interrupts() {
}

void platform_init() {
}

static void timer_wrap() {
  time_high++;
}

timeval_t current_time() {
  return ((timeval_t)time_high << 16) + 0;
}

void set_output(int output, char value) {
}

