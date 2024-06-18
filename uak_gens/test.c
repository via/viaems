#include <stdint.h>

#include "md_cortex_m.h"
#include "syscalls.h"

int entry(void) {
  uak_syscall_set_pwm(2, 0.5f);
  return uak_syscall_get_cycle_count();
}

int exception(int n, void *a) {
	uak_syscall_dispatcher(n, a);
}

