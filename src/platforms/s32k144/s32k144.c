#include <stdbool.h>
#include "cortex-m4.h"
#include "s32k1xx.h"
#include "s32k144_enet.h"

#include "platform.h"
#include "scheduler.h"
#include "config.h"
#include "decoder.h"
#include "sensors.h"
#include "viaems.h"
#include "sim.h"
#include "util.h"
#include "rtt.h"

#include <stdio.h>
#include <string.h>

static struct viaems s32k148_viaems = { 0 };

enum s32k1xx_port {
  PORT_DISABLED,
  PORT_A,
  PORT_B,
  PORT_C,
  PORT_D,
  PORT_E,
};

struct s32k1xx_pincfg {
  enum s32k1xx_port port;
  uint8_t num;
  uint8_t alt;
};

struct s32k1xx_cfg {
  struct s32k1xx_pincfg FTM[4][8];
  struct s32k1xx_pincfg ADC0[8];
  struct s32k1xx_pincfg ADC1[8];
};

const struct s32k1xx_cfg s32k1xx_cfg = {
  .FTM = {
    [0] = {
      { PORT_D, 15, .alt = 2 },
      { PORT_D, 16, .alt = 2 },
      { PORT_C, 2, .alt = 2 },
      { PORT_C, 3, .alt = 2 },
      { PORT_B, 4, .alt = 2 },
      { PORT_B, 5, .alt = 2 },
      { PORT_E, 8, .alt = 2 },
      { PORT_E, 9, .alt = 2 },
    },
    [1] = {
      { PORT_B, 2, .alt = 2 },
      { PORT_B, 3, .alt = 2 },
      { PORT_C, 14, .alt = 2 },
      { PORT_C, 15, .alt = 2 },
      { PORT_A, 10, .alt = 2 },
      { PORT_A, 11, .alt = 2 },
      { PORT_C, 0, .alt = 2 },
      { PORT_C, 1, .alt = 2 },
    },
    [2] = {
      { PORT_C, 5, .alt = 2 },
      { .port = PORT_DISABLED },
      { .port = PORT_DISABLED },
      { PORT_D, 5, .alt = 2 },
      { PORT_E, 10, .alt = 4 },
      { PORT_E, 11, .alt = 4 },
      { .port = PORT_DISABLED },
      { .port = PORT_DISABLED },
    },
    [3] = {
      { PORT_A, 2, .alt = 2 },
      { PORT_A, 3, .alt = 2 },
      { PORT_C, 6, .alt = 4 },
      { PORT_C, 7, .alt = 4 },
      { PORT_D, 2, .alt = 2 },
      { PORT_D, 3, .alt = 2 },
      { PORT_E, 2, .alt = 4 },
      { PORT_E, 6, .alt = 4 },
    },
  },
};

// Note to self: maybe we should just use a 40 MHz tickrate! gives <1 rpm delta for a 60 tooth 
// wheel at 6000 rpms, loops every 107 seconds. If it comes for free, why not?
//
// LPIT is set to trigger CH 1 at 5 khz. CH1 is routed to both FTM0 and FTM3 to reset them
//
// FTM0 counts at 40 MHz. It is used as the timebase and for input capture
// FTM3 counts at 40 MHz. It is used as the output compares for outputs 1-8
// FTM? counts at 40 MHz. It is used as the output compares for outputs 9-16

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

  *PCC_FLEXCAN0 = PCC_CGC;
  *PCC_LPUART2 = PCC_CGC | PCC_PCS(6); // Use SPLLDIV2 (40 MHz) for UART

  *PCC_LPIT = PCC_CGC | PCC_PCS(6); // Use SPLLDIV2 for LPIT
  *PCC_FTM0 = PCC_CGC;
  *PCC_FTM1 = PCC_CGC;
  *PCC_FTM2 = PCC_CGC;
  *PCC_FTM3 = PCC_CGC;
  *PCC_DMAMUX = PCC_CGC;

  *PCC_PDB0 = PCC_CGC;
  *PCC_ADC0 = PCC_CGC | PCC_PCS(6); // SPLLDIV2 (40 MHz) for ADC0

//  *PCC_ENET = PCC_CGC;
  *PCC_CRC = PCC_CGC;

  *PCC_LPSPI1 = PCC_CGC | PCC_PCS(6); // SPLLDIV2 (40 MHz) for SPI1
//  *PCC_LPSPI1 = PCC_CGC | PCC_PCS(3);   // FIRCDIV2 (24 MHz) for SPI1
}

