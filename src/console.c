/* For strtok_r */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 1
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "calculations.h"
#include "config.h"
#include "console.h"
#include "decoder.h"
#include "platform.h"
#include "sensors.h"
#include "stats.h"

static struct console console;

#define CONSOLE_NODES_MAX 32
static struct console_node console_nodes[CONSOLE_NODES_MAX] = { 0 };
static struct console_node console_config = {
  .id = "config",
  .children = &console_nodes[0],
};

void console_add_config(struct console_node *node) {
  for (int i = 0; i < CONSOLE_NODES_MAX; i++) {
    if (!console_nodes[i].id) {
      memcpy(&console_nodes[i], node, sizeof(struct console_node));
      return;
    }
  }
}

uint32_t console_current_time;

#define MAX_CONSOLE_FEED_NODES 128
struct console_feed_node console_feed_nodes[MAX_CONSOLE_FEED_NODES] = {
  { .id = "cputime", .uint32_ptr = &console_current_time },
};

void console_add_feed_node(struct console_feed_node *node) {
  for (int i = 0; i < MAX_CONSOLE_FEED_NODES; i++) {
    if (!console_feed_nodes[i].id) {
      /* This node is unused, set it */
      //      console_nodes[i] = *node;
      memcpy(&console_feed_nodes[i], node, sizeof(struct console_feed_node));
      return;
    }
  }
}

#ifndef GIT_DESCRIBE
#define GIT_DESCRIBE "unknown"
static const char *git_describe = GIT_DESCRIBE;
#endif

void console_record_event(struct logged_event ev) {}

void console_init() {}

int console_parse_request(char *dest, char *line) {
  (void)dest;
  (void)line;
  return 0;
}

void console_describe_type(CborEncoder *enc, const char *type) {
  cbor_encode_text_stringz(enc, "_type");
  cbor_encode_text_stringz(enc, type);
}

void console_describe_description(CborEncoder *enc, const char *description) {
  cbor_encode_text_stringz(enc, "description");
  cbor_encode_text_stringz(enc, description);
}

void console_describe_choices(CborEncoder *enc, const char **choices) {
  cbor_encode_text_stringz(enc, "choices");
  CborEncoder list_encoder;
  cbor_encoder_create_array(enc, &list_encoder, CborIndefiniteLength);
  for (const char **i = &choices[0]; *i != NULL; i++) {
    cbor_encode_text_stringz(&list_encoder, *i);
  }
  cbor_encoder_close_container(enc, &list_encoder);
}

static size_t console_feed_line_keys(uint8_t *dest, size_t bsize) {
  CborEncoder encoder;
  cbor_encoder_init(&encoder, dest, bsize, 0);

  CborEncoder top_encoder;
  cbor_encoder_create_map(&encoder, &top_encoder, 2);
  cbor_encode_text_stringz(&top_encoder, "type");
  cbor_encode_text_stringz(&top_encoder, "description");

  cbor_encode_text_stringz(&top_encoder, "keys");
  CborEncoder key_list_encoder;
  cbor_encoder_create_array(
    &top_encoder, &key_list_encoder, CborIndefiniteLength);
  for (int i = 0; i < MAX_CONSOLE_FEED_NODES; i++) {
    const struct console_feed_node *node = &console_feed_nodes[i];
    if (!node->id) {
      break;
    }
    cbor_encode_text_stringz(&key_list_encoder, node->id);
  }
  cbor_encoder_close_container(&top_encoder, &key_list_encoder);
  cbor_encoder_close_container(&encoder, &top_encoder);
  return cbor_encoder_get_buffer_size(&encoder, dest);
}

static size_t console_feed_line(uint8_t *dest, size_t bsize) {
  CborEncoder encoder;

  console_current_time = current_time();
  cbor_encoder_init(&encoder, dest, bsize, 0);

  CborEncoder top_encoder;
  cbor_encoder_create_map(&encoder, &top_encoder, 2);
  cbor_encode_text_stringz(&top_encoder, "type");
  cbor_encode_text_stringz(&top_encoder, "feed");

  cbor_encode_text_stringz(&top_encoder, "values");
  CborEncoder value_list_encoder;
  cbor_encoder_create_array(
    &top_encoder, &value_list_encoder, CborIndefiniteLength);
  for (int i = 0; i < MAX_CONSOLE_FEED_NODES; i++) {
    const struct console_feed_node *node = &console_feed_nodes[i];
    if (!node->id) {
      break;
    }
    if (node->uint32_ptr) {
      cbor_encode_uint(&value_list_encoder, *node->uint32_ptr);
    } else if (node->float_ptr) {
      cbor_encode_float(&value_list_encoder, *node->float_ptr);
    } else {
      cbor_encode_text_stringz(&value_list_encoder, "invalid");
    }
  }
  cbor_encoder_close_container(&top_encoder, &value_list_encoder);
  cbor_encoder_close_container(&encoder, &top_encoder);
  return cbor_encoder_get_buffer_size(&encoder, dest);
}

