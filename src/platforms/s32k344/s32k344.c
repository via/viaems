#include "S32K344.h"
#include <stdint.h>
#include <stdbool.h>

#include "platform.h"
#include "util.h"

#include <FreeRTOS.h>
#include "task.h"

timeval_t current_time(void) {
  return IP_STM_0->CNT;
}

uint64_t cycle_count(void) {
  return DWT->CYCCNT;
}

uint64_t cycles_to_ns(uint64_t cycles) {
  return 1000 * cycles / 160;
}

_Atomic bool event_timer_override = false;

void schedule_event_timer(timeval_t t) {
  disable_interrupts();
  event_timer_override = false;
  IP_STM_0->CHANNEL[0].CCR = STM_CCR_CEN(0);
  IP_STM_0->CHANNEL[0].CIR = STM_CIR_CIF(1);

  IP_STM_0->CHANNEL[0].CMP = t;
  IP_STM_0->CHANNEL[0].CCR = STM_CCR_CEN(1);

  if (time_before(t, current_time())) {
    event_timer_override = true;
    NVIC_SetPendingIRQ(STM0_IRQn);
  }
  enable_interrupts();
}

void setup_pll(void) {
  if (IP_PLL->PLLSR & PLL_PLLSR_LOCK_MASK) {
    return;
  }
  /* Reset PLL config */
  IP_PLL->PLLODIV[0] = 0;
  IP_PLL->PLLODIV[1] = 0;
  IP_PLL->PLLCR = 1;

  /* Configure integer multiplier of 16 MHz XTAL by 60 to get VCO of 960 MHz */
  IP_PLL->PLLDV = PLL_PLLDV_RDIV(0) | PLL_PLLDV_MFI(60);
  IP_PLL->PLLDV |= PLL_PLLDV_ODIV2(2); /* VCO / 2 -> 480 > MHz */
  IP_PLL->PLLODIV[0] = PLL_PLLODIV_DIV(2); /* Divide by 3 -> 160 MHz */
  IP_PLL->PLLODIV[1] = PLL_PLLODIV_DIV(2);

  /* Enable and Wait for stable */
  IP_PLL->PLLCR = 0;
  while (!(IP_PLL->PLLSR & PLL_PLLSR_LOCK_MASK));
  IP_PLL->PLLODIV[0] |= PLL_PLLODIV_DE_MASK;
  IP_PLL->PLLODIV[1] |= PLL_PLLODIV_DE_MASK;
}

void setup_clock_tree(void) {
  //mux 0 dc 0 (core clk)  => 0 (160 Mhz)
  //mux 0 dc 1 (aips plat clk)  => 1 (80 Mhz)
  //mux 0 dc 2 (aips slow clk)  => 3 (40 Mhz)
  //mux 0 dc 3 (hse clk)  => 1 (80 Mhz)
  //mux 0 dc 4 (dcm clk)  => 3 (40 Mhz)
  //mux 0 dc 5 (lbist clk)  => 3 (40 Mhz)
  //mux 0 dc 6 (qspi clk)  => 0 (160 Mhz)

  IP_MC_CGM->MUX_0_DC_0 = MC_CGM_MUX_0_DC_0_DE(1) | MC_CGM_MUX_0_DC_0_DIV(0);
  IP_MC_CGM->MUX_0_DC_1 = MC_CGM_MUX_0_DC_1_DE(1) | MC_CGM_MUX_0_DC_1_DIV(1);
  IP_MC_CGM->MUX_0_DC_2 = MC_CGM_MUX_0_DC_2_DE(1) | MC_CGM_MUX_0_DC_2_DIV(3);
  IP_MC_CGM->MUX_0_DC_3 = MC_CGM_MUX_0_DC_3_DE(1) | MC_CGM_MUX_0_DC_3_DIV(1);
  IP_MC_CGM->MUX_0_DC_4 = MC_CGM_MUX_0_DC_4_DE(1) | MC_CGM_MUX_0_DC_4_DIV(3);
  IP_MC_CGM->MUX_0_DC_5 = MC_CGM_MUX_0_DC_5_DE(1) | MC_CGM_MUX_0_DC_5_DIV(3);
  IP_MC_CGM->MUX_0_DC_6 = MC_CGM_MUX_0_DC_6_DE(1) | MC_CGM_MUX_0_DC_6_DIV(0);
  IP_MC_CGM->MUX_0_CSC = MC_CGM_MUX_0_CSC_SELCTL(8) | MC_CGM_MUX_0_CSC_CLK_SW(1);
  while ((IP_MC_CGM->MUX_0_CSS & MC_CGM_MUX_0_CSS_SELSTAT_MASK) != MC_CGM_MUX_0_CSS_SELSTAT(8));
}
void STM0_Handler(void) {
  if (!event_timer_override && (IP_STM_0->CHANNEL[0].CIR & STM_CIR_CIF(1)) == 0) {
    return;
  }
  event_timer_override = false;
  IP_STM_0->CHANNEL[0].CIR = STM_CIR_CIF(1);
  scheduler_callback_timer_execute();
}

