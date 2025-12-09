#ifndef _TABLE_H
#define _TABLE_H

#include <stdint.h>

#define MAX_AXIS_SIZE 24
#define MAX_TABLE_TITLE_SIZE 24

struct table_axis {
  char name[MAX_TABLE_TITLE_SIZE + 1];
  uint32_t num;
  float values[MAX_AXIS_SIZE];
};

struct table_1d {
  char title[MAX_TABLE_TITLE_SIZE + 1];
  struct table_axis cols;
  float data[MAX_AXIS_SIZE];
};

struct table_2d {
  char title[MAX_TABLE_TITLE_SIZE + 1];
  struct table_axis rows;
  struct table_axis cols;
  float data[MAX_AXIS_SIZE][MAX_AXIS_SIZE];
};

float interpolate_table_oneaxis(const struct table_1d *, float column);
float interpolate_table_twoaxis(const struct table_2d *,
                                float row,
                                float column);

int table_valid_oneaxis(const struct table_1d *);
int table_valid_twoaxis(const struct table_2d *);

#ifdef UNITTEST
#include <check.h>
TCase *setup_table_tests(void);
#endif

#endif
