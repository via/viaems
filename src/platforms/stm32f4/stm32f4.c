#include "stm32f427xx.h"
#include <stdint.h>

#include "platform.h"
#include "controllers.h"

#include <FreeRTOS.h>
#include "task.h"

#ifndef CRYSTAL_FREQ
#define CRYSTAL_FREQ 8
#endif

void platform_enable_event_logging() {}

void platform_disable_event_logging() {}

#define BOOTLOADER_FLAG 0x56780123
static volatile uint32_t bootloader_flag = 0;
void platform_reset_into_bootloader() {
  bootloader_flag = BOOTLOADER_FLAG;
  NVIC_SystemReset();
}

void disable_interrupts() {
  __disable_irq();
}

void enable_interrupts() {
  __enable_irq();
}

uint64_t cycles_to_ns(uint64_t cycles) {
  return cycles * 1000 / 168;
}

uint64_t cycle_count() {
  return DWT->CYCCNT;
}

bool interrupts_enabled() {
  return __get_PRIMASK() == 0;
}

void set_output(int output, char value) {
  if (value) {
    GPIOD->ODR |= (1 << output);
  } else {
    GPIOD->ODR &= ~(1 << output);
  }
}

void set_gpio(int output, char value) {
  if (value) {
    GPIOE->ODR |= (1 << output);
  } else {
    GPIOE->ODR &= ~(1 << output);
  }
}

int get_gpio(int output) {
  return (GPIOE->IDR & (1 << output)) > 0;
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
  while ((PWR->CR & PWR_CSR_ODRDY) == 0)
    ; /* Wait for overdrive ready */

  PWR->CR |= PWR_CR_ODSWEN;
  while ((PWR->CR & PWR_CSR_ODSWRDY) == 0)
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
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  DWT->CYCCNT = 0;
}

static void reset_watchdog() {
  IWDG->KR = 0x0000AAAA;
}

static void setup_watchdog() {
  IWDG->KR = 0x0000CCCC; /* Start watchdog */
  IWDG->KR = 0x00005555; /* Magic unlock sequence */
  IWDG->RLR = 245;       /* Approx 30 mS */

  DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_IWDG_STOP;
  reset_watchdog();
}

#if 0
void SysTick_Handler(void) {
  run_tasks();
  reset_watchdog();
}

static void setup_systick() {
  NVIC_SetPriority(SysTick_IRQn, 15);
  SysTick->LOAD = 1680000; /* 100 Hz at 168 MHz HCLK */
  SysTick->CTRL = SysTick_CTRL_TICKINT_Msk |
                  SysTick_CTRL_CLKSOURCE_Msk | /* Use HCLK */
                  SysTick_CTRL_ENABLE_Msk;
}
#endif

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

  uint32_t *src;
  uint32_t *dest;
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

void platform_load_config() {
  volatile unsigned *src, *dest;
  for (src = &_configdata_loadaddr, dest = &_sconfigdata; dest < &_econfigdata;
       src++, dest++) {
    *dest = *src;
  }
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
  int main();
  (void)main();
}

/* Use usb to send text from newlib printf */
int __attribute__((externally_visible))
_write(int fd, const char *buf, size_t count) {
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
extern void stm32f4_configure_adc(void);
extern void stm32f4_configure_pwm(void);

void platform_init() {
  enable_peripherals();
  setup_clocks();

  setup_dwt();

  NVIC_SetPriorityGrouping(3); /* 16 priority preemption levels */

  //setup_systick();
//  setup_watchdog();
  setup_gpios();

//  stm32f4_configure_scheduler();
//  stm32f4_configure_usb();
//  stm32f4_configure_adc();
//  stm32f4_configure_pwm();
}

void platform_benchmark_init() {
  enable_peripherals();
  setup_clocks();
  setup_dwt();
  NVIC_SetPriorityGrouping(3); /* 16 priority preemption levels */
  stm32f4_configure_usb();
}

void itm_debug(const char *s) {
  for (; *s != '\0'; s++) {
    ITM_SendChar(*s);
  }
}

// FreeRTOS
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize ) {
  static StaticTask_t xIdleTaskTCB;
  static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

  *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
  *ppxIdleTaskStackBuffer = uxIdleTaskStack;
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

void vApplicationStackOverflowHook( 
  TaskHandle_t xTask __attribute__((unused)),     
  char *pcTaskName __attribute__((unused))) { 
  for (;;); 
}
