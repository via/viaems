#ifndef _CONSOLE_H
#define _CONSOLE_H

#include "platform.h"

#define CONSOLE_BUFFER_SIZE 3072

struct console {
  /* Internal */
  volatile unsigned int needs_processing;
  char txbuffer[CONSOLE_BUFFER_SIZE];
  char rxbuffer[CONSOLE_BUFFER_SIZE];
};

struct logged_event {
  timeval_t time;
  enum {
    EVENT_NONE,
    EVENT_OUTPUT,
    EVENT_GPIO,
    EVENT_TRIGGER0,
    EVENT_TRIGGER1,
    EVENT_TOOTHTIMES,
  } type;
  uint16_t value;
  timeval_t teeth_times[24];
};

void console_init();
void console_process();

void console_record_event(struct logged_event *);

#ifdef UNITTEST
#include <check.h>
TCase *setup_console_tests();
#endif

#endif
