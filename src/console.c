/* For strtok_r */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 1
#endif

#include "console.h"
#include "calculations.h"
#include "config.h"
#include "decoder.h"
#include "platform.h"
#include "sensors.h"
#include "stats.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static struct console console;

struct console_config_node {
  const char *name;
  void (*get)(const struct console_config_node *self,
              char *dest,
              char *remaining);
  void (*set)(const struct console_config_node *self, char *remaining);
  void *val;
};

static const struct console_config_node *console_search_node(
  const struct console_config_node *nodes,
  const char *var) {

  if (!var) {
    return NULL;
  }
  const struct console_config_node *n;
  for (n = nodes; n->name; n++) {
    if (!strcmp(n->name, var)) {
      return n;
    }
  }
  return NULL;
}

static struct console_feed_config {
  size_t n_nodes;
  const struct console_config_node *nodes[64];
} console_feed_config = {
  .n_nodes = 0,
  .nodes = { 0 },
};

static struct console_config_node console_config_nodes[];
static void console_set_feed(const struct console_config_node *self,
                             char *remaining) {
  assert(self);
  assert(self->val);

  unsigned int cur_entry = 0;
  struct console_feed_config *c = self->val;
  const char *cur_var = strtok(remaining, ",");
  while (cur_var) {
    const struct console_config_node *n =
      console_search_node(console_config_nodes, cur_var);
    if (n) {
      c->nodes[cur_entry] = n;
      cur_entry++;
    }
    cur_var = strtok(NULL, ",");
  }
  c->n_nodes = cur_entry;
}

static void console_get_feed(const struct console_config_node *self,
                             char *dest,
                             char *remaining __attribute__((unused))) {
  assert(self);
  assert(self->val);

  struct console_feed_config *c = self->val;
  unsigned int i;
  if (!c->n_nodes) {
    strcat(dest, "");
    return;
  }
  for (i = 0; i < c->n_nodes - 1; i++) {
    strcat(dest, c->nodes[i]->name);
    strcat(dest, ",");
  }
  strcat(dest, c->nodes[i]->name);
}

static void console_get_time(const struct console_config_node *self
                             __attribute__((unused)),
                             char *dest,
                             char *remaining __attribute__((unused))) {
  sprintf(dest, "%u", (unsigned int)current_time());
}

static void console_get_float(const struct console_config_node *self,
                              char *dest,
                              char *remaining __attribute__((unused))) {
  float *v = self->val;
  sprintf(dest, "%.4f", *v);
}

static void console_set_float(const struct console_config_node *self,
                              char *remaining) {
  float *v = self->val;
  *v = atof(remaining);
}

static void console_get_uint(const struct console_config_node *self,
                             char *dest,
                             char *remaining __attribute__((unused))) {
  unsigned int *v = self->val;
  sprintf(dest, "%u", *v);
}

static void console_set_uint(const struct console_config_node *self,
                             char *remaining) {
  unsigned int *v = self->val;
  *v = (unsigned int)atoi(remaining);
}

static void console_get_fuel_cut(const struct console_config_node *self
                                 __attribute__((unused)),
                                 char *dest,
                                 char *remaining __attribute__((unused))) {
  sprintf(dest, "%u", fuel_cut());
}

static void console_get_ignition_cut(const struct console_config_node *self
                                     __attribute__((unused)),
                                     char *dest,
                                     char *remaining __attribute__((unused))) {
  sprintf(dest, "%u", ignition_cut());
}

static int parse_keyval_pair(char **key, char **val, char **str) {
  char *saveptr;
  *key = strtok_r(*str, "=", &saveptr);
  if (!*key) {
    return 0;
  }
  *val = strtok_r(NULL, " ", &saveptr);
  if (!*val || !strlen(*val)) {
    return 0;
  }
  *str = saveptr;
  return 1;
}

static int console_set_table_element(struct table *t, const char *k, float v) {
  unsigned int row, col;
  int n_parsed = sscanf(k, "[%d][%d]", &row, &col);
  if ((t->num_axis == 1) && (n_parsed == 1) && (row < t->axis[0].num)) {
    t->data.one[row] = v;
    return 1;
  } else if ((t->num_axis == 2) && (n_parsed == 2) && (row < t->axis[1].num) &&
             (col < t->axis[0].num)) {
    t->data.two[row][col] = v;
    return 1;
  }
  return 0;
}

static int console_get_table_element(const struct table *t,
                                     const char *k,
                                     float *v) {
  int row, col;
  int n_parsed = sscanf(k, "[%d][%d]", &row, &col);
  if ((t->num_axis == 1) && (n_parsed == 1) && (row < t->axis[0].num)) {
    *v = t->data.one[row];
    return 1;
  } else if ((t->num_axis == 2) && (n_parsed == 2) && (row < t->axis[1].num) &&
             (col < t->axis[0].num)) {
    *v = t->data.two[row][col];
    return 1;
  }
  return 0;
}

static void console_set_table_axis_labels(struct table_axis *t, char *list) {
  unsigned int cur = 0;
  if ((list[0] != '[') || (list[strlen(list) - 1] != ']')) {
    return;
  }

  char *saveptr;
  char *curlabel = list;
  curlabel = strtok_r(list, "[,]", &saveptr);
  while (curlabel) {
    if ((cur >= t->num) || (cur >= MAX_AXIS_SIZE)) {
      return;
    }
    t->values[cur] = atof(curlabel);
    curlabel = strtok_r(NULL, ",]", &saveptr);
    cur++;
  }
}

static void console_set_table(const struct console_config_node *self,
                              char *remaining) {
  assert(self);
  assert(self->val);
  assert(remaining);

  struct table *t = self->val;

  char *k, *v;
  while (parse_keyval_pair(&k, &v, &remaining)) {
    if (!strcmp("naxis", k)) {
      t->num_axis = atoi(v);
    } else if (!strcmp("name", k)) {
      strncpy(t->title, v, 31);
    } else if (!strcmp("cols", k)) {
      t->axis[0].num = atoi(v);
    } else if (!strcmp("colname", k)) {
      strcpy(t->axis[0].name, v);
    } else if (!strcmp("rows", k)) {
      t->axis[1].num = atoi(v);
    } else if (!strcmp("rowname", k)) {
      strcpy(t->axis[1].name, v);
    } else if (!strcmp("collabels", k)) {
      console_set_table_axis_labels(&t->axis[0], v);
    } else if (!strcmp("rowlabels", k)) {
      console_set_table_axis_labels(&t->axis[1], v);
    } else if (k[0] == '[') {
      console_set_table_element(t, k, atof(v));
    }
  }
}
static void console_get_table_axis_labels(const struct table_axis *t,
                                          char *dest) {
  char buf[32];
  strcat(dest, "[");
  for (int i = 0; i < t->num; ++i) {
    sprintf(buf, "%.1f", t->values[i]);
    strcat(dest, buf);
    if (i != t->num - 1) {
      strcat(dest, ",");
    }
  }
  strcat(dest, "]");
}

static void console_get_table(const struct console_config_node *self,
                              char *dest,
                              char *remaining) {
  assert(self);

  if (!self->val) {
    strcpy(dest, "null");
    return;
  }

  const struct table *t = self->val;
  if (remaining && (remaining[0] == '[')) {
    float val;
    char *saveptr;
    char *element = strtok_r(remaining, " ", &saveptr);
    do {
      if (console_get_table_element(t, element, &val)) {
        dest += sprintf(dest, "%s%.2f", (element == remaining) ? "" : " ", val);
      } else {
        strcpy(dest, "invalid");
      }
    } while ((element = strtok_r(NULL, " ", &saveptr)));
  } else {
    dest += sprintf(dest,
                    "name=%s naxis=%d cols=%d colname=%s "
                    "rows=%d rowname=%s ",
                    t->title,
                    t->num_axis,
                    t->axis[0].num,
                    t->axis[0].name,
                    t->axis[1].num,
                    t->axis[1].name);

    strcat(dest, "collabels=");
    console_get_table_axis_labels(&t->axis[0], dest);

    strcat(dest, " rowlabels=");
    console_get_table_axis_labels(&t->axis[1], dest);
  }
}

