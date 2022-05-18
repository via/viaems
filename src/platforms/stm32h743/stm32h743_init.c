#include "device.h"

#include "platform.h"

void platform_enable_event_logging() {}

void platform_disable_event_logging() {}

void platform_reset_into_bootloader() {}

void set_pwm(int pin, float val) {
  (void)pin;
  (void)val;
}

void disable_interrupts() {
}

void enable_interrupts() {
}

timeval_t current_time() {
  return 0;
}

uint64_t cycles_to_ns(uint64_t cycles) {
  return cycles * 1000 / 400;
}

uint64_t cycle_count() {
  return *((uint32_t *)0xE0001004);
}

timeval_t platform_output_earliest_schedulable_time() {
}


void set_event_timer(timeval_t t) {
}

timeval_t get_event_timer() {
  return 0;
}

void clear_event_timer() {
}

void disable_event_timer() {
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
  const char *c = ptr;
  for (int i = 0; i < max; i++) {
    while (((USART1->ISR) & USART1_ISR_TXE) != USART1_ISR_TXE);
    USART1->TDR = c[i];
  }
  return max;
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

/* Common symbols exported by the linker script(s): */
typedef void (*funcp_t) (void);
extern uint32_t _data_loadaddr, _data, _edata, _ebss;
extern funcp_t __preinit_array_start, __preinit_array_end;
extern funcp_t __init_array_start, __init_array_end;
extern funcp_t __fini_array_start, __fini_array_end;


void __attribute__ ((weak)) reset_handler(void)
{
	volatile uint32_t *src, *dest;
	funcp_t *fp;

	for (src = &_data_loadaddr, dest = &_data;
		dest < &_edata;
		src++, dest++) {
		*dest = *src;
	}

	while (dest < &_ebss) {
		*dest++ = 0;
	}

  FPU_CPACR->CPACR |= ((3UL << (10*2))|(3UL << (11*2)));  /* set CP10 and CP11 Full Access */
  SCB->CCR |= SCB_CCR_STKALIGN;

	/* Constructors. */
	for (fp = &__preinit_array_start; fp < &__preinit_array_end; fp++) {
		(*fp)();
	}
	for (fp = &__init_array_start; fp < &__init_array_end; fp++) {
		(*fp)();
	}

	/* Call the application's entry point. */
	(void)main();

	/* Destructors. */
	for (fp = &__fini_array_start; fp < &__fini_array_end; fp++) {
		(*fp)();
	}

}

static lowlevel_init() {


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
}

/* Setup power, HSE, clock tree, and flash wait states */
static void setup_clocks() {
  /* Go to VOS1 */
  PWR->CR3 |= PWR_CR3_SCUEN | PWR_CR3_LDOEN;
  PWR->D3CR |= PWR_D3CR_VOS;
  while (((PWR->D3CR) & PWR_D3CR_VOSRDY) != PWR_D3CR_VOSRDY);

  /* Set Flash WS=2 and WRHIGHFREQ=2 due to 400 MHz (200 MHz AXI) */
  Flash->ACR = Flash_ACR_LATENCY_VAL(2) | Flash_ACR_WRHIGHFREQ_VAL(2);
  /* Validate WS setting */
  while (((Flash->ACR) & Flash_ACR_LATENCY) != Flash_ACR_LATENCY_VAL(2));

  /* Enable HSE and wait for it */
  RCC->CR &= ~RCC_CR_HSEBYP;
  RCC->CR |= RCC_CR_HSEON;
  while (((RCC->CR) & RCC_CR_HSERDY) != RCC_CR_HSERDY);

  /* Set HSE as source, DIVM to 1 for PLL 1/2/3 */
  RCC->PLLCKSELR = RCC_PLLCKSELR_DIVM1_VAL(1) |  /* DIVM = 1 */
                   RCC_PLLCKSELR_DIVM2_VAL(1) |  /* DIVM = 1 */ 
                   RCC_PLLCKSELR_DIVM3_VAL(1) |  /* DIVM = 1 */ 
                   RCC_PLLCKSELR_PLLSRC_VAL(2); /* HSE Source */

  RCC->PLLCFGR = RCC_PLLCFGR_DIVP1EN |  /* Enable PLL1 P */
                 RCC_PLLCFGR_DIVQ1EN |  /* Enable PLL1 Q */
                 RCC_PLLCFGR_PLL1RGE_VAL(2) | /* PLL1 Clock range 4-8 MHz */
                 RCC_PLLCFGR_PLL2RGE_VAL(2) | /* PLL2 Clock range 4-8 MHz */
                 RCC_PLLCFGR_PLL3RGE_VAL(2) | /* PLL3 Clock range 4-8 MHz */
                 RCC_PLLCFGR_PLL1VCOSEL | /* PLL1 Medium Range VCU */
                 RCC_PLLCFGR_PLL2VCOSEL | /* PLL2 Medium Range VCU */
                 RCC_PLLCFGR_PLL3VCOSEL; /* PLL3 Medium Range VCU */

  RCC->PLL1DIVR = RCC_PLL1DIVR_DIVQ1_VAL(4) | /* PLL1 Q = 5 */
                  RCC_PLL1DIVR_DIVP1_VAL(0) | /* PLL1 P = 1 */
                  RCC_PLL1DIVR_DIVN1_VAL(49); /* PLL1 N = 50 */

  /* Enable PLL1 and wait for it to be ready */
  RCC->CR |= RCC_CR_PLL1ON;
  while (((RCC->CR) & RCC_CR_PLL1RDY) != RCC_CR_PLL1RDY);

  RCC->D1CFGR = RCC_D1CFGR_D1CPRE_VAL(0) | /* CPRE = 1 */
                RCC_D1CFGR_D1PPRE_VAL(4) | /* D1PPRE = /2 */
                RCC_D1CFGR_HPRE_VAL(8);  /* HPRE = /2 */

  RCC->D2CFGR = RCC_D2CFGR_D2PPRE1_VAL(4) | /* D2PPRE1 = /2 */
                RCC_D2CFGR_D2PPRE2_VAL(4); /* D2PPRE1 = /2 */

  RCC->D3CFGR = RCC_D3CFGR_D3PPRE_VAL(4); /* D3PPRE = /2 */

  /* Enable PLL1P for System clock */
  RCC->CFGR |= RCC_CFGR_SW_VAL(3);

}

static void configure_usart1() {
  /* 8 bits, no parity, fifo enabled */
  USART1->CR1 = USART1_CR1_FIFOEN | USART1_CR1_M1_VAL(0) | USART1_CR1_M0_VAL(0);
  USART1->CR2 &= ~USART1_CR2_STOP; /* STOP=0, 1 stop bit */
  USART1->BRR = 868; /* 115200 Baud at 100 MHz APB Clock */

  USART1->CR1 |= USART1_CR1_TE;
  USART1->CR1 |= USART1_CR1_UE;

  GPIOA->MODER &= ~(GPIOA_MODER_MODE9 | GPIOA_MODER_MODE10);
  GPIOA->MODER |= GPIOA_MODER_MODE9_VAL(2) | GPIOA_MODER_MODE10_VAL(2); /* A9/A10 in AF mode */
  GPIOA->AFRH |= GPIOA_AFRH_AFSEL9_VAL(7) | GPIOA_AFRH_AFSEL10_VAL(7); /* AF7 */
}


static void setup_caches() {
  *((uint32_t *)0xE000EF50) = 0; /* Invalidate I-cache */
  SCB->CCR |= SCB_CCR_IC; /* Enable I-Cache */

  *((uint32_t *)0xE0001000) |= 1; /* Enable DWT Cycle Counter */
  *((uint32_t *)0xE0001004) = 0; /* Reset cycle counter */

}
void platform_init() {
  lowlevel_init();

  setup_clocks();

  RCC->APB4ENR |= RCC_APB4ENR_SYSCFGEN;
  RCC->AHB4ENR |= RCC_AHB4ENR_GPIOCEN; /*Enable GPIOC Clock */
  RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN; /*Enable GPIOA Clock */
  RCC->APB2ENR |= RCC_APB2ENR_USART1EN; /*Enable USART1 Clock */

  GPIOC->MODER = GPIOC_MODER_MODE0_VAL(1) | GPIOC_MODER_MODE1_VAL(1) | GPIOC_MODER_MODE2_VAL(1);
  GPIOC->PUPDR = GPIOC_PUPDR_PUPD0_VAL(2) | GPIOC_PUPDR_PUPD1_VAL(2) | GPIOC_PUPDR_PUPD2_VAL(2);
  GPIOC->OSPEEDR = GPIOC_OSPEEDR_OSPEED0_VAL(3) | GPIOC_OSPEEDR_OSPEED1_VAL(3) | GPIOC_OSPEEDR_OSPEED2_VAL(3);

  GPIOC->ODR = 0x2;

  setup_caches();
  configure_usart1();
  setup_tim8();
}

void platform_benchmark_init() {
  lowlevel_init();

  setup_clocks();

  RCC->APB4ENR |= RCC_APB4ENR_SYSCFGEN;
  RCC->AHB4ENR |= RCC_AHB4ENR_GPIOCEN; /*Enable GPIOC Clock */
  RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN; /*Enable GPIOA Clock */
  RCC->APB2ENR |= RCC_APB2ENR_USART1EN; /*Enable USART1 Clock */

  setup_caches();
  configure_usart1();
}

