#include <stdint.h>
#include "stm32h743xx.h"

#include "platform.h"

void platform_enable_event_logging() {}

void platform_disable_event_logging() {}

void platform_reset_into_bootloader() {}

void set_pwm(int pin, float val) {
  (void)pin;
  (void)val;
}

void disable_interrupts() {
  __asm__("cpsid i");
}

void enable_interrupts() {
  __asm__("cpsie i");
}

uint64_t cycles_to_ns(uint64_t cycles) {
  return cycles * 1000 / 400;
}

uint64_t cycle_count() {
  return *((uint32_t *)0xE0001004);
}

int interrupts_enabled() {
  return 0;
}

void set_output(int output, char value) {
}

int get_output(int output) {
}

void set_gpio(int output, char value) {
}

void adc_gather(void *_adc) {
  (void)_adc;
}

void set_test_trigger_rpm(uint32_t rpm) {
  (void)rpm;
}

uint32_t get_test_trigger_rpm() {
  return 0;
}

void platform_save_config() {}

void platform_load_config() {}

size_t console_read(void *ptr, size_t max) {
  (void)ptr;
  (void)max;
  return 0;
}

size_t console_write(const void *ptr, size_t max) {
  (void)ptr;
  (void)max;
#if 0
  const char *c = ptr;
  for (int i = 0; i < max; i++) {
    while (((USART1->ISR) & USART_ISR_TXFE) != USART_ISR_TXFE);
    USART1->TDR = c[i];
  }
#endif
  return max;
}

int __attribute__((externally_visible))
_write(int fd, const char *buf, size_t count) {
  (void)fd;
  const char *c = buf;
  for (int i = 0; i < count; i++) {
    while (((USART1->ISR) & USART_ISR_TXFE) != USART_ISR_TXFE);
    USART1->TDR = c[i];
  }
}

