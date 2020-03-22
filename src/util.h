#ifndef UTIL_H
#define UTIL_H

#include "platform.h"

timeval_t time_diff(timeval_t t1, timeval_t t2);
int time_before(timeval_t n, timeval_t x);
timeval_t time_from_us(unsigned int us);
unsigned int rpm_from_time_diff(timeval_t t1, degrees_t degrees);
timeval_t time_from_rpm_diff(unsigned int rpm, degrees_t degrees);
degrees_t degrees_from_time_diff(timeval_t, unsigned int);
int time_in_range(timeval_t val, timeval_t t1, timeval_t t2);
degrees_t clamp_angle(degrees_t, degrees_t);

#ifdef UNITTEST
#include <check.h>
TCase *setup_util_tests();
#endif

#endif
