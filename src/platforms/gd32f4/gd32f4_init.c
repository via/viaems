#include <stdbool.h>
#include <stdint.h>

#include "gd32f4xx_fwdgt.h"
#include "gd32f4xx_rcu.h"
#include "platform.h"
#include "tasks.h"

#include "gd32f4xx.h"

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

void disable_interrupts() {
  __disable_irq();
}

void enable_interrupts() {
  __enable_irq();
}

bool interrupts_enabled() {
  return __get_PRIMASK() == 0;
}

void set_output(int output, char value) {
  if (value) {
    GPIO_OCTL(GPIOD) |= (1 << output);
  } else {
    GPIO_OCTL(GPIOD) &= ~(1 << output);
  }
}

void set_gpio(int output, char value) {
  if (value) {
    GPIO_OCTL(GPIOE) |= (1 << output);
  } else {
    GPIO_OCTL(GPIOE) &= ~(1 << output);
  }
}

int __attribute__((externally_visible))
_write(int fd, const char *buf, size_t count) {
  (void)fd;
  size_t pos = 0;
  while (pos < count) {
    pos += console_write(buf + pos, count - pos);
  }
  return count;
}

static void reset_watchdog() {
  FWDGT_CTL = 0x0000AAAA;
}

static void setup_watchdog() {
  DBG_CTL1 |= DBG_CTL1_FWDGT_HOLD;

  FWDGT_CTL = 0x0000CCCC; /* Start watchdog */
  FWDGT_CTL = 0x00005555; /* Magic unlock sequence */
  FWDGT_RLD = 1250;       /* Approx 30 mS */
  reset_watchdog();
}

void systick_handler(void) {
  run_tasks();
  reset_watchdog();
}

static void setup_systick() {
  DBG_CTL1 |= DBG_CTL1_FWDGT_HOLD;

  /* Systick is driven by AHB / 8 = 25 MHz
   * Reload value 250000 for 25 MHz Systick and 10 ms period */
  SysTick->LOAD = 250000;
  SysTick->CTRL = SysTick_CTRL_ENABLE_Msk |
                  SysTick_CTRL_TICKINT_Msk; /* Enable interrupt and systick */

  NVIC_SetPriority(SysTick_IRQn,
                   NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 4, 0));
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

  rcu_periph_clock_disable(RCU_GPIOA);
  rcu_periph_clock_disable(RCU_GPIOB);
  rcu_periph_clock_disable(RCU_GPIOC);
  rcu_periph_clock_disable(RCU_GPIOD);
  rcu_periph_clock_disable(RCU_GPIOE);

  rcu_periph_clock_disable(RCU_SPI0);
  rcu_periph_clock_disable(RCU_SPI1);
  rcu_periph_clock_disable(RCU_SPI2);

  rcu_periph_clock_disable(RCU_DMA0);
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

  rcu_periph_clock_enable(RCU_GPIOA);
  rcu_periph_clock_enable(RCU_GPIOB);
  rcu_periph_clock_enable(RCU_GPIOC);
  rcu_periph_clock_enable(RCU_GPIOD);
  rcu_periph_clock_enable(RCU_GPIOE);

  rcu_periph_clock_enable(RCU_SPI0);
  rcu_periph_clock_enable(RCU_SPI1);
  rcu_periph_clock_enable(RCU_SPI2);

  rcu_periph_clock_enable(RCU_DMA0);
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

/* Common symbols exported by the linker script(s): */
extern uint32_t _data_loadaddr, _sdata, _edata, _ebss;

void reset_handler(void) {
  volatile uint32_t *src, *dest;
  for (src = &_data_loadaddr, dest = &_sdata; dest < &_edata; src++, dest++) {
    *dest = *src;
  }

  while (dest < &_ebss) {
    *dest++ = 0;
  }

  system_init();

  int main();
  (void)main();
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
  disable_interrupts();
  handle_emergency_shutdown();
  platform_disable_periphs();

  /* Ensure watchdog won't stop us from flashing */
  FWDGT_CTL = 0x00005555;
  FWDGT_PSC = FWDGT_PSC_DIV256;

  fmc_unlock();
  /* Erase sectors 8 (512 KB into main flash) */
  fmc_sector_erase(CTL_SECTOR_NUMBER_8);

  uint32_t *src, *dest;
  for (dest = &_configdata_loadaddr, src = &_sconfigdata; src < &_econfigdata;
       src++, dest++) {
    fmc_word_program((uint32_t)dest, *src);
  }

  fmc_lock();
  reset_handler();
}

void platform_reset_into_bootloader() {
  /* TODO: unimplemented */
}

extern void gd32f470_configure_scheduler(void);
extern void gd32f470_console_init(void);
extern void gd32f470_configure_adc(void);
extern void gd32f470_configure_pwm(void);

extern void gd32f4xx_configure_spi_flash(void);
extern void gd32f4xx_spi_transaction(const uint8_t *tx,
                                     uint8_t *rx,
                                     size_t len);

void platform_init() {
  NVIC_SetPriorityGrouping(3); /* 16 priority preemption levels */

  gd32f470_configure_scheduler();
  gd32f470_console_init();
  gd32f470_configure_adc();
  gd32f470_configure_pwm();

  gd32f4xx_configure_spi_flash();
  gd32f4xx_configure_sdcard();

  setup_gpios();
  setup_systick();
  setup_watchdog();
  setup_dwt();
}

void platform_benchmark_init() {}