static void configure_pins(void) {

  // Set C16 and C17 to FlexCAN2
//  *PORTC_PCRn(16) |= PORT_PCRn_MUX(3);
//  *PORTC_PCRn(17) |= PORT_PCRn_MUX(3);
//  PTE5 and PTE4 as FlexCAN0
//  *PORTE_PCRn(5) |= PORT_PCRn_MUX(5);
 // *PORTE_PCRn(4) |= PORT_PCRn_MUX(5);
  
  // Configure PTD7 as LPUART2 TX
  *PORTD_PCRn(7) |= PORT_PCRn_MUX(2);
  // Configure PTE3 as LPUART2 RTS
  *PORTE_PCRn(3) |= PORT_PCRn_MUX(3);

  *PORTD_PCRn(0) |= PORT_PCRn_MUX(3) | PORT_PCRn_DSE;  // SCK
  *PORTD_PCRn(1) |= PORT_PCRn_MUX(3) | PORT_PCRn_DSE;  // MOSI
  *PORTE_PCRn(0) |= PORT_PCRn_MUX(5);  // MISO
  *PORTE_PCRn(1) |= PORT_PCRn_MUX(5) | PORT_PCRn_DSE;  // CS

#if 0
  // FTM0 pins
  *PORTD_PCRn(15) |= PORT_PCRn_MUX(2);  // PTD15
  *PORTD_PCRn(16) |= PORT_PCRn_MUX(2);  // PTD16
  *PORTC_PCRn(2)  |= PORT_PCRn_MUX(2);  // PTC2
  *PORTC_PCRn(3)  |= PORT_PCRn_MUX(2);  // PTC3
  *PORTB_PCRn(4)  |= PORT_PCRn_MUX(2);  // PTB4
  *PORTB_PCRn(5)  |= PORT_PCRn_MUX(2);  // PTB5
  *PORTE_PCRn(8)  |= PORT_PCRn_MUX(2);  // PTE8
  *PORTE_PCRn(9)  |= PORT_PCRn_MUX(2);  // PTE9

  // FTM1 pins
  *PORTB_PCRn(2)   |= PORT_PCRn_MUX(2);  // PTB2
  *PORTB_PCRn(3)   |= PORT_PCRn_MUX(2);  // PTB3
  *PORTC_PCRn(14)  |= PORT_PCRn_MUX(2);  // PTC14
  *PORTC_PCRn(15)  |= PORT_PCRn_MUX(2);  // PTC15
  *PORTA_PCRn(10)  |= PORT_PCRn_MUX(2);  // PTA10
  *PORTA_PCRn(11)  |= PORT_PCRn_MUX(2);  // PTA11
  *PORTC_PCRn(0)   |= PORT_PCRn_MUX(2);  // PTC0
  *PORTC_PCRn(1)   |= PORT_PCRn_MUX(2);  // PTC1

  // FTM2
  *PORTC_PCRn(5)  |= PORT_PCRn_MUX(2);  // CH0 - PTC5
  *PORTD_PCRn(5)  |= PORT_PCRn_MUX(2);  // CH3 - PTD5
  *PORTE_PCRn(10) |= PORT_PCRn_MUX(4);  // CH4 - PTE10
  *PORTE_PCRn(11) |= PORT_PCRn_MUX(4);  // CH5 - PTE11

  // FTM3 pins
  *PORTA_PCRn(2) |= PORT_PCRn_MUX(2);  // PTA2
  *PORTA_PCRn(3) |= PORT_PCRn_MUX(2);  // PTA3
  *PORTC_PCRn(6) |= PORT_PCRn_MUX(4);  // PTC6
  *PORTC_PCRn(7) |= PORT_PCRn_MUX(4);  // PTC7
  *PORTD_PCRn(2) |= PORT_PCRn_MUX(2);  // PTD2
  *PORTD_PCRn(3) |= PORT_PCRn_MUX(2);  // PTD3
  *PORTE_PCRn(2) |= PORT_PCRn_MUX(4);  // PTE2
  *PORTE_PCRn(6) |= PORT_PCRn_MUX(4);  // PTE6
#endif                                       //

  for (int f = 0; f < 3; f++) {
    for (int p = 0; p < 8; p++) {
      struct s32k1xx_pincfg cfg = s32k1xx_cfg.FTM[f][p];

      switch (cfg.port) {
        case PORT_A: *PORTA_PCRn(cfg.num) = (*PORTA_PCRn(cfg.num) & ~PORT_PCRn_MUX(7)) | PORT_PCRn_MUX(cfg.alt); break;
        case PORT_B: *PORTB_PCRn(cfg.num) = (*PORTB_PCRn(cfg.num) & ~PORT_PCRn_MUX(7)) | PORT_PCRn_MUX(cfg.alt); break;
        case PORT_C: *PORTC_PCRn(cfg.num) = (*PORTC_PCRn(cfg.num) & ~PORT_PCRn_MUX(7)) | PORT_PCRn_MUX(cfg.alt); break;
        case PORT_D: *PORTD_PCRn(cfg.num) = (*PORTD_PCRn(cfg.num) & ~PORT_PCRn_MUX(7)) | PORT_PCRn_MUX(cfg.alt); break;
        case PORT_E: *PORTE_PCRn(cfg.num) = (*PORTE_PCRn(cfg.num) & ~PORT_PCRn_MUX(7)) | PORT_PCRn_MUX(cfg.alt); break;
        case PORT_DISABLED: break;
      }
    }
  }


  // Set RTS low
  *GPIOE_PDDR |= (1u << 3);
  *GPIOE_PCOR = (1u << 3);

  // E0 E1 debug/timing pins
//  *PORTE_PCRn(0) |= PORT_PCRn_MUX(1);
//  *PORTE_PCRn(1) |= PORT_PCRn_MUX(1);
//  *GPIOE_PDDR |= (1u << 0) | (1u << 1);

#if 0
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
  // PTC1 - RXD-1
  *PORTC_PCRn(1) |= PORT_PCRn_MUX(5);
  // PTC0 - RXD1
  *PORTC_PCRn(0) |= PORT_PCRn_MUX(4);
  // PTB4 - MDIO
  *PORTB_PCRn(4) |= PORT_PCRn_MUX(5);
  // PTC3 - PHY_RST as output gpio
  *PORTC_PCRn(3) |= PORT_PCRn_MUX(1);
  *GPIOC_PDDR |= (1u << 3);
  *GPIOC_PSOR = (1u << 3);
#endif
}

