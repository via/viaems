#ifndef _CONSOLE_H
#define _CONSOLE_H

#define CONSOLE_BUFFER_SIZE 1024

struct console {
  /* Internal */
  volatile unsigned int needs_processing;
  char txbuffer[CONSOLE_BUFFER_SIZE];
  char rxbuffer[CONSOLE_BUFFER_SIZE];
};

void console_init();
void console_process();

#ifdef UNITTEST
#include <check.h>
TCase *setup_console_tests();
#endif

#endif
