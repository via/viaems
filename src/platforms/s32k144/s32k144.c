#include <stdbool.h>
#include "cortex-m4.h"
#include "s32k1xx.h"
#include "s32k144_enet.h"

#include "config.h"
#include "decoder.h"
#include "sensors.h"
#include "viaems.h"

#include <stdio.h>
#include <string.h>

static struct viaems s32k148_viaems = { 0 };

static void disable_watchdog(void) {
  *WDOG_CS = *WDOG_CS & ~WDOG_CS_EN;
  *WDOG_TOVAL = 0xffff;
}

static void enable_peripheral_clocks(void) {
  *PCC_PORTA = PCC_CGC;
  *PCC_PORTB = PCC_CGC;
  *PCC_PORTC = PCC_CGC;
  *PCC_PORTD = PCC_CGC;
  *PCC_PORTE = PCC_CGC;

//  *PCC_FLEXCAN2 = PCC_CGC;
  *PCC_LPUART2 = PCC_CGC | PCC_PCS(3); // Use FIRCDIV2 for UART

  *PCC_LPIT = PCC_CGC | PCC_PCS(6); // Use SPLLDIV2 for LPIT
  *PCC_FTM0 = PCC_CGC;
  *PCC_FTM3 = PCC_CGC;
  *PCC_DMAMUX = PCC_CGC;

  *PCC_PDB0 = PCC_CGC;
  *PCC_ADC0 = PCC_CGC | PCC_PCS(6); // SPLLDIV2 (40 MHz) for ADC0

  *PCC_ENET = PCC_CGC;

}

static void configure_pins(void) {

  // Set C16 and C17 to FlexCAN2
//  *PORTC_PCRn(16) |= PORT_PCRn_MUX(3);
//  *PORTC_PCRn(17) |= PORT_PCRn_MUX(3);
  

  // Configure PTE12 as LPUART2 TX
  *PORTE_PCRn(12) |= PORT_PCRn_MUX(3);

  // Configure PTA0 as TRGMUX OUT 3
  *PORTA_PCRn(0) |= PORT_PCRn_MUX(7);
  // Configure PTA1 as TRGMUX OUT 0
  *PORTA_PCRn(1) |= PORT_PCRn_MUX(7);

  // Configure PTA2 as FTM3_CH0
  *PORTA_PCRn(2) |= PORT_PCRn_MUX(2);
  // Configure PTA3 as FTM3_CH1
  *PORTA_PCRn(3) |= PORT_PCRn_MUX(2);

  // Configure PTC0 as FTM0 CH0
//  *PORTC_PCRn(0) |= PORT_PCRn_MUX(2);  // pin needed for eth


  // EMAC pins
  // PTC2 - TXD0
  *PORTC_PCRn(2) |= PORT_PCRn_MUX(5) | PORT_PCRn_DSE;
  // PTD7 - TXD1
  *PORTD_PCRn(7) |= PORT_PCRn_MUX(5) | PORT_PCRn_DSE;
  // PTE8 - MDC
  *PORTE_PCRn(8) |= PORT_PCRn_MUX(5);
  // PTD12 - TXEN
  *PORTD_PCRn(12) |= PORT_PCRn_MUX(5) | PORT_PCRn_DSE;
  // PTD11 - TXCLK
  *PORTD_PCRn(11) |= PORT_PCRn_MUX(5);
  // PTC17 - CRS_DV
  *PORTC_PCRn(17) |= PORT_PCRn_MUX(5);
  // PTC1 - RXD0
  *PORTC_PCRn(1) |= PORT_PCRn_MUX(5);
  // PTC0 - RXD1
  *PORTC_PCRn(0) |= PORT_PCRn_MUX(4);
  // PTB4 - MDIO
  *PORTB_PCRn(4) |= PORT_PCRn_MUX(5);
  // PTC3 - PHY_RST as output gpio
  *PORTC_PCRn(3) |= PORT_PCRn_MUX(1);
  *GPIOC_PDDR |= (1u << 3);
  *GPIOC_PSOR = (1u << 3);
}

