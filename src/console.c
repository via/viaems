/* For strtok_r */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 1
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "console.h"
#include "calculations.h"
#include "config.h"
#include "decoder.h"
#include "platform.h"
#include "sensors.h"
#include "stats.h"


static struct console console;
static struct console_node decoder = {
  .id = "decoder",
  .fields = {
    {.id = "type", .choices = {"cam24+1", "tfi", NULL}, .description = "decoder type"},
    {.id = "offset", .float_ptr=&config.decoder.offset, .description="decoder offset angle from trigger tooth"},
  }
};

static struct console_node console_output_elements[] = {
  {.fields = {
    {.id="inverted", .uint32_ptr=&config.events[0].inverted},
    {.id="angle", .float_ptr=&config.events[0].angle},
             },
  },
};

static struct console_node outputs = {
  .id = "outputs",
  .is_list = 1,
  .list_size = 24,
  .elements = console_output_elements,
};


#define CONSOLE_NODES_MAX 32
static struct console_node console_nodes[CONSOLE_NODES_MAX] = {0};

void console_add_config(struct console_node *node) {
  for (int i = 0; i < CONSOLE_NODES_MAX; i++) {
    if (!console_nodes[i].id) {
      /* This node is unused, set it */
//      console_nodes[i] = *node;
      memcpy(&console_nodes[i], node, sizeof(struct console_node));
      return;
    }
  }
}


uint32_t console_current_time;

#define MAX_CONSOLE_FEED_NODES 128
struct console_feed_node console_feed_nodes[MAX_CONSOLE_FEED_NODES] = {
  {.id="cputime", .uint32_ptr=&console_current_time},
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

void console_record_event(struct logged_event ev) {
}

console_init() {
}
int console_parse_request(char *dest, char *line) {
  (void)dest;
  (void)line;

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
  cbor_encoder_create_array(&top_encoder, &key_list_encoder, CborIndefiniteLength);
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
  cbor_encoder_create_array(&top_encoder, &value_list_encoder, CborIndefiniteLength);
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

struct {
  struct {
    int in_progress;
    size_t max;
    const char *src;
    char *ptr;
  } rx, tx;
} console_state = {
  .tx = { .src = console.txbuffer, .in_progress = 0 },
  .rx = { .src = console.rxbuffer, .in_progress = 0 },
};

int console_read_full(char *buf, size_t max) {
  if (console_state.rx.in_progress) {
    size_t r = console_state.rx.max - 1 -
               (size_t)(console_state.rx.ptr - console_state.rx.src);
    if (r == 0) {
      console_state.rx.in_progress = 0;
      return 0;
    }
    r = console_read(console_state.rx.ptr, r);
    if (r) {
      console_state.rx.ptr += r;
      if (memchr(console_state.rx.src,
                 '\r',
                 (uint16_t)(console_state.rx.ptr - console_state.rx.src)) ||
          memchr(console_state.rx.src,
                 '\n',
                 (uint16_t)(console_state.rx.ptr - console_state.rx.src))) {
        console_state.rx.in_progress = 0;
        return 1;
      }
    }
  } else {
    console_state.rx.in_progress = 1;
    console_state.rx.max = max;
    console_state.rx.src = buf;
    console_state.rx.ptr = buf;

    memset(buf, 0, max);
  }
  return 0;
}

int console_write_full(char *buf, size_t max) {
  if (console_state.tx.in_progress) {
    size_t r = console_state.tx.max -
               (size_t)(console_state.tx.ptr - console_state.tx.src);
    r = console_write(console_state.tx.ptr, r);
    if (r) {
      console_state.tx.ptr += r;
      if (console_state.tx.max ==
          (uint16_t)(console_state.tx.ptr - console_state.tx.src)) {
        console_state.tx.in_progress = 0;
        return 1;
      }
    }
  } else {
    console_state.tx.in_progress = 1;
    console_state.tx.max = max;
    console_state.tx.src = buf;
    console_state.tx.ptr = buf;
  }
  return 0;
}

static void console_process_rx() {
  char *out = console.txbuffer;
  char *in = strtok(console.rxbuffer, "\r\n");
  char *response = out + 2; /* Allow for status character */
  strcpy(response, "");

  if (!in) {
    /* Allow just raw \n's in the case of hosted mode */
    in = strtok(console.rxbuffer, "\n");
  }
  int success = console_parse_request(response, in);
  strcat(response, "\r\n");

  out[0] = success ? '*' : '-';
  out[1] = ' ';
  console_write_full(out, strlen(out));
}

void console_process() {

  static int pending_request = 0;
  stats_start_timing(STATS_CONSOLE_TIME);
  if (!pending_request &&
      console_read_full(console.rxbuffer, CONSOLE_BUFFER_SIZE)) {
    pending_request = 1;
  }

  /* We don't ever want to interrupt a feedline */
  if (pending_request && !console_state.tx.in_progress) {
    console_process_rx();
    pending_request = 0;
  }

  /* We're still sending packets for a tx line, keep doing just that */
  if (console_state.tx.in_progress) {
    console_write_full(console.txbuffer, 0);
    stats_finish_timing(STATS_CONSOLE_TIME);
    return;
  }

  console.txbuffer[0] = '\0';
  static int count = 0;
  if (count == 0) {
    size_t len = console_feed_line_keys((uint8_t*)console.txbuffer, CONSOLE_BUFFER_SIZE);
    console_write_full(console.txbuffer, len);
  } else {
    size_t len = console_feed_line((uint8_t*)console.txbuffer, CONSOLE_BUFFER_SIZE);
    console_write_full(console.txbuffer, len);
  }
  count++;
  if (count == 100) {
    count = 0;
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