int console_write_full(const char *buf, size_t len) {
  size_t remaining = len;
  const char *ptr = buf;
  while (remaining) {
    ssize_t written = console_write(ptr, remaining);
    if (written > 0) {
      remaining -= written;
      ptr += written;
    }
  }
  return 1;
}

static uint8_t rx_buffer[4096];
static uint8_t *rx_buffer_ptr = rx_buffer;

static void console_shift_rx_buffer(size_t amt) {
  memmove(&rx_buffer[0], &rx_buffer[amt], (rx_buffer_ptr - rx_buffer));
  rx_buffer_ptr -= amt;
}

static size_t console_try_read() {
  size_t remaining = sizeof(rx_buffer) - (rx_buffer_ptr - rx_buffer);

  size_t read_amt = console_read(rx_buffer_ptr, remaining);

  rx_buffer_ptr += read_amt;

  size_t len = rx_buffer_ptr - rx_buffer;
  if (len == 0) {
    return 0;
  }

  CborParser parser;
  CborValue value;
  
  /* We want to determine if we have a valid cbor object in buffer.  If the
   * buffer doesn't start with a map, it is not valid, and we want to advance
   * byte-by-byte until we find a start of map */
  do {
    if (cbor_parser_init(rx_buffer, len, 0, &parser, &value) == CborNoError) {
      if (cbor_value_is_map(&value)) {
        break;
      }
    }
    console_shift_rx_buffer(1);

    /* We've exhausted the buffer */
    if (rx_buffer_ptr == rx_buffer) {
      return 0;
    }
  } while(1);

  /* Maybe we have a valid object. if so, return.
   * If we don't, it could be that we haven't read enough. We will read until
   * the end of the buffer, but if its maxed out, reset everything
   */
  if (cbor_value_validate_basic(&value) == CborNoError) {
    cbor_value_advance(&value);
    const uint8_t *next = cbor_value_get_next_byte(&value);
    return (next - rx_buffer);
  }

  if (len == sizeof(rx_buffer)) {
    rx_buffer_ptr = rx_buffer;
  }
  return 0;
}

static void report_success(CborEncoder *enc, bool success) {
  cbor_encode_text_stringz(enc, "success");
  cbor_encode_boolean(enc, success);
}

void report_cbor_parsing_error(CborEncoder *enc, const char *msg, CborError err) {
  char txtbuf[256];
  snprintf(txtbuf, sizeof(txtbuf), "%s: %s", msg, cbor_error_string(err));
  cbor_encode_text_stringz(enc, "error");
  cbor_encode_text_stringz(enc, txtbuf);
  report_success(enc, false);
}

void report_parsing_error(CborEncoder *enc, const char *msg) {
  cbor_encode_text_stringz(enc, "error");
  cbor_encode_text_stringz(enc, msg);
  report_success(enc, false);
}

static void console_request_ping(CborEncoder *enc) {
  cbor_encode_text_stringz(enc, "response");
  cbor_encode_text_stringz(enc, "pong");
  report_success(enc, true);
}

static void describe_primitive_config_node(CborEncoder *enc, const struct console_node *node) {
  CborEncoder map_encoder;
  cbor_encoder_create_map(enc, &map_encoder, CborIndefiniteLength);
  if (node->description) {
    cbor_encode_text_stringz(&map_encoder, "description");
    cbor_encode_text_stringz(&map_encoder, node->description);
  }
  if (node->uint32_ptr) {
    console_describe_type(&map_encoder, "uint32");
  }
  if (node->float_ptr) {
    console_describe_type(&map_encoder, "float");
  }
  cbor_encoder_close_container(enc, &map_encoder);
}

static void describe_config_node(CborEncoder *enc, const struct console_node *node) {
  cbor_encode_text_stringz(enc, node->id);

  if (node->describe) {
    /* Describe function override takes precedence */
    node->describe(enc, node);
  } else if (node->children) {
    /* Recurse to children */
    CborEncoder child_encoder;
    cbor_encoder_create_map(enc, &child_encoder, CborIndefiniteLength);
    for (const struct console_node *child = node->children; child->id != NULL; child++) {
      describe_config_node(&child_encoder, child);
    }
    cbor_encoder_close_container(enc, &child_encoder);
  } else {
    describe_primitive_config_node(enc, node);
  }
}

static void console_request_structure(CborEncoder *enc) {
  CborEncoder response;

  cbor_encode_text_stringz(enc, "response");
  cbor_encoder_create_map(enc, &response, 1);

  describe_config_node(&response, &console_config);

  CborEncoder types;
  cbor_encode_text_stringz(&response, "types");
  cbor_encoder_create_map(&response, &types, CborIndefiniteLength);
  
  cbor_encoder_close_container(&response, &types);

  cbor_encoder_close_container(enc, &response);
  report_success(enc, true);
}