static void configure_system_clocks(void) {
  // Board has 8 MHz crystal
  *SCG_SOSCCFG = SCG_SOSCCFG_RANGE(3) | SCG_SOSCCFG_EREFS; 
  *SCG_SOSCCSR |= SCG_SOSCCSR_SOSCEN;
                             //
  while ((*SCG_SOSCCSR & SCG_SOSCCSR_SOSCVLD) == 0); // Wait for valid

  // PLL out = PLLin / (PREDIV+1) * (MULT+16)
  // PLL out = 8     / (0+1)      * (24+16) = 320
  *SCG_SPLLCFG = SCG_SPLLCFG_MULT(24) | SCG_SPLLCFG_PREDIV(0);
  /* SPLLDIV1 = 80 MHz, SPLLDIV2 = 40 MHz */
  *SCG_SPLLDIV = SCG_SPLLDIV_SPLLDIV1(2) | SCG_SPLLDIV_SPLLDIV2(3);
  *SCG_SPLLCSR = SCG_SPLLCSR_SPLLEN; // enable PLL

  while ((*SCG_SPLLCSR & SCG_SPLLCSR_SPLLVLD) == 0); // Wait for valid

  *SCG_RCCR = SCG_RCCR_SCS(6)      | // SPLL source (160 MHz)
              SCG_RCCR_DIVCORE(1)  | // Core Div/2  (80 MHz)
              SCG_RCCR_DIVBUS(1)   | // Bus (Core) Div/2 (40 MHz)
              SCG_RCCR_DIVSLOW(2);   // Slow (Core) Div/3 (26 MHz)

  while ((*SCG_CSR & SCG_CSR_SCS(0xF)) != SCG_CSR_SCS(6)); // Wait for SPLL to be used as clock
  
  *SCG_FIRCDIV = (2u << 8);

}

static void setup_can(void) {
  *FLEXCAN2_CTRL1 = (1u << 13); // CLKSRC to peripheral clock
  *FLEXCAN2_MCR &= ~(1u << 31); // Clear MDIS

  while ((*FLEXCAN2_MCR & (1u << 24)) == 0); // Wait for FRZACK

  // Configure for 40 MHz peripheral clock, 500 kbit bus rate, so 80 TQ per bit.
  // (SYNC(1) + EPROPSEG(46) + ESEG1(16) + ESEG2(16), 80% sample point
  *FLEXCAN2_CBT = (1u << 31) | // BTF=1
                  (1u << 21) | // EPRESDIV=1
                  (15u << 16) | // ERJW=15
                  (46u << 10) | // EPROPSET=46
                  (15u << 5) | // EPSEG1=15
                  (15u << 0);  // EPSEG2=15

 // // 2 Mbit bus, so 20 TQ per bit
 // *FLEXCAN0_FDCBT = (1u << 20) | // FPRESDIV=1
 //                   (3u << 16) | // FRJW=3
 //                   (7u << 10) | // FPROPSEG=7
 //                   (7u << 5) |  // FPSEG1=7
 //                   (3u << 0);  // FPSEG2=3


//  *FLEXCAN0_FDCTRL = (1u << 31) | // FDRATE
//                     (3u << 16) | // MBDSR0 = 3 (64 byte msgs)
//                     (1u << 15) | // TCDEN
//                     (31u << 8);  // TDCOFF
                     
  for (int i = 0; i < 128; i++) {
    FLEXCAN2_MEM[0] = 0x0;
  }

  *FLEXCAN2_MCR = (0x1Fu << 0) | // MAXMB=31
                  (3u << 8);   // Reject all RX
                               //
  while ((*FLEXCAN2_MCR & (1u << 27)) != 0); // Wait for NOTRDY to clear




}

static void setup_uart(void) {
  // Use LPUART0 on PTC2/3 at 115200
  // SBR at 12 gives 24000000 / ((16+1) * 12) = 117646, 2% error
  *LPUART2_BAUD = LPUART_BAUD_OSR(16) | LPUART_BAUD_SBR(12);
  *LPUART2_CTRL = LPUART_CTRL_TE;
}