static void configure_system_clocks(void) {
  // Board has 8 MHz crystal
  *SCG_SOSCCFG = SCG_SOSCCFG_RANGE(3) | SCG_SOSCCFG_EREFS; // | (1u << 3); 
  *SCG_SOSCCSR |= SCG_SOSCCSR_SOSCEN;
                             //
  while ((*SCG_SOSCCSR & SCG_SOSCCSR_SOSCVLD) == 0); // Wait for valid

  // PLL out = PLLin / (PREDIV+1) * (MULT+16)
  // PLL out = 16     / (1+1)      * (24+16) = 320
  *SCG_SPLLCFG = SCG_SPLLCFG_MULT(24) | SCG_SPLLCFG_PREDIV(1);
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

static void setup_spi0(void) {

}

static void setup_can0(void) {


  *FLEXCAN0_CTRL1 = (1u << 13); // CLKSRC to peripheral clock
  *FLEXCAN0_MCR &= ~(1u << 31); // Clear MDIS


  while ((*FLEXCAN0_MCR & (1u << 24)) == 0); // Wait for FRZACK

  *FLEXCAN0_MCR |= (1u << 25);
  while ((*FLEXCAN0_MCR & (1u << 25)) != 0); // Wait for FRZACK

  // Params copied from https://github.com/nxp-auto-support/s32k148_cookbook/blob/master/S32K148_Project_FlexCan_FdFrames/src/CAN_FD.c
  // Literally the only set of params i've gotten FD working with. Modified for 500kbit base rate for OBD2
  

  // Configure for 80 MHz peripheral clock, div 1, 500 kbit bus rate, so 80 TQ per bit.
  // (SYNC(1) + EPROPSEG(46) + ESEG1(18) +2 + ESEG2(12) + 1, 83.75% sample point
  *FLEXCAN0_CBT = (1u << 31) | // BTF=1
                  (1u << 21) | // EPRESDIV=10  8
                 (12 << 16) | // ERJW=15      60
                 (46 << 10) | // EPROPSET=5  20
                 (18 << 5) | // EPSEG1=8   32
                 (12 << 0);  // EPSEG2=2   8




  // Configure for 80 MHz peripheral clock, div 0, 4000 kbit bus rate, so 20 TQ per bit.
  // (SYNC(1) + EPROPSEG(7) + ESEG1(6) + 1 + ESEG2(4) + 1, 80% sample point

  *FLEXCAN0_FDCBT = (0u << 20) | // FPRESDIV=1
                    (4 << 16) | // FRJW=3
                    (8 << 10) | // FPROPSEG=2
                    (6 << 5) |  // FPSEG1=8
                    (4 << 0);  // FPSEG2=4


  *FLEXCAN0_FDCTRL = (1u << 31) | // FDRATE
                     (3u << 16) | // MBDSR0 = 3 (64 byte msgs)
                     (1u << 15) | // TCDEN
                     (5 << 8);  // TDCOFF
                     
  for (int i = 0; i < 128; i++) {
    FLEXCAN0_MEM[0] = 0x0;
  }

  *FLEXCAN0_IFLAG1 = 0x1;
  *FLEXCAN0_CTRL2 |= (1u << 12); // ISOCANFDEN
  *FLEXCAN0_MCR = (6 << 0) | // MAXMB=31
                  (1u << 11) | // FDEN
                  (3u << 8);   // Reject all RX
                               //
  while ((*FLEXCAN0_MCR & (1u << 27)) != 0); // Wait for NOTRDY to clear
}

static void send_can_frame(size_t len, uint8_t bytes[8]) {

  // Find a MB, but for now assume first one
  volatile uint32_t *MB = &FLEXCAN0_MEM[0];

  *FLEXCAN0_IFLAG1 = 0xf;

  MB[2] = __builtin_bswap32(((uint32_t *)bytes)[0]);
  MB[3] = __builtin_bswap32(((uint32_t *)bytes)[1]);

  MB[4] = __builtin_bswap32(((uint32_t *)bytes)[0]);
  MB[5] = __builtin_bswap32(((uint32_t *)bytes)[1]);
  
  MB[6] = __builtin_bswap32(((uint32_t *)bytes)[0]);
  MB[7] = __builtin_bswap32(((uint32_t *)bytes)[1]);

  MB[8] = __builtin_bswap32(((uint32_t *)bytes)[0]);
  MB[9] = __builtin_bswap32(((uint32_t *)bytes)[1]);

  MB[10] = __builtin_bswap32(((uint32_t *)bytes)[0]);
  MB[11] = __builtin_bswap32(((uint32_t *)bytes)[1]);

  MB[12] = __builtin_bswap32(((uint32_t *)bytes)[0]);
  MB[13] = __builtin_bswap32(((uint32_t *)bytes)[1]);
  
  MB[14] = __builtin_bswap32(((uint32_t *)bytes)[0]);
  MB[15] = __builtin_bswap32(((uint32_t *)bytes)[1]);

  MB[16] = __builtin_bswap32(((uint32_t *)bytes)[0]);
  MB[17] = __builtin_bswap32(((uint32_t *)bytes)[1]);

  MB[1] = FLEXCAN_MB1_ID(0x554);
  MB[0] = FLEXCAN_MB0_EDL | 
          FLEXCAN_MB0_BRS | 
//          FLEXCAN_MB0_SRR | 
          FLEXCAN_MB0_CODE(0xC) | 
          FLEXCAN_MB0_DLC(15);

  MB = &FLEXCAN0_MEM[18];

  MB[2] = __builtin_bswap32(((uint32_t *)bytes)[0]);
  MB[3] = __builtin_bswap32(((uint32_t *)bytes)[1]);

  MB[4] = __builtin_bswap32(((uint32_t *)bytes)[0]);
  MB[5] = __builtin_bswap32(((uint32_t *)bytes)[1]);
  
  MB[6] = __builtin_bswap32(((uint32_t *)bytes)[0]);
  MB[7] = __builtin_bswap32(((uint32_t *)bytes)[1]);

  MB[8] = __builtin_bswap32(((uint32_t *)bytes)[0]);
  MB[9] = __builtin_bswap32(((uint32_t *)bytes)[1]);

  MB[10] = __builtin_bswap32(((uint32_t *)bytes)[0]);
  MB[11] = __builtin_bswap32(((uint32_t *)bytes)[1]);

  MB[12] = __builtin_bswap32(((uint32_t *)bytes)[0]);
  MB[13] = __builtin_bswap32(((uint32_t *)bytes)[1]);
  
  MB[14] = __builtin_bswap32(((uint32_t *)bytes)[0]);
  MB[15] = __builtin_bswap32(((uint32_t *)bytes)[1]);

  MB[16] = __builtin_bswap32(((uint32_t *)bytes)[0]);
  MB[17] = __builtin_bswap32(((uint32_t *)bytes)[1]);

  MB[1] = FLEXCAN_MB1_ID(0x555);
  MB[0] = FLEXCAN_MB0_EDL | 
          FLEXCAN_MB0_BRS | 
//          FLEXCAN_MB0_SRR | 
          FLEXCAN_MB0_CODE(0xC) | 
          FLEXCAN_MB0_DLC(15);

  MB = &FLEXCAN0_MEM[36];

  MB[2] = __builtin_bswap32(((uint32_t *)bytes)[0]);
  MB[3] = __builtin_bswap32(((uint32_t *)bytes)[1]);

  MB[4] = __builtin_bswap32(((uint32_t *)bytes)[0]);
  MB[5] = __builtin_bswap32(((uint32_t *)bytes)[1]);
  
  MB[6] = __builtin_bswap32(((uint32_t *)bytes)[0]);
  MB[7] = __builtin_bswap32(((uint32_t *)bytes)[1]);

  MB[8] = __builtin_bswap32(((uint32_t *)bytes)[0]);
  MB[9] = __builtin_bswap32(((uint32_t *)bytes)[1]);

  MB[10] = __builtin_bswap32(((uint32_t *)bytes)[0]);
  MB[11] = __builtin_bswap32(((uint32_t *)bytes)[1]);

  MB[12] = __builtin_bswap32(((uint32_t *)bytes)[0]);
  MB[13] = __builtin_bswap32(((uint32_t *)bytes)[1]);
  
  MB[14] = __builtin_bswap32(((uint32_t *)bytes)[0]);
  MB[15] = __builtin_bswap32(((uint32_t *)bytes)[1]);

  MB[16] = __builtin_bswap32(((uint32_t *)bytes)[0]);
  MB[17] = __builtin_bswap32(((uint32_t *)bytes)[1]);

  MB[1] = FLEXCAN_MB1_ID(0x556);
  MB[0] = FLEXCAN_MB0_EDL | 
          FLEXCAN_MB0_BRS | 
//          FLEXCAN_MB0_SRR | 
          FLEXCAN_MB0_CODE(0xC) | 
          FLEXCAN_MB0_DLC(15);

  MB = &FLEXCAN0_MEM[54];

  MB[2] = __builtin_bswap32(((uint32_t *)bytes)[0]);
  MB[3] = __builtin_bswap32(((uint32_t *)bytes)[1]);

  MB[4] = __builtin_bswap32(((uint32_t *)bytes)[0]);
  MB[5] = __builtin_bswap32(((uint32_t *)bytes)[1]);
  
  MB[6] = __builtin_bswap32(((uint32_t *)bytes)[0]);
  MB[7] = __builtin_bswap32(((uint32_t *)bytes)[1]);

  MB[8] = __builtin_bswap32(((uint32_t *)bytes)[0]);
  MB[9] = __builtin_bswap32(((uint32_t *)bytes)[1]);

  MB[10] = __builtin_bswap32(((uint32_t *)bytes)[0]);
  MB[11] = __builtin_bswap32(((uint32_t *)bytes)[1]);

  MB[12] = __builtin_bswap32(((uint32_t *)bytes)[0]);
  MB[13] = __builtin_bswap32(((uint32_t *)bytes)[1]);
  
  MB[14] = __builtin_bswap32(((uint32_t *)bytes)[0]);
  MB[15] = __builtin_bswap32(((uint32_t *)bytes)[1]);

  MB[16] = __builtin_bswap32(((uint32_t *)bytes)[0]);
  MB[17] = __builtin_bswap32(((uint32_t *)bytes)[1]);

  MB[1] = FLEXCAN_MB1_ID(0x200);
  MB[0] = FLEXCAN_MB0_EDL | 
          FLEXCAN_MB0_BRS | 
//          FLEXCAN_MB0_SRR | 
          FLEXCAN_MB0_CODE(0xC) | 
          FLEXCAN_MB0_DLC(15);

  while ((*FLEXCAN0_IFLAG1 & 0xf) == 0);

}

static void setup_uart(void) {
  // SBR at 1 gives 40000000 / ((9+1) * 1) = 4000000 (4 MBaud)
  *LPUART2_BAUD = LPUART_BAUD_OSR(9) | LPUART_BAUD_SBR(1) | (1u << 17); // (Sample both edges)
  *LPUART2_FIFO |= (1u << 7); // enable fifo
  *LPUART2_WATER |= 3;
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
#if 0 
  size_t pos = 0;
  while (pos < count) {
    write_character(buf[pos]);
    pos += 1;
  }
  return count;
#else
  if (rtt_write(buf, count)) {
    return count;
  } else {
    return 0;
  }
#endif
}

enum s32k1xx_ftm_pin_type {
  FTM_PIN_DISABLED,
  FTM_PIN_GPIO_IN,
  FTM_PIN_GPIO_OUT,
  FTM_PIN_SCHED_OUT,
  FTM_PIN_LSPWM_OUT,
  FTM_PIN_FREQ_IN,
  FTM_PIN_TRIGGER_IN,
  FTM_PIN_SYNC_IN,
  FTM_PIN_HSPWM_OUT, /* This mode consumes two adjacent even->odd pins as a pair:
                        Other pin can either be DISABLED or also HSPWM_OUT to
                        act as complementary output */
};

/* In order, all of FTM0, FTM1, FTM2, FTM3 */
enum s32k1xx_ftm_pin_type ftm_pin_types[32] = { 
  FTM_PIN_TRIGGER_IN,  // Hard-code for now
  FTM_PIN_SYNC_IN,     // Hard-code for now
  FTM_PIN_HSPWM_OUT,
  FTM_PIN_DISABLED,
  FTM_PIN_HSPWM_OUT,
  FTM_PIN_DISABLED,
  FTM_PIN_DISABLED,
  FTM_PIN_GPIO_OUT,

#if 1
  FTM_PIN_SCHED_OUT,
  FTM_PIN_SCHED_OUT,
  FTM_PIN_SCHED_OUT,
  FTM_PIN_SCHED_OUT,
  FTM_PIN_SCHED_OUT,
  FTM_PIN_SCHED_OUT,
  FTM_PIN_SCHED_OUT,
  FTM_PIN_SCHED_OUT,
#else
  FTM_PIN_HSPWM_OUT,
  FTM_PIN_DISABLED,
  FTM_PIN_HSPWM_OUT,
  FTM_PIN_DISABLED,
  FTM_PIN_HSPWM_OUT,
  FTM_PIN_DISABLED,
  FTM_PIN_HSPWM_OUT,
  FTM_PIN_DISABLED,
#endif

  FTM_PIN_HSPWM_OUT,
  FTM_PIN_DISABLED,
  FTM_PIN_DISABLED,
  FTM_PIN_HSPWM_OUT,
  FTM_PIN_LSPWM_OUT,
  FTM_PIN_LSPWM_OUT,
  FTM_PIN_DISABLED,
  FTM_PIN_DISABLED,

#if 1
  FTM_PIN_SCHED_OUT,
  FTM_PIN_SCHED_OUT,
  FTM_PIN_SCHED_OUT,
  FTM_PIN_SCHED_OUT,
  FTM_PIN_SCHED_OUT,
  FTM_PIN_SCHED_OUT,
  FTM_PIN_SCHED_OUT,
  FTM_PIN_SCHED_OUT,
#else
  FTM_PIN_HSPWM_OUT,
  FTM_PIN_DISABLED,
  FTM_PIN_HSPWM_OUT,
  FTM_PIN_DISABLED,
  FTM_PIN_HSPWM_OUT,
  FTM_PIN_DISABLED,
  FTM_PIN_HSPWM_OUT,
  FTM_PIN_DISABLED,
#endif
};


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

static bool sim_wakeup_enabled = false;
static uint32_t sim_wakeup_time = 0;

void set_sim_wakeup(timeval_t t) {  
  sim_wakeup_enabled = true;
  sim_wakeup_time = t;
}

static uint32_t current_lpit_base_time = 0;

struct lspwm_state {
  uint32_t period;
  bool active;
  uint32_t remaining;
  uint16_t shift;
};

static void do_lspwm(struct lspwm_state *s, float duty, uint32_t FTM, uint8_t pin) {

  *FTM_CnV(FTM, pin) = 0xFFFF;

  /* If remaining < 8000, we will need a transition */
  if (s->remaining < 8000) {
    uint16_t t1 = s->remaining;

    /* Calculate next remaining, factoring in any prior phase shift */
    uint16_t after_t1 = 0;
    if (s->active) {
      s->remaining = s->period * (1.0f - duty) - s->shift;
    } else {
      s->remaining = s->period * duty - s->shift;
    }
    s->shift = 0;

    s->active = !s->active;

    // If t1 + after_t2 is still inside this window, we need to phase shift to get the second
    // edge to the next one. Store the offset so that we can remove it somewhere else.
    if (t1 + s->remaining < 8000) {
      uint16_t shift = 8000 - (t1 + s->remaining);
      t1 += shift;
      s->shift = shift;
    }


    *FTM_CnV(FTM, pin) = t1;
    s->remaining -= (8000 - t1);

  } else {
    s->remaining -= 8000;
  }
}

struct hspwm_state {
  uint32_t period; // In FTM ticks at 40 MHz

  bool active;
  uint32_t remaining;
};

static uint32_t ftm_from_pin(const uint32_t pin) {
  return FTM_BASE(pin >> 3);
}

static uint32_t ftm_channel_from_pin(const uint32_t pin) {
  return pin & 0x7u;
}

static struct hspwm_state hspwm_states[32] = {
  { .period = (40000000 / 201) },
  { .period = (40000000 / 201) },
  { .period = (40000000 / 201) },
  { .period = (40000000 / 201) },
  { .period = (40000000 / 201) },
  { .period = (40000000 / 201) },
  { .period = (40000000 / 201) },
  { .period = (40000000 / 201) },
  { .period = (40000000 / 201) },
  { .period = (40000000 / 201) },
  { .period = (40000000 / 201) },
  { .period = (40000000 / 201) },
  { .period = (40000000 / 201) },
  { .period = (40000000 / 201) },
  { .period = (40000000 / 201) },
  { .period = (40000000 / 201) },
  { .period = (40000000 / 201) },
  { .period = (40000000 / 201) },
  { .period = (40000000 / 201) },
  { .period = (40000000 / 201) },
  { .period = (40000000 / 201) },
  { .period = (40000000 / 201) },
  { .period = (40000000 / 201) },
  { .period = (40000000 / 201) },
  { .period = (40000000 / 201) },
  { .period = (40000000 / 201) },
  { .period = (40000000 / 201) },
  { .period = (40000000 / 201) },
  { .period = (40000000 / 201) },
  { .period = (40000000 / 201) },
  { .period = (40000000 / 201) },
  { .period = (40000000 / 201) },
};

static struct lspwm_state lspwm_states[32] = {
  { .period = (40000000 / 200) },
  { .period = (40000000 / 200) },
  { .period = (40000000 / 200) },
  { .period = (40000000 / 200) },
  { .period = (40000000 / 200) },
  { .period = (40000000 / 200) },
  { .period = (40000000 / 200) },
  { .period = (40000000 / 200) },
  { .period = (40000000 / 200) },
  { .period = (40000000 / 200) },
  { .period = (40000000 / 200) },
  { .period = (40000000 / 200) },
  { .period = (40000000 / 200) },
  { .period = (40000000 / 200) },
  { .period = (40000000 / 200) },
  { .period = (40000000 / 200) },
  { .period = (40000000 / 200) },
  { .period = (40000000 / 200) },
  { .period = (40000000 / 200) },
  { .period = (40000000 / 200) },
  { .period = (40000000 / 200) },
  { .period = (40000000 / 200) },
  { .period = (40000000 / 200) },
  { .period = (40000000 / 200) },
  { .period = (40000000 / 200) },
  { .period = (40000000 / 200) },
  { .period = (40000000 / 200) },
  { .period = (40000000 / 200) },
  { .period = (40000000 / 200) },
  { .period = (40000000 / 200) },
  { .period = (40000000 / 200) },
  { .period = (40000000 / 200) },
};

static void do_hspwm(struct hspwm_state *s, float duty, uint32_t FTM, uint8_t pin) {
  uint8_t p1 = pin & 0xFE;  
  uint8_t p2 = p1 + 1;

  *FTM_CnV(FTM, p1) = 0xFFFF;
  *FTM_CnV(FTM, p2) = 0xFFFF;

  /* If remaining < 8000, we will need at least one transition */
  if (s->remaining < 8000) {
    uint16_t t1 = s->remaining;
    *FTM_CnV(FTM, p1) = t1;

    /* Calculate next remaining */
    if (s->active) {
      s->remaining = s->period * (1.0f - duty);
    } else {
      s->remaining = s->period * duty;
    }
    s->active = !s->active;

    /* Do we need a second transition? */
    if (s->remaining + t1 < 8000) {
      uint16_t t2 = t1 + s->remaining ;
      *FTM_CnV(FTM, p2) = t2;

      /* Calculate next remaining */
      if (s->active) {
        s->remaining = s->period * (1.0f - duty);
      } else {
        s->remaining = s->period * duty;
      }
      s->active = !s->active;
      s->remaining -= (8000 - t2);
    } else {
      s->remaining -= (8000 - t1);
    }

  } else {
    s->remaining -= 8000;
  }
}


/* To correctly set fired events as fired, we need to double buffer
 * the plans */
static struct platform_plan plans[2] = { 0 };
static size_t current_plan = 0;

static bool offcycle = false;
void LPIT0_Ch1_IRQHandler(void) {
//  *GPIOE_PSOR = 1;
  *LPIT_MSR = 2; // Clear flag
  __asm__("dsb");
  __asm__("isb");

  current_plan = (current_plan == 0) ? 1 : 0;
  struct platform_plan *plan = &plans[current_plan];

  /* Retire this plan's events */
  for (int i = 0; i < plan->n_events; i++) {
    struct schedule_entry *s = plan->schedule[i];
    s->state = SCHED_FIRED;
  }

  current_lpit_base_time += 8000;
  
  struct engine_update update = {
    .sensors = sensors_get_values(&s32k148_viaems.sensors),
    .position = decoder_get_engine_position(&s32k148_viaems.decoder), 
    .current_time = current_lpit_base_time,
  };
  plan->schedulable_start = current_lpit_base_time + 8000;
  plan->schedulable_end = current_lpit_base_time + 8000 * 2;
  plan->n_events = 0;

//  *GPIOE_PSOR = (1 << 1);

  viaems_reschedule(&s32k148_viaems, &update, plan);

  plan->pwm[20] = 0.02;
  plan->pwm[21] = 0.03;

//  *GPIOE_PCOR = (1 << 1);

  uint16_t sched_out_ftm_values[32];

  // Ensure any value we don't explicitly set will be reset and not trigger
  for (int i = 0; i < 32; i++) {
    sched_out_ftm_values[i] = 0xFFFF;
  }

  for (int i = 0; i < plan->n_events; i++) {
    struct schedule_entry *s = plan->schedule[i];

    uint32_t ftm_time = s->time - plan->schedulable_start;
    if (sched_out_ftm_values[s->pin] != 0xffff) {
      // TODO, this is a fault condition
    }
    sched_out_ftm_values[s->pin] = ftm_time;
    s->state = SCHED_SUBMITTED;
  }

  // All buffered registers don't show our writes in a read, so we need to
  // maintain our changes locally and set them once.
  uint32_t INVCTRL = 0;
  uint32_t SWOCTRL = 0;

  for (int pin = 0; pin < 32; pin++) {
    uint32_t FTM = ftm_from_pin(pin);
    uint32_t FTM_CH = ftm_channel_from_pin(pin);
    switch (ftm_pin_types[pin]) {
      case FTM_PIN_SCHED_OUT: {
        *FTM_CnV(FTM, FTM_CH) = sched_out_ftm_values[pin];
                              }
        break;
      case FTM_PIN_GPIO_OUT:
        SWOCTRL |= (1u << FTM_CH);
        if ((plan->gpio & (1 << pin)) != 0) {
          SWOCTRL |= (1u << (FTM_CH + 8));
        }
        break;
      case FTM_PIN_HSPWM_OUT:
        if (hspwm_states[pin].active) {
          INVCTRL |= (1u << (FTM_CH >> 1));
        }
        do_hspwm(&hspwm_states[pin], plan->pwm[pin], FTM, FTM_CH);
        break;
      case FTM_PIN_LSPWM_OUT:
        do_lspwm(&lspwm_states[pin], plan->pwm[pin], FTM, FTM_CH);
        break;
      default:
        break;
    }
    if ((pin & 0x7) == 0x7) {
      // End of an FTM, synchronize INVCTRL and SWOCTRL
      *FTM_INVCTRL(FTM) = INVCTRL;
      INVCTRL = 0;

      *FTM_SWOCTRL(FTM) = SWOCTRL;
      SWOCTRL = 0;
    }
  }

//  *GPIOE_PCOR = 1;

  // Handle sim callbacks if between now and next lpit rollover
  while (sim_wakeup_enabled && 
         time_in_range(sim_wakeup_time, 
                       current_lpit_base_time, 
                       current_lpit_base_time + 8000)) {
    sim_wakeup_callback(&s32k148_viaems.decoder);
  }



}

uint32_t current_time(void) {

  /* Get current time in 4MHz increments based on:
   * - FTM0's 40 MHz counter that represents the time inside a 200 uS window,
   *   from 0 to 7999.
   * - current_lpit_base_time, which is incremented in the LPIT's interrupt 
   *   by 8000 ticks.
   *
   * Special cases:
   * - The rollover has occured (due to LPIT's trigger) but the interrupt has
   *   not yet incremented the counter.  We can detect this by checking if
   *   there is a pending interrupt request for LPIT's CH1. This check can race
   *   with the interrupt's clearing of the status register, so it must be done
   *   with that interrupt disabled
   * - Even with interrupts disabled, the FTM0 counter value overflowing can
   *   race with when we fetch it vs when we read the overflow flag. For example
   *   the counter could be 7999 when we read it, and when we read LPIT_MSR it
   *   has overflowed.  We get around this by assuming if the counter is in the
   *   second half that we have not missed any overflow.
   */
  
  _disable_interrupts();

  uint32_t ftm_value = *FTM0_CNT;
  uint32_t time = current_lpit_base_time;
  bool rollover_flag = *LPIT_MSR & 0x2;

  _enable_interrupts();

  if (rollover_flag && (ftm_value < 4000)) {
    time += 8000;
  }

  return time + ftm_value;
}

static void do_ftm_captures(void) {
}

void FTM0_Ch0_Ch1_IRQHandler(void) {

  /* Use same rollover logic as current_time() */
  uint32_t lpit_time = current_lpit_base_time;
  bool rollover_flag = *LPIT_MSR & 0x2;

  bool ch0_fired = false;
  bool ch1_fired = false;
  timeval_t ch0_time;
  timeval_t ch1_time;

  if (*FTM0_CnSC(0) & FTM_CnSC_CHF) {
    ch0_fired = true;
    ch0_time = *FTM0_CnV(0);
    if (rollover_flag && (ch0_time < 4000)) {
      ch0_time += 8000;
    }
    *FTM0_CnSC(0) &= ~FTM_CnSC_CHF;
    ch0_time += lpit_time;
  }

  if (*FTM0_CnSC(1) & FTM_CnSC_CHF) {
    ch1_fired = true;
    ch1_time = *FTM0_CnV(1);
    if (rollover_flag && (ch1_time < 4000)) {
      ch1_time += 8000;
    }
    *FTM0_CnSC(1) &= ~FTM_CnSC_CHF;
    ch1_time += lpit_time;
  }

  const trigger_type ch0_type = s32k148_viaems.config->trigger_inputs[0].type;
  const trigger_type ch1_type = s32k148_viaems.config->trigger_inputs[1].type;

  if (ch0_fired && ch1_fired && time_before(ch1_time, ch0_time)) {
    decoder_update(&s32k148_viaems.decoder,
                   &(struct trigger_event){ .time = ch1_time, .type = ch1_type });
    decoder_update(&s32k148_viaems.decoder,
                   &(struct trigger_event){ .time = ch0_time, .type = ch0_type });

  } else {
    if (ch0_fired) {
      decoder_update(&s32k148_viaems.decoder,
          &(struct trigger_event){ .time = ch0_time, .type = ch0_type });
    }
    if (ch1_fired) {
      decoder_update(&s32k148_viaems.decoder,
                     &(struct trigger_event){ .time = ch1_time, .type = ch1_type });
    }
  }


}

static void setup_lpit(void) {
   /* Configure channel 1 for 5 KHz clock:
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
  /* Enable FTM0 to count up at 40 MHz from 0-7999, with the reset to 0
   * synchronized with the LPIT CH1 5 KHz clock.  An interrupt fires on this
   * rollover or on input captures on CH0/1, which allows this to act as a time
   * base
   */

  *TRGMUX_FTM0 = TRGMUX_SEL0(TRGMUX_SRC_LPIT_CH1); // LPIT1 triggers FTM0
  
  *FTM0_CnSC(0) = FTM_CnSC_ELSA | FTM_CnSC_CHIE; // Input capture, enable
                                                 // interrupt
  *FTM0_CnSC(1) = FTM_CnSC_ELSA | FTM_CnSC_CHIE; // Input capture, enable
                                                 // interrupt

  *FTM0_CNTIN = 0;
  *FTM0_MOD = 0xFFFF;
  *FTM0_CNT = 0;
  // Enable counter reset from trgmux input (LPIT CH 1, 5 KHz)
  *FTM0_SYNC = FTM_SYNC_TRIG0;
  *FTM0_SYNCONF = FTM_SYNCONF_HWRSTCNT | FTM_SYNCONF_SYNCMODE | FTM_SYNCONF_HWTRIGMODE;
  *FTM0_SC = FTM_SC_PS(1) | FTM_SC_SCS(1); // Use 80 MHz sys clock divided by 2

  *FTM0_EXTTRIG |= FTM_EXTTRIG_INITTRIGEN;
  *NVIC_ISER((99/32)) = (1 << (99 & 0x1F));
} 

static void configure_ftm(void) {
  /* Configure all FTMs to count up at 40 MHz from 0-7999 with a reset to 0
   * synchronized by the LPIT CH1 5 KHz clock.
   */

  *TRGMUX_FTM0 = TRGMUX_SEL0(TRGMUX_SRC_LPIT_CH1); // LPIT1 triggers FTM0
  *TRGMUX_FTM1 = TRGMUX_SEL0(TRGMUX_SRC_LPIT_CH1); // LPIT1 triggers FTM0
  *TRGMUX_FTM2 = TRGMUX_SEL0(TRGMUX_SRC_LPIT_CH1); // LPIT1 triggers FTM0
  *TRGMUX_FTM3 = TRGMUX_SEL0(TRGMUX_SRC_LPIT_CH1); // LPIT1 triggers FTM0


  for (int f = 0; f < 4; f++) {
    uint32_t FTM = FTM_BASE(f);
    *FTM_CNTIN(FTM) = 0;
    *FTM_MOD(FTM) = 7999;
    *FTM_CNT(FTM) = 0;
    *FTM_MODE(FTM) = FTM_MODE_WPDIS | FTM_MODE_FTMEN;
    *FTM_SYNC(FTM) = FTM_SYNC_TRIG0;
    *FTM_SYNCONF(FTM) = FTM_SYNCONF_HWRSTCNT | 
                        FTM_SYNCONF_SYNCMODE | 
                        FTM_SYNCONF_HWTRIGMODE | 
                        FTM_SYNCONF_HWINVC |
                        FTM_SYNCONF_HWSOC |
                        FTM_SYNCONF_SWOC |
                        FTM_SYNCONF_INVC |
                        FTM_SYNCONF_HWWRBUF;

    *FTM_COMBINE(FTM) = FTM_COMBINE_SYNCNE0 | // CH0 and CH1 synchronized reg loads
                        FTM_COMBINE_SYNCNE1 |
                        FTM_COMBINE_SYNCNE2 |
                        FTM_COMBINE_SYNCNE3;
  }

  for (int pin = 0; pin < 32; pin++) {
    uint32_t FTM = ftm_from_pin(pin);
    uint32_t FTM_CH = ftm_channel_from_pin(pin);

    switch (ftm_pin_types[pin]) {
      case FTM_PIN_GPIO_OUT:
        *FTM_CnSC(FTM, FTM_CH) = FTM_CnSC_ELSA | FTM_CnSC_MSA; // Output compare
        break;

      case FTM_PIN_SCHED_OUT:
      case FTM_PIN_LSPWM_OUT:
        *FTM_CnSC(FTM, FTM_CH) = FTM_CnSC_ELSA | FTM_CnSC_MSA; // Output compare
        break;
      case FTM_PIN_HSPWM_OUT: {
        // HSPWM requires both channels, so incompatible with other FTM use cases
        uint32_t other_pin = pin ^ 0x1u;
        switch (ftm_pin_types[other_pin]) {
          case FTM_PIN_LSPWM_OUT:
          case FTM_PIN_SCHED_OUT:
          case FTM_PIN_FREQ_IN:
          case FTM_PIN_SYNC_IN:
          case FTM_PIN_TRIGGER_IN:
            // TODO report config error
            break;
          default:
            break;
        }

        uint32_t FTM_OTHER_CH = ftm_channel_from_pin(other_pin);
        *FTM_CnSC(FTM, FTM_CH) = FTM_CnSC_ELSB;
        *FTM_CnSC(FTM, FTM_OTHER_CH) = FTM_CnSC_ELSB;
        switch (FTM_CH) {
          case 0:
          case 1:
            *FTM_COMBINE(FTM) |= FTM_COMBINE_COMP0 | FTM_COMBINE_COMBINE0;
            break;
          case 2:
          case 3:
            *FTM_COMBINE(FTM) |= FTM_COMBINE_COMP1 | FTM_COMBINE_COMBINE1;
            break;
          case 4:
          case 5:
            *FTM_COMBINE(FTM) |= FTM_COMBINE_COMP2 | FTM_COMBINE_COMBINE2;
            break;
          case 6:
          case 7:
            *FTM_COMBINE(FTM) |= FTM_COMBINE_COMP3 | FTM_COMBINE_COMBINE3;
            break;
        }
        break;
                              }
      default:
        break;

    }
    *FTM_CnV(FTM, FTM_CH) = 0xffff;

  }

  for (int f = 0; f < 4; f++) {
    uint32_t FTM = FTM_BASE(f);
    *FTM_SC(FTM) = FTM_SC_PS(1) |    // prescale divide by 2 (40 MHz)
                   FTM_SC_SCS(1) |   // Use 80 MHz sys clock
                   FTM_SC_PWMEN(0xff);
  }
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
  //
  // Experiment with FTM3_CH2/3 for combined "fast" custom pwm on PTD2

  *TRGMUX_FTM3 = TRGMUX_SEL0(TRGMUX_SRC_LPIT_CH1); // LPIT1 triggers FTM0
  
  *FTM3_CnSC(0) = FTM_CnSC_ELSA | FTM_CnSC_MSA; // Output compare
  *FTM3_CnSC(1) = FTM_CnSC_ELSA | FTM_CnSC_MSA; // Output compare

  *FTM3_CnSC(4) = FTM_CnSC_ELSB;
  *FTM3_CnSC(5) = FTM_CnSC_ELSB;

  *FTM3_CNTIN = 0;
  *FTM3_MOD = 7999;

  *FTM3_CnV(0) = 0xffff;
  *FTM3_CnV(1) = 0xffff;

  *FTM3_CnV(4) = 0xffff;
  *FTM3_CnV(5) = 0xffff;

  *FTM3_CNT = 0;
  // Enable counter reset from trgmux input (LPIT CH 1, 5 KHz)
  *FTM3_MODE = FTM_MODE_WPDIS | FTM_MODE_FTMEN;
  *FTM3_SYNC = FTM_SYNC_TRIG0;
  *FTM3_SYNCONF = FTM_SYNCONF_HWRSTCNT | 
                  FTM_SYNCONF_SYNCMODE | 
                  FTM_SYNCONF_HWTRIGMODE | 
                  FTM_SYNCONF_HWINVC |
                  FTM_SYNCONF_INVC |
                  FTM_SYNCONF_HWWRBUF;

  *FTM3_COMBINE = FTM_COMBINE_SYNCNE0 | // CH0 and CH1 synchronized reg loads
                  FTM_COMBINE_SYNCNE2 | // CH4/5 combined CVn/n+1 load
                  FTM_COMBINE_COMP2 | // CH4/5 combined CVn/n+1 load
                  FTM_COMBINE_COMBINE2; // CH4/5 combined

  *FTM3_SC = FTM_SC_PS(1) |    // prescale divide by 2 (40 MHz)
             FTM_SC_SCS(1) |   // Use 80 MHz sys clock
             FTM_SC_PWMEN(0x3 | 0x30);  // Enable ch0/1 output ch4/5



}

#if NEVER_USED
// Just left as an example of setting up DMA

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
#endif

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
  *ADC0_SC1(7) = 0x7;

}

