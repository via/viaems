#include <assert.h>

#include "table.h"

/* Find the highest axis position that is not greater than the provided value */
static int axis_find_cell_lower(const struct table_axis *axis, float value) {
  assert(axis->num > 0);
  assert(value >= axis->values[0]);
  assert(value <= axis->values[axis->num - 1]);

  int x1 = 0;
  int len = axis->num;
  int middle = x1 + (len / 2);

  while (len > 1) {
    if (value > axis->values[middle]) {
      x1 = middle;
    }
    len = (len + 1) / 2;
    middle = x1 + (len / 2);
  }
  return x1;
}

float interpolate_table_oneaxis(const struct table_1d *t, float val) {
  const struct table_axis *axis = &t->cols;

  /* Clamp to bottom */
  if (val < axis->values[0]) {
    val = axis->values[0];
  }

  /* Clamp to top */
  if (val > axis->values[axis->num - 1]) {
    val = axis->values[axis->num - 1];
  }

  const int index = axis_find_cell_lower(axis, val);

  const float x1 = axis->values[index];
  const float x2 = axis->values[index + 1];

  const float partial = (val - x1) / (x2 - x1);
  const float first_val = t->data[index];
  const float second_val = t->data[index + 1];
  return ((second_val - first_val) * partial) + first_val;
}

float interpolate_table_twoaxis(const struct table_2d *t, float x, float y) {
  const struct table_axis *xaxis = &t->cols;
  const struct table_axis *yaxis = &t->rows;

  /* Clamp X to bottom */
  if (x < xaxis->values[0]) {
    x = xaxis->values[0];
  }
  /* Clamp X to top */
  if (x > xaxis->values[xaxis->num - 1]) {
    x = xaxis->values[xaxis->num - 1];
  }
  /* Clamp Y to bottom */
  if (y < yaxis->values[0]) {
    y = yaxis->values[0];
  }
  /* Clamp Y to top */
  if (y > yaxis->values[yaxis->num - 1]) {
    y = yaxis->values[yaxis->num - 1];
  }

  const int x1_ind = axis_find_cell_lower(xaxis, x);
  const int y1_ind = axis_find_cell_lower(yaxis, y);

  const float x1 = xaxis->values[x1_ind];
  const float x2 = xaxis->values[x1_ind + 1];
  const float y1 = yaxis->values[y1_ind];
  const float y2 = yaxis->values[y1_ind + 1];

  const float xy1 = (x2 - x) / (x2 - x1) * t->data[y1_ind][x1_ind] +
                    (x - x1) / (x2 - x1) * t->data[y1_ind][x1_ind + 1];
  const float xy2 = (x2 - x) / (x2 - x1) * t->data[y1_ind + 1][x1_ind] +
                    (x - x1) / (x2 - x1) * t->data[y1_ind + 1][x1_ind + 1];
  const float xy = (y2 - y) / (y2 - y1) * xy1 + (y - y1) / (y2 - y1) * xy2;
  return xy;
}

static int table_valid_axis(const struct table_axis *a) {
  if (a->num > MAX_AXIS_SIZE) {
    return 0;
  }
  for (unsigned i = 0; i < a->num - 1; i++) {
    if (a->values[i] > a->values[i + 1]) {
      return 0;
    }
  }
  return 1;
}

int table_valid_oneaxis(const struct table_1d *t) {

  if (!table_valid_axis(&t->cols)) {
    return 0;
  }
  return 1;
}

int table_valid_twoaxis(const struct table_2d *t) {

  if (!table_valid_axis(&t->cols)) {
    return 0;
  }
  if (!table_valid_axis(&t->rows)) {
    return 0;
  }
  return 1;
}

#ifdef UNITTEST
#include <check.h>

static struct table_1d t1 = {
  .cols = {
    .num = 5,
    .values = { 5, 10, 12, 15, 20 },
  },
  .data = { 50, 100, 120, 150, 200 },
};

static struct table_2d t2 = {
  .cols = {
    .num = 4,
    .values = {5, 10, 15, 20},
  },
  .rows = {
    .num = 4,
    .values = {-50, -40, -30, -20}
  },
  .data = {
    {50, 100, 250, 200},
    {60, 110, 260, 210},
    {70, 120, 270, 220},
    {80, 130, 280, 230},
  },
};

