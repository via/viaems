#include <stdbool.h>
#include <stdint.h>

#include "config.h"
#include "decoder.h"
#include "gd32f4xx_fwdgt.h"
#include "gd32f4xx_rcu.h"
#include "platform.h"
#include "tasks.h"
#include "viaems.h"

#include "gd32f4xx.h"

struct viaems gd32f4_viaems;

void platform_enable_event_logging() {}

void platform_disable_event_logging() {}

uint64_t cycles_to_ns(uint64_t cycles) {
  return cycles * 1000 / 192;
}

uint64_t cycle_count() {
  return DWT->CYCCNT;
}

static void setup_dwt() {
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  DWT->CYCCNT = 0;
}

int __attribute__((externally_visible)) _write(int fd,
                                               const char *buf,
                                               size_t count) {
  (void)fd;
  size_t pos = 0;
  while (pos < count) {
    pos += console_write(buf + pos, count - pos);
  }
  return count;
}

void reset_watchdog() {
  FWDGT_CTL = 0x0000AAAA;
}

void set_gpio_port(uint32_t value) {
  GPIO_OCTL(GPIOE) = value;
}

static void setup_watchdog() {
  DBG_CTL1 |= DBG_CTL1_FWDGT_HOLD;

  FWDGT_CTL = 0x0000CCCC; /* Start watchdog */
  FWDGT_CTL = 0x00005555; /* Magic unlock sequence */
  FWDGT_RLD = 250;        /* Approx 35 mS */
  reset_watchdog();
}

static void setup_gpios(void) {
  GPIO_CTL(GPIOE) = 0x55555555;  /* All of GPIOD is output */
  GPIO_OSPD(GPIOE) = 0xffffffff; /* All GPIOD set to High speed*/
}

static void platform_disable_periphs(void) {
  rcu_periph_clock_disable(RCU_PMU);
  rcu_periph_clock_disable(RCU_SYSCFG);

  rcu_periph_clock_disable(RCU_TIMER0);
  rcu_periph_clock_disable(RCU_TIMER1);
  rcu_periph_clock_disable(RCU_TIMER2);
  rcu_periph_clock_disable(RCU_TIMER7);
  rcu_periph_clock_disable(RCU_TIMER8);

  rcu_periph_clock_disable(RCU_GPIOA);
  rcu_periph_clock_disable(RCU_GPIOC);
  rcu_periph_clock_disable(RCU_GPIOD);
  rcu_periph_clock_disable(RCU_GPIOE);

  rcu_periph_clock_disable(RCU_SPI0);

  rcu_periph_clock_disable(RCU_DMA1);
  rcu_periph_clock_disable(RCU_USBFS);
}

static void platform_enable_periphs(void) {
  rcu_periph_clock_enable(RCU_PMU);
  rcu_periph_clock_enable(RCU_SYSCFG);

  rcu_periph_clock_enable(RCU_TIMER0);
  rcu_periph_clock_enable(RCU_TIMER1);
  rcu_periph_clock_enable(RCU_TIMER2);
  rcu_periph_clock_enable(RCU_TIMER7);
  rcu_periph_clock_enable(RCU_TIMER8);

  rcu_periph_clock_enable(RCU_GPIOA);
  rcu_periph_clock_enable(RCU_GPIOC);
  rcu_periph_clock_enable(RCU_GPIOD);
  rcu_periph_clock_enable(RCU_GPIOE);

  rcu_periph_clock_enable(RCU_SPI0);

  rcu_periph_clock_enable(RCU_DMA1);
  rcu_periph_clock_enable(RCU_USBFS);
}