bool get_config_node(CborEncoder *enc, const struct console_node *node, CborValue *path) {
  CborError err;

  if (node->get) {
    /* Get function override takes precedence */
    node->get(enc, node, path);
  } else if (!cbor_value_at_end(path)) {
    /* We have a path we need to descend down */
    for (const struct console_node *child = node->children; child->id != NULL; child++) {
      bool match;
      if ((err = cbor_value_text_string_equals(path, child->id, &match)) != CborNoError) {
        cbor_encode_null(enc);
        return false;
      }
      if (match) {
        cbor_value_advance(path);
        return get_config_node(enc, child, path);
      }
    }
    /* If we're still here we did not find a match */
    cbor_encode_null(enc);
    return false;
  } else if (node->children) {
    CborEncoder child_encoder;
    cbor_encoder_create_map(enc, &child_encoder, CborIndefiniteLength);
    for (const struct console_node *child = node->children; child->id != NULL; child++) {
      cbor_encode_text_stringz(&child_encoder, child->id);
      get_config_node(&child_encoder, child, path);
    }
    cbor_encoder_close_container(enc, &child_encoder);
  } else if (node->uint32_ptr) {
    cbor_encode_int(enc, *node->uint32_ptr);
  } else if (node->float_ptr) {
    cbor_encode_float(enc, *node->float_ptr);
  }
  return true;
}

static void console_request_get(CborEncoder *enc, CborValue *value) {
  CborValue path;
  CborValue pathlist;
  CborError err;
  err = cbor_value_map_find_value(value, "path", &path);
  if (err) {
    report_cbor_parsing_error(enc, "request has no 'path'", err);
    return;
  }
  if (!cbor_value_is_array(&path)) {
    report_parsing_error(enc, "request 'path' not an array");
    return;
  }
  err = cbor_value_enter_container(&path, &pathlist);
  if (err) {
    report_cbor_parsing_error(enc, "could not read 'path'", err);
    return;
  }

  cbor_encode_text_stringz(enc, "response");
  if (!get_config_node(enc, &console_config, &pathlist)) {
    report_parsing_error(enc, "unable to complete request");
    return;
  }
}


static void console_process_request(int len) {
  CborParser parser;
  CborValue value;
  CborError err;
  int req_id = -1;

  uint8_t response[4096];
  CborEncoder encoder;
  CborEncoder response_map;

  cbor_encoder_init(&encoder, response, sizeof(response), 0);

  if (cbor_encoder_create_map(&encoder, &response_map, CborIndefiniteLength) != CborNoError) {
    return;
  }
  if (cbor_encode_text_stringz(&response_map, "type")) {
    return;
  }
  if (cbor_encode_text_stringz(&response_map, "response")) {
    return;
  }

  err = cbor_parser_init(rx_buffer, len, 0, &parser, &value);
  if (err) {
    return;
  }

  CborValue request_id_value;
  if (!cbor_value_map_find_value(&value, "id", &request_id_value)) {
    if (cbor_value_is_integer(&request_id_value)) {
      cbor_value_get_int(&request_id_value, &req_id);
    }
  }
  if (cbor_encode_text_stringz(&response_map, "id")) {
    return;
  }
  if (cbor_encode_int(&response_map, req_id)) {
    return;
  }

  CborValue request_method_value;
  err = cbor_value_map_find_value(&value, "method", &request_method_value);
  if (cbor_value_get_type(&request_method_value) == CborInvalidType) {
    report_cbor_parsing_error(&response_map, "request has no 'method'", err);
  } else {
    bool match;
    cbor_value_text_string_equals(&request_method_value, "ping", &match);
    if (match) {
      console_request_ping(&response_map);
    }

    cbor_value_text_string_equals(&request_method_value, "structure", &match);
    if (match) {
      console_request_structure(&response_map);
    }
    
    cbor_value_text_string_equals(&request_method_value, "get", &match);
    if (match) {
      console_request_get(&response_map, &value);
    }
  }

  if (cbor_encoder_close_container(&encoder, &response_map)) {
    return;
  }

  size_t write_size = cbor_encoder_get_buffer_size(&encoder, response);
  console_write_full(response, write_size);
}

void console_process() {
  static timeval_t last_desc_time = 0;

  size_t read_size;
  if ((read_size = console_try_read())) {
    /* Parse a request from the client */
    console_process_request(read_size);
    console_shift_rx_buffer(read_size);
    
  }
    
  /* Has it been 100ms since the last description? */
  if (time_diff(current_time(), last_desc_time) > time_from_us(100000)) {
    /* If so, print a description message */
    size_t len =
      console_feed_line_keys((uint8_t *)console.txbuffer, CONSOLE_BUFFER_SIZE);
    console_write_full(console.txbuffer, len);
    last_desc_time = current_time();
  } else {
    /* Otherwise a feed message */
    size_t len =
      console_feed_line((uint8_t *)console.txbuffer, CONSOLE_BUFFER_SIZE);
    console_write_full(console.txbuffer, len);
  }
  stats_finish_timing(STATS_CONSOLE_TIME);
}

#ifdef UNITTEST
#include <check.h>

TCase *setup_console_tests() {
  TCase *console_tests = tcase_create("console");
  return console_tests;
}

#endif