static void console_save_to_flash(const struct console_config_node *self
                                  __attribute__((unused)),
                                  char *rem __attribute__((unused))) {
  platform_save_config();
}

static void console_bootloader(const struct console_config_node *self
                               __attribute__((unused)),
                               char *dest __attribute__((unused)),
                               char *rem __attribute__((unused))) {
  platform_reset_into_bootloader();
}

static void console_get_sensor(const struct console_config_node *self,
                               char *dest,
                               char *remaining __attribute__((unused))) {
  assert(self);
  assert(self->val);

  const struct sensor_input *s = self->val;

  const char *source = "";
  switch (s->source) {
  case SENSOR_NONE:
    source = "disabled";
    break;
  case SENSOR_ADC:
    source = "adc";
    break;
  case SENSOR_FREQ:
    source = "freq";
    break;
  case SENSOR_DIGITAL:
    source = "digital";
    break;
  case SENSOR_PWM:
    source = "pwm";
    break;
  case SENSOR_CONST:
    source = "const";
    break;
  }

  const char *method = "";
  switch (s->method) {
  case METHOD_LINEAR:
    method = "linear";
    break;
  case METHOD_LINEAR_WINDOWED:
    method = "linear-window";
    break;
  case METHOD_TABLE:
    method = "table";
    break;
  case METHOD_THERM:
    method = "therm";
    break;
  }

  dest += sprintf(dest, "source=%s method=%s pin=%d ", source, method, s->pin);
  if (s->source == SENSOR_CONST) {
    dest += sprintf(dest, "fixed-val=%.2f ", s->params.fixed_value);
  } else if (s->method == METHOD_LINEAR) {
    dest += sprintf(dest,
                    "range-min=%.2f range-max=%.2f ",
                    s->params.range.min,
                    s->params.range.max);
  } else if (s->method == METHOD_LINEAR_WINDOWED) {
    dest += sprintf(dest,
                    "range-min=%.2f range-max=%.2f ",
                    s->params.range.min,
                    s->params.range.max);
  } else if (s->method == METHOD_TABLE) {
    dest += sprintf(dest, "table=%s ", "unsupported");
  } else if (s->method == METHOD_THERM) {
    dest += sprintf(dest,
                    "therm-bias=%.2f therm-a=%e therm-b=%e therm-c=%e ",
                    s->params.therm.bias,
                    s->params.therm.a,
                    s->params.therm.b,
                    s->params.therm.c);
  }
  sprintf(dest,
          "fault-min=%u fault-max=%u fault-val=%.2f lag=%f",
          (unsigned int)s->fault_config.min,
          (unsigned int)s->fault_config.max,
          s->fault_config.fault_value,
          s->lag);
}

static sensor_source console_sensor_source_from_str(const char *str) {
  if (!strcmp("disabled", str)) {
    return SENSOR_NONE;
  } else if (!strcmp("adc", str)) {
    return SENSOR_ADC;
  } else if (!strcmp("freq", str)) {
    return SENSOR_FREQ;
  } else if (!strcmp("digital", str)) {
    return SENSOR_DIGITAL;
  } else if (!strcmp("pwm", str)) {
    return SENSOR_PWM;
  } else if (!strcmp("const", str)) {
    return SENSOR_CONST;
  }
  return SENSOR_NONE;
}

static sensor_method console_sensor_method_from_str(const char *str) {
  if (!strcmp("linear", str)) {
    return METHOD_LINEAR;
  } else if (!strcmp("linear-windowed", str)) {
    return METHOD_LINEAR_WINDOWED;
  } else if (!strcmp("table", str)) {
    return METHOD_TABLE;
  } else if (!strcmp("therm", str)) {
    return METHOD_THERM;
  }
  return METHOD_TABLE;
}

static void console_set_sensor(const struct console_config_node *self,
                               char *remaining) {
  assert(self);
  assert(self->val);
  assert(remaining);

  struct sensor_input *s = self->val;

  char *k, *v;
  while (parse_keyval_pair(&k, &v, &remaining)) {
    if (!strcmp("source", k)) {
      s->source = console_sensor_source_from_str(v);
    } else if (!strcmp("method", k)) {
      s->method = console_sensor_method_from_str(v);
    } else if (!strcmp("pin", k)) {
      s->pin = atoi(v);
    } else if (!strcmp("lag", k)) {
      s->lag = atof(v);
    } else if (!strcmp("fixed-val", k)) {
      s->params.fixed_value = atof(v);
    } else if (!strcmp("range-min", k)) {
      s->params.range.min = atof(v);
    } else if (!strcmp("range-max", k)) {
      s->params.range.max = atof(v);
    } else if (!strcmp("therm-bias", k)) {
      s->params.therm.bias = atof(v);
    } else if (!strcmp("therm-a", k)) {
      s->params.therm.a = atof(v);
    } else if (!strcmp("therm-b", k)) {
      s->params.therm.b = atof(v);
    } else if (!strcmp("therm-c", k)) {
      s->params.therm.c = atof(v);
    } else if (!strcmp("fault-min", k)) {
      s->fault_config.min = atoi(v);
    } else if (!strcmp("fault-max", k)) {
      s->fault_config.max = atoi(v);
    } else if (!strcmp("fault-val", k)) {
      s->fault_config.fault_value = atof(v);
    }
  }
}

static void console_get_sensor_fault(const struct console_config_node *self,
                                     char *dest,
                                     char *remaining __attribute__((unused))) {
  const struct sensor_input *t = self->val;
  const char *result;

  switch (t->fault) {
  case FAULT_RANGE:
    result = "range";
    break;
  case FAULT_CONN:
    result = "connection";
    break;
  default:
    result = "-";
    break;
  }
  strcat(dest, result);
}

static void console_get_decoder_loss_reason(
  const struct console_config_node *self,
  char *dest,
  char *remaining __attribute__((unused))) {
  assert(self);
  assert(self->val);

  const decoder_loss_reason *s = self->val;
  const char *result = "";

  switch (*s) {
  case DECODER_NO_LOSS:
    result = "none";
    break;
  case DECODER_VARIATION:
    result = "variation";
    break;
  case DECODER_TRIGGERCOUNT_HIGH:
    result = "triggers+";
    break;
  case DECODER_TRIGGERCOUNT_LOW:
    result = "triggers-";
    break;
  case DECODER_EXPIRED:
    result = "expired";
    break;
  }
  strcat(dest, result);
}

static void console_get_decoder_state(const struct console_config_node *self,
                                      char *dest,
                                      char *remaining __attribute__((unused))) {
  assert(self);
  assert(self->val);

  const decoder_state *s = self->val;
  const char *result = "";

  switch (*s) {
  case DECODER_NOSYNC:
    result = "none";
    break;
  case DECODER_RPM:
    result = "rpm";
    break;
  case DECODER_SYNC:
    result = "full";
    break;
  }
  strcat(dest, result);
}

static void console_get_trigger(const struct console_config_node *self,
                                char *dest,
                                char *remaining __attribute__((unused))) {
  assert(self);
  assert(self->val);

  const trigger_type *t = self->val;
  const char *result = "";

  switch (*t) {
  case FORD_TFI:
    result = "tfi";
    break;
  case TOYOTA_24_1_CAS:
    result = "cam24+1";
    break;
  }
  strcat(dest, result);
}

