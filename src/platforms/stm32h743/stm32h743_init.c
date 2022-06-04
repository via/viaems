#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/gpio.h>

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

uint64_t cycle_count() {
  return 0;
}

uint64_t cycles_to_ns(uint64_t s) { return 0; }

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
  const char *c = ptr;
  for (int i = 0; i < max; i++) {
    usart_send(USART1, c[i]);
  }
}
void platform_benchmark_init() {}

static void setup_clocks() {
  struct rcc_pll_config pll_config = {
    .sysclock_source = RCC_PLL,
    .pll_source = RCC_PLLCKSELR_PLLSRC_HSE,
    .hse_frequency = 8000000,
    .pll1 = {
      .divm = 1,
      .divn = 50,
      .divp = 1,
      .divq = 5,
    },
    .core_pre = RCC_D1CFGR_D1CPRE_BYP,
    .hpre = RCC_D1CFGR_D1HPRE_DIV2,
    .ppre1 = RCC_D2CFGR_D2PPRE_DIV2, 
    .ppre2 = RCC_D2CFGR_D2PPRE_DIV2,
    .ppre3 = RCC_D1CFGR_D1PPRE_DIV2,
    .ppre4 = RCC_D3CFGR_D3PPRE_DIV2,
    .flash_waitstates = 2,
    .voltage_scale = PWR_VOS_SCALE_1,
    .power_mode = PWR_SYS_SCU_LDO,
  };
  rcc_clock_setup_pll(&pll_config);
  /* TODO: USB 33 */
}

static void configure_usart1() {
  usart_set_baudrate(USART1, 115200);
  usart_set_databits(USART1, 8);
  usart_set_stopbits(USART1, 1);
  usart_set_mode(USART1, USART_MODE_TX_RX);
  usart_enable(USART1);

  gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6 | GPIO7);
  gpio_set_af(GPIOB, GPIO_AF7, GPIO6 | GPIO7);

#if 0
  /* 8 bits, no parity, fifo enabled */
  USART1->CR1 = USART1_CR1_FIFOEN | USART1_CR1_M1_VAL(0) | USART1_CR1_M0_VAL(0);
  USART1->CR2 &= ~USART1_CR2_STOP; /* STOP=0, 1 stop bit */
  USART1->BRR = 868; /* 115200 Baud at 100 MHz APB Clock */

  USART1->CR1 |= USART1_CR1_TE;
  USART1->CR1 |= USART1_CR1_UE;

  GPIOA->MODER &= ~(GPIOA_MODER_MODE9 | GPIOA_MODER_MODE10);
  GPIOA->MODER |= GPIOA_MODER_MODE9_VAL(2) | GPIOA_MODER_MODE10_VAL(2); /* A9/A10 in AF mode */
  GPIOA->AFRH |= GPIOA_AFRH_AFSEL9_VAL(7) | GPIOA_AFRH_AFSEL10_VAL(7); /* AF7 */
#endif
}


void platform_init() {
  /* TODO: Icache */

  setup_clocks();

  rcc_periph_clock_enable(RCC_SYSCFG);
  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_GPIOC);
  rcc_periph_clock_enable(RCC_USART1);

  configure_usart1();
}
