#include "sensors.h"
#include "stm32f427xx.h"
#undef ETH // STM32 headers define this for ethernet
#include "config.h"
#include "platform.h"
#include "tasks.h"
#include "viaems.h"
#include <stdint.h>

#ifndef CRYSTAL_FREQ
#define CRYSTAL_FREQ 8
#endif

struct viaems stm32f4_viaems;

uint64_t cycles_to_ns(uint64_t cycles) {
  return cycles * 1000 / 168;
}

uint64_t cycle_count() {
  return DWT->CYCCNT;
}

void set_gpio_port(uint32_t output) {
  GPIOE->ODR = output;
}

static void enable_peripherals(void) {
  RCC->APB1ENR |= RCC_APB1ENR_PWREN | RCC_APB1ENR_TIM2EN | RCC_APB1ENR_TIM3EN;

  RCC->APB2ENR |= RCC_APB2ENR_TIM1EN | RCC_APB2ENR_TIM8EN | RCC_APB2ENR_TIM9EN |
                  RCC_APB2ENR_SPI1EN;

  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN |
                  RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN |
                  RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMA2EN;

  RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;
}

/* Setup power, HSE, clock tree, and flash wait states */
static void setup_clocks() {
  RCC->CR |= RCC_CR_HSEON;
  while ((RCC->CR & RCC_CR_HSERDY) == 0)
    ; /* Wait for HSE to be stable */

  /* Configure PLL with given crystal to produce a 168 MHz P and 48 MHz Q (For
   * USB) */
  RCC->PLLCFGR = RCC_PLLCFGR_PLLSRC_HSE |
                 _VAL2FLD(RCC_PLLCFGR_PLLM, CRYSTAL_FREQ) | /* PLL M = 8 */
                 _VAL2FLD(RCC_PLLCFGR_PLLN, 336) |          /* PLL N = 336 */
                 _VAL2FLD(RCC_PLLCFGR_PLLQ, 7) |            /* PLL Q = 7 */
                 _VAL2FLD(RCC_PLLCFGR_PLLP, 0);             /* PLL P = 2 */

  RCC->CR |= RCC_CR_PLLON;
  while ((RCC->CR & RCC_CR_PLLRDY) == 0)
    ; /* Wait for PLL to be stable */

  PWR->CR |= PWR_CR_ODEN;
  while ((PWR->CSR & PWR_CSR_ODRDY) == 0)
    ; /* Wait for overdrive ready */

  PWR->CR |= PWR_CR_ODSWEN;
  while ((PWR->CSR & PWR_CSR_ODSWRDY) == 0)
    ; /* Wait for overdrive switch */

  RCC->CFGR = RCC_CFGR_PPRE2_DIV2 |       /* APB2 = HCLK / 2 = 84 MHz */
              RCC_CFGR_PPRE1_DIV4 |       /* APB1 = HCLK / 4 = 42 MHz */
              _VAL2FLD(RCC_CFGR_HPRE, 0); /* AHB = HCLK = 168 MHz */

  FLASH->ACR =
    FLASH_ACR_LATENCY_5WS | FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN;

  while ((RCC->CR & RCC_CR_PLLRDY) == 0)
    ; /* Wait for PLL lock */

  RCC->CFGR |= RCC_CFGR_SW_PLL; /* Enable PLL as system clock source */
}

static void setup_dwt() {
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  DWT->CYCCNT = 0;
}

void reset_watchdog(void) {
  IWDG->KR = 0x0000AAAA;
}

static void setup_watchdog() {
  IWDG->KR = 0x0000CCCC; /* Start watchdog */
  IWDG->KR = 0x00005555; /* Magic unlock sequence */
  IWDG->RLR = 245;       /* Approx 30 mS */

  DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_IWDG_STOP;
  reset_watchdog();
}

static void ensure_db1m(void) {

  bool needs_update = (FLASH->OPTCR & FLASH_OPTCR_DB1M) == 0;
  bool onemeg_chip  = *((uint16_t *)FLASHSIZE_BASE) == 0x400;

  if (!needs_update || !onemeg_chip) {
    return;
  }

  /* Unlock FLASH_OPTCR */
  FLASH->OPTKEYR = 0x08192A3B;
  FLASH->OPTKEYR = 0x4C5D6E7F;

  FLASH->OPTCR |= FLASH_OPTCR_DB1M;

  FLASH->OPTCR |= FLASH_OPTCR_OPTSTRT;
  while (FLASH->SR & FLASH_SR_BSY)
    ;

  FLASH->OPTCR |= FLASH_OPTCR_OPTLOCK;

}