static void console_set_trigger(const struct console_config_node *self,
                                char *remaining __attribute__((unused))) {
  assert(self);
  assert(self->val);

  trigger_type *t = self->val;
  if (!strcmp("tfi", remaining)) {
    *t = FORD_TFI;
  } else if (!strcmp("cam24+1", remaining)) {
    *t = TOYOTA_24_1_CAS;
  }
}

static void console_get_dwell_type(const struct console_config_node *self,
                                   char *dest,
                                   char *remaining __attribute__((unused))) {
  assert(self);
  assert(self->val);

  const dwell_type *t = self->val;
  const char *result = "";

  switch (*t) {
  case DWELL_FIXED_DUTY:
    result = "fixed-duty";
    break;
  case DWELL_FIXED_TIME:
    result = "fixed-time";
    break;
  case DWELL_BRV:
    result = "brv";
    break;
  }
  strcat(dest, result);
}

static void console_set_dwell_type(const struct console_config_node *self,
                                   char *remaining __attribute__((unused))) {
  assert(self);
  assert(self->val);

  dwell_type *t = self->val;
  if (!strcmp("fixed-duty", remaining)) {
    *t = DWELL_FIXED_DUTY;
  } else if (!strcmp("fixed-time", remaining)) {
    *t = DWELL_FIXED_TIME;
  } else if (!strcmp("brv", remaining)) {
    *t = DWELL_BRV;
  }
}

static void console_get_stats(const struct console_config_node *self
                              __attribute((unused)),
                              char *dest,
                              char *remaining __attribute__((unused))) {

  const struct stats_entry *e;
  for (e = &stats_entries[0]; e != &stats_entries[STATS_LAST]; ++e) {
    dest += sprintf(dest,
                    "  %s min/avg/max (uS) = %u/%u/%u\r\n",
                    e->name,
                    (unsigned int)e->min,
                    (unsigned int)e->avg,
                    (unsigned int)e->max);
  }
}

static void console_get_events(const struct console_config_node *self
                               __attribute__((unused)),
                               char *dest,
                               char *remaining) {

  if (!remaining || !strcmp("", remaining)) {
    sprintf(dest, "num_events=%d", config.num_events);
    return;
  }
  unsigned int ev_n = atoi(strtok(remaining, " "));
  if (ev_n < config.num_events) {
    const struct output_event *ev = &config.events[ev_n];
    const char *ev_type = "";
    switch (ev->type) {
    case FUEL_EVENT:
      ev_type = "fuel";
      break;
    case IGNITION_EVENT:
      ev_type = "ignition";
      break;
    default:
      ev_type = "disabled";
      break;
    }
    sprintf(dest,
            "type=%s angle=%.0f output=%d inverted=%d",
            ev_type,
            ev->angle,
            ev->pin,
            ev->inverted);
  } else {
    strcpy(dest, "event out of range");
  }
}

static void console_set_events(const struct console_config_node *self
                               __attribute__((unused)),
                               char *remaining) {

  char *k, *v;
  char *saveptr;

  if (!remaining) {
    return;
  }
  if (!strncmp("num_events=", remaining, 11)) {
    if (parse_keyval_pair(&k, &v, &remaining)) {
      config.num_events = atoi(v);
    }
    return;
  }

  unsigned int ev_n = atoi(strtok_r(remaining, " ", &saveptr));
  if (ev_n >= config.num_events) {
    return;
  }
  struct output_event *ev = &config.events[ev_n];

  remaining = saveptr;
  while (parse_keyval_pair(&k, &v, &remaining)) {
    if (!strcmp("type", k)) {
      if (!strcmp("fuel", v)) {
        ev->type = FUEL_EVENT;
      } else if (!strcmp("ignition", v)) {
        ev->type = IGNITION_EVENT;
      } else if (!strcmp("disabled", v)) {
        ev->type = DISABLED_EVENT;
      }
    } else if (!strcmp("angle", k)) {
      ev->angle = atoi(v);
    } else if (!strcmp("output", k)) {
      ev->pin = atoi(v);
    } else if (!strcmp("inverted", k)) {
      ev->inverted = atoi(v);
    }
  }
}

static void console_get_freq(const struct console_config_node *self
                             __attribute__((unused)),
                             char *dest,
                             char *remaining) {

  const int num_freqs = sizeof(config.freq_inputs) / sizeof(struct freq_input);
  if (!remaining || !strcmp("", remaining)) {
    sprintf(dest, "num_freq=%d", num_freqs);
    return;
  }

  int freq_n = atoi(strtok(remaining, " "));
  if (freq_n >= num_freqs) {
    strcpy(dest, "Frequency input out of range");
    return;
  }
  const char *edge, *type;
  switch (config.freq_inputs[freq_n].edge) {
  case RISING_EDGE:
    edge = "rising";
    break;
  case FALLING_EDGE:
    edge = "falling";
    break;
  case BOTH_EDGES:
  default:
    edge = "both";
    break;
  }

  switch (config.freq_inputs[freq_n].type) {
  case TRIGGER:
    type = "trigger";
    break;
  case FREQ:
  default:
    type = "freq";
    break;
  }

  sprintf(dest, "edge=%s type=%s", edge, type);
}

static void console_set_freq(const struct console_config_node *self
                             __attribute__((unused)),
                             char *remaining) {

  char *k, *v;
  char *saveptr;
  const int num_freqs = sizeof(config.freq_inputs) / sizeof(struct freq_input);

  if (!remaining) {
    return;
  }

  int freq_n = atoi(strtok_r(remaining, " ", &saveptr));
  if (freq_n >= num_freqs) {
    return;
  }
  struct freq_input *freq = &config.freq_inputs[freq_n];

  remaining = saveptr;
  while (parse_keyval_pair(&k, &v, &remaining)) {
    if (!strcmp("type", k)) {
      if (!strcmp("trigger", v)) {
        freq->type = TRIGGER;
      } else if (!strcmp("freq", v)) {
        freq->type = FREQ;
      }
    } else if (!strcmp("edge", k)) {
      if (!strcmp("rising", v)) {
        freq->edge = RISING_EDGE;
      } else if (!strcmp("falling", v)) {
        freq->edge = FALLING_EDGE;
      } else if (!strcmp("both", v)) {
        freq->edge = BOTH_EDGES;
      }
    }
  }
}

static struct {
  int enabled;
  struct logged_event events[32];
  int read;
  int write;
} event_log;

static struct logged_event platform_get_logged_event() {
  if (!event_log.enabled || (event_log.read == event_log.write)) {
    return (struct logged_event){ .type = EVENT_NONE };
  }
  struct logged_event ret = event_log.events[event_log.read];
  event_log.read = (event_log.read + 1) %
                   (sizeof(event_log.events) / sizeof(event_log.events[0]));
  return ret;
}

void console_record_event(struct logged_event ev) {
  if (!event_log.enabled) {
    return;
  }

  int size = (sizeof(event_log.events) / sizeof(event_log.events[0]));
  if ((event_log.write + 1) % size == event_log.read) {
    return;
  }

  event_log.events[event_log.write] = ev;
  event_log.write = (event_log.write + 1) % size;
}

static struct timed_callback trig_cb = { 0 };
static void console_fire_trigger_callback(void *_a) {
  (void)_a;

  decoder_update_scheduling(0, trig_cb.time);
}

static void console_set_trigger_callback(const struct console_config_node *self,
                                         char *remaining) {
  (void)self;
  timeval_t stoptime = atoi(remaining);
  trig_cb.callback = console_fire_trigger_callback;
  schedule_callback(&trig_cb, stoptime);
}

static void console_set_event_logging(const struct console_config_node *self,
                                      char *remaining) {
  (void)self;

  if (!strncmp(remaining, "on", 2)) {
    platform_enable_event_logging();
    event_log.enabled = 1;
  } else {
    platform_disable_event_logging();
    event_log.enabled = 0;
  }
}