static void write_character(char c) {
  while ((*LPUART2_STAT & LPUART_STAT_TRDE) == 0); // TRDE
  *LPUART2_DATA = c;
}

void write_string(const char *c) {
  while (*c != '\0') {
    write_character(*c);
    c++;
  }
}

/* Use usb to send text from newlib printf */
int __attribute__((externally_visible))
_write(int fd, const char *buf, size_t count) {
  (void)fd;
  size_t pos = 0;
  while (pos < count) {
    write_character(buf[pos]);
    pos += 1;
  }
  return count;
}

static volatile uint32_t ftm_rollover_counter = 0;
static volatile bool ftm_synchronized = false;

struct oev {
  int pin;
  uint32_t start;
  uint32_t stop;
};

struct oev oevs[16] = {
  { .pin = 0, .start = 10000, .stop = 11000 },
  { .pin = 1, .start = 10000, .stop = 11000 },

  { .pin = 0, .start = 20000, .stop = 30000 },
  { .pin = 1, .start = 20001, .stop = 30001 },

  { .pin = 0, .start = 40000, .stop = 42000 },
  { .pin = 1, .start = 39000, .stop = 42001 },

  { .pin = 0, .start = 50000, .stop = 60000 },
  { .pin = 1, .start = 60000, .stop = 70000 },

  { .pin = 0, .start = 80000, .stop = 90000 },
  { .pin = 1, .start = 80000, .stop = 90000 },

  { .pin = 0, .start = 91000, .stop = 110000 },
  { .pin = 1, .start = 100000, .stop = 110000 },

  { .pin = 0, .start = 112000, .stop = 113000 },
  { .pin = 1, .start = 112000, .stop = 113000 },

  { .pin = 0, .start = 113300, .stop = 140000 },
  { .pin = 1, .start = 113301, .stop = 140000 },
};

static uint32_t time = 0;
static bool fail = false;
void LPIT0_Ch1_IRQHandler(void) {
//  *GPIOE_PSOR = 1;
  ftm_rollover_counter++;
  ftm_synchronized = true;
  *LPIT_MSR = 2; // Clear flag
  __asm__("dsb");
  __asm__("isb");

  
  struct engine_update update = {
    .sensors = sensors_get_values(&s32k148_viaems.sensors),
    .position = decoder_get_engine_position(&s32k148_viaems.decoder), 
    .current_time = current_time(),
  };
  struct platform_plan plan = {
    .schedulable_start = 0,
    .schedulable_end = 0,
  };

  viaems_reschedule(&s32k148_viaems, &update, &plan);

//  time += 200; 
//  uint16_t channels[] = {0xFFFF, 0xFFFF};
//  for (int i = 0; i < 16; i++) {
//    uint16_t *ch = &channels[oevs[i].pin];
//    if ((oevs[i].start >= time) && (oevs[i].start < (time + 200))) {
//      if (*ch != 0xffff) fail = true;
//      *ch = (oevs[i].start - time) * 40;
//    }
//    if ((oevs[i].stop >= time) && (oevs[i].stop < (time + 200))) {
//      if (*ch != 0xffff) fail = true;
//      *ch = (oevs[i].stop - time) * 40;
//    }
//  }
//  *FTM3_CnV(0) = channels[0];
//  *FTM3_CnV(1) = channels[1];

//  *GPIOE_PCOR = 1;
}

