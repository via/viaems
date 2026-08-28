#ifndef RTT_H
#define RTT_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

bool rtt_write(const uint8_t *data, const size_t len);
bool rtt_debug(const char *data, uint32_t W, uint32_t X, uint32_t Y, uint32_t Z);

#endif