static void console_set_freeze(const struct console_config_node *self,
                               char *remaining) {
  (void)self;
  (void)remaining;
}

static void console_set_test_trigger(const struct console_config_node *self,
                                     char *remaining) {
  (void)self;

  uint32_t rpm = atoi(remaining);
  set_test_trigger_rpm(rpm);
}

#ifndef GIT_DESCRIBE
#define GIT_DESCRIBE "unknown"
static const char *git_describe = GIT_DESCRIBE;
#endif

static void console_get_version(const struct console_config_node *self,
                                char *dest,
                                char *remaining) {
  (void)self;
  (void)remaining;

  strcpy(dest, GIT_DESCRIBE);
}

static struct console_config_node console_config_nodes[] = {
  /* Config hierarchy */
  { .name = "config" },
  { .name = "config.feed",
    .get = console_get_feed,
    .set = console_set_feed,
    .val = &console_feed_config },

  { .name = "config.tables" },
  { .name = "config.tables.timing",
    .val = &timing_vs_rpm_and_map,
    .get = console_get_table,
    .set = console_set_table },
  { .name = "config.tables.iat_timing_adjust",
    .get = console_get_table,
    .set = console_set_table },
  { .name = "config.tables.clt_timing_adjust",
    .get = console_get_table,
    .set = console_set_table },
  { .name = "config.tables.clt_pw_adjust",
    .get = console_get_table,
    .set = console_set_table },
  { .name = "config.tables.ve",
    .val = &ve_vs_rpm_and_map,
    .get = console_get_table,
    .set = console_set_table },
  { .name = "config.tables.commanded_lambda",
    .val = &lambda_vs_rpm_and_map,
    .get = console_get_table,
    .set = console_set_table },
  { .name = "config.tables.injector_dead_time",
    .val = &injector_dead_time,
    .get = console_get_table,
    .set = console_set_table },
  { .name = "config.tables.engine_temp_enrich",
    .val = &enrich_vs_temp_and_map,
    .get = console_get_table,
    .set = console_set_table },
  { .name = "config.tables.tipin_enrich_amount",
    .val = &tipin_vs_tpsrate_and_tps,
    .get = console_get_table,
    .set = console_set_table },
  { .name = "config.tables.tipin_enrich_duration",
    .val = &tipin_duration_vs_rpm,
    .get = console_get_table,
    .set = console_set_table },
  { .name = "config.tables.boost_control",
    .val = &boost_control_pwm,
    .get = console_get_table,
    .set = console_set_table },
  { .name = "config.tables.dwell",
    .val = &dwell_ms_vs_brv,
    .get = console_get_table,
    .set = console_set_table },

  /* Decoding */
  { .name = "config.decoder" },
  { .name = "config.decoder.trigger",
    .val = &config.decoder.type,
    .get = console_get_trigger,
    .set = console_set_trigger },
  { .name = "config.decoder.max_variance",
    .val = &config.decoder.trigger_max_rpm_change,
    .get = console_get_float,
    .set = console_set_float },
  { .name = "config.decoder.offset",
    .val = &config.decoder.offset,
    .get = console_get_float,
    .set = console_set_float },
  { .name = "config.decoder.min_rpm",
    .val = &config.decoder.trigger_min_rpm,
    .get = console_get_uint,
    .set = console_set_uint },

  /* Fueling */
  { .name = "config.fueling" },
  { .name = "config.fueling.injector_cc",
    .val = &config.fueling.injector_cc_per_minute,
    .get = console_get_float,
    .set = console_set_float },
  { .name = "config.fueling.cyclinder_cc",
    .val = &config.fueling.cylinder_cc,
    .get = console_get_float,
    .set = console_set_float },
  { .name = "config.fueling.fuel_stoich_ratio",
    .val = &config.fueling.fuel_stoich_ratio,
    .get = console_get_float,
    .set = console_set_float },
  { .name = "config.fueling.injections_per_cycle",
    .val = &config.fueling.injections_per_cycle,
    .get = console_get_uint,
    .set = console_set_uint },
  { .name = "config.fueling.fuel_pump_pin",
    .val = &config.fueling.fuel_pump_pin,
    .get = console_get_uint,
    .set = console_set_uint },
  { .name = "config.fueling.crank_enrich_rpm",
    .val = &config.fueling.crank_enrich_config.crank_rpm,
    .get = console_get_float,
    .set = console_set_float },
  { .name = "config.fueling.crank_enrich_temp",
    .val = &config.fueling.crank_enrich_config.cutoff_temperature,
    .get = console_get_float,
    .set = console_set_float },
  { .name = "config.fueling.crank_enrich_amt",
    .val = &config.fueling.crank_enrich_config.enrich_amt,
    .get = console_get_float,
    .set = console_set_float },

  /* Ignition */
  { .name = "config.ignition" },
  { .name = "config.ignition.dwell_type",
    .val = &config.ignition.dwell,
    .get = console_get_dwell_type,
    .set = console_set_dwell_type },
  { .name = "config.ignition.dwell_duty",
    .val = &config.ignition.dwell_duty,
    .get = console_get_float,
    .set = console_set_float },
  { .name = "config.ignition.dwell_us",
    .val = &config.ignition.dwell_us,
    .get = console_get_float,
    .set = console_set_float },
  { .name = "config.ignition.min_fire_time_us",
    .val = &config.ignition.min_fire_time_us,
    .get = console_get_uint,
    .set = console_set_uint },

  /* Sensors */
  { .name = "config.sensors" },
  { .name = "config.sensors.map",
    .val = &config.sensors[SENSOR_MAP],
    .get = console_get_sensor,
    .set = console_set_sensor },
  { .name = "config.sensors.iat",
    .val = &config.sensors[SENSOR_IAT],
    .get = console_get_sensor,
    .set = console_set_sensor },
  { .name = "config.sensors.clt",
    .val = &config.sensors[SENSOR_CLT],
    .get = console_get_sensor,
    .set = console_set_sensor },
  { .name = "config.sensors.brv",
    .val = &config.sensors[SENSOR_BRV],
    .get = console_get_sensor,
    .set = console_set_sensor },
  { .name = "config.sensors.tps",
    .val = &config.sensors[SENSOR_TPS],
    .get = console_get_sensor,
    .set = console_set_sensor },
  { .name = "config.sensors.aap",
    .val = &config.sensors[SENSOR_AAP],
    .get = console_get_sensor,
    .set = console_set_sensor },
  { .name = "config.sensors.frt",
    .val = &config.sensors[SENSOR_FRT],
    .get = console_get_sensor,
    .set = console_set_sensor },
  { .name = "config.sensors.ego",
    .val = &config.sensors[SENSOR_EGO],
    .get = console_get_sensor,
    .set = console_set_sensor },

  { .name = "config.events",
    .get = console_get_events,
    .set = console_set_events },

  /* Tasks */
  { .name = "config.tasks" },
  { .name = "config.tasks.boost_control.overboost",
    .val = &config.boost_control.overboost,
    .get = console_get_float,
    .set = console_set_float },
  { .name = "config.tasks.boost_control.pin",
    .val = &config.boost_control.pin,
    .get = console_get_uint,
    .set = console_set_uint },
  { .name = "config.tasks.boost_control.threshold",
    .val = &config.boost_control.threshhold_kpa,
    .get = console_get_float,
    .set = console_set_float },
  { .name = "config.tasks.cel.pin",
    .val = &config.cel.pin,
    .get = console_get_uint,
    .set = console_set_uint },
  { .name = "config.tasks.cel.lean_boost_kpa",
    .val = &config.cel.lean_boost_kpa,
    .get = console_get_float,
    .set = console_set_float },
  { .name = "config.tasks.cel.lean_boost_ego",
    .val = &config.cel.lean_boost_ego,
    .get = console_get_float,
    .set = console_set_float },