void setup_power(void) {
  /* Enable Last Mile on PMC */
  IP_PMC->CONFIG |= PMC_CONFIG_LMEN(1);
}

void setup_fxosc(void) {
  if (IP_FXOSC->CTRL & 0x1) {
    return;
  }
  /* Set FXOSC and wait for stable */
  IP_FXOSC->CTRL = FXOSC_CTRL_OSC_BYP(0) | FXOSC_CTRL_COMP_EN(1);
  
  // I made these up! do math and then write docs
  IP_FXOSC->CTRL |= FXOSC_CTRL_GM_SEL(12);
  IP_FXOSC->CTRL |= FXOSC_CTRL_EOCV(157);
  IP_FXOSC->CTRL |= FXOSC_CTRL_OSCON(1);
  while (!(IP_FXOSC->STAT & FXOSC_STAT_OSC_STAT_MASK));
}

void setup_periph_clocks(void) {
  IP_MC_ME->PRTN0_COFB1_CLKEN |= MC_ME_PRTN0_COFB1_CLKEN_REQ34_MASK; /* eMIOS 0 CLKEN */

  IP_MC_ME->PRTN1_COFB0_CLKEN |= MC_ME_PRTN1_COFB0_CLKEN_REQ29_MASK; /* STM0 CLKEN */
  IP_MC_ME->PRTN1_COFB1_CLKEN |= MC_ME_PRTN1_COFB1_CLKEN_REQ56_MASK; /* PLL1 CLKEN */

  IP_MC_ME->PRTN0_PUPD = 1;
  IP_MC_ME->PRTN1_PUPD = 1;
  IP_MC_ME->CTL_KEY = 0x5af0;
  IP_MC_ME->CTL_KEY = 0xa50f;

  while (!(IP_MC_ME->PRTN0_COFB1_STAT & MC_ME_PRTN0_COFB1_STAT_BLOCK34_MASK));
  while (!(IP_MC_ME->PRTN1_COFB0_STAT & MC_ME_PRTN1_COFB0_STAT_BLOCK29_MASK));
  while (!(IP_MC_ME->PRTN1_COFB1_STAT & MC_ME_PRTN1_COFB1_STAT_BLOCK56_MASK));
}

void setup_caches(void) {
  SCB_EnableICache();
}

static void setup_dwt() {
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  DWT->CYCCNT = 0;
}