START_TEST(check_axis_find_cell) {
  const struct table_axis oddaxis = {
    .num = 3,
    .values = { 10, 20, 30 },
  };

  ck_assert_int_eq(axis_find_cell_lower(&oddaxis, 10), 0);
  ck_assert_int_eq(axis_find_cell_lower(&oddaxis, 15), 0);
  ck_assert_int_eq(axis_find_cell_lower(&oddaxis, 20), 0);
  ck_assert_int_eq(axis_find_cell_lower(&oddaxis, 25), 1);
  ck_assert_int_eq(axis_find_cell_lower(&oddaxis, 30), 1);

  const struct table_axis evenaxis = {
    .num = 6,
    .values = { 10, 20, 30, 40, 50, 60 },
  };

  ck_assert_int_eq(axis_find_cell_lower(&evenaxis, 10), 0);
  ck_assert_int_eq(axis_find_cell_lower(&evenaxis, 15), 0);
  ck_assert_int_eq(axis_find_cell_lower(&evenaxis, 20), 0);
  ck_assert_int_eq(axis_find_cell_lower(&evenaxis, 25), 1);
  ck_assert_int_eq(axis_find_cell_lower(&evenaxis, 30), 1);
  ck_assert_int_eq(axis_find_cell_lower(&evenaxis, 35), 2);
  ck_assert_int_eq(axis_find_cell_lower(&evenaxis, 40), 2);
  ck_assert_int_eq(axis_find_cell_lower(&evenaxis, 45), 3);
  ck_assert_int_eq(axis_find_cell_lower(&evenaxis, 50), 3);
  ck_assert_int_eq(axis_find_cell_lower(&evenaxis, 55), 4);
  ck_assert_int_eq(axis_find_cell_lower(&evenaxis, 60), 4);
}
END_TEST

START_TEST(check_table_oneaxis_interpolate) {
  ck_assert(interpolate_table_oneaxis(&t1, 7.5) == 75);
  ck_assert(interpolate_table_oneaxis(&t1, 5) == 50);
  ck_assert(interpolate_table_oneaxis(&t1, 10) == 100);
  ck_assert(interpolate_table_oneaxis(&t1, 20) == 200);
}
END_TEST

START_TEST(check_table_twoaxis_interpolate) {
  ck_assert(interpolate_table_twoaxis(&t2, 5, -50) == 50);
  ck_assert(interpolate_table_twoaxis(&t2, 7.5, -50) == 75);
  ck_assert(interpolate_table_twoaxis(&t2, 5, -45) == 55);
  ck_assert(interpolate_table_twoaxis(&t2, 7.5, -45) == 80);
}
END_TEST

START_TEST(check_table_oneaxis_fullsize_clamp) {
  struct table_1d table = {
    .cols = {
      .num = 24,
    },
  };

  for (int i = 0; i < 24; i++) {
    table.cols.values[i] = i;
    table.data[i] = i;
  }

  /* Check value at lowest index */
  ck_assert(interpolate_table_oneaxis(&table, 0) == 0);

  /* Check clamping at low end */
  ck_assert(interpolate_table_oneaxis(&table, -10) == 0);

  /* Check value at highest index */
  ck_assert(interpolate_table_oneaxis(&table, 23) == 23);

  /* Check clamping at highest index */
  ck_assert(interpolate_table_oneaxis(&table, 25) == 23);
}
END_TEST

START_TEST(check_table_twoaxis_fullsize_clamp) {
  struct table_2d table = {
    .cols = {
      .num = 24,
    },
    .rows = {
      .num = 24,
    },
  };

  for (int i = 0; i < 24; i++) {
    table.cols.values[i] = i;
    table.rows.values[i] = i;
    for (int j = 0; j < 24; j++) {
      table.data[i][j] = i + j;
    }
  }

  /*
   *   1      2      3
   *      * * * * *
   *      * * * * *
   *   4  * * * * *  5
   *      * * * * *
   *      * * * * *
   *   6      7      8
   */

  /* Check 1 */
  ck_assert(interpolate_table_twoaxis(&table, -10, -10) == 0);

  /* Check 2 */
  ck_assert(interpolate_table_twoaxis(&table, 10, -10) == 10);

  /* Check 3 */
  ck_assert(interpolate_table_twoaxis(&table, 25, -10) == 23);

  /* Check 4 */
  ck_assert(interpolate_table_twoaxis(&table, -10, 10) == 10);

  /* Check 5 */
  ck_assert(interpolate_table_twoaxis(&table, 25, 10) == 33);

  /* Check 6 */
  ck_assert(interpolate_table_twoaxis(&table, -10, 25) == 23);

  /* Check 7 */
  ck_assert(interpolate_table_twoaxis(&table, 10, 25) == 33);

  /* Check 8 */
  ck_assert(interpolate_table_twoaxis(&table, 25, 25) == 46);

  /* Check the four corners */
  ck_assert(interpolate_table_twoaxis(&table, 0, 0) == 0);
  ck_assert(interpolate_table_twoaxis(&table, 0, 23) == 23);
  ck_assert(interpolate_table_twoaxis(&table, 23, 0) == 23);
  ck_assert(interpolate_table_twoaxis(&table, 23, 23) == 46);
}
END_TEST

TCase *setup_table_tests() {
  TCase *table_tests = tcase_create("tables");
  tcase_add_test(table_tests, check_axis_find_cell);
  tcase_add_test(table_tests, check_table_oneaxis_interpolate);
  tcase_add_test(table_tests, check_table_twoaxis_interpolate);
  tcase_add_test(table_tests, check_table_oneaxis_fullsize_clamp);
  tcase_add_test(table_tests, check_table_twoaxis_fullsize_clamp);
  return table_tests;
}

#endif
