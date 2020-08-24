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

typedef enum {
  CONSOLE_GET,
  CONSOLE_SET,
  CONSOLE_DESCRIBE,
  CONSOLE_STRUCTURE,
} console_request_type;

struct console_request_context {
  console_request_type type;
  CborEncoder *response;
  CborValue *path;

  bool is_filtered;
  bool is_completed;
};

typedef void (*console_renderer)(struct console_request_context *ctx,
                                 void *ptr);

/* High level rendering helpers */
void render_uint32_map_field(struct console_request_context *ctx,
                             const char *id,
                             const char *description,
                             uint32_t *ptr);

void render_float_map_field(struct console_request_context *ctx,
                            const char *id,
                            const char *description,
                            float *ptr);

void render_map_map_field(struct console_request_context *ctx,
                          const char *id,
                          console_renderer map_renderer,
                          void *ptr);

void render_custom_map_field(struct console_request_context *ctx,
                             const char *id,
                             console_renderer field_renderer,
                             void *ptr);

void render_array_map_field(struct console_request_context *ctx,
                            const char *id,
                            console_renderer array_renderer,
                            void *ptr);

void render_map_array_field(struct console_request_context *ctx,
                            int index,
                            console_renderer map_renderer,
                            void *ptr);

/* Low level object rendering */
void render_uint32_object(struct console_request_context *ctx,
                          const char *description,
                          uint32_t *ptr);
void render_float_object(struct console_request_context *ctx,
                         const char *description,
                         float *ptr);

void render_map_object(struct console_request_context *ctx,
                       console_renderer map_renderer,
                       void *ptr);
void render_array_object(struct console_request_context *ctx,
                         console_renderer array_renderer,
                         void *ptr);

bool descend_map_field(struct console_request_context *ctx, const char *id);
bool descend_array_field(struct console_request_context *ctx, int index);

struct console_feed_node {
  const char *id;
  const char *description;
  const uint32_t *uint32_ptr;
  const float *float_ptr;
};

void console_init();
void console_add_feed_node(struct console_feed_node *);

void console_describe_choices(CborEncoder *, const char *choices[]);
void render_description_field(CborEncoder *, const char *desc);
void render_type_field(CborEncoder *, const char *type);

void console_process();

void console_record_event(struct logged_event);

#ifdef UNITTEST
#include <check.h>
TCase *setup_console_tests();
#endif

#endif