static void setup_emios(void) {
  /* We will use eMIOS_0 for trigger inputs. 
   *
   * The UCs are only 16 bit counters, but we can use a 4 MHz time base and
   * start them at the same time we start STM_0. As long as we read the 16bit
   * capture values *and* the STM0 CLK within 8 ms (half the period), we can
   * combine the two to get a 32 bit capture time that is aligned with STM_0.
   *
   * First, we configure UC23 as a timebase for Counter Bus A at 4MHZ. The
   * Global prescaler is set to 40 for a 4 MHz clock, and UC23 divides that by
   * 1, in upcounting mode.
   *
   * Then, UCX and UXY are configured in single action capture using UC23 as the
   * timebase.
   */

  // Is also EMIOS_0_CH19_Y, set ALT1
  IP_SIUL2->MSCR[142] = SIUL2_MSCR_OBE_MASK | SIUL2_MSCR_SSS_0(1);

#if 0
  // CH19: mode to gpio out (1), EDPOL for value
  IP_EMIOS_0->CH.UC[19].C = eMIOS_C_MODE(1) | eMIOS_C_EDPOL(0);
#endif

  IP_EMIOS_0->CH.UC[23].C &= ~eMIOS_C_MODE_MASK;
  IP_EMIOS_0->CH.UC[22].C &= ~eMIOS_C_MODE_MASK;
  IP_EMIOS_0->CH.UC[19].C &= ~eMIOS_C_MODE_MASK;

  /* As per RM 64.7.8: */

  /* (1) Disable the GPC */

  //IP_EMIOS_0->MCR &= ~eMIOS_MCR_GPREN_MASK;
  IP_EMIOS_0->MCR = 0;

  /* (2) For UC23 as a 4 MHz timebase for bus A: */
  IP_EMIOS_0->CH.UC[23].C = 0; /* (2a) Disable the CP (reset everything) */
  IP_EMIOS_0->CH.UC[23].CNT = 0; /* (2b) Write initial CNT */
  IP_EMIOS_0->CH.UC[23].A = 0xffff; /* (2c) Write initial A, count from 0-0xffff */
  IP_EMIOS_0->CH.UC[23].C = eMIOS_C_MODE(0x12); /* (2d) Mode: Upcounting modulus
                                                   counter, clear at end */
  IP_EMIOS_0->CH.UC[23].C2 = eMIOS_C2_UCEXTPRE(0); /* (2e) Set divide by 1 */

  IP_EMIOS_0->CH.UC[23].C |= eMIOS_C_UCPREN(1); /* (2f) Enable prescaler */
  IP_EMIOS_0->CH.UC[23].C |= eMIOS_C_FREN(1); /* Freeze counter in debug mode */

  /* Use UC22 as a 250 KHz clock for freq/pwm */
  IP_EMIOS_0->CH.UC[22].C = 0; /* (2a) Disable the CP (reset everything) */
  IP_EMIOS_0->CH.UC[22].CNT = 0; /* (2b) Write initial CNT */
  IP_EMIOS_0->CH.UC[22].A = 0xffff; /* (2c) Write initial A, count from 0-0xffff */
  IP_EMIOS_0->CH.UC[22].C = eMIOS_C_MODE(0x12); /* (2d) Mode: Upcounting modulus
                                                   counter, clear at end */
  IP_EMIOS_0->CH.UC[22].C2 = eMIOS_C2_UCEXTPRE(15); /* (2e) Set divide by 16 */

  IP_EMIOS_0->CH.UC[22].C |= eMIOS_C_UCPREN(1); /* (2f) Enable prescaler */
  IP_EMIOS_0->CH.UC[22].C |= eMIOS_C_FREN(1); /* Freeze counter in debug mode */

  /* (3) Set up our actual UCs for triggers */
  // Set up 19 to toggle as slow as we can with UC22, about 1/2 s period
  IP_EMIOS_0->CH.UC[19].C = 0; /* (2a) Disable the CP (reset everything) */
  IP_EMIOS_0->CH.UC[19].A = 0xffff; /* (2c) Write initial A, count from 0-0xffff */
//  edsel = 1, mode saoc
  IP_EMIOS_0->CH.UC[19].C = eMIOS_C_MODE(0x3) | eMIOS_C_EDSEL(1) | eMIOS_C_BSL(2); /* (2d) Mode: Upcounting modulus
                                                   counter, clear at end */
  IP_EMIOS_0->CH.UC[19].C2 = eMIOS_C2_UCEXTPRE(15); /* (2e) Set divide by 16 */

  IP_EMIOS_0->CH.UC[19].C |= eMIOS_C_UCPREN(1); /* (2f) Enable prescaler */
  IP_EMIOS_0->CH.UC[19].C |= eMIOS_C_FREN(1); /* Freeze counter in debug mode */

  /* (4) Set the prescaler to produce a 4 MHz clock from CORE_CLK */
  IP_EMIOS_0->MCR |= eMIOS_MCR_GPRE(39);
  uint32_t emios0_mcr = IP_EMIOS_0->MCR;

//  uint32_t stm0_cr = STM_CR_TEN_MASK | STM_CR_FRZ_MASK | STM_CR_CPS(19);
  emios0_mcr |= eMIOS_MCR_GPREN(1);
  IP_STM_0->CR |= STM_CR_TEN_MASK;
  IP_EMIOS_0->MCR = emios0_mcr;

  /* (5) Enable GTBE */               
  IP_EMIOS_0->MCR |= eMIOS_MCR_GTBE(1);

  IP_EMIOS_0->MCR |= eMIOS_MCR_FRZ(1); /* Enable global freeze in debug */
}

