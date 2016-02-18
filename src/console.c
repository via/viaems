#include "platform.h"
#include "console.h"
#include "config.h"
#include "calculations.h"
#include "decoder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

enum {
  CONSOLE_FEED,
  CONSOLE_CMDLINE,
} console_state = CONSOLE_FEED;

typedef enum {
  CONFIG_FLOAT,
  CONFIG_UINT,
  CONFIG_TABLE,
} config_type;

struct console_var {
  char name[32];
  union {
    unsigned int *uint_v;
    float *float_v;
    struct table *table_v;
  } value;
  config_type type;
};
static struct console_var vars[] = {
  {.name="config.timing", .type=CONFIG_TABLE, .value={.table_v=&timing_vs_rpm_and_map}},
  {.name="config.rpm_stop", .type=CONFIG_UINT, .value={.uint_v=&config.rpm_stop}},
  {.name="config.offset", .type=CONFIG_UINT, .value={.uint_v=(unsigned int*)&config.decoder.offset}},
  {.name="config.rpm_start", .type=CONFIG_UINT, .value={.uint_v=&config.rpm_start}},
};

static int console_get_number(float *val) {
  const char *str;
  float num;

  str = strtok(NULL, " ");
  if (!str) {
    return 0;
  }

  num = strtof(str, NULL);
  *val = num;
  return 1;
}

static struct console_var *
get_console_var(const char *var) {
  for (unsigned int i = 0; i < sizeof(vars) / sizeof(struct console_var); ++i) {
    if (strncmp(var, vars[i].name, strlen(vars[i].name)) == 0) {
      return &vars[i];
    }
  }
  return NULL;
}

static void handle_get_uint(struct console_var *var) {
  sprintf(config.console.txbuffer, "%s=%d\r\n", var->name, *var->value.uint_v);
}

static void handle_set_uint(struct console_var *var) {
  float v;

  if (!console_get_number(&v)) {
    return;
  }
  *var->value.uint_v = (int)v;
  sprintf(config.console.txbuffer, "%s=%d\r\n", var->name, *var->value.uint_v);
}

static void handle_get_float(struct console_var *var) {
  sprintf(config.console.txbuffer, "%s=%f\r\n", var->name, *var->value.float_v);
}

static void handle_set_float(struct console_var *var) {
  float v;

  if (!console_get_number(&v)) {
    return;
  }
  *var->value.float_v = v;
  sprintf(config.console.txbuffer, "%s=%f\r\n", var->name, *var->value.float_v);
}

static float *get_table_element(char *locstr, struct console_var *var) {
  struct table *t = var->value.table_v;
  unsigned int x, y, res;
  res = sscanf(strtok(NULL, " "), "%d][%d]", &x, &y);
  if (t->num_axis != res) {
    return NULL;
  }
  switch (t->num_axis) {
    case 1:
      if (x >= t->axis[0].num) {
        return NULL;
      }
      sprintf(locstr, "%s(%s=%d)",
          t->title, t->axis[0].name, t->axis[0].values[(int)x]);
      return &t->data.one[x];
      break;
    case 2:
      if ((x >= t->axis[0].num) || (y >= t->axis[1].num)) {
        return NULL;
      }
      sprintf(locstr, "%s(%s=%d, %s=%d)",
          t->title, t->axis[0].name, t->axis[0].values[(int)x],
          t->axis[1].name, t->axis[1].values[(int)y]);
      return &t->data.two[y][x];
      break;
  }
  return NULL;
}