uint32_t current_time(void) {

  /* Get current time in 4MHz increments based on:
   * - FTM0's 80 MHz counter that represents the time inside a 200 uS window,
   *   from 0 to 15999
   * - ftm_rollover_counter, which is incremented in the LPIT's interrupt and
   *   counts the number of rollovers of FTM0.
   *
   * Special cases:
   * - The rollover has occured (due to LPIT's trigger) but the interrupt has
   *   not yet incremented the counter.  We can detect this by checking if
   *   there is a pending interrupt request for LPIT's CH1. This check can race
   *   with the interrupt's clearing of the status register, so it must be done
   *   with that interrupt disabled
   * - Even with interrupts disabled, the FTM0 counter value overflowing can
   *   race with when we fetch it vs when we read the overflow flag. For example
   *   the counter could be 15999 when we read it, and when we read LPIT_MSR it
   *   has overflowed.  We get around this by assuming if the counter is in the
   *   second half that we have not missed any overflow.
   */
  
  if (!ftm_synchronized) {
    return 0;
  }

  disable_interrupts();

  uint32_t ftm_value = *FTM0_CNT;
  uint32_t rollovers = ftm_rollover_counter;
  bool rollover_flag = *LPIT_MSR & 0x2;

  enable_interrupts();

  uint32_t time_4m = rollovers * 800;
  if (rollover_flag && (ftm_value < 8000)) {
    time_4m += 800;
  }
  time_4m += (ftm_value / 20);

  return time_4m;
}

volatile uint32_t last_capture = 0;
void FTM0_Ch0_Ch1_IRQHandler(void) {

  uint32_t ftm_value = *FTM0_CnV(0);
  uint32_t rollovers = ftm_rollover_counter;
  bool rollover_flag = *LPIT_MSR & 0x2;

  *FTM0_CnSC(0) &= ~(1u << 7);

  uint32_t time_4m = (rollovers * 800) + (ftm_value / 20); 

  if (rollover_flag && (ftm_value < 8000)) {
    time_4m += 800;
  }
  last_capture = time_4m;

}

static void setup_lpit(void) {
  /* Configure channel 0 for 1 MHz clock:
   *   - used to drive DMA for outputs
   * Configure channel 1 for 5 KHz clock:
   *   - used to drive ADC
   *   - used to drive reset of FTM0 for trigger captures
   */

  /* FTM0 gets reset at 5 KHz, counts 250 nS intervals */


  /* TRGMUX OUT 0 gets LPIT 0 and TRGMUX OUT 3 gets LPIT1 for testing */
//  *TRGMUX_EXTOUT0 = (0x12 << 24) | (0x11 << 0); 

  /* TRGMUX OUT 0 gets FTM0 and TRGMUX OUT 3 gets LPIT1 for testing */
  *TRGMUX_EXTOUT0 = TRGMUX_SEL0(TRGMUX_SRC_LPIT_CH0) | 
                    TRGMUX_SEL3(TRGMUX_SRC_LPIT_CH1);


  *LPIT_MCR = 1; // M_CEN
  *LPIT_TVAL0 = 40 - 1;
  *LPIT_TVAL1 = 40 * 200 - 1;

  *LPIT_MIER |= 2; // Enable IRQ
  *NVIC_ISER((49/32)) = (1 << (49 & 0x1F));
}

static void start_lpit(void) {
  // Enable both similtaneously
  *LPIT_SETTEN = 0x3;
}


static void setup_ftm0(void) {
  /* Enable FTM0 to count up at 80 MHz from 0-15999, with the reset to 0
   * synchronized with the LPIT CH1 5 KHz clock.  An interrupt fires on this
   * rollover or on input captures on CH0/1, which allows this to act as a time
   * base
   */

  *TRGMUX_FTM0 = TRGMUX_SEL0(TRGMUX_SRC_LPIT_CH1); // LPIT1 triggers FTM0
  
  *FTM0_CnSC(0) = FTM_CnSC_ELSA | FTM_CnSC_CHIE; // Input capture, enable
                                                 // interrupt

  *FTM0_CNTIN = 0;
  *FTM0_MOD = 0xFFFF;
  *FTM0_CNT = 0;
  // Enable counter reset from trgmux input (LPIT CH 1, 5 KHz)
  *FTM0_SYNC = FTM_SYNC_TRIG0;
  *FTM0_SYNCONF = FTM_SYNCONF_HWRSTCNT | FTM_SYNCONF_SYNCMODE | FTM_SYNCONF_HWTRIGMODE;
  *FTM0_SC = FTM_SC_SCS(1); // Use 80 MHz sys clock

  *FTM0_EXTTRIG |= FTM_EXTTRIG_INITTRIGEN;
  *NVIC_ISER((99/32)) = (1 << (99 & 0x1F));
} 

