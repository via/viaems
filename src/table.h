#ifndef _TABLE_H
#define _TABLE_H

#define MAX_AXIS_SIZE 24

struct table_axis {
  char name[32];
  unsigned char num; /* Max of 24 */
  float values[MAX_AXIS_SIZE];
};
  

struct table {
  char title[32];
  unsigned int num_axis;
  struct table_axis axis[2];
  union {
    float one [MAX_AXIS_SIZE];
    float two [MAX_AXIS_SIZE][MAX_AXIS_SIZE];
  } data;
};

float interpolate_table_oneaxis(struct table *, float a);
float interpolate_table_twoaxis(struct table *, float a1, float a2);

int table_valid(struct table *);

#ifdef UNITTEST
#include <check.h>
TCase *setup_table_tests();
#endif

#endif

