#ifndef _CALCULATIONS_H
#define _CALCULATIONS_H

struct calculated_values {
  float timing_advance;
  int rpm_limit_cut;
};

extern struct calculated_values calculated_values;

void calculate_ignition();
void calculate_fueling();
int ignition_cut();

#endif
