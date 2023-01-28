#ifndef _SIM_H
#define _SIM_H

#include <stdint.h>

void execute_test_trigger(void *_w);
void set_test_trigger_rpm(uint32_t rpm);
uint32_t get_test_trigger_rpm(void);

#endif
