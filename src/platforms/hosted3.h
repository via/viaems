#ifndef HOSTED3_H
#define HOSTED3_H

#include <platform.h>

typedef enum event_type {
  TRIGGER0,
  TRIGGER0_W_TIME,
  TRIGGER1,
  TRIGGER1_W_TIME,
  SCHEDULED_EVENT,
  OUTPUT_CHANGED,
  GPIO_CHANGED,
} event_type;

struct event {
  event_type type;
  timeval_t time;
  uint16_t values;
};

#endif

