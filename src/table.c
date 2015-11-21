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