static void handle_get_table_info(struct console_var *var) {
  struct table *t;
  char fbuf[24];

  t = var->value.table_v;
  sprintf(config.console.txbuffer, "title: %s, num_axis: %d\r\n", 
      t->title, t->num_axis);
  if (t->num_axis == 2) {
    strcat(config.console.txbuffer, t->axis[1].name);
    strcat(config.console.txbuffer, ": [ ");
    for (int i = 0; i < t->axis[1].num; ++i) {
      snprintf(fbuf, 24, "%d ", t->axis[1].values[i]);
      strcat(config.console.txbuffer, fbuf);
    }
    strcat(config.console.txbuffer, "]\r\n");
  }
  strcat(config.console.txbuffer, t->axis[0].name);
    strcat(config.console.txbuffer, ": [ ");
  for (int i = 0; i < t->axis[0].num; ++i) {
    snprintf(fbuf, 24, "%d ", t->axis[0].values[i]);
    strcat(config.console.txbuffer, fbuf);
  }
  strcat(config.console.txbuffer, "]\r\n");
}

static void handle_get_table(struct console_var *var) {
  char locstr[64];
  float *val;

  val = get_table_element(locstr, var);
  if (!val) {
    return handle_get_table_info(var);
  }
  sprintf(config.console.txbuffer, "%s = %f\r\n",
      locstr, *val);
}

static void handle_set_table(struct console_var *var) {
  char locstr[64];
  float *val;
  float newval;

  val = get_table_element(locstr, var);
  if (!val) {
    return;
  }

  if (!console_get_number(&newval)) {
    return;
  }

  *val = newval;
  sprintf(config.console.txbuffer, "%s = %f\r\n",
      locstr, *val);
}

void console_set_test_trigger() {
  float rpm;
  if (!console_get_number(&rpm)) {
    return;
  }
  enable_test_trigger(FORD_TFI, (unsigned int)rpm);
}

static void console_status() {
  snprintf(config.console.txbuffer, CONSOLE_BUFFER_SIZE,
      "* rpm=%d sync=%d variance=%1.3f t0_count=%d map=%3.1f adv=%2.1f dwell_us=%d\r\n",
      config.decoder.rpm,
      config.decoder.valid,
      config.decoder.trigger_cur_rpm_change,
      config.decoder.t0_count,
      config.sensors[SENSOR_MAP].processed_value,
      calculated_values.timing_advance,
      calculated_values.dwell_us);
}


static void console_process_rx() {
  console_state = CONSOLE_CMDLINE;
  const char *c;
  struct console_var *v;

  c = strtok(config.console.rxbuffer, " ");
  if (c && strncasecmp(c, "feed", 4) == 0) {
    console_state = CONSOLE_FEED;
    usart_rx_reset();
    return;
  }
  if (c && strncasecmp(c, "get", 3) == 0) {
    v = get_console_var(strtok(NULL, " ["));
    switch(v->type) {
      case CONFIG_UINT:
        handle_get_uint(v);
        break;
      case CONFIG_FLOAT:
        handle_get_float(v);
        break;
      case CONFIG_TABLE:
        handle_get_table(v);
        break;
    }
  } else if (c && strncasecmp(c, "set", 3) == 0) {
    v = get_console_var(strtok(NULL, " ["));
    switch(v->type) {
      case CONFIG_UINT:
        handle_set_uint(v);
        break;
      case CONFIG_FLOAT:
        handle_set_float(v);
        break;
      case CONFIG_TABLE:
        handle_set_table(v);
        break;
    }
  } else if (c && strncasecmp(c, "status", 6) == 0) {
    console_status();
  } else if (c && strncasecmp(c, "testtrigger", 11) == 0) {
    console_set_test_trigger();
  } else {
    strcpy(config.console.txbuffer, "\r\n");
  }
  strcat(config.console.txbuffer, "# ");
  usart_tx(config.console.txbuffer, strlen(config.console.txbuffer));
  usart_rx_reset();

}

void console_process() {

  if (usart_rx_ready()) {
    console_process_rx();
  }

  /*  if tx buffer is usable, print regular status update */
  if (!usart_tx_ready()) {
    return;
  }

  if (console_state == CONSOLE_FEED) {
    console_status();
    usart_tx(config.console.txbuffer, strlen(config.console.txbuffer));
  }


}