static void setup_ftm3(void) {
  /* Enable FTM3 to count up at 40 MHz from 0-7999 with the reset to 0
   * synchronized with the LPIT CH1 5 KHz clock. 
   * - We set each channel to reload CnV on hardware trigger such that we can
   *   use the LPIT interrupt to load the double-buffered CnV and have it
   *   immediately take effect on the next reload.  This allows the output
   *   compare to reliably be set anywhere in the 200 uS window.
   * - Output compare is configured as toggle. If the upcoming 200 uS window has
   *   no toggles for a channel, CnV is set to 0xFFFF so that it will not fire.
   */

  *TRGMUX_FTM3 = TRGMUX_SEL0(TRGMUX_SRC_LPIT_CH1); // LPIT1 triggers FTM0
  
  *FTM3_CnSC(0) = FTM_CnSC_ELSA | FTM_CnSC_MSA; // Output compare
  *FTM3_CnSC(1) = FTM_CnSC_ELSA | FTM_CnSC_MSA; // Output compare

  *FTM3_CNTIN = 0;
  *FTM3_MOD = 7999;

  *FTM3_CnV(0) = 0xffff;
  *FTM3_CnV(1) = 0xffff;

  *FTM3_CNT = 0;
  // Enable counter reset from trgmux input (LPIT CH 1, 5 KHz)
  *FTM3_MODE = FTM_MODE_WPDIS | FTM_MODE_FTMEN;
  *FTM3_SYNC = FTM_SYNC_TRIG0;
  *FTM3_SYNCONF = FTM_SYNCONF_HWRSTCNT | 
                  FTM_SYNCONF_SYNCMODE | 
                  FTM_SYNCONF_HWTRIGMODE | 
                  FTM_SYNCONF_HWWRBUF;

  *FTM3_COMBINE = FTM_COMBINE_SYNCNE0; // CH0 and CH1 synchronized reg loads

  *FTM3_SC = FTM_SC_PS(1) |    // prescale divide by 2 (40 MHz)
             FTM_SC_SCS(1) |   // Use 80 MHz sys clock
             FTM_SC_PWMEN(3);  // Enable ch0/1 output


}

struct value {
  uint16_t set;
  uint16_t clear;
} __attribute__((packed));

struct value values[200];

static void initialize_values(void) {
  uint16_t current_value = 0;
  for (int i = 0; i < 200; i += 1) {
    uint16_t new_value = current_value + 1;

    uint16_t sets = (current_value ^ new_value) & new_value;
    uint16_t clears = (current_value ^ new_value) & current_value;

    values[i].set = sets;
    values[i].clear = clears;
    current_value = new_value;
  }

}

static void setup_programmed_outputs(void) {
  initialize_values();
  
  *DMA_CR |= DMA_CR_EMLM | // Minor loop mapping
             DMA_CR_ERCA;  // Round Robin
  *DMA_TCD_SADDR(0) = (uint32_t)&values[0];
  *DMA_TCD_SOFF(0) = 2;
  *DMA_TCD_ATTR(0) = DMA_TCD_ATTR_SSIZE(1) | // 16 bit source
                     DMA_TCD_ATTR_DSIZE(1);  // 16 bit dest



  // After each minor loop iteration move the destination back 8 bytes.
  *DMA_TCD_NBYTES_MLOFFYES(0) = DMA_TCD_NBYTES_DMLOE |
                                DMA_TCD_NBYTES_MLOFF(-8) |
                                DMA_TCD_NBYTES_NBYTES(4);


  *DMA_TCD_SLAST(0) = (uint32_t)(-800);

  *DMA_TCD_DADDR(0) = (uint32_t)GPIOE_PSOR;
  *DMA_TCD_DOFF(0) = 4; // Increment 4 bytes to dest
  *DMA_TCD_CITER_ELINKNO(0) = 200;
  *DMA_TCD_DLASTSGA(0) = (-8); // Minor loop offset ignored on last minor loop
                               // so we still need a -8 offset
  *DMA_TCD_BITER_ELINKNO(0) = 200;

  *DMA_ERQ = 1; // Enable CH0


  *TRGMUX_DMAMUX0 = TRGMUX_SEL0(TRGMUX_SRC_LPIT_CH0);

  // Use DMAMUX CH0 since it uses LPIT CH0 as Trigger, with always-on peripheral
  *DMAMUX_CHCFG(0) = DMAMUX_CHCFG_ENBL | 
                     DMAMUX_CHCFG_TRIG |
                     DMAMUX_CHCFG_SOURCE(62);
}