static _Atomic bool bump = false; 
void ADC0_IRQHandler(void) {
  *ADC0_R(7);
  bump = true;
}

// Our strategy should be:
// Configure PDB 0 CH0 for back-to-back within itself
// Configure PDB 0 CH1 for back to back within itself
// 
// Configure PDB0 Ch0 Trg 0 to be triggered by LPIT for ADC0 ch0-7
// Configure PDB0 Ch1 Trg 0 delay to be 50 uS (*1) for ADC0 8-15
// Configure PDB0 Interrupt delay to something like 100 uS (*2), enough to be sure CH1 is done
// For now, handle the interrupt. Later, trigger DMA, and have dma interrupt do sensor calcs
//
// Configure PDB1 CH0 for back to back within itself for trig 0-3
// Configure PDB1 CH1 to be triggered by a 50 kHz (or more) LPIT channel. for knock
// Trigger DMA. Have DMA transfer count set so that we only process in batches at 5 kHz
//
// (*2) total time, including finishing up of sensor calcs, should ideally be
// done in time for our reschedule. Depending on sample time, *1 could probably be more like 50 uS. Do some tests, give
// wiggle room, try to have everything done as close as can be done to the end to reduce time
// between samples and reschedule

static void setup_pdb(void) {
  // Configure PDB0 to back-to-back trigger CH0-CH7 as ADC inputs 8-15

  *PDB0_MOD = 0xffff;
  *PDB0_IDLY = 12000; // 150 uS at 80 MHz
  *PDB0_SC = PDB_SC_TRGSEL(0x0) | // TRGMUX input
             PDB_SC_PDBEN |
             PDB_SC_PDBIE |
             PDB_SC_LDOK;

  *PDB0_CH0C1 = PDB_CHnC1_BB(0xfe) | // Enable back to back trigger on last 7
                PDB_CHnC1_TOS(0)   |
                PDB_CHnC1_EN(0xff);

  *TRGMUX_PDB0 = TRGMUX_SEL0(TRGMUX_SRC_LPIT_CH1); // LPIT1 triggers PDB0
  nvic_enable_irq(52); // Replace with DMA
}