static void system_init(void) {
  SCB->CPACR |=
    ((3UL << 10 * 2) | (3UL << 11 * 2)); /* set CP10 and CP11 Full Access */

  rcu_deinit();
  platform_enable_periphs();

  /* Enable 8 MHz Crystal and wait for it to stablize */
  rcu_osci_on(RCU_HXTAL);
  rcu_osci_stab_wait(RCU_HXTAL);

  pmu_ldo_output_select(PMU_LDOVS_HIGH);

  rcu_ahb_clock_config(RCU_AHB_CKSYS_DIV1);   /* 192 MHz */
  rcu_apb1_clock_config(RCU_APB1_CKAHB_DIV4); /* 48 MHz */
  rcu_apb2_clock_config(RCU_APB2_CKAHB_DIV2); /* 96 MHz */

  /* Configure the main PLL: Divide by crystal to get 1 MHz, multiple
   * by 384 to get a frequency that works on any f450 or 470 */

  /* example: PSC = 8, PLL_N = 384, PLL_P = 2, PLL_Q = 10 */
  rcu_pll_config(RCU_PLLSRC_HXTAL, CRYSTAL_FREQ, 384, 2, 8);

  rcu_osci_on(RCU_PLL_CK);
  rcu_osci_stab_wait(RCU_PLL_CK);

  pmu_highdriver_mode_enable();
  pmu_highdriver_switch_select(PMU_HIGHDR_SWITCH_EN);

  rcu_system_clock_source_config(RCU_CKSYSSRC_PLLP);
}

#define BOOTLOADER_FLAG 0x56780123
static uint32_t bootloader_flag = 0;
void platform_reset_into_bootloader() {
  bootloader_flag = BOOTLOADER_FLAG;
  NVIC_SystemReset();
}

/* Common symbols exported by the linker script(s): */
extern uint32_t _data_loadaddr, _sdata, _edata, _ebss;

void platform_configure(bool is_benchmark);
void platform_load_config();

void reset_handler(void) {
  if (bootloader_flag == BOOTLOADER_FLAG) {
    bootloader_flag = 0;
    /* We've set this flag and reset the cpu, jump to system bootloader */
    uint32_t *bootloader_msp = (uint32_t *)0x1fff0000;
    uint32_t *bootloader_addr = bootloader_msp + 1;

    /* Ensure bootloader is mapped at 0x00000000 */
    rcu_periph_clock_enable(RCU_SYSCFG);
    syscfg_bootmode_config(SYSCFG_BOOTMODE_BOOTLOADER);
    __set_MSP(*bootloader_msp);
    void (*bootloader)() = (void (*)(void))(*bootloader_addr);
    bootloader();
  }

  volatile uint32_t *src, *dest;
  for (src = &_data_loadaddr, dest = &_sdata; dest < &_edata; src++, dest++) {
    *dest = *src;
  }

  while (dest < &_ebss) {
    *dest++ = 0;
  }

  system_init();

  platform_load_config();
  bool benchmark_enabled = false;
#ifdef BENCHMARK
  benchmark_enabled = true;
#endif

  if (benchmark_enabled) {
    platform_configure(true);
    int start_benchmarks(void);
    start_benchmarks();
  } else {
    viaems_init(&gd32f4_viaems, &default_config);
    platform_configure(false);

    while (true) {
      viaems_idle(&gd32f4_viaems, current_time());
    }
  }
}
extern uint32_t _configdata_loadaddr, _sconfigdata, _econfigdata;
void platform_load_config() {
  uint32_t *src, *dest;
  for (src = &_configdata_loadaddr, dest = &_sconfigdata; dest < &_econfigdata;
       src++, dest++) {
    *dest = *src;
  }
}

void platform_save_config() {
  __disable_irq();
  platform_disable_periphs();

  /* Ensure watchdog won't stop us from flashing */
  FWDGT_CTL = 0x00005555;
  FWDGT_PSC = FWDGT_PSC_DIV256;

  fmc_unlock();
  /* Erase sectors 7 (384 KB to 512 KB of main flash) */
  fmc_sector_erase(CTL_SECTOR_NUMBER_7);

  uint32_t *src, *dest;
  for (dest = &_configdata_loadaddr, src = &_sconfigdata; src < &_econfigdata;
       src++, dest++) {
    fmc_word_program((uint32_t)dest, *src);
  }

  fmc_lock();
  reset_handler();
}

extern void gd32f4xx_configure_scheduler(void);
extern void gd32f4xx_console_init(void);
extern void gd32f4xx_configure_adc(void);
extern void gd32f4xx_configure_pwm(void);

void platform_configure(bool is_benchmark) {

  NVIC_SetPriorityGrouping(3); /* 16 priority preemption levels */

  if (!is_benchmark) {
    gd32f4xx_configure_scheduler();
    gd32f4xx_configure_adc();
    gd32f4xx_configure_pwm();

    setup_gpios();
    setup_watchdog();
  }

  setup_dwt();
  gd32f4xx_console_init();
}
