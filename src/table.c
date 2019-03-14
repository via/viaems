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

static int table_valid_axis(struct table_axis *a) {
  if (a->num > MAX_AXIS_SIZE) {
    return 0;
  }
  for (int i = 0; i < a->num - 1; i++) {
    if (a->values[i] > a->values[i + 1]) {
      return 0;
    }
  }
  return 1;
}

int table_valid(struct table *t) {
  
  if ((t->num_axis != 1) && (t->num_axis != 2)) {
    return 0;
  }

  if (!table_valid_axis(&t->axis[0])) {
    return 0;
  }

  if ((t->num_axis == 2) && !table_valid_axis(&t->axis[1])) {
    return 0;
  }

  return 1;
}


#ifdef UNITTEST
#include <check.h>

static struct table t1 = {
  .num_axis = 1,
  .axis = { { 
    .num = 4,
    .values = {5, 10, 15, 20},
   } },
   .data = {
     .one = {50, 100, 150, 200}
   },
};

static struct table t2 = {
  .num_axis = 2,
  .axis = { { 
    .num = 4,
    .values = {5, 10, 15, 20},
   }, {
    .num = 4,
    .values = {-50, -40, -30, -20}
    } },
   .data = {
     .two = { {50, 100, 250, 200},
              {60, 110, 260, 210},
              {70, 120, 270, 220},
              {80, 130, 280, 230} },
   },
};

START_TEST(check_table_oneaxis_interpolate) {
  ck_assert(interpolate_table_oneaxis(&t1, 7.5) == 75);
  ck_assert(interpolate_table_oneaxis(&t1, 5) == 50);
  ck_assert(interpolate_table_oneaxis(&t1, 10) == 100);
  ck_assert(interpolate_table_oneaxis(&t1, 20) == 200);
} END_TEST

START_TEST(check_table_oneaxis_clamp) {
  ck_assert(interpolate_table_oneaxis(&t1, 0) == 50);
  ck_assert(interpolate_table_oneaxis(&t1, 30) == 200);
} END_TEST

START_TEST(check_table_twoaxis_interpolate) {
  ck_assert(interpolate_table_twoaxis(&t2, 5, -50) == 50);
  ck_assert(interpolate_table_twoaxis(&t2, 7.5, -50) == 75);
  ck_assert(interpolate_table_twoaxis(&t2, 5, -45) == 55);
  ck_assert(interpolate_table_twoaxis(&t2, 7.5, -45) == 80);
} END_TEST

START_TEST(check_table_twoaxis_clamp) {
  ck_assert(interpolate_table_twoaxis(&t2, 10, -60) == 100);
  ck_assert(interpolate_table_twoaxis(&t2, 10, 0) == 130);
  ck_assert(interpolate_table_twoaxis(&t2, 0, -40) == 60);
  ck_assert(interpolate_table_twoaxis(&t2, 30, -40) == 210);
  ck_assert(interpolate_table_twoaxis(&t2, 30, -45) == 205);
} END_TEST

TCase *setup_table_tests() {
  TCase *table_tests = tcase_create("tables");
  tcase_add_test(table_tests, check_table_oneaxis_interpolate);
  tcase_add_test(table_tests, check_table_oneaxis_clamp);
  tcase_add_test(table_tests, check_table_twoaxis_interpolate);
  tcase_add_test(table_tests, check_table_twoaxis_clamp);
  return table_tests;
}

#endif

