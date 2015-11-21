#include "table.h"

float
interpolate_table_oneaxis(struct table *t, float val) {
  if (t->num_axis != 1) {
    while(1);
  }
  /* Clamp to bottom */
  if (val < t->axis[0].values[0]) {
    val = t->axis[0].values[0];
  }
  /* Clamp to top */
  if (val > t->axis[0].values[t->axis[0].num - 1]) {
    val = t->axis[0].values[t->axis[0].num - 1];
  }
  for (int x = 0; x < t->axis[0].num - 1; ++x) {
    float first_axis = t->axis[0].values[x];
    float second_axis = t->axis[0].values[x + 1];
    if ((val >= first_axis) &&
        (val <= second_axis)) {
      float partial = (val - first_axis) / (second_axis - first_axis);
      float first_val = t->data.one[x];
      float second_val = t->data.one[x + 1];
      return ((second_val - first_val) * partial) + first_val;
    }
  }
  return 0;
}

float
interpolate_table_twoaxis(struct table *t, float x, float y) {
  float x1 = 0, x2 = 0, y1 = 0, y2 = 0;
  int x1_ind = 0, y1_ind = 0;
  float xy1, xy2, xy;
  if (t->num_axis != 2) {
    while(1);
  }
  /* Clamp X to bottom */
  if (x < t->axis[0].values[0]) {
    x = t->axis[0].values[0];
  }
  /* Clamp X to top */
  if (x > t->axis[0].values[t->axis[0].num - 1]) {
    x = t->axis[0].values[t->axis[0].num - 1];
  }
  /* Clamp Y to bottom */
  if (y < t->axis[1].values[0]) {
    y = t->axis[1].values[0];
  }
  /* Clamp Y to top */
  if (y > t->axis[1].values[t->axis[1].num - 1]) {
    y = t->axis[1].values[t->axis[1].num - 1];
  }
  for (x1_ind = 0; x1_ind < t->axis[0].num - 1; ++x1_ind) {
    x1 = t->axis[0].values[x1_ind];
    x2 = t->axis[0].values[x1_ind + 1];
    if ((x >= x1) && (x <= x2)) {
      break;
    }
  }
  for (y1_ind = 0; y1_ind < t->axis[1].num - 1; ++y1_ind) {
    y1 = t->axis[1].values[y1_ind];
    y2 = t->axis[1].values[y1_ind + 1];
    if ((y >= y1) && (y <= y2)) {
      break;
    }
  }
  xy1 = (x2 - x) / (x2 - x1) * t->data.two[y1_ind][x1_ind] + 
        (x - x1) / (x2 - x1) * t->data.two[y1_ind][x1_ind + 1];
  xy2 = (x2 - x) / (x2 - x1) * t->data.two[y1_ind + 1][x1_ind] + 
        (x - x1) / (x2 - x1) * t->data.two[y1_ind + 1][x1_ind + 1];
  xy = (y2 - y) / (y2 - y1) * xy1 +
       (y - y1) / (y2 - y1) * xy2;
  return xy;
}

