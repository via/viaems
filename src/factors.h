#ifndef FACTORS_H
#define FACTORS_H

struct analog_inputs {
  unsigned int IAT;
  unsigned int CLT;
  unsigned int MAP;
};

unsigned int lookup_advance(unsigned int MAP, unsigned int RPM);

#endif
