#include "util.h"
#include "platform.h"
#include "limits.h"

unsigned int rpm_from_time_diff(timeval_t t1, degrees_t deg) {
  timeval_t ticks_per_degree = t1 / deg;
  unsigned int rpm  = (TICKRATE / 6) / ticks_per_degree;
  return rpm;
}

timeval_t time_from_rpm_diff(unsigned int rpm, degrees_t deg) {
  timeval_t ticks_per_degree = (TICKRATE / 6) / rpm;
  return ticks_per_degree * deg;
}

timeval_t time_from_us(unsigned int us) {
  timeval_t ticks = us * (TICKRATE / 1000000);
  return ticks;
}

/* True if n is before x */
int time_before(timeval_t n, timeval_t x) {
  signed int res = n - x;
  return (res < 0);
}

int time_in_range(timeval_t val, timeval_t t1, timeval_t t2) {
  if (t2 >= t1) {
    /* No timer wrap */
    if ((val >= t1) && (val <= t2)) {
      return 1;
    }
  } else {
    if ((val >= t1) || (val < t2)) {
      return 1;
    }
  }
  return 0;
}
timeval_t time_diff(timeval_t later, timeval_t earlier) {
  return later - earlier;
}

degrees_t clamp_angle(int ang, degrees_t max) {
  while (ang < 0) {
    ang += max;
  }
  while (ang >= max) {
    ang -= max;
  }
  return (degrees_t) ang;
}
