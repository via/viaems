#ifndef UTIL_H
#define UTIL_H

#include "platform.h"

timeval_t time_diff(timeval_t t1, timeval_t t2);
unsigned int rpm_from_time_diff(timeval_t t1, degrees_t degrees);
timeval_t time_from_rpm_diff(unsigned int rpm, degrees_t degrees);
int time_in_range(timeval_t val, timeval_t t1, timeval_t t2);

#endif