extern unsigned _configdata_loadaddr, _sconfigdata, _econfigdata;
void platform_save_config() {

  /* Unlock FLASH_CR */
  FLASH->KEYR = 0x45670123;
  FLASH->KEYR = 0xCDEF89AB;

  FLASH->CR &= ~FLASH_CR_PSIZE;
  FLASH->CR |= _VAL2FLD(FLASH_CR_PSIZE, 2); /* 32 bit write parallelism */
  for (int s = 12; s <= 15; s++) {
    uint16_t s_to_erase =
      (0x10 - 12) + s; /* Bank 2 sectors start at 0x10 vs 12*/
    FLASH->CR &= ~(FLASH_CR_SNB);
    FLASH->CR |=
      FLASH_CR_SER | _VAL2FLD(FLASH_CR_SNB, s_to_erase); /* Erase sector 8 */
    FLASH->CR |= FLASH_CR_STRT;
    while (FLASH->SR & FLASH_SR_BSY)
      ;
  }

  FLASH->ACR &= ~FLASH_ACR_DCEN; /* Errata 2.2.12: Disable flash data cache
                                    while programming */
  FLASH->CR |= FLASH_CR_PG;

  unsigned *src;
  unsigned *dest;
  for (src = &_sconfigdata, dest = &_configdata_loadaddr; src < &_econfigdata;
       src++, dest++) {
    *dest = *src;
    /* Wait for BSY to clear */
    while (FLASH->SR & FLASH_SR_BSY)
      ;
  }

  FLASH->CR |= FLASH_CR_LOCK; /* Re-lock */

  FLASH->ACR |= FLASH_ACR_DCRST; /* Flush and re-enable data cache */
  FLASH->ACR |= FLASH_ACR_DCEN;
}

extern struct config default_config;

static struct config *platform_load_config(void) {
  volatile unsigned *src, *dest;
  for (src = &_configdata_loadaddr, dest = &_sconfigdata; dest < &_econfigdata;
       src++, dest++) {
    *dest = *src;
  }
  return &default_config;
}

/* Use usb to send text from newlib printf */
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

static void setup_gpios(void) {
  GPIOE->MODER = 0x55555555;   /* All of GPIOE is output */
  GPIOE->OSPEEDR = 0xffffffff; /* All GPIOE set to High speed*/
}

extern void stm32f4_configure_scheduler(void);
extern void stm32f4_configure_usb(void);
extern void stm32f4_configure_sensors(void);
extern void stm32f4_configure_pwm(void);

static void platform_configure(bool is_benchmark) {

  enable_peripherals();
  setup_clocks();

  setup_dwt();

  NVIC_SetPriorityGrouping(3); /* 16 priority preemption levels */

  if (!is_benchmark) {
    setup_watchdog();
    setup_gpios();

    stm32f4_configure_scheduler();
    stm32f4_configure_sensors();
    stm32f4_configure_pwm();
  }

  stm32f4_configure_usb();
}

#define BOOTLOADER_FLAG 0x56780123
static volatile uint32_t bootloader_flag = 0;
void platform_reset_into_bootloader() {
  bootloader_flag = BOOTLOADER_FLAG;
  NVIC_SystemReset();
}

/* Common symbols exported by the linker script(s): */
extern uint32_t _data_loadaddr, _sdata, _edata, _ebss;

void Reset_Handler(void) {
  bool jump_to_bootloader = false;
  if (bootloader_flag == BOOTLOADER_FLAG) {
    bootloader_flag = 0;
    jump_to_bootloader = true;
  }

  volatile uint32_t *src, *dest;
  for (src = &_data_loadaddr, dest = &_sdata; dest < &_edata; src++, dest++) {
    *dest = *src;
  }

  while (dest < &_ebss) {
    *dest++ = 0;
  }

  SCB->CPACR |=
    ((3UL << (10 * 2)) | (3UL << (11 * 2))); /* set CP10 and CP11 Full Access */
  SCB->CCR |= SCB_CCR_STKALIGN_Msk;

  ensure_db1m();

  if (jump_to_bootloader) {
    /* We've set this flag and reset the cpu, jump to system bootloader */
    RCC->AHB1ENR |= RCC_APB2ENR_SYSCFGEN;

    volatile uint32_t *bootloader_msp = (uint32_t *)0x1fff0000; /* Per AN2606 */
    volatile uint32_t *bootloader_addr = bootloader_msp + 1;

    __set_MSP(*bootloader_msp);
    void (*bootloader)() = (void (*)(void))(*bootloader_addr);

    SYSCFG->MEMRMP =
      _VAL2FLD(SYSCFG_MEMRMP_MEM_MODE, 0x1); /* Remap System Mem */
    bootloader();
  }

  /* Call the application's entry point. */
  struct config *config = platform_load_config();
  bool benchmark_enabled = false;
#ifdef BENCHMARK
  benchmark_enabled = true;
#endif

  if (benchmark_enabled) {
    platform_configure(true);
    int start_benchmarks(void);
    start_benchmarks();
  } else {
    viaems_init(&stm32f4_viaems, config);
    platform_configure(false);
    viaems_idle(&stm32f4_viaems);
  }
}