static void setup_adc(void) {
  // Set ADC clock to 4 MHz via prescaler
  *ADC0_CFG1 = ADC_CFG1_ADIV(3) |  // Divide by 8 = 5 MHz
               ADC_CFG1_MODE(1) |  // 12 bit
               ADC_CFG1_ADICLK(0); // ALTCLK 1
                                   //
  *ADC0_CFG2 = ADC_CFG2_SMPLTS(255); // 256 sample clocks for calibration

  // Set all calibration values to 0
  *ADC0_CLPS = 0;
  *ADC0_CLP3 = 0;
  *ADC0_CLP2 = 0;
  *ADC0_CLP1 = 0;
  *ADC0_CLP0 = 0;
  *ADC0_CLPX = 0;
  *ADC0_CLP9 = 0;

  *ADC0_SC3 = ADC_SC3_CAL |
              ADC_SC3_AVGE|
              ADC_SC3_AVGS(3); // average 32 samples

  while ((*ADC0_SC1(0) & ADC_SC1_COCO) == 0);


  *ADC0_CFG1 = ADC_CFG1_ADIV(0) |  // Divide by 1 = 40 MHz
               ADC_CFG1_MODE(1) |  // 12 bit
               ADC_CFG1_ADICLK(0); // ALTCLK 1
  *ADC0_SC3 = 0;

  // Share PTB13 to CH8 on ADC1
  *SIM_CHIPCTL |= SIM_CHIPCTL_ADC_INTERLEAVE_EN(4);

  *ADC0_SC2 |= ADC_SC2_ADTRG; // Enable triggering from PDB0

  *ADC0_SC1(0) = 8; // External input 8
  *ADC0_SC1(1) = 0x1D; // REF L
  *ADC0_SC1(2) = 0x1E; // REF H
  *ADC0_SC1(3) = 0x1B; // Band Gap
  *ADC0_SC1(4) = 8; // External input 8
  *ADC0_SC1(5) = 0x1D; 
  *ADC0_SC1(6) = 0x1E; 
  *ADC0_SC1(7) = ADC_SC1_AIEN | 0x1B;

  nvic_enable_irq(39);
}

static _Atomic bool bump = false; 
void ADC0_IRQHandler(void) {
  *ADC0_R(7);
  bump = true;
}

static void setup_pdb(void) {
  // Configure PDB0 to back-to-back trigger CH0-CH7 as ADC inputs 8-15

  *PDB0_MOD = 0xffff;
  *PDB0_SC = PDB_SC_TRGSEL(0xF) |
             PDB_SC_PDBEN |
             PDB_SC_LDOK;

  *PDB0_CH0C1 = PDB_CHnC1_BB(0xfe) | // Enable back to back trigger on last 7
                PDB_CHnC1_TOS(0)   |
                PDB_CHnC1_EN(0xff);

}

static void enable_cache(void) {
  // Invalidate both ways and enable cache
  *LMEM_PCCCR = LMEM_PCCCR_GO | 
                LMEM_PCCCR_INVW1 |
                LMEM_PCCCR_INVW0 |
                LMEM_PCCCR_ENCACHE;
}


extern uint32_t _data_loadaddr, _sdata, _edata;
extern uint32_t _configdata_loadaddr, _sconfigdata, _econfigdata;
extern uint32_t _ebss;
extern uint32_t _text_loadaddr, _stext, _etext;
uint32_t max = 0;

