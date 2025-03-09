#ifndef _CONSOLE_H
#define _CONSOLE_H

#include "platform.h"
#include "decoder.h"
#include "sim.h"

#include "interface/types.pb.h"

struct logged_event {
  timeval_t time;
  uint32_t seq;
  enum {
    EVENT_NONE,
    EVENT_OUTPUT,
    EVENT_GPIO,
    EVENT_TRIGGER,
  } type;
  uint32_t value;
};

void console_process(void);
void console_record_event(struct logged_event);

void console_publish_trigger_update(const struct decoder_event *ev);
void console_publish_engine_update();

#ifdef UNITTEST
#include <check.h>
TCase *setup_console_tests(void);
#endif

#endif