void PDB0_IRQHandler(void) {
  *PDB0_SC &= ~PDB_SC_PDBIF;
  struct adc_update update = {
    .time = current_lpit_base_time,
    .valid = true,
    .values = {
      (5 * *ADC0_R(0)) / 4096.0f,
      (5 * *ADC0_R(1)) / 4096.0f,
      (5 * *ADC0_R(2)) / 4096.0f,
      (5 * *ADC0_R(3)) / 4096.0f,
      (5 * *ADC0_R(4)) / 4096.0f,
      (5 * *ADC0_R(5)) / 4096.0f,
      (5 * *ADC0_R(6)) / 4096.0f,
      (5 * *ADC0_R(7)) / 4096.0f,
    },
  };

  _disable_interrupts();
  struct engine_position pos =
    decoder_get_engine_position(&s32k148_viaems.decoder);
  _enable_interrupts();
  sensor_update_adc(&s32k148_viaems.sensors, &pos, &update);
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

__attribute__((no_stack_protector))
void SystemInit(void) {
  *SCB_CPACR |= ((3UL << (10 * 2)) | (3UL << (11 * 2))); /* set CP10 and CP11 Full Access */
#if 1
  // copy text segment to SRAM_L
  volatile uint32_t *src, *dest;
  for (src = &_text_loadaddr, dest = &_stext; dest < &_etext; src++, dest++) {
    *dest = *src;
  }
#endif
}



int startup(void) {

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

#if HWTEST_PINS
  void mcxe246_hwtest_pinheaders(void);
  mcxe246_hwtest_pinheaders();
#endif


  configure_system_clocks();
  enable_peripheral_clocks();
  configure_pins();

  enable_cache();

  bool benchmark_enabled = false;
#ifdef BENCHMARK
  benchmark_enabled = true;
#endif

  viaems_init(&s32k148_viaems, &default_config);

  setup_uart();

  if (!benchmark_enabled) {
    //setup_can0();
    setup_lpit();
    //setup_ftm0();
//    setup_ftm3();
    configure_ftm();
    setup_adc();
    setup_pdb();
    start_lpit();

    void configure_spi(void);
    configure_spi();
  }

//  *((volatile uint32_t *)(0x4000D000)) &= ~0x1ULL;
//  *((volatile uint32_t *)(0x4000D800)) |= (6u << 18); // RW for ENET to all

                                                      // memory via alt
  void configure_spi(void);
  void send_spi(void);

#if BENCHMARK
  write_string("Startup complete!\r\n");
  int start_benchmarks(void);
  while (true) {
    start_benchmarks();
  }
#else

  set_test_trigger_rpm(1000);

  while (true) {
    viaems_idle(&s32k148_viaems, current_time());
    //send_spi();
  }
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
//

#define STACK_CHK_GUARD 0xe2dee396
uintptr_t  __attribute__((externally_visible)) __stack_chk_guard = STACK_CHK_GUARD;

__attribute__((noreturn)) __attribute__((externally_visible))
void __stack_chk_fail(void) {
  while(1);
}