/* Setup power, HSE, clock tree, and flash wait states */
static void setup_clocks() {
  /* Go to VOS1 */
  PWR->CR3 |= PWR_CR3_SCUEN | PWR_CR3_LDOEN;
  PWR->D3CR |= PWR_D3CR_VOS;
  while (((PWR->D3CR) & PWR_D3CR_VOSRDY) != PWR_D3CR_VOSRDY);

  /* Set Flash WS=2 and WRHIGHFREQ=2 due to 400 MHz (200 MHz AXI) */
  FLASH->ACR = FLASH_ACR_LATENCY_2WS | FLASH_ACR_WRHIGHFREQ_1;
  /* Validate WS setting */
  while ((FLASH->ACR & FLASH_ACR_LATENCY_Msk) != FLASH_ACR_LATENCY_2WS);

  /* Enable HSE and wait for it */
  RCC->CR &= ~RCC_CR_HSEBYP;
  RCC->CR |= RCC_CR_HSEON;
  while (((RCC->CR) & RCC_CR_HSERDY) != RCC_CR_HSERDY);

  /* Set HSE as source, DIVM to 1 for PLL 1/2/3 */
  RCC->PLLCKSELR = _VAL2FLD(RCC_PLLCKSELR_DIVM1, 1) |  /* DIVM = 1 */
                   _VAL2FLD(RCC_PLLCKSELR_DIVM2, 1) |  /* DIVM = 1 */ 
                   _VAL2FLD(RCC_PLLCKSELR_DIVM3, 1) |  /* DIVM = 1 */ 
                   _VAL2FLD(RCC_PLLCKSELR_PLLSRC, 2); /* HSE Source */

  RCC->PLLCFGR = RCC_PLLCFGR_DIVP1EN |  /* Enable PLL1 P */
                 RCC_PLLCFGR_DIVP3EN |  /* Enable PLL3 P */
                 RCC_PLLCFGR_DIVQ3EN |  /* Enable PLL3 Q */
                 _VAL2FLD(RCC_PLLCFGR_PLL1RGE, 2) | /* PLL1 Clock range 4-8 MHz */
                 _VAL2FLD(RCC_PLLCFGR_PLL2RGE, 2) | /* PLL2 Clock range 4-8 MHz */
                 _VAL2FLD(RCC_PLLCFGR_PLL3RGE, 2) | /* PLL3 Clock range 4-8 MHz */
                 RCC_PLLCFGR_PLL1VCOSEL | /* PLL1 Medium Range VCU */
                 RCC_PLLCFGR_PLL2VCOSEL | /* PLL2 Medium Range VCU */
                 RCC_PLLCFGR_PLL3VCOSEL; /* PLL3 Medium Range VCU */

  RCC->PLL1DIVR =  _VAL2FLD(RCC_PLL1DIVR_N1, 49) |  /* PLL1 N = 50, 400 MHz */
                   _VAL2FLD(RCC_PLL1DIVR_P1, 0); /* PLL1 P = 1 */
                  

  /* Enable PLL1 and wait for it to be ready */
  RCC->CR |= RCC_CR_PLL1ON;
  while (((RCC->CR) & RCC_CR_PLL1RDY) != RCC_CR_PLL1RDY);

  RCC->D1CFGR = RCC_D1CFGR_D1CPRE_DIV1 | /* CPRE = 1 */
                RCC_D1CFGR_D1PPRE_DIV2 | /* D1PPRE = /2 */
                RCC_D1CFGR_HPRE_DIV2;  /* HPRE = /2 */

  RCC->D2CFGR = RCC_D2CFGR_D2PPRE1_DIV2 | /* D2PPRE1 = /2 */
                RCC_D2CFGR_D2PPRE2_DIV2; /* D2PPRE1 = /2 */

  RCC->D3CFGR = RCC_D3CFGR_D3PPRE_DIV2; /* D3PPRE = /2 */

  /* Enable PLL1P for System clock */
  RCC->CFGR |= _VAL2FLD(RCC_CFGR_SW, 3);

  /* Configure PLL3 with a frequency of 48 MHz for SPI and USB */
  RCC->PLL3DIVR = _VAL2FLD(RCC_PLL3DIVR_N3, 5) | /* N = 6, provides 48 MHz internally */
                  _VAL2FLD(RCC_PLL3DIVR_P3, 4) | /* P = 5, provides 9.6 MHz to SPI1 */
                  _VAL2FLD(RCC_PLL3DIVR_Q3, 0); /* Q = 1, 48 MHz for USB1 */

  /* Enable PLL3 and wait for it to be ready */
  RCC->CR |= RCC_CR_PLL3ON;
  while (((RCC->CR) & RCC_CR_PLL3RDY) != RCC_CR_PLL3RDY);

  RCC->D2CCIP1R = _VAL2FLD(RCC_D2CCIP1R_SPI123SEL, 2); /* SPI1 uses PLL3P */
  RCC->D2CCIP2R = _VAL2FLD(RCC_D2CCIP2R_USBSEL, 2); /* USB uses PLL3Q */

  /* Clock enables for all peripherals used */
  RCC->APB4ENR |= RCC_APB4ENR_SYSCFGEN;
  RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN;
  RCC->AHB4ENR |= RCC_AHB4ENR_GPIOBEN;
  RCC->AHB4ENR |= RCC_AHB4ENR_GPIODEN;
  RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
  RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
  RCC->APB2ENR |= RCC_APB2ENR_TIM15EN;
  RCC->APB2ENR |= RCC_APB2ENR_TIM8EN;
  RCC->APB1LENR |= RCC_APB1LENR_TIM2EN;
  RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
}

static void configure_usart1() {
  /* 8 bits, no parity, fifo enabled */
  USART1->CR1 = USART_CR1_FIFOEN;
  USART1->CR2 &= ~USART_CR2_STOP; /* STOP=0, 1 stop bit */
  USART1->BRR = 868; /* 115200 Baud at 100 MHz APB Clock */

  USART1->CR1 |= USART_CR1_TE;
  USART1->CR1 |= USART_CR1_UE;

  GPIOB->MODER &= ~(GPIO_MODER_MODE6 | GPIO_MODER_MODE7);
  GPIOB->MODER |= GPIO_MODER_MODE6_1 | GPIO_MODER_MODE7_1; /* B6/B7 in AF mode */
  GPIOB->AFR[0] |= _VAL2FLD(GPIO_AFRL_AFSEL6, 7) | 
                   _VAL2FLD(GPIO_AFRL_AFSEL7, 7);
}


