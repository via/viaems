#ifndef _CONSOLE_H
#define _CONSOLE_H

#include <cbor.h>

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
  } type;
  uint16_t value;
};

typedef void (*console_field_get)(CborEncoder *encoder, void *ptr);
typedef void (*console_field_describe)(CborEncoder *encoder, void *ptr);

struct console_node;
struct console_field {
  const char *id;
  const char *description;

  /* Fixed types */
  const uint32_t *uint32_ptr;
  const float *float_ptr;
  const struct console_node *node_ptr;

  /* Custom types */
  void *ptr;
  console_field_get get;
  console_field_describe describe;
};

struct console_node {
  const char *id;
  const char *type;

  /* If node is a list, it contains a list of other console_nodes */
  int is_list;
  const struct console_node *elements;
  unsigned int list_size;

  /* Otherwise, just a list of fields */
  const struct console_field fields[32];
};

struct console_feed_node {
  const char *id;
  const char *description;
  const uint32_t *uint32_ptr;
  const float *float_ptr;
};

void console_init();
void console_add_feed_node(struct console_feed_node *);
void console_add_config(struct console_node *);

void console_describe_choices(CborEncoder *, const char *choices[]);
void console_describe_type(CborEncoder *, const char *type);

void console_process();

void console_record_event(struct logged_event);

#ifdef UNITTEST
#include <check.h>
TCase *setup_console_tests();
#endif

#endif
