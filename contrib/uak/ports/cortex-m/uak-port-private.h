#ifndef PORTS_CORTEX_M_PORT_PRIVATE_H
#define PORTS_CORTEX_M_PORT_PRIVATE_H

struct cortex_mpu_region {
  uint32_t rbar;
  uint32_t rasr;
};

struct uak_port_context {
  void *context;
  struct mpu_region regions[8];
};

#endif
