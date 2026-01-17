
#ifndef STREAM_H
#define STREAM_H

#include <stdbool.h>
#include <stdint.h>

#include "platform.h"
#include "crc.h"

/* The functions exported from stream.c implement the platform message api prototyped
 * in platform.h for platforms that do not natively have one, e.g. USB or serial console.  
 *
 * When native messaging is used, these implementations are not used.
 */


#ifdef UNITTEST
#include <check.h>
TCase *setup_stream_tests(void);
#endif

#endif