static void setup_caches() {
  *((uint32_t *)0xE000EF50) = 0; /* Invalidate I-cache */
  SCB->CCR |= SCB_CCR_IC_Msk; /* Enable I-Cache */
  __asm__("dsb");
  __asm__("isb");
}

static void setup_dwt() {
  *((uint32_t *)0xE0001000) |= 1; /* Enable DWT Cycle Counter */
  *((uint32_t *)0xE0001004) = 0; /* Reset cycle counter */
}

static void reset_watchdog() {
  IWDG1->KR = 0x0000AAAA;
}

static void setup_watchdog() {
  IWDG1->KR = 0x0000CCCC; /* Start watchdog */
  IWDG1->KR = 0x00005555; /* Magic unlock sequence */
  IWDG1->RLR = 240; /* Approx 30 mS */
  reset_watchdog();
}

void SysTick_Handler(void) {
  run_tasks();
  reset_watchdog();
}


static void setup_systick() {
  /* Reload value 500000 for 50 MHz Systick and 10 ms period */
  *((uint32_t *)0xE000E014) = 500000;
  *((uint32_t *)0xE000E010) = 0x3; /* Enable interrupt and systick */

  /* Systick (15) set to priority 16 */
  NVIC_SetPriority(SysTick_IRQn, 255);

}

/* Common symbols exported by the linker script(s): */
extern uint32_t _sidata, _sdata, _edata, _ebss;
extern uint32_t _configdata_loadaddr, _sconfigdata, _econfigdata;

void Default_Handler(void) {
}

void Reset_Handler(void)
{
  volatile uint32_t *src, *dest;

  for (src = &_sidata, dest = &_sdata;
      dest < &_edata;
      src++, dest++) {
    *dest = *src;
  }

  while (dest < &_ebss) {
    *dest++ = 0;
  }

  for (src = &_configdata_loadaddr, dest = &_sconfigdata;
      dest < &_econfigdata;
      src++, dest++) {
    *dest = *src;
  }

  /* Set HSION bit */
  RCC->CR |= RCC_CR_HSION;

  /* Reset CFGR register */
  RCC->CFGR = 0x00000000;

  /* Reset HSEON, HSECSSON, CSION, HSI48ON, CSIKERON, PLL1ON, PLL2ON and PLL3ON bits */
  RCC->CR &= 0xEAF6ED7FU;

  /* Reset D1CFGR register */
  RCC->D1CFGR = 0x00000000;

  /* Reset D2CFGR register */
  RCC->D2CFGR = 0x00000000;

  /* Reset D3CFGR register */
  RCC->D3CFGR = 0x00000000;
  /* Reset PLLCKSELR register */
  RCC->PLLCKSELR = 0x02020200;

  /* Reset PLLCFGR register */
  RCC->PLLCFGR = 0x01FF0000;
  /* Reset PLL1DIVR register */
  RCC->PLL1DIVR = 0x01010280;
  /* Reset PLL1FRACR register */
  RCC->PLL1FRACR = 0x00000000;

  /* Reset PLL2DIVR register */
  RCC->PLL2DIVR = 0x01010280;

  /* Reset PLL2FRACR register */

  RCC->PLL2FRACR = 0x00000000;
  /* Reset PLL3DIVR register */
  RCC->PLL3DIVR = 0x01010280;

  /* Reset PLL3FRACR register */
  RCC->PLL3FRACR = 0x00000000;

  /* Reset HSEBYP bit */
  RCC->CR &= 0xFFFBFFFFU;

  /* Disable all interrupts */
  RCC->CIER = 0x00000000;

  SCB->CPACR |= ((3UL << (10*2))|(3UL << (11*2)));  /* set CP10 and CP11 Full Access */
  SCB->CCR |= SCB_CCR_STKALIGN_Msk;

  /* Call the application's entry point. */
  int main();
  (void)main();

}

void platform_init() {
  setup_clocks();

  setup_caches();
  setup_dwt();
  configure_usart1();
//  platform_init_scheduler();
  setup_systick();
//  setup_watchdog();
  platform_configure_sensors();
  platform_configure_usb();
}

void platform_benchmark_init() {
  setup_clocks();

  setup_caches();
  configure_usart1();
}

