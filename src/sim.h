#ifndef _SIM_H
#define _SIM_H

#include <stdint.h>

struct decoder;
void sim_wakeup_callback(struct decoder *);
void set_test_trigger_rpm(uint32_t rpm);
uint32_t get_test_trigger_rpm(void);

#endif
