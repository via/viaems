#include "platform.h"
#include "console.h"
#include "config.h"
#include "calculations.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
  CONSOLE_FEED,
  CONSOLE_CMDLINE,
} console_state = CONSOLE_FEED;

static struct table *console_get_table(const char *tname) {
  if (!tname) {
    return NULL;
  }

  if (strncmp(tname, "timing", 6) == 0) {
    return config.timing;
  } else {
    strcpy(config.console.txbuffer, "Unknown table! Valid tables: timing\r\n");
    return NULL;
  }
}

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

static void console_set_twotable_cell() {
  const char *tname;
  struct table *t;
  float xcol, ycol, newval;

  tname = strtok(NULL, " ");
  t = console_get_table(tname);
  if (!t) {
    return;
  }

  if (!console_get_number(&xcol) ||
      !console_get_number(&ycol) ||
      !console_get_number(&newval)) {
    return;
  }

  if (((int)xcol < 0) || (int)xcol >= t->axis[0].num) {
    return;
  }
  if (((int)ycol < 0) || (int)ycol >= t->axis[1].num) {
    return;
  }

  t->data.two[(int)ycol][(int)xcol] = newval;

  sprintf(config.console.txbuffer, "%s(%s=%d, %s=%d) = %f\r\n",
      t->title, t->axis[0].name, t->axis[0].values[(int)xcol],
      t->axis[1].name, t->axis[1].values[(int)ycol],
      t->data.two[(int)ycol][(int)xcol]);
}


static void console_get_twotable_cell() {
  const char *tname;
  const struct table *t;
  float xcol, ycol;

  tname = strtok(NULL, " ");
  t = console_get_table(tname);
  if (!t) {
    return;
  }

  if (!console_get_number(&xcol) ||
      !console_get_number(&ycol)) {
    return;
  }

  if (((int)xcol < 0) || (int)xcol >= t->axis[0].num) {
    return;
  }
  if (((int)ycol < 0) || (int)ycol >= t->axis[1].num) {
    return;
  }

  sprintf(config.console.txbuffer, "%s(%s=%d, %s=%d) = %f\r\n",
      t->title, t->axis[0].name, t->axis[0].values[(int)xcol],
      t->axis[1].name, t->axis[1].values[(int)ycol],
      t->data.two[(int)ycol][(int)xcol]);

}

static void console_get_table_info() {
  const char *tname;
  const struct table *t;
  char fbuf[24];

  tname = strtok(NULL, " ");
  t = console_get_table(tname);
  if (!t) {
    return;
  }

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

static void console_status() {
  snprintf(config.console.txbuffer, CONSOLE_BUFFER_SIZE,
      "* rpm=%d sync=%d t0_count=%d map=%f adv=%f\r\n",
      config.decoder.rpm,
      config.decoder.valid,
      config.decoder.t0_count,
      config.adc[ADC_MAP].processed_value,
      calculated_values.timing_advance);
}


static void console_process_rx() {
  console_state = CONSOLE_CMDLINE;
  const char *c = strtok(config.console.rxbuffer, " ");
  if (c && strncasecmp(c, "feed", 4) == 0) {
    console_state = CONSOLE_FEED;
    usart_rx_reset();
    return;
  }
  if (c && strncasecmp(c, "gettablecell", 12) == 0) {
    console_get_twotable_cell();
  } else if (c && strncasecmp(c, "settablecell", 12) == 0) {
    console_set_twotable_cell();
  } else if (c && strncasecmp(c, "gettable", 8) == 0) {
    console_get_table_info();
  } else if (c && strncasecmp(c, "status", 6) == 0) {
    console_status();
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
