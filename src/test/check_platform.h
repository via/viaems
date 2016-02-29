#ifndef _CHECK_PLATFORM_H
#define _CHECK_PLATFORM_H
#include <check.h>

void check_platform_reset();
void set_current_time(timeval_t);

TCase *setup_scheduler_tests();
TCase *setup_decoder_tests();

#endif

