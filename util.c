#include "util.h"
#include "platform.h"
#include "limits.h"

unsigned int rpm_from_time_diff(timeval_t t1, degrees_t deg) {
  unsigned int rpm  = (TICKRATE / 6 * deg) / t1;
  return rpm;
}

timeval_t time_from_rpm_diff(unsigned int rpm, degrees_t deg) {
  timeval_t ticks = (TICKRATE / 6 * deg) / rpm;
  return ticks;
}

int time_in_range(timeval_t val, timeval_t t1, timeval_t t2) {
  return 0;
}

timeval_t time_diff(timeval_t later, timeval_t earlier) {
  if (later > earlier) {
    return later - earlier;
  } else {
    return UINT_MAX - (earlier - later);
  }
}