  /* Hardware config */
  { .name = "config.hardware" },
  { .name = "config.hardware.freq",
    .get = console_get_freq,
    .set = console_set_freq },
  { .name = "config.hardware.pwm" },

  { .name = "status" },
  { .name = "status.current_time", .get = console_get_time },

  { .name = "status.decoder.state",
    .val = &config.decoder.state,
    .get = console_get_decoder_state },
  { .name = "status.decoder.loss_reason",
    .val = &config.decoder.loss,
    .get = console_get_decoder_loss_reason },
  { .name = "status.decoder.t0_count",
    .val = &config.decoder.t0_count,
    .get = console_get_uint },
  { .name = "status.decoder.t1_count",
    .val = &config.decoder.t1_count,
    .get = console_get_uint },
  { .name = "status.decoder.rpm",
    .val = &config.decoder.rpm,
    .get = console_get_uint },
  { .name = "status.decoder.rpm_variance",
    .val = &config.decoder.trigger_cur_rpm_change,
    .get = console_get_float },

  { .name = "status.fueling.cut", .get = console_get_fuel_cut },
  { .name = "status.fueling.pw_us",
    .val = &calculated_values.fueling_us,
    .get = console_get_uint },
  { .name = "status.fueling.ete",
    .val = &calculated_values.ete,
    .get = console_get_float },
  { .name = "status.fueling.ve",
    .val = &calculated_values.ve,
    .get = console_get_float },
  { .name = "status.fueling.lambda",
    .val = &calculated_values.lambda,
    .get = console_get_float },
  { .name = "status.fueling.idt",
    .val = &calculated_values.idt,
    .get = console_get_float },
  { .name = "status.fueling.fuel_volume",
    .val = &calculated_values.fuelvol_per_cycle,
    .get = console_get_float },
  { .name = "status.fueling.tipin",
    .val = &calculated_values.tipin,
    .get = console_get_float },

  { .name = "status.ignition.timing_advance",
    .val = &calculated_values.timing_advance,
    .get = console_get_float },
  { .name = "status.ignition.dwell_us",
    .val = &calculated_values.dwell_us,
    .get = console_get_uint },
  { .name = "status.ignition.cut", .get = console_get_ignition_cut },

  /* Sensor values */
  { .name = "status.sensors.map",
    .val = &config.sensors[SENSOR_MAP].processed_value,
    .get = console_get_float },
  { .name = "status.sensors.map.fault",
    .val = &config.sensors[SENSOR_MAP].fault,
    .get = console_get_sensor_fault },
  { .name = "status.sensors.iat",
    .val = &config.sensors[SENSOR_IAT].processed_value,
    .get = console_get_float },
  { .name = "status.sensors.iat.fault",
    .val = &config.sensors[SENSOR_IAT].fault,
    .get = console_get_sensor_fault },
  { .name = "status.sensors.clt",
    .val = &config.sensors[SENSOR_CLT].processed_value,
    .get = console_get_float },
  { .name = "status.sensors.clt.fault",
    .val = &config.sensors[SENSOR_CLT].fault,
    .get = console_get_sensor_fault },
  { .name = "status.sensors.brv",
    .val = &config.sensors[SENSOR_BRV].processed_value,
    .get = console_get_float },
  { .name = "status.sensors.brv.fault",
    .val = &config.sensors[SENSOR_BRV].fault,
    .get = console_get_sensor_fault },
  { .name = "status.sensors.tps",
    .val = &config.sensors[SENSOR_TPS].processed_value,
    .get = console_get_float },
  { .name = "status.sensors.tps.fault",
    .val = &config.sensors[SENSOR_TPS].fault,
    .get = console_get_sensor_fault },
  { .name = "status.sensors.aap",
    .val = &config.sensors[SENSOR_AAP].processed_value,
    .get = console_get_float },
  { .name = "status.sensors.aap.fault",
    .val = &config.sensors[SENSOR_AAP].fault,
    .get = console_get_sensor_fault },
  { .name = "status.sensors.frt",
    .val = &config.sensors[SENSOR_FRT].processed_value,
    .get = console_get_float },
  { .name = "status.sensors.frt.fault",
    .val = &config.sensors[SENSOR_FRT].fault,
    .get = console_get_sensor_fault },
  { .name = "status.sensors.ego",
    .val = &config.sensors[SENSOR_EGO].processed_value,
    .get = console_get_float },
  { .name = "status.sensors.ego.fault",
    .val = &config.sensors[SENSOR_EGO].fault,
    .get = console_get_sensor_fault },

  /* Misc commands */
  { .name = "flash", .set = console_save_to_flash },
  { .name = "stats", .get = console_get_stats },
  { .name = "bootloader", .get = console_bootloader },
  { .name = "version", .get = console_get_version },

  /* Host commands */
  { .name = "sim" },
  { .name = "sim.trigger_time", .set = console_set_trigger_callback },
  { .name = "sim.test_trigger", .set = console_set_test_trigger },
  { .name = "sim.event_logging", .set = console_set_event_logging },
  { .name = "sim.freeze", .set = console_set_freeze },
  { 0 },
};

/* Lists all immediate prefixes in node list nodes */
static void console_list_prefix(const struct console_config_node *nodes,
                                char *dest,
                                const char *prefix) {

  const struct console_config_node *node;
  for (node = nodes; node->name; node++) {
    /* If prefix doesn't match */
    if (prefix && strlen(prefix) &&
        strncmp(node->name, prefix, strlen(prefix))) {
      continue;
    }
    strcat(dest, node->name);
    strcat(dest, " ");
  }
}

int console_parse_request(char *dest, char *line) {
  char *action = strtok(line, " ");
  char *var = strtok(NULL, " ");
  char *rem = strtok(NULL, "\0");

  if (!action) {
    strcat(dest, "invalid action");
    return 0;
  }

  const struct console_config_node *node =
    console_search_node(console_config_nodes, var);
  if (!strcmp("list", action)) {
    console_list_prefix(console_config_nodes, dest, var);
    return 1;
  } else if (!node) {
    strcpy(dest, "invalid config node");
    return 0;
  } else if (!strcmp("get", action)) {
    if (node->get) {
      node->get(node, dest, rem);
      return 1;
    } else {
      strcat(dest, "Config node ");
      strncat(dest, node->name, 64);
      strcat(dest, " does not support get");
    }
  } else if (!strcmp("set", action)) {
    if (node->set) {
      node->set(node, rem);
      return 1;
    } else {
      strcat(dest, "Config node ");
      strncat(dest, node->name, 64);
      strcat(dest, " does not support set");
    }
  } else {
    strcpy(dest, "invalid action");
  }
  return 0;
}

void console_init() {
  const char *console_feed_defaults[] = {
    "status.current_time",
    "status.decoder.state",
    "status.decoder.rpm",
    "status.decoder.loss_reason",
    "status.decoder.rpm_variance",
    "status.decoder.t0_count",
    "status.decoder.t1_count",
    "status.ignition.timing_advance",
    "status.fueling.pw_us",
    "status.fueling.fuel_volume",
    "status.fueling.ete",
    "status.fueling.ve",
    "status.fueling.lambda",
    "status.fueling.idt",
    "status.fueling.tipin",
    "status.sensors.brv",
    "status.sensors.map",
    "status.sensors.aap",
    "status.sensors.tps",
    "status.sensors.iat",
    "status.sensors.clt",
    "status.sensors.ego",
    NULL,
  };

  int count = 0;
  while (console_feed_defaults[count]) {
    console_feed_config.nodes[count] =
      console_search_node(console_config_nodes, console_feed_defaults[count]);
    assert(console_feed_config.nodes[count]);
    count++;
  }
  console_feed_config.n_nodes = count;
}

