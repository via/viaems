/* For strtok_r */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 1
#endif

#include "platform.h"
#include "console.h"
#include "config.h"
#include "calculations.h"
#include "decoder.h"
#include "sensors.h"
#include "stats.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>


struct console_config_node {
  char *name;
  void (*get)(const struct console_config_node *self, char *dest, char *remaining);
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
  .nodes = {0},
};

static struct console_config_node console_config_nodes[];
static void console_set_feed(const struct console_config_node *self, char *remaining) {
  assert(self);
  assert(self->val);

  unsigned int cur_entry = 0;
  struct console_feed_config *c = self->val;
  const char *cur_var = strtok(remaining, ",");
  while (cur_var) {
    const struct console_config_node *n = console_search_node(console_config_nodes, cur_var);
    if (n) {
      c->nodes[cur_entry] = n;
      cur_entry++;
    }
    cur_var = strtok(NULL, ",");
  }
  c->n_nodes = cur_entry;
}

static void console_get_feed(const struct console_config_node *self, 
    char *dest, char *remaining __attribute__((unused))) {
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


static void console_get_time(
    const struct console_config_node *self __attribute__((unused)),
    char *dest,
    char *remaining __attribute__((unused))) {
  sprintf(dest, "%u", (unsigned int)current_time());
}

static void console_get_float(const struct console_config_node *self, 
    char *dest, char *remaining __attribute__((unused))) {
  float *v = self->val;
  sprintf(dest, "%.2f", *v);
}

static void console_set_float(const struct console_config_node *self, char *remaining) {
  float *v = self->val;
  *v = atof(remaining);
}

static void console_get_uint(const struct console_config_node *self, 
    char *dest, char *remaining __attribute__((unused))) {
  unsigned int *v = self->val;
  sprintf(dest, "%u", *v);
}

static void console_set_uint(const struct console_config_node *self, char *remaining) {
  unsigned int *v = self->val;
  *v = (unsigned int)atoi(remaining);
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


static void console_set_table_element(struct table *t, const char *k, float v) {
  int row, col;
  int n_parsed = sscanf(k, "[%d][%d]", &row, &col);
  if ((t->num_axis == 1) && (n_parsed == 1)) {
    t->data.one[row] = v;
  } else if ((t->num_axis) == 2 && (n_parsed) == 2) {
    t->data.two[row][col] = v;
  }
}

static int console_get_table_element(const struct table *t, const char *k, float *v) {
  int row, col;
  int n_parsed = sscanf(k, "[%d][%d]", &row, &col);
  if ((t->num_axis == 1) && (n_parsed == 1)) {
    *v = t->data.one[row];
    return 1;
  } else if ((t->num_axis) == 2 && (n_parsed) == 2) {
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
  while(curlabel) {
    if ((cur >= t->num) || (cur >= MAX_AXIS_SIZE)) {
      return;
    }
    t->values[cur] = atoi(curlabel);
    curlabel = strtok_r(NULL, ",]", &saveptr);
    cur++;
  }
}

static void console_set_table(const struct console_config_node *self, char *remaining) {
  assert(self);
  assert(self->val);
  assert(remaining);

  struct table *t = self->val;

  char *k, *v;
  while(parse_keyval_pair(&k, &v, &remaining)) {
    if (!strcmp("naxis", k)) {
      t->num_axis = atoi(v);
    } else if (!strcmp("rows", k)) {
      t->axis[0].num = atoi(v);
    } else if (!strcmp("rowname", k)) {
      strcpy(t->axis[0].name, v);
    } else if (!strcmp("cols", k)) {
      t->axis[1].num = atoi(v);
    } else if (!strcmp("colname", k)) {
      strcpy(t->axis[1].name, k);
    } else if (!strcmp("rowlabels", k)) {
      console_set_table_axis_labels(&t->axis[0], v);
    } else if (!strcmp("collabels", k)) {
      console_set_table_axis_labels(&t->axis[1], v);
    } else if (k[0] == '[') {
      console_set_table_element(t, k, atof(v));
    }
  }
}
static void console_get_table_axis_labels(const struct table_axis *t, char *dest) {
  char buf[32];
  strcat(dest, "[");
  for (int i = 0; i < t->num; ++i) {
    sprintf(buf, "%d", t->values[i]);
    strcat(dest, buf);
    if (i != t->num - 1) {
      strcat(dest, ",");
    }
  }
  strcat(dest, "]");
}

static void console_get_table(const struct console_config_node *self, char *dest, char *remaining) {
  assert(self);
  assert(self->val);

  const struct table *t = self->val;
  if (remaining && (remaining[0] == '[')) {
    float val;
    if (console_get_table_element(t, remaining, &val)) {
      sprintf(dest, "%f", val);
    } else {
      strcpy(dest, "invalid");
    }
  } else {
    dest += sprintf(dest, "name=%s naxis=%d rows=%d rowname=%s "
        "cols=%d colname=%s ",
        t->title, t->num_axis, 
        t->axis[0].num, t->axis[0].name, 
        t->axis[1].num, t->axis[1].name);

    strcat(dest, "rowlabels=");
    console_get_table_axis_labels(&t->axis[0], dest);

    strcat(dest, " collabels=");
    console_get_table_axis_labels(&t->axis[1], dest);
  }


}

static void console_save_to_flash(
    const struct console_config_node *self __attribute__((unused)), 
    char *rem __attribute__((unused))) {
  platform_save_config();
}

static void console_get_sensor(const struct console_config_node *self, char *dest,
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
    case METHOD_TABLE:
      method = "table";
      break;
  }

  dest += sprintf(dest, "source=%s method=%s pin=%d ", source, method, s->pin);
  if (s->source == SENSOR_CONST) {
    dest += sprintf(dest, "fixed-val=%f ", s->params.fixed_value);
  } else if (s->method == METHOD_LINEAR) {
    dest += sprintf(dest, "range-min=%f range-max=%f ", 
        s->params.range.min, s->params.range.max);
  } else if (s->method == METHOD_TABLE) {
    dest += sprintf(dest, "table=%s ", "unsupported");
  }
  sprintf(dest, "fault-min=%u fault-max=%u fault-val=%f", 
      (unsigned int)s->fault_config.min, 
      (unsigned int)s->fault_config.max, 
      s->fault_config.fault_value);

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
  } else if (!strcmp("table", str)) {
    return METHOD_TABLE;
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
  while(parse_keyval_pair(&k, &v, &remaining)) {
    if (!strcmp("source", k)) {
      s->source = console_sensor_source_from_str(v);
    } else if (!strcmp("method", k)) {
      s->method = console_sensor_method_from_str(v);
    } else if (!strcmp("pin", k)) {
      s->pin = atoi(v);
    } else if (!strcmp("fixed-val", k)) {
      s->params.fixed_value = atof(v);
    } else if (!strcmp("range-min", k)) {
      s->params.range.min = atof(v);
    } else if (!strcmp("range-max", k)) {
      s->params.range.max = atoi(v);
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
    char *dest, char *remaining __attribute__((unused))) {
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

static void console_get_decoder_loss_reason(const struct console_config_node *self, 
    char *dest, char *remaining __attribute__((unused))) {
  assert(self);
  assert(self->val);

  const decoder_loss_reason *s = self->val;
  const char *result = "";

  switch (*s) {
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
    char *dest, char *remaining __attribute__((unused))) {
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
    char *dest, char *remaining __attribute__((unused))) {
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
    char *dest, char *remaining __attribute__((unused))) {
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
  }
}

static void console_get_events(
    const struct console_config_node *self __attribute__((unused)), 
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
      case ADC_EVENT:
        ev_type = "adc";
        break;
      default:
        ev_type = "disabled";
        break;
    }
    sprintf(dest, "type=%s angle=%d output=%d inverted=%d", 
        ev_type, ev->angle, ev->output_id, ev->inverted);
  } else {
    strcpy(dest, "event out of range");
  }
}

static void console_set_events(
    const struct console_config_node *self __attribute__((unused)), 
    char *remaining) {

  char *saveptr;
  unsigned int ev_n = atoi(strtok_r(remaining, " ", &saveptr));
  if (ev_n >= config.num_events) {
    return;
  }
  struct output_event *ev = &config.events[ev_n];

  char *k, *v;
  remaining = saveptr;
  while(parse_keyval_pair(&k, &v, &remaining)) {
    if (!strcmp("type", k)) {
      if (!strcmp("fuel", v)) {
        ev->type = FUEL_EVENT;
      } else if (!strcmp("ignition", v)) {
        ev->type = IGNITION_EVENT;
      } else if (!strcmp("adc", v)) {
        ev->type = ADC_EVENT;
      } else if (!strcmp("disabled", v)) {
        ev->type = DISABLED_EVENT;
      }
    } else if (!strcmp("angle", k)) {
      ev->angle = atoi(v);
    } else if (!strcmp("output", k)) {
      ev->output_id = atoi(v);
    } else if (!strcmp("inverted", k)) {
      ev->inverted = atoi(v);
    }
  }
}

static struct console_config_node console_config_nodes[] = {
  /* Config hierarchy */
  {.name="config"},
  {.name="config.feed", .get=console_get_feed, .set=console_set_feed, 
    .val=&console_feed_config},

  {.name="config.tables"},
  {.name="config.tables.timing", .val=&timing_vs_rpm_and_map,
   .get=console_get_table, .set=console_set_table},
  {.name="config.tables.iat_timing_adjust",
   .get=console_get_table, .set=console_set_table},
  {.name="config.tables.clt_timing_adjust",
   .get=console_get_table, .set=console_set_table},
  {.name="config.tables.ve",
   .get=console_get_table, .set=console_set_table},
  {.name="config.tables.commanded_lambda",
   .get=console_get_table, .set=console_set_table},
  {.name="config.tables.injector_pw_compensation", .val=&injector_dead_time,
   .get=console_get_table, .set=console_set_table},

  /* Decoding */
  {.name="config.decoder"},
  {.name="config.decoder.trigger", .val=&config.trigger,
   .get=console_get_trigger, .set=console_set_trigger},
  {.name="config.decoder.max_variance", .val=&config.decoder.trigger_cur_rpm_change,
   .get=console_get_float, .set=console_set_float},
  {.name="config.decoder.offset", .val=&config.decoder.offset,
   .get=console_get_uint, .set=console_set_uint},
  {.name="config.decoder.min_rpm", .val=&config.decoder.trigger_min_rpm,
   .get=console_get_uint, .set=console_set_uint},

  /* Fueling */
  {.name="config.fueling"},
  {.name="config.fueling.injector_cc", .val=&config.fueling.injector_cc_per_minute,
   .get=console_get_float, .set=console_set_float},
  {.name="config.fueling.cyclinder_cc", .val=&config.fueling.cylinder_cc,
   .get=console_get_float, .set=console_set_float},
  {.name="config.fueling.fuel_stoich_ratio", .val=&config.fueling.fuel_stoich_ratio,
   .get=console_get_float, .set=console_set_float},
  {.name="config.fueling.injections_per_cycle", .val=&config.fueling.injections_per_cycle,
   .get=console_get_uint, .set=console_set_uint},

  /* Ignition */
  {.name="config.ignition"},
  {.name="config.ignition.dwell_type", .val=&config.ignition.dwell,
   .get=console_get_dwell_type, .set=console_set_dwell_type},
  {.name="config.ignition.dwell_duty", .val=&config.ignition.dwell_duty,
   .get=console_get_float, .set=console_set_float},
  {.name="config.ignition.dwell_us", .val=&config.ignition.dwell_us,
   .get=console_get_float, .set=console_set_float},
  {.name="config.ignition.min_fire_time_us", .val=&config.ignition.min_fire_time_us,
   .get=console_get_uint, .set=console_set_uint},

  /* Sensors */
  {.name="config.sensors"},
  {.name="config.sensors.map", .val=&config.sensors[SENSOR_MAP],
   .get=console_get_sensor, .set=console_set_sensor},
  {.name="config.sensors.map.value", .val=&config.sensors[SENSOR_MAP].processed_value,
   .get=console_get_float},
  {.name="config.sensors.map.fault", .val=&config.sensors[SENSOR_MAP].fault,
   .get=console_get_sensor_fault},
  {.name="config.sensors.iat", .val=&config.sensors[SENSOR_IAT],
   .get=console_get_sensor, .set=console_set_sensor},
  {.name="config.sensors.iat.value", .val=&config.sensors[SENSOR_IAT].processed_value,
   .get=console_get_float},
  {.name="config.sensors.iat.fault", .val=&config.sensors[SENSOR_IAT].fault,
   .get=console_get_sensor_fault},
  {.name="config.sensors.clt", .val=&config.sensors[SENSOR_CLT],
   .get=console_get_sensor, .set=console_set_sensor},
  {.name="config.sensors.clt.value", .val=&config.sensors[SENSOR_CLT].processed_value,
   .get=console_get_float},
  {.name="config.sensors.clt.fault", .val=&config.sensors[SENSOR_CLT].fault,
   .get=console_get_sensor_fault},
  {.name="config.sensors.brv", .val=&config.sensors[SENSOR_BRV],
   .get=console_get_sensor, .set=console_set_sensor},
  {.name="config.sensors.brv.value", .val=&config.sensors[SENSOR_BRV].processed_value,
   .get=console_get_float},
  {.name="config.sensors.brv.fault", .val=&config.sensors[SENSOR_BRV].fault,
   .get=console_get_sensor_fault},
  {.name="config.sensors.tps", .val=&config.sensors[SENSOR_TPS],
   .get=console_get_sensor, .set=console_set_sensor},
  {.name="config.sensors.tps.value", .val=&config.sensors[SENSOR_TPS].processed_value,
   .get=console_get_float},
  {.name="config.sensors.tps.fault", .val=&config.sensors[SENSOR_TPS].fault,
   .get=console_get_sensor_fault},
  {.name="config.sensors.aap", .val=&config.sensors[SENSOR_AAP],
   .get=console_get_sensor, .set=console_set_sensor},
  {.name="config.sensors.aap.value", .val=&config.sensors[SENSOR_AAP].processed_value,
   .get=console_get_float},
  {.name="config.sensors.aap.fault", .val=&config.sensors[SENSOR_AAP].fault,
   .get=console_get_sensor_fault},
  {.name="config.sensors.frt", .val=&config.sensors[SENSOR_FRT],
   .get=console_get_sensor, .set=console_set_sensor},
  {.name="config.sensors.frt.value", .val=&config.sensors[SENSOR_FRT].processed_value,
   .get=console_get_float},
  {.name="config.sensors.frt.fault", .val=&config.sensors[SENSOR_FRT].fault,
   .get=console_get_sensor_fault},
  {.name="config.sensors.ego", .val=&config.sensors[SENSOR_EGO],
   .get=console_get_sensor, .set=console_set_sensor},
  {.name="config.sensors.ego.value", .val=&config.sensors[SENSOR_EGO].processed_value,
   .get=console_get_float},
  {.name="config.sensors.ego.fault", .val=&config.sensors[SENSOR_EGO].fault,
   .get=console_get_sensor_fault},

  {.name="config.events", .get=console_get_events, .set=console_set_events},

  {.name="status"},
  {.name="status.current_time", .get=console_get_time},
  {.name="status.timing_advance", 
    .val=&calculated_values.timing_advance, .get=console_get_float},
  {.name="status.decoder_state", .val=&config.decoder.state,
   .get=console_get_decoder_state},
  {.name="status.decoder_loss_reason", .val=&config.decoder.loss,
   .get=console_get_decoder_loss_reason},
  {.name="status.decoder_t0_count", .val=&config.decoder.t0_count,
   .get=console_get_uint},
  {.name="status.decoder_t1_count", .val=&config.decoder.t1_count,
   .get=console_get_uint},
  {.name="status.rpm", .val=&config.decoder.rpm,
   .get=console_get_uint},
  {.name="status.rpm_variance", .val=&config.decoder.trigger_cur_rpm_change,
   .get=console_get_float},
  {.name="status.rpm_cut", .val=&calculated_values.rpm_limit_cut,
   .get=console_get_uint},
  {.name="status.fueling_us", .val=&calculated_values.fueling_us,
   .get=console_get_uint},
  {.name="status.dwell_us", .val=&calculated_values.dwell_us,
   .get=console_get_uint},

  /* Misc commands */
  {.name="flash", .set=console_save_to_flash},
  {0},
};

/* Lists all immediate prefixes in node list nodes */
static void console_list_prefix(const struct console_config_node *nodes, 
  char *dest, const char *prefix) {

  const struct console_config_node *node;
  for (node = nodes; node->name; node++) {
    /* If prefix doesn't match */
    if (strncmp(node->name, prefix, strlen(prefix))) {
      continue;
    }
    /* It looks like the root name, or no extra element */
    if (strlen(node->name) <= strlen(prefix) + 1) {
      continue;
    }
    /* Contains another delimiter, so not an immediate subelement */
    if (strchr(node->name + strlen(prefix) + 1, '.')) {
      continue;
    }
    strcat(dest, node->name);
    strcat(dest, " ");
  }
}


void console_parse_request(char *dest, char *line) {
  char *action = strtok(line, " ");
  char *var = strtok(NULL, " ");
  char *rem = strtok(NULL, "\0");

  if (!action || !var) {
    strcpy(dest, "invalid input");
    return;
  }

  const struct console_config_node *node = console_search_node(console_config_nodes, var);
  if (!node) {
    strcat(dest, "Config node not found");
    return;
  }
  if (!strcmp("list", action)) {
    console_list_prefix(console_config_nodes, dest, var);
  } else if (!strcmp("get", action)) {
    if (node->get) {
      node->get(node, dest, rem);
    } else {
      strcat(dest, "Config node does not support get");
    }
  } else if (!strcmp("set", action)) {
    if (node->set) {
      node->set(node, rem);
    } else {
      strcat(dest, "Config node does not support set");
    } 
  } else {
    strcat(dest, "invalid action");
  }
}

void console_init() {
  const char *console_feed_defaults[] = {
    "status.current_time",
    "status.decoder_state",
    "status.rpm",
    "status.rpm_variance",
    "status.timing_advance",
    "config.sensors.map.value",
    "config.sensors.map.fault",
    "config.sensors.iat.value",
    "config.sensors.iat.fault",
    "config.sensors.clt.value",
    "config.sensors.clt.fault",
    "config.sensors.brv.value",
    "config.sensors.brv.fault",
    "config.sensors.tps.value",
    "config.sensors.tps.fault",
    "config.sensors.aap.value",
    "config.sensors.aap.fault",
    "config.sensors.frt.value",
    "config.sensors.frt.fault",
    "config.sensors.ego.value",
    "config.sensors.ego.fault",
    NULL,
  };

  int count = 0;
  while (console_feed_defaults[count]) {
    console_feed_config.nodes[count] = 
      console_search_node(console_config_nodes, console_feed_defaults[count]);
    count++;
  }
  console_feed_config.n_nodes = count;
}

static void console_feed_line(char *dest) {

  unsigned int i;
  char temp[64];
  for (i = 0; i < console_feed_config.n_nodes; i++) {
    const struct console_config_node *node = console_feed_config.nodes[i];
    strcpy(temp, "");
    if (!node->get) {
      continue;
    }
    node->get(node, temp, "");
    strcat(dest, temp);
    if (i < console_feed_config.n_nodes - 1) {
      strcat(dest, ",");
    }
  }
  strcat(dest, "\r\n");
}

static void console_process_rx() {
  char *out = config.console.txbuffer;
  char *in = strtok(config.console.rxbuffer, "\n");
  out += sprintf(out, "* ");
  console_parse_request(out, in);
  strcat(out, "\r\n");
  usart_tx(config.console.txbuffer, strlen(config.console.txbuffer));
  usart_rx_reset();

}

void console_process() {

  stats_start_timing(STATS_CONSOLE_TIME);
  if (usart_rx_ready()) {
    console_process_rx();
  }

  /*  if tx buffer is usable, print regular status update */
  if (!usart_tx_ready()) {
    stats_finish_timing(STATS_CONSOLE_TIME);
    return;
  }

  config.console.txbuffer[0] = '\0';
  if (console_feed_config.n_nodes) {
    console_feed_line(config.console.txbuffer);
    usart_tx(config.console.txbuffer, strlen(config.console.txbuffer));
  }

  stats_finish_timing(STATS_CONSOLE_TIME);
}

#ifdef UNITTEST
#include <check.h>

static struct console_config_node test_nodes[] = {
  {.name="test1"},
  {.name="test2"},
  {.name="test2.testA"},
  {.name="test3.testA"},
  {.name="test3.testA.test1"},
  {0},
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
  ck_assert_ptr_eq(console_search_node(test_nodes, "test2.testA"), &test_nodes[2]);
  ck_assert_ptr_eq(console_search_node(test_nodes, "test3.testA.test1"), &test_nodes[4]);

} END_TEST

START_TEST(check_console_get_time) {
  char buf[32] = {0};
  console_get_time(NULL, buf, NULL);
  ck_assert(strlen(buf) != 0);
} END_TEST

START_TEST(check_console_get_float) {
  float v;
  struct console_config_node t = {
    .val = &v,
  };

  v = 1.0f;
  char buf[32];
  console_get_float(&t, buf, NULL);
  ck_assert_str_eq(buf, "1.00");
} END_TEST

START_TEST(check_console_set_float) {
  float v;
  struct console_config_node t = {
    .val = &v,
  };

  char buf[] = "1.00";
  console_set_float(&t, buf);
  ck_assert_float_eq(v, 1.00);
} END_TEST

START_TEST(check_console_get_uint) {
  unsigned int v;
  struct console_config_node t = {
    .val = &v,
  };

  v = 200;
  char buf[32];
  console_get_uint(&t, buf, NULL);
  ck_assert_str_eq(buf, "200");
} END_TEST

START_TEST(check_console_set_uint) {
  unsigned int v;
  struct console_config_node t = {
    .val = &v,
  };

  char buf[] = "200";
  console_set_uint(&t, buf);
  ck_assert_uint_eq(v, 200);
} END_TEST

START_TEST(check_parse_keyval_pair) {
  char *buf = malloc(64);

  strcpy(buf, "");
  char *k = NULL, *v = NULL;

  ck_assert_int_eq(parse_keyval_pair(&k, &v, &buf), 0);
  ck_assert_ptr_null(k);
  ck_assert_ptr_null(v);

  k = NULL; k = NULL;
  strcpy(buf, "");
  ck_assert_int_eq(parse_keyval_pair(&k, &v, &buf), 0);

  k = NULL; k = NULL;
  strcpy(buf, "keywithnoval");
  ck_assert_int_eq(parse_keyval_pair(&k, &v, &buf), 0);

  k = NULL; k = NULL;
  strcpy(buf, "keywithnoval=");
  ck_assert_int_eq(parse_keyval_pair(&k, &v, &buf), 0);

  k = NULL; k = NULL;
  strcpy(buf, "keywithval=val");
  ck_assert_int_eq(parse_keyval_pair(&k, &v, &buf), 1);
  ck_assert_str_eq(k, "keywithval");
  ck_assert_str_eq(v, "val");

  k = NULL; k = NULL;
  strcpy(buf, "keywithval=val anotherkey=anotherval");
  ck_assert_int_eq(parse_keyval_pair(&k, &v, &buf), 1);
  ck_assert_str_eq(k, "keywithval");
  ck_assert_str_eq(v, "val");

  ck_assert_int_eq(parse_keyval_pair(&k, &v, &buf), 1);
  ck_assert_str_eq(k, "anotherkey");
  ck_assert_str_eq(v, "anotherval");

} END_TEST

TCase *setup_console_tests() {
  TCase *console_tests = tcase_create("console");
  tcase_add_test(console_tests, check_console_search_node);
  tcase_add_test(console_tests, check_console_get_time);
  tcase_add_test(console_tests, check_console_get_float);
  tcase_add_test(console_tests, check_console_set_float);
  tcase_add_test(console_tests, check_console_get_uint);
  tcase_add_test(console_tests, check_console_set_uint);
  tcase_add_test(console_tests, check_parse_keyval_pair);
  return console_tests;
}

#endif