void setup_stm0(void) {
  IP_MC_CGM->MUX_1_CSC = 
    MC_CGM_MUX_1_CSC_SELCTL(0x16) | /* Use PLAT clock: 80 MHz */
    MC_CGM_MUX_1_CSC_CLK_SW(1); 

  IP_MC_CGM->MUX_1_DC_0 = MC_CGM_MUX_1_DC_0_DE_MASK; /* Enable clock */
  IP_STM_0->CNT = 0;
  //IP_STM_0->CR = 0;
  IP_STM_0->CR = STM_CR_FRZ_MASK | 
//                 STM_CR_TEN_MASK | /* Enable Timer */
                 STM_CR_CPS(19); /* Divide by 20 -> 4 MHz */

  NVIC_SetPriority(STM0_IRQn, 10);
  IP_STM_0->CHANNEL[0].CIR = STM_CIR_CIF(1);
  NVIC_ClearPendingIRQ(STM0_IRQn);
  NVIC_EnableIRQ(STM0_IRQn);
}

double timer_diff __attribute__((externally_visible)) = 0.0;
int biggest_delta __attribute__((externally_visible)) = 0;
void platform_init(void) {

  disable_interrupts();
  NVIC_SetPriorityGrouping(3);

  setup_dwt();
  setup_power();
  setup_periph_clocks();

  setup_fxosc();
  setup_pll();
  setup_clock_tree();
  setup_caches();

  setup_stm0();
  setup_emios();
  int32_t sum = 0;
  for (int i = 0; i < 10000000; i++) {
    disable_interrupts();
    uint16_t s = IP_STM_0->CNT & 0xffff;
    uint16_t e = IP_EMIOS_0->CH.UC[23].CNT;
    enable_interrupts();
    int16_t d = e - s;
    if (abs(d) > biggest_delta) {biggest_delta = abs(d); }
    sum += d;
  }

  timer_diff = sum / 10000000.0;
  while (1);

}

void platform_benchmark_init(void) {
  platform_init();
}

void disable_interrupts(void) {}
void enable_interrupts(void) {}
bool interrupts_enabled(void) {}

void set_output(int output, char value) {}
int get_output(int output) { return 0; }
void set_gpio(int output, char value) {}
int get_gpio(int output) { return 0; }
void set_pwm(int output, float percent) {}

size_t console_read(void *buf, size_t max) {return 0;}
size_t console_write(const void *buf, size_t count) {return 0;}

extern unsigned _configdata_loadaddr, _sconfigdata, _econfigdata;
void platform_load_config(void) {
  volatile unsigned *src, *dest;
  for (src = &_configdata_loadaddr, dest = &_sconfigdata; dest < &_econfigdata;
       src++, dest++) {
    *dest = *src;
  }
}
void platform_save_config(void) {}

void platform_enable_event_logging(void) {}
void platform_disable_event_logging(void) {}
void platform_reset_into_bootloader(void) {}

uint32_t platform_adc_samplerate(void) { return 5000; }
uint32_t platform_knock_samplerate(void) {return 50000; }

/* Returns the earliest time that may still be scheduled.  This can only change
 * when buffers are swapped, so it is safe to use this value to schedule events
 * if interrupts are disabled */
timeval_t platform_output_earliest_schedulable_time(void) { return 0; }

void itm_event(uint32_t val);
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
