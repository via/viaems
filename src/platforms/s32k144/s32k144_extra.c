#include <stdint.h>
#include "platform.h"

#include "cortex-m4.h"


void schedule_event_timer(timeval_t t) {
}

uint64_t cycle_count(void);
uint64_t cycles_to_ns(uint64_t cycles);

void disable_interrupts() {
  __asm__("cpsid i");
}

void enable_interrupts() {
  __asm__("cpsie i");
}

uint64_t cycles_to_ns(uint64_t cycles) {
  return (uint64_t)cycles * 1000 / 80;
}

uint64_t cycle_count() {
  return *DWT_CYCCNT;
}

bool interrupts_enabled() {
  uint32_t result;
  __asm__ volatile ("MRS %0, primask" : "=r" (result) );
  return result == 0;
}


extern unsigned _configdata_loadaddr, _sconfigdata, _econfigdata;
void platform_load_config() {
  volatile unsigned *src, *dest;
  for (src = &_configdata_loadaddr, dest = &_sconfigdata; dest < &_econfigdata;
       src++, dest++) {
    *dest = *src;
  }
}


uint32_t platform_adc_samplerate(void) { return 5000; }
uint32_t platform_knock_samplerate(void) { return 5000; }

timeval_t platform_output_earliest_schedulable_time(void) { return 0; }
void platform_benchmark_init(void) { }

void set_gpio(int output, char v) { }
void set_pwm(int output, float v) { }

void platform_save_config(void) { }

void platform_reset_into_bootloader(void) { }

void set_sim_wakeup(timeval_t t) {  }