uint32_t last_time = 0;
uint32_t maxi = 0;

void SystemInit(void) {
  *SCB_CPACR |= ((3UL << (10 * 2)) | (3UL << (11 * 2))); /* set CP10 and CP11 Full Access */
  // copy text segment to SRAM_L
  volatile uint32_t *src, *dest;
  for (src = &_text_loadaddr, dest = &_stext; dest < &_etext; src++, dest++) {
    *dest = *src;
  }
}

int startup(void) {
  disable_watchdog();

  // copy data
  volatile uint32_t *src, *dest;
  for (src = &_data_loadaddr, dest = &_sdata; dest < &_edata; src++, dest++) {
    *dest = *src;
  }

  // copy configdata
  for (src = &_configdata_loadaddr, dest = &_sconfigdata; dest < &_econfigdata; src++, dest++) {
    *dest = *src;
  }

  // clear bss
  while (dest < &_ebss) {
    *dest++ = 0;
  }

  *DWT_CTRL |= DWT_CTRL_CYCCNTENA;

  configure_system_clocks();
  enable_peripheral_clocks();

  enable_cache();


  setup_uart();
//  setup_can();
  setup_lpit();
  setup_ftm0();
  setup_ftm3();
  //setup_programmed_outputs();

  configure_pins();

  viaems_init(&s32k148_viaems, &default_config);
  start_lpit();
//
//  setup_adc();
//  setup_pdb();
//
//  char buf[128];
  write_string("Startup complete!\r\n");

//  *((volatile uint32_t *)(0x4000D000)) &= ~0x1ULL;
  *((volatile uint32_t *)(0x4000D800)) |= (6u << 18); // RW for ENET to all

                                                      // memory via alt
#if BENCH
  int start_benchmarks(void);
  start_benchmarks();
#else

  struct enet_config conf;
  enet_initialize(&conf);

  viaems_idle(&s32k148_viaems);
#endif

}
//
//  sprintf(buf, "  CLPS: %lx\r\n", *ADC0_CLPS);
//  write_string(buf);
//  sprintf(buf, "  CLP3: %lx\r\n", *ADC0_CLP3);
//  write_string(buf);
//  sprintf(buf, "  CLP2: %lx\r\n", *ADC0_CLP2);
//  write_string(buf);
//  sprintf(buf, "  CLP1: %lx\r\n", *ADC0_CLP1);
//  write_string(buf);
//  sprintf(buf, "  CLP0: %lx\r\n", *ADC0_CLP0);
//  write_string(buf);
//  sprintf(buf, "  CLPX: %lx\r\n", *ADC0_CLPX);
//  write_string(buf);
//  sprintf(buf, "  CLP9: %lx\r\n", *ADC0_CLP9);
//  write_string(buf);
//
//
//
//  last_time = get_current_time();
//
//
//  while (true) {
//    uint32_t this_time = get_current_time();
//    while (this_time - last_time < 4000000) {
//      this_time = get_current_time();
//    }
//    last_time = this_time;
//
//    uint32_t ADCvals[8];
//    for (int i = 0; i < 8; i++) {
//      ADCvals[i] = *ADC0_R(i);
//    }
//    sprintf(buf, "ADC Values: %lu %lu %lu %lu %lu %lu %lu %lu\r\n", 
//            ADCvals[0], ADCvals[1],
//            ADCvals[2], ADCvals[3],
//            ADCvals[4], ADCvals[5],
//            ADCvals[6], ADCvals[7]);
//    write_string(buf);
//
//    uint32_t cycles = get_current_time();
//    *PDB0_SC |= PDB_SC_SWTRIG;
//    while (bump == false);
//    bump = false;
//    cycles = get_current_time() - cycles;
//
//    /* With SMPLTS at 255 this takes about 60 uS */
//    sprintf(buf, "    took %lu uS\r\n", cycles / 4);
//    write_string(buf);
//  }
//}
//
