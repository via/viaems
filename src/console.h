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

struct console_node;

typedef void (*console_node_get)(CborEncoder *encoder, const struct console_node *node, CborValue *path);
typedef void (*console_node_describe)(CborEncoder *encoder, const struct console_node *node);

struct console_node {
  const char *id;
  const char *description;

  /* Leaf types */
  const uint32_t *uint32_ptr;
  const float *float_ptr;

  /* Custom types */
  void *ptr;
  console_node_get get;
  console_node_describe describe;

  /* Container type */
  const struct console_node *children;

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
void console_describe_description(CborEncoder *, const char *desc);
void console_describe_type(CborEncoder *, const char *type);
void console_describe_node(CborEncoder *, const struct console_node *node);

void console_process();

void console_record_event(struct logged_event);

#ifdef UNITTEST
#include <check.h>
TCase *setup_console_tests();
#endif

#endif