static void console_feed_line(char *dest) {

  unsigned int i;
  char temp[64];
  char empty[1] = "";
  for (i = 0; i < console_feed_config.n_nodes; i++) {
    const struct console_config_node *node = console_feed_config.nodes[i];
    strcpy(temp, "");
    if (!node->get) {
      continue;
    }
    node->get(node, temp, empty);
    strcat(dest, temp);
    if (i < console_feed_config.n_nodes - 1) {
      strcat(dest, ",");
    }
  }
  strcat(dest, "\r\n");
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

static int console_output_events() {
  strcpy(console.txbuffer, "");
  int num_ev = 0;
  struct logged_event ev = platform_get_logged_event();
  while ((ev.type != EVENT_NONE) && (num_ev != 16)) {
    num_ev++;

    char tempbuf[32];
    switch (ev.type) {
    case EVENT_OUTPUT:
      sprintf(
        tempbuf, "# OUTPUTS %lu %2x\r\n", (unsigned long)ev.time, ev.value);
      break;
    case EVENT_GPIO:
      sprintf(tempbuf, "# GPIO %lu %2x\r\n", (unsigned long)ev.time, ev.value);
      break;
    case EVENT_TRIGGER0:
      sprintf(tempbuf, "# TRIGGER0 %lu\r\n", (unsigned long)ev.time);
      break;
    case EVENT_TRIGGER1:
      sprintf(tempbuf, "# TRIGGER1 %lu\r\n", (unsigned long)ev.time);
      break;
    default:
      break;
    }
    strcat(console.txbuffer, tempbuf);

    ev = platform_get_logged_event();
  }

  if (num_ev != 0) {
    console_write_full(console.txbuffer, strlen(console.txbuffer));
  }
  return num_ev;
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
  if (!console_output_events() && console_feed_config.n_nodes) {
    console_feed_line(console.txbuffer);
    console_write_full(console.txbuffer, strlen(console.txbuffer));
  }

  stats_finish_timing(STATS_CONSOLE_TIME);
}

#ifdef UNITTEST
#include <check.h>

static struct console_config_node test_nodes[] = {
  { .name = "test1" },
  { .name = "test2" },
  { .name = "test2.testA" },
  { .name = "test3.testA" },
  { .name = "test3.testA.test1" },
  { .name = "test2.testB" },
  { .name = "test2.testB.test2" },
  { 0 },
};

START_TEST(check_console_search_node) {

  ck_assert_ptr_null(console_search_node(test_nodes, NULL));
  ck_assert_ptr_null(console_search_node(test_nodes, ""));
  ck_assert_ptr_null(console_search_node(test_nodes, "test5"));
  ck_assert_ptr_null(console_search_node(test_nodes, "test3"));
  ck_assert_ptr_null(console_search_node(test_nodes, "test1.testA"));
  ck_assert_ptr_null(console_search_node(test_nodes, "test1.testA.test1"));

  ck_assert_ptr_eq(console_search_node(test_nodes, "test1"), &test_nodes[0]);
  ck_assert_ptr_eq(console_search_node(test_nodes, "test2"), &test_nodes[1]);
  ck_assert_ptr_eq(console_search_node(test_nodes, "test2.testA"),
                   &test_nodes[2]);
  ck_assert_ptr_eq(console_search_node(test_nodes, "test3.testA.test1"),
                   &test_nodes[4]);
}
END_TEST

START_TEST(check_console_list_prefix) {
  char buf[128];

  strcpy(buf, "");
  console_list_prefix(test_nodes, buf, "test2");
  ck_assert_str_eq(buf, "test2 test2.testA test2.testB test2.testB.test2 ");

  strcpy(buf, "");
  console_list_prefix(test_nodes, buf, "test1");
  ck_assert_str_eq(buf, "test1 ");

  strcpy(buf, "");
  console_list_prefix(test_nodes, buf, "test3");
  ck_assert_str_eq(buf, "test3.testA test3.testA.test1 ");
}
END_TEST

START_TEST(check_console_get_time) {
  char buf[32] = { 0 };
  console_get_time(NULL, buf, NULL);
  ck_assert(strlen(buf) != 0);
}
END_TEST

START_TEST(check_console_get_float) {
  float v;
  struct console_config_node t = {
    .val = &v,
  };

  v = 1.0f;
  char buf[32];
  console_get_float(&t, buf, NULL);
  ck_assert_str_eq(buf, "1.0000");
}
END_TEST

START_TEST(check_console_set_float) {
  float v;
  struct console_config_node t = {
    .val = &v,
  };

  char buf[] = "1.00";
  console_set_float(&t, buf);
  ck_assert_float_eq(v, 1.00);
}
END_TEST

START_TEST(check_console_get_uint) {
  unsigned int v;
  struct console_config_node t = {
    .val = &v,
  };

  v = 200;
  char buf[32];
  console_get_uint(&t, buf, NULL);
  ck_assert_str_eq(buf, "200");
}
END_TEST

START_TEST(check_console_set_uint) {
  unsigned int v;
  struct console_config_node t = {
    .val = &v,
  };

  char buf[] = "200";
  console_set_uint(&t, buf);
  ck_assert_uint_eq(v, 200);
}
END_TEST

START_TEST(check_parse_keyval_pair) {
  char *buf = malloc(64);
  char *orig = buf;

  strcpy(buf, "");
  char *k = NULL, *v = NULL;

  ck_assert_int_eq(parse_keyval_pair(&k, &v, &buf), 0);
  ck_assert_ptr_null(k);
  ck_assert_ptr_null(v);

  k = NULL;
  k = NULL;
  strcpy(buf, "");
  ck_assert_int_eq(parse_keyval_pair(&k, &v, &buf), 0);

  k = NULL;
  k = NULL;
  strcpy(buf, "keywithnoval");
  ck_assert_int_eq(parse_keyval_pair(&k, &v, &buf), 0);

  k = NULL;
  k = NULL;
  strcpy(buf, "keywithnoval=");
  ck_assert_int_eq(parse_keyval_pair(&k, &v, &buf), 0);

  k = NULL;
  k = NULL;
  strcpy(buf, "keywithval=val");
  ck_assert_int_eq(parse_keyval_pair(&k, &v, &buf), 1);
  ck_assert_str_eq(k, "keywithval");
  ck_assert_str_eq(v, "val");

  k = NULL;
  k = NULL;
  strcpy(buf, "keywithval=val anotherkey=anotherval");
  ck_assert_int_eq(parse_keyval_pair(&k, &v, &buf), 1);
  ck_assert_str_eq(k, "keywithval");
  ck_assert_str_eq(v, "val");

  ck_assert_int_eq(parse_keyval_pair(&k, &v, &buf), 1);
  ck_assert_str_eq(k, "anotherkey");
  ck_assert_str_eq(v, "anotherval");

  free(orig);
}
END_TEST

START_TEST(check_console_set_table_element_oneaxis) {
  struct table t1 = {
    .num_axis = 1,
    .axis[0] = {
      .num = 6,
      .values = {0},
    },
  };

  /* Out of bounds */
  ck_assert_int_eq(console_set_table_element(&t1, "[10]", 1.0), 0);

  /*Wrong type */
  ck_assert_int_eq(console_set_table_element(&t1, "[3][3]", 1.0), 0);

  /* Bad formatting */
  ck_assert_int_eq(console_set_table_element(&t1, "3", 1.0), 0);
  ck_assert_int_eq(console_set_table_element(&t1, "3]", 1.0), 0);

  ck_assert_int_eq(console_set_table_element(&t1, "[3]", 1.0), 1);
  ck_assert_float_eq(t1.data.one[3], 1.0);
}
END_TEST

START_TEST(check_console_set_table_element_twoaxis) {
  struct table t2 = {
    .num_axis = 2,
    .axis[0] = {
      .num = 6,
    }, 
    .axis[1] = {
      .num = 6,
    },
  };

  /* Out of bounds */
  ck_assert_int_eq(console_set_table_element(&t2, "[10][10]", 1.0), 0);

  /*Wrong type */
  ck_assert_int_eq(console_set_table_element(&t2, "[3]", 1.0), 0);

  /* Bad formatting */
  ck_assert_int_eq(console_set_table_element(&t2, "3", 1.0), 0);
  ck_assert_int_eq(console_set_table_element(&t2, "3]4]", 1.0), 0);

  ck_assert_int_eq(console_set_table_element(&t2, "[3][2]", 1.0), 1);
  ck_assert_float_eq(t2.data.two[3][2], 1.0);
}
END_TEST

START_TEST(check_console_get_table_element_oneaxis) {
  const struct table t1 = {
    .num_axis = 2,
    .axis[0] = {
      .num = 3,
    },
    .axis[1] = {
      .num = 3,
    },
    .data = {
      .two = {
        {5, 6, 7, 8, 9},
        {12, 13, 14, 15, 16},
        {18, 19, 20, 21, 22},
      },
    },
  };

  float v;
  /* Out of bounds */
  ck_assert_int_eq(console_get_table_element(&t1, "[10][2]", &v), 0);

  /*Wrong type */
  ck_assert_int_eq(console_get_table_element(&t1, "[2]", &v), 0);

  /* Bad formatting */
  ck_assert_int_eq(console_get_table_element(&t1, "1", &v), 0);
  ck_assert_int_eq(console_get_table_element(&t1, "1]1", &v), 0);

  ck_assert_int_eq(console_get_table_element(&t1, "[1][1]", &v), 1);
  ck_assert_float_eq(v, 13.0);
}
END_TEST

START_TEST(check_console_get_table_element_twoaxis) {
  struct table t2 = {
    .num_axis = 2,
    .axis[0] = {
      .num = 6,
      .values = {0},
    }, 
    .axis[1] = {
      .num = 6,
      .values = {0},
    },
  };

  /* Out of bounds */
  ck_assert_int_eq(console_set_table_element(&t2, "[10][10]", 1.0), 0);

  /*Wrong type */
  ck_assert_int_eq(console_set_table_element(&t2, "[3]", 1.0), 0);

  /* Bad formatting */
  ck_assert_int_eq(console_set_table_element(&t2, "3", 1.0), 0);
  ck_assert_int_eq(console_set_table_element(&t2, "3]4]", 1.0), 0);

  ck_assert_int_eq(console_set_table_element(&t2, "[3][2]", 1.0), 1);
  ck_assert_float_eq(t2.data.two[3][2], 1.0);
}
END_TEST

START_TEST(check_console_set_table_axis_labels) {
  struct table_axis a = {
    .num = 4,
    .values = { 0 },
  };
  char buf[32];

  /* Out of bounds */
  strcpy(buf, "[1,2,3,4,5]");
  console_set_table_axis_labels(&a, buf);
  ck_assert_int_eq(a.values[3], 4);
  ck_assert_int_eq(a.values[4], 0);

  /* Malformed */
  strcpy(buf, "10,11,12,13,15");
  console_set_table_axis_labels(&a, buf);
  ck_assert_int_eq(a.values[3], 4);
  ck_assert_int_eq(a.values[4], 0);

  strcpy(buf, "[8,9,10]");
  console_set_table_axis_labels(&a, buf);
  ck_assert_int_eq(a.values[0], 8);
  ck_assert_int_eq(a.values[1], 9);
  ck_assert_int_eq(a.values[2], 10);
}
END_TEST

START_TEST(check_console_set_table) {
  struct table t = { 0 };
  const struct console_config_node n = { .val = &t };

  char buf[] = "name=test naxis=2 rows=3 cols=3 rowname=row colname=col "
               "rowlabels=[1,2,3] collabels=[5,6,7.2] [0][0]=5.0";

  console_set_table(&n, buf);
  ck_assert_str_eq(t.title, "test");
  ck_assert_int_eq(t.num_axis, 2);
  ck_assert_int_eq(t.axis[0].num, 3);
  ck_assert_int_eq(t.axis[1].num, 3);
  ck_assert_str_eq(t.axis[0].name, "col");
  ck_assert_str_eq(t.axis[1].name, "row");

  ck_assert_float_eq(t.axis[1].values[0], 1);
  ck_assert_float_eq(t.axis[1].values[1], 2);
  ck_assert_float_eq(t.axis[1].values[2], 3);

  ck_assert_float_eq(t.axis[0].values[0], 5);
  ck_assert_float_eq(t.axis[0].values[1], 6);
  ck_assert_float_eq(t.axis[0].values[2], 7.2);

  ck_assert_float_eq(t.data.two[0][0], 5.0);

  char buf2[] = "[0][0]=1.1 [0][1]=1.2 [0][2]=1.3 [1][0]=2.0";
  console_set_table(&n, buf2);
  ck_assert_float_eq(t.data.two[0][0], 1.1);
  ck_assert_float_eq(t.data.two[0][1], 1.2);
  ck_assert_float_eq(t.data.two[0][2], 1.3);
  ck_assert_float_eq(t.data.two[1][0], 2.0);
}
END_TEST

START_TEST(check_console_get_table) {
  struct table t;
  const struct console_config_node n = { .val = &t };

  char buf[512];
  t = (struct table){
    .title = "test",
    .num_axis = 2,
    .axis[0] = {
      .name = "cols", .num = 3,
      .values = {1, 2.0, 3},
    },
    .axis[1] = {
      .name = "rows", .num = 3,
      .values = {5, 6, 7},
    },
    .data = {
      .two = {
        {1, 2, 3},
        {4, 6, 7},
        {7, 8, 9},
      },
    },
  };

  char cmd[] = "";
  console_get_table(&n, buf, cmd);
  ck_assert_ptr_nonnull(strstr(buf, "name=test"));
  ck_assert_ptr_nonnull(strstr(buf, "naxis=2"));
  ck_assert_ptr_nonnull(strstr(buf, "rows=3"));
  ck_assert_ptr_nonnull(strstr(buf, "rowname=rows"));
  ck_assert_ptr_nonnull(strstr(buf, "rowlabels=[5.0,6.0,7.0]"));
  ck_assert_ptr_nonnull(strstr(buf, "cols=3"));
  ck_assert_ptr_nonnull(strstr(buf, "colname=cols"));
  ck_assert_ptr_nonnull(strstr(buf, "collabels=[1.0,2.0,3.0]"));

  char cmd2[] = "[1][1]";
  console_get_table(&n, buf, cmd2);
  ck_assert_str_eq(buf, "6.00");
}
END_TEST

START_TEST(check_console_get_sensor) {
  struct sensor_input s_const = {
    .source = SENSOR_CONST,
    .params = {
      .fixed_value = 12.0,
    },
    .fault_config = {
      .min = 1,
      .max = 2,
      .fault_value = 1.5,
    },
  };
  struct console_config_node n = { .val = &s_const };
  char buf[256];
  char cmd[] = "";

  strcpy(buf, "");
  console_get_sensor(&n, buf, cmd);
  ck_assert_ptr_nonnull(strstr(buf, "source=const"));
  ck_assert_ptr_nonnull(strstr(buf, "fixed-val=12.00"));
  ck_assert_ptr_nonnull(strstr(buf, "fault-min=1"));
  ck_assert_ptr_nonnull(strstr(buf, "fault-max=2"));
  ck_assert_ptr_nonnull(strstr(buf, "fault-val=1.50"));

  struct sensor_input s_linear = {
    .pin = 1,
    .source = SENSOR_ADC,
    .method = METHOD_LINEAR,
    .lag = 20,
    .params = {
      .range = {
        .min = 12,
        .max = 20,
      },
    },
  };
  n.val = &s_linear;

  strcpy(buf, "");
  console_get_sensor(&n, buf, cmd);
  ck_assert_ptr_nonnull(strstr(buf, "source=adc"));
  ck_assert_ptr_nonnull(strstr(buf, "method=linear"));
  ck_assert_ptr_nonnull(strstr(buf, "range-min=12.00"));
  ck_assert_ptr_nonnull(strstr(buf, "range-max=20.00"));
  ck_assert_ptr_nonnull(strstr(buf, "pin=1"));
  ck_assert_ptr_nonnull(strstr(buf, "lag=20.00"));
}
END_TEST

START_TEST(check_console_set_sensor) {
  struct sensor_input s = { 0 };
  const struct console_config_node n = { .val = &s };

  char buf[] = "source=freq pin=2 method=linear lag=0.2 range-min=20.1 "
               "range-max=100.1 fault-min=200 fault-max=400, fault-val=19.2";

  console_set_sensor(&n, buf);
  ck_assert_int_eq(s.source, SENSOR_FREQ);
  ck_assert_int_eq(s.method, METHOD_LINEAR);
  ck_assert_int_eq(s.pin, 2);
  ck_assert_float_eq(s.lag, 0.2);
  ck_assert_float_eq(s.params.range.min, 20.1);
  ck_assert_float_eq(s.params.range.max, 100.1);
  ck_assert_float_eq(s.fault_config.fault_value, 19.2);
  ck_assert_int_eq(s.fault_config.min, 200);
  ck_assert_int_eq(s.fault_config.max, 400);

  char buf2[] = "source=const fixed-val=101.1";
  console_set_sensor(&n, buf2);
  ck_assert_int_eq(s.source, SENSOR_CONST);
  ck_assert_float_eq(s.params.fixed_value, 101.1);
}
END_TEST

START_TEST(check_console_get_events) {

  char cmd[16] = "";
  char buf[128];

  config.num_events = 4;
  console_get_events(NULL, buf, cmd);
  ck_assert_str_eq(buf, "num_events=4");

  config.events[3] = (struct output_event){
    .type = FUEL_EVENT,
    .inverted = 1,
    .angle = 100,
    .pin = 5,
  };

  strcpy(cmd, "3");
  console_get_events(NULL, buf, cmd);
  ck_assert_ptr_nonnull(strstr(buf, "type=fuel"));
  ck_assert_ptr_nonnull(strstr(buf, "angle=100"));
  ck_assert_ptr_nonnull(strstr(buf, "output=5"));
  ck_assert_ptr_nonnull(strstr(buf, "inverted=1"));

  config.events[3] = (struct output_event){
    .type = IGNITION_EVENT,
    .inverted = 0,
    .angle = 5,
    .pin = 4,
  };

  strcpy(cmd, "3");
  console_get_events(NULL, buf, cmd);
  ck_assert_ptr_nonnull(strstr(buf, "type=ignition"));
  ck_assert_ptr_nonnull(strstr(buf, "angle=5"));
  ck_assert_ptr_nonnull(strstr(buf, "output=4"));
  ck_assert_ptr_nonnull(strstr(buf, "inverted=0"));
}
END_TEST

START_TEST(check_console_set_events) {

  char buf[] = "num_events=4";
  console_set_events(NULL, buf);
  ck_assert_int_eq(config.num_events, 4);

  char buf2[] = "2 type=fuel inverted=1 angle=12 output=6";

  console_set_events(NULL, buf2);
  ck_assert_int_eq(config.events[2].type, FUEL_EVENT);
  ck_assert_int_eq(config.events[2].inverted, 1);
  ck_assert_int_eq(config.events[2].angle, 12);
  ck_assert_int_eq(config.events[2].pin, 6);
}
END_TEST

START_TEST(check_console_get_freq) {

  char cmd[16] = "";
  char buf[128];

  config.num_events = 4;
  console_get_freq(NULL, buf, cmd);
  ck_assert_str_eq(buf, "num_freq=4");

  config.freq_inputs[0] = (struct freq_input){
    .type = TRIGGER,
    .edge = FALLING_EDGE,
  };

  strcpy(cmd, "0");
  console_get_freq(NULL, buf, cmd);
  ck_assert_ptr_nonnull(strstr(buf, "edge=falling"));
  ck_assert_ptr_nonnull(strstr(buf, "type=trigger"));

  config.freq_inputs[0] = (struct freq_input){
    .type = FREQ,
    .edge = RISING_EDGE,
  };

  strcpy(cmd, "0");
  console_get_freq(NULL, buf, cmd);
  ck_assert_ptr_nonnull(strstr(buf, "edge=rising"));
  ck_assert_ptr_nonnull(strstr(buf, "type=freq"));

  config.freq_inputs[0] = (struct freq_input){
    .type = FREQ,
    .edge = BOTH_EDGES,
  };

  strcpy(cmd, "0");
  console_get_freq(NULL, buf, cmd);
  ck_assert_ptr_nonnull(strstr(buf, "edge=both"));
  ck_assert_ptr_nonnull(strstr(buf, "type=freq"));
}
END_TEST

START_TEST(check_console_set_freq) {

  char buf[] = "2 type=trigger edge=falling";

  console_set_freq(NULL, buf);
  ck_assert_int_eq(config.freq_inputs[2].type, TRIGGER);
  ck_assert_int_eq(config.freq_inputs[2].edge, FALLING_EDGE);

  char buf2[] = "1 type=freq edge=rising";

  console_set_freq(NULL, buf2);
  ck_assert_int_eq(config.freq_inputs[1].type, FREQ);
  ck_assert_int_eq(config.freq_inputs[1].edge, RISING_EDGE);

  char buf3[] = "1 type=freq edge=both";

  console_set_freq(NULL, buf3);
  ck_assert_int_eq(config.freq_inputs[1].type, FREQ);
  ck_assert_int_eq(config.freq_inputs[1].edge, BOTH_EDGES);
}
END_TEST

TCase *setup_console_tests() {
  TCase *console_tests = tcase_create("console");
  tcase_add_test(console_tests, check_console_search_node);
  tcase_add_test(console_tests, check_console_list_prefix);
  tcase_add_test(console_tests, check_console_get_time);
  tcase_add_test(console_tests, check_console_get_float);
  tcase_add_test(console_tests, check_console_set_float);
  tcase_add_test(console_tests, check_console_get_uint);
  tcase_add_test(console_tests, check_console_set_uint);
  tcase_add_test(console_tests, check_parse_keyval_pair);
  tcase_add_test(console_tests, check_console_set_table_element_oneaxis);
  tcase_add_test(console_tests, check_console_set_table_element_twoaxis);
  tcase_add_test(console_tests, check_console_get_table_element_oneaxis);
  tcase_add_test(console_tests, check_console_get_table_element_twoaxis);
  tcase_add_test(console_tests, check_console_set_table_axis_labels);
  tcase_add_test(console_tests, check_console_set_table);
  tcase_add_test(console_tests, check_console_get_table);
  tcase_add_test(console_tests, check_console_set_sensor);
  tcase_add_test(console_tests, check_console_get_sensor);
  tcase_add_test(console_tests, check_console_set_events);
  tcase_add_test(console_tests, check_console_get_events);
  tcase_add_test(console_tests, check_console_set_freq);
  tcase_add_test(console_tests, check_console_get_freq);
  return console_tests;
}

#endif
