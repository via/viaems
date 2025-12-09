#ifndef S32K1XX_H
#define S32K1XX_H

#include <stdint.h>

#include "cortex-m4.h"

#define WDOG_BASE                0x40052000u

#define WDOG_CS                  MEM32(WDOG_BASE + 0x0u)
#define WDOG_CNT                 MEM32(WDOG_BASE + 0x4u)
#define WDOG_TOVAL               MEM32(WDOG_BASE + 0x8u)
#define WDOG_WIN                 MEM32(WDOG_BASE + 0xCu)

#define WDOG_CS_EN               (1u << 7)

#define GPIOA_BASE          0x400FF000u
#define GPIOB_BASE          0x400FF040u
#define GPIOC_BASE          0x400FF080u
#define GPIOD_BASE          0x400FF0C0u
#define GPIOE_BASE          0x400FF100u

#define GPIOC_PDOR          MEM32(GPIOC_BASE + 0x0u)
#define GPIOC_PSOR          MEM32(GPIOC_BASE + 0x4u)
#define GPIOC_PCOR          MEM32(GPIOC_BASE + 0x8u)
#define GPIOC_PDDR          MEM32(GPIOC_BASE + 0x14u)

#define GPIOE_PDOR          MEM32(GPIOE_BASE + 0x0u)
#define GPIOE_PSOR          MEM32(GPIOE_BASE + 0x4u)
#define GPIOE_PCOR          MEM32(GPIOE_BASE + 0x8u)
#define GPIOE_PDDR          MEM32(GPIOE_BASE + 0x14u)

#define PCC_BASE           0x40065000u
#define PCC_DMAMUX         MEM32(PCC_BASE + 0x84u)
#define PCC_FTM3           MEM32(PCC_BASE + 0x98u)
#define PCC_ADC1           MEM32(PCC_BASE + 0x9Cu)
#define PCC_PDB1           MEM32(PCC_BASE + 0xC4u)
#define PCC_FLEXCAN2       MEM32(PCC_BASE + 0xACu)
#define PCC_PDB0           MEM32(PCC_BASE + 0xD8u)
#define PCC_LPIT           MEM32(PCC_BASE + 0xDCu)
#define PCC_FTM0           MEM32(PCC_BASE + 0xE0u)
#define PCC_FTM1           MEM32(PCC_BASE + 0xE4u)
#define PCC_FTM2           MEM32(PCC_BASE + 0xE8u)
#define PCC_ADC0           MEM32(PCC_BASE + 0xECu)
#define PCC_PORTA          MEM32(PCC_BASE + 0x124u)
#define PCC_PORTB          MEM32(PCC_BASE + 0x128u)
#define PCC_PORTC          MEM32(PCC_BASE + 0x12Cu)
#define PCC_PORTD          MEM32(PCC_BASE + 0x130u)
#define PCC_PORTE          MEM32(PCC_BASE + 0x134u)
#define PCC_FLEXIO         MEM32(PCC_BASE + 0x168u)
#define PCC_LPUART0        MEM32(PCC_BASE + 0x1A8u)
#define PCC_LPUART1        MEM32(PCC_BASE + 0x1ACu)
#define PCC_LPUART2        MEM32(PCC_BASE + 0x1B0u)
#define PCC_ENET           MEM32(PCC_BASE + 0x1E4u)

#define PCC_CGC            (1u << 30)
#define PCC_PCS(X)         ((uint32_t)(X) << 24)
#define PCC_FRAC           (1u << 3)
#define PCC_PCD(X)         ((uint32_t)(X) << 0)

#define PORTA_BASE    0x40049000u
#define PORTA_PCRn(x) MEM32(PORTA_BASE + 4u * (x))

#define PORTB_BASE    0x4004A000u
#define PORTB_PCRn(x) MEM32(PORTB_BASE + 4u * (x))

#define PORTC_BASE    0x4004B000u
#define PORTC_PCRn(x) MEM32(PORTC_BASE + 4u * (x))

#define PORTD_BASE    0x4004C000u
#define PORTD_PCRn(x) MEM32(PORTD_BASE + 4u * (x))

#define PORTE_BASE    0x4004D000u
#define PORTE_PCRn(x) MEM32(PORTE_BASE + 4u * (x))

#define PORT_PCRn_LK     (1u << 15)
#define PORT_PCRn_DSE    (1u << 6)
#define PORT_PCRn_MUX(X) ((uint32_t)(X) << 8)


#define SCG_BASE          0x40064000u
#define SCG_VERID         MEM32(SCG_BASE + 0x0u)
#define SCG_PARAM         MEM32(SCG_BASE + 0x4u)
#define SCG_CSR           MEM32(SCG_BASE + 0x10u)
#define SCG_RCCR          MEM32(SCG_BASE + 0x14u)
#define SCG_VCCR          MEM32(SCG_BASE + 0x18u)
#define SCG_HCCR          MEM32(SCG_BASE + 0x1Cu)
#define SCG_CLKOUTCNFG    MEM32(SCG_BASE + 0x20u)

#define SCG_SOSCCSR       MEM32(SCG_BASE + 0x100u)
#define SCG_SOSCDIV       MEM32(SCG_BASE + 0x104u)
#define SCG_SOSCCFG       MEM32(SCG_BASE + 0x108u) 

#define SCG_SIRCCSR       MEM32(SCG_BASE + 0x200u)   
#define SCG_SIRCDIV       MEM32(SCG_BASE + 0x204u)   
#define SCG_SIRCCFG       MEM32(SCG_BASE + 0x208u)   

#define SCG_FIRCCSR       MEM32(SCG_BASE + 0x300u)   
#define SCG_FIRCDIV       MEM32(SCG_BASE + 0x304u)   
#define SCG_FIRCCFG       MEM32(SCG_BASE + 0x308u)   

#define SCG_SPLLCSR       MEM32(SCG_BASE + 0x600u)   
#define SCG_SPLLDIV       MEM32(SCG_BASE + 0x604u)   
#define SCG_SPLLCFG       MEM32(SCG_BASE + 0x608u)   

#define SCG_SOSCCFG_RANGE(X) ((uint32_t)(X) << 4)
#define SCG_SOSCCFG_EREFS    (1u << 2)

#define SCG_SOSCCSR_SOSCVLD  (1u << 24)
#define SCG_SOSCCSR_SOSCEN   (1u << 0)

#define SCG_SPLLCFG_MULT(X)     ((uint32_t)(X) << 16)
#define SCG_SPLLCFG_PREDIV(X)   ((uint32_t)(X) << 8)
#define SCG_SPLLCFG_SOURCE      (1u << 0)

#define SCG_SPLLCSR_SPLLVLD     (1u << 24)
#define SCG_SPLLCSR_SPLLEN      (1u << 0)

#define SCG_SPLLDIV_SPLLDIV2(X) ((uint32_t)(X) << 8)
#define SCG_SPLLDIV_SPLLDIV1(X) ((uint32_t)(X) << 0)

#define SCG_RCCR_SCS(X)         ((uint32_t)(X) << 24)
#define SCG_RCCR_DIVCORE(X)     ((uint32_t)(X) << 16)
#define SCG_RCCR_DIVBUS(X)      ((uint32_t)(X) << 4)
#define SCG_RCCR_DIVSLOW(X)     ((uint32_t)(X) << 0)

#define SCG_CSR_SCS(X)          ((uint32_t)(X) << 24)


#define LMEM_BASE          0xE0082000u
#define LMEM_PCCCR         MEM32(LMEM_BASE + 0)

#define LMEM_PCCCR_GO      (1u << 31)
#define LMEM_PCCCR_PUSHW1  (1u << 27)
#define LMEM_PCCCR_INVW1   (1u << 26)
#define LMEM_PCCCR_PUSHW0  (1u << 25)
#define LMEM_PCCCR_INVW0   (1u << 24)
#define LMEM_PCCCR_ENCACHE (1u << 0)


#define LPUART0_BASE      0x4006A000u
#define LPUART0_BAUD      MEM32(LPUART0_BASE + 0x10u)
#define LPUART0_STAT      MEM32(LPUART0_BASE + 0x14u)
#define LPUART0_CTRL      MEM32(LPUART0_BASE + 0x18u)
#define LPUART0_DATA      MEM32(LPUART0_BASE + 0x1Cu)

#define LPUART2_BASE      0x4006C000u
#define LPUART2_BAUD      MEM32(LPUART2_BASE + 0x10u)
#define LPUART2_STAT      MEM32(LPUART2_BASE + 0x14u)
#define LPUART2_CTRL      MEM32(LPUART2_BASE + 0x18u)
#define LPUART2_DATA      MEM32(LPUART2_BASE + 0x1Cu)

#define LPUART_BAUD_OSR(X) ((uint32_t)(X) << 24)
#define LPUART_BAUD_SBR(X) ((uint32_t)(X) << 0)
#define LPUART_CTRL_TE     (1u << 19)
#define LPUART_STAT_TRDE   (1u << 23)

#define FLEXCAN2_BASE         0x4002B000u
#define FLEXCAN2_MCR          MEM32(FLEXCAN2_BASE + 0x0u)
#define FLEXCAN2_CTRL1        MEM32(FLEXCAN2_BASE + 0x4u)
#define FLEXCAN2_TIMER        MEM32(FLEXCAN2_BASE + 0x8u)
#define FLEXCAN2_RXMGMASK     MEM32(FLEXCAN2_BASE + 0x10u)
#define FLEXCAN2_RX14MASK     MEM32(FLEXCAN2_BASE + 0x14u)
#define FLEXCAN2_RX15MASK     MEM32(FLEXCAN2_BASE + 0x18u)
#define FLEXCAN2_ECR          MEM32(FLEXCAN2_BASE + 0x1Cu)
#define FLEXCAN2_ESR1         MEM32(FLEXCAN2_BASE + 0x20u)
#define FLEXCAN2_IMASK1       MEM32(FLEXCAN2_BASE + 0x28u)
#define FLEXCAN2_IFLAG1       MEM32(FLEXCAN2_BASE + 0x30u)
#define FLEXCAN2_CTRL2        MEM32(FLEXCAN2_BASE + 0x34u)
#define FLEXCAN2_ESR2         MEM32(FLEXCAN2_BASE + 0x38u)
#define FLEXCAN2_CRCR         MEM32(FLEXCAN2_BASE + 0x44u)
#define FLEXCAN2_RXFGMASK     MEM32(FLEXCAN2_BASE + 0x48u)
#define FLEXCAN2_RXFIR        MEM32(FLEXCAN2_BASE + 0x4Cu)
#define FLEXCAN2_CBT          MEM32(FLEXCAN2_BASE + 0x50u)

#define FLEXCAN2_MEM          MEM32(FLEXCAN2_BASE + 0x80)

#define FLEXCAN2_FDCTRL       MEM32(FLEXCAN2_BASE + 0xC00u)
#define FLEXCAN2_FDCBT        MEM32(FLEXCAN2_BASE + 0xC04u)
#define FLEXCAN2_FDCRC        MEM32(FLEXCAN2_BASE + 0xC08u)

#define FLEXIO_BASE              0x4005A000u
#define FLEXIO_VERID             MEM32(FLEXIO_BASE + 0x0)
#define FLEXIO_PARAM             MEM32(FLEXIO_BASE + 0x4)
#define FLEXIO_CTRL              MEM32(FLEXIO_BASE + 0x8)
#define FLEXIO_PIN               MEM32(FLEXIO_BASE + 0xC)
#define FLEXIO_SHIFTSTAT         MEM32(FLEXIO_BASE + 0x10)
#define FLEXIO_SHIFTERR          MEM32(FLEXIO_BASE + 0x14)
#define FLEXIO_TIMSTAT           MEM32(FLEXIO_BASE + 0x18)
#define FLEXIO_SHIFTSIEN         MEM32(FLEXIO_BASE + 0x20)
#define FLEXIO_SHIFTEIEN         MEM32(FLEXIO_BASE + 0x24)
#define FLEXIO_TIMIEN            MEM32(FLEXIO_BASE + 0x28)
#define FLEXIO_SHIFTSDEN         MEM32(FLEXIO_BASE + 0x30)
#define FLEXIO_SHIFTCTL(x)       MEM32(FLEXIO_BASE + 0x80 + 4u * (x))
#define FLEXIO_SHIFTCFG(x)       MEM32(FLEXIO_BASE + 0x100 + 4u * (x))
#define FLEXIO_SHIFTBUF(x)       MEM32(FLEXIO_BASE + 0x200 + 4u * (x))
#define FLEXIO_SHIFTBUFBIS(x)    MEM32(FLEXIO_BASE + 0x280 + 4u * (x))
#define FLEXIO_SHIFTBUFBYS(x)    MEM32(FLEXIO_BASE + 0x300 + 4u * (x))
#define FLEXIO_SHIFTBUFBBS(x)    MEM32(FLEXIO_BASE + 0x380 + 4u * (x))
#define FLEXIO_TIMCTL(x)         MEM32(FLEXIO_BASE + 0x400 + 4u * (x))
#define FLEXIO_TIMCFG(x)         MEM32(FLEXIO_BASE + 0x480 + 4u * (x))
#define FLEXIO_TIMCMP(x)         MEM32(FLEXIO_BASE + 0x500 + 4u * (x))


#define LPIT_BASE                0x40037000u
#define LPIT_VERID               MEM32(LPIT_BASE + 0x0)
#define LPIT_PARAM               MEM32(LPIT_BASE + 0x4)
#define LPIT_MCR                 MEM32(LPIT_BASE + 0x8)
#define LPIT_MSR                 MEM32(LPIT_BASE + 0xC)
#define LPIT_MIER                MEM32(LPIT_BASE + 0x10)
#define LPIT_SETTEN              MEM32(LPIT_BASE + 0x14)
#define LPIT_CLRTEN              MEM32(LPIT_BASE + 0x18)
#define LPIT_TVAL0               MEM32(LPIT_BASE + 0x20)
#define LPIT_CVAL0               MEM32(LPIT_BASE + 0x24)
#define LPIT_TCTRL0              MEM32(LPIT_BASE + 0x28)
#define LPIT_TVAL1               MEM32(LPIT_BASE + 0x30)
#define LPIT_CVAL1               MEM32(LPIT_BASE + 0x34)
#define LPIT_TCTRL1              MEM32(LPIT_BASE + 0x38)
#define LPIT_TVAL2               MEM32(LPIT_BASE + 0x40)
#define LPIT_CVAL2               MEM32(LPIT_BASE + 0x44)
#define LPIT_TCTRL2              MEM32(LPIT_BASE + 0x48)
#define LPIT_TVAL3               MEM32(LPIT_BASE + 0x50)
#define LPIT_CVAL3               MEM32(LPIT_BASE + 0x54)
#define LPIT_TCTRL3              MEM32(LPIT_BASE + 0x58)

#define TRGMUX_BASE              0x40063000u
#define TRGMUX_DMAMUX0           MEM32(TRGMUX_BASE + 0x0)
#define TRGMUX_EXTOUT0           MEM32(TRGMUX_BASE + 0x4)
#define TRGMUX_FTM0              MEM32(TRGMUX_BASE + 0x28)
#define TRGMUX_FTM1              MEM32(TRGMUX_BASE + 0x2C)
#define TRGMUX_FTM2              MEM32(TRGMUX_BASE + 0x30)
#define TRGMUX_FTM3              MEM32(TRGMUX_BASE + 0x34)

#define TRGMUX_SEL0(X)           ((uint32_t)(X) << 0)
#define TRGMUX_SEL1(X)           ((uint32_t)(X) << 8)
#define TRGMUX_SEL2(X)           ((uint32_t)(X) << 16)
#define TRGMUX_SEL3(X)           ((uint32_t)(X) << 26)

#define TRGMUX_SRC_LPIT_CH0      0x11u
#define TRGMUX_SRC_LPIT_CH1      0x12u
#define TRGMUX_SRC_FTM0          0x16u

#define FTM0_BASE                0x40038000u
#define FTM1_BASE                0x40039000u
#define FTM2_BASE                0x4003A000u
#define FTM3_BASE                0x40026000u

#define FTM0_SC                  MEM32(FTM0_BASE + 0x0)
#define FTM0_CNT                 MEM32(FTM0_BASE + 0x4)
#define FTM0_MOD                 MEM32(FTM0_BASE + 0x8)
#define FTM0_CnSC(N)             MEM32(FTM0_BASE + 0xC + 8 * (N))
#define FTM0_CnV(N)              MEM32(FTM0_BASE + 0x10 + 8 * (N))
#define FTM0_CNTIN               MEM32(FTM0_BASE + 0x4C)
#define FTM0_STATUS              MEM32(FTM0_BASE + 0x50)
#define FTM0_MODE                MEM32(FTM0_BASE + 0x54)
#define FTM0_SYNC                MEM32(FTM0_BASE + 0x58)
#define FTM0_OUTINIT             MEM32(FTM0_BASE + 0x5C)
#define FTM0_OUTMASK             MEM32(FTM0_BASE + 0x60)
#define FTM0_COMBINE             MEM32(FTM0_BASE + 0x64)
#define FTM0_DEADTIME            MEM32(FTM0_BASE + 0x68)
#define FTM0_EXTTRIG             MEM32(FTM0_BASE + 0x6C)
#define FTM0_POL                 MEM32(FTM0_BASE + 0x70)
#define FTM0_FMS                 MEM32(FTM0_BASE + 0x74)
#define FTM0_FILTER              MEM32(FTM0_BASE + 0x78)
#define FTM0_FRTCRTL             MEM32(FTM0_BASE + 0x7C)
#define FTM0_QDCTRL              MEM32(FTM0_BASE + 0x80)
#define FTM0_CONF                MEM32(FTM0_BASE + 0x84)
#define FTM0_FLTPOL              MEM32(FTM0_BASE + 0x88)
#define FTM0_SYNCONF             MEM32(FTM0_BASE + 0x8C)
#define FTM0_INVCTRL             MEM32(FTM0_BASE + 0x90)
#define FTM0_SWOCTRL             MEM32(FTM0_BASE + 0x94)
#define FTM0_PWMLOAD             MEM32(FTM0_BASE + 0x98)
#define FTM0_HCR                 MEM32(FTM0_BASE + 0x9C)
#define FTM0_PAIR0DEADTIME       MEM32(FTM0_BASE + 0xA0)
#define FTM0_PAIR1DEADTIME       MEM32(FTM0_BASE + 0xA8)
#define FTM0_PAIR2DEADTIME       MEM32(FTM0_BASE + 0xB0)
#define FTM0_PAIR3DEADTIME       MEM32(FTM0_BASE + 0xB8)
#define FTM0_MOD_MIRROR          MEM32(FTM0_BASE + 0x200)
#define FTM0_CXV_MIRROR(X)       MEM32(FTM0_BASE + 0x204 + 4 * X))

#define FTM3_SC                  MEM32(FTM3_BASE + 0x0)
#define FTM3_CNT                 MEM32(FTM3_BASE + 0x4)
#define FTM3_MOD                 MEM32(FTM3_BASE + 0x8)
#define FTM3_CnSC(N)             MEM32(FTM3_BASE + 0xC + 8 * (N))
#define FTM3_CnV(N)              MEM32(FTM3_BASE + 0x10 + 8 * (N))
#define FTM3_CNTIN               MEM32(FTM3_BASE + 0x4C)
#define FTM3_STATUS              MEM32(FTM3_BASE + 0x50)
#define FTM3_MODE                MEM32(FTM3_BASE + 0x54)
#define FTM3_SYNC                MEM32(FTM3_BASE + 0x58)
#define FTM3_OUTINIT             MEM32(FTM3_BASE + 0x5C)
#define FTM3_OUTMASK             MEM32(FTM3_BASE + 0x60)
#define FTM3_COMBINE             MEM32(FTM3_BASE + 0x64)
#define FTM3_DEADTIME            MEM32(FTM3_BASE + 0x68)
#define FTM3_EXTTRIG             MEM32(FTM3_BASE + 0x6C)
#define FTM3_POL                 MEM32(FTM3_BASE + 0x70)
#define FTM3_FMS                 MEM32(FTM3_BASE + 0x74)
#define FTM3_FILTER              MEM32(FTM3_BASE + 0x78)
#define FTM3_FRTCRTL             MEM32(FTM3_BASE + 0x7C)
#define FTM3_QDCTRL              MEM32(FTM3_BASE + 0x80)
#define FTM3_CONF                MEM32(FTM3_BASE + 0x84)
#define FTM3_FLTPOL              MEM32(FTM3_BASE + 0x88)
#define FTM3_SYNCONF             MEM32(FTM3_BASE + 0x8C)
#define FTM3_INVCTRL             MEM32(FTM3_BASE + 0x90)
#define FTM3_SWOCTRL             MEM32(FTM3_BASE + 0x94)
#define FTM3_PWMLOAD             MEM32(FTM3_BASE + 0x98)
#define FTM3_HCR                 MEM32(FTM3_BASE + 0x9C)
#define FTM3_PAIR0DEADTIME       MEM32(FTM3_BASE + 0xA0)
#define FTM3_PAIR1DEADTIME       MEM32(FTM3_BASE + 0xA8)
#define FTM3_PAIR2DEADTIME       MEM32(FTM3_BASE + 0xB0)
#define FTM3_PAIR3DEADTIME       MEM32(FTM3_BASE + 0xB8)
#define FTM3_MOD_MIRROR          MEM32(FTM3_BASE + 0x200)
#define FTM3_CXV_MIRROR(X)       MEM32(FTM3_BASE + 0x204 + 4 * X))

#define FTM_MODE_WPDIS           (1u << 2)
#define FTM_MODE_FTMEN           (1u << 0)
#define FTM_COMBINE_SYNCNE0      (1u << 5)
#define FTM_SYNC_TRIG0           (1u << 4)
#define FTM_SYNCONF_HWRSTCNT     (1u << 16)
#define FTM_SYNCONF_SYNCMODE     (1u << 7)
#define FTM_SYNCONF_HWWRBUF      (1u << 17)
#define FTM_SYNCONF_HWTRIGMODE   (1u << 0)
#define FTM_SC_SCS(X)            ((uint32_t)(X) << 3)
#define FTM_SC_PS(X)             ((uint32_t)(X) << 0)
#define FTM_SC_PWMEN(X)          ((uint32_t)(X) << 16)
#define FTM_CnSC_ELSA            (1u << 2)
#define FTM_CnSC_MSA             (1u << 4)
#define FTM_CnSC_CHIE            (1u << 6)
#define FTM_EXTTRIG_INITTRIGEN   (1u << 6)

#define DMA_BASE                     0x40008000u
#define DMA_CR                       MEM32(DMA_BASE + 0x0)
#define DMA_ES                       MEM32(DMA_BASE + 0x4)
#define DMA_ERQ                      MEM32(DMA_BASE + 0xC)
#define DMA_EEI                      MEM32(DMA_BASE + 0x14)
#define DMA_REEI                     MEM8(DMA_BASE + 0x18)
#define DMA_SEEI                     MEM8(DMA_BASE + 0x19)
#define DMA_CERQ                     MEM8(DMA_BASE + 0x1A)
#define DMA_SERQ                     MEM8(DMA_BASE + 0x1B)
#define DMA_CDNE                     MEM8(DMA_BASE + 0x1C)
#define DMA_SSRT                     MEM8(DMA_BASE + 0x1D)
#define DMA_CERR                     MEM8(DMA_BASE + 0x1E)
#define DMA_CINT                     MEM8(DMA_BASE + 0x1F)
#define DMA_INT                      MEM32(DMA_BASE + 0x24)
#define DMA_ERR                      MEM32(DMA_BASE + 0x2C)
#define DMA_HRS                      MEM32(DMA_BASE + 0x34)
#define DMA_EARS                     MEM32(DMA_BASE + 0x44)
#define DMA_DCHPRI3                  MEM8(DMA_BASE + 0x100)
#define DMA_DCHPRI2                  MEM8(DMA_BASE + 0x101)
#define DMA_DCHPRI1                  MEM8(DMA_BASE + 0x102)
#define DMA_DCHPRI0                  MEM8(DMA_BASE + 0x103)
#define DMA_DCHPRI7                  MEM8(DMA_BASE + 0x104)
#define DMA_DCHPRI6                  MEM8(DMA_BASE + 0x105)
#define DMA_DCHPRI5                  MEM8(DMA_BASE + 0x106)
#define DMA_DCHPRI4                  MEM8(DMA_BASE + 0x107)
#define DMA_DCHPRI11                 MEM8(DMA_BASE + 0x108)
#define DMA_DCHPRI10                 MEM8(DMA_BASE + 0x109)
#define DMA_DCHPRI9                  MEM8(DMA_BASE + 0x10A)
#define DMA_DCHPRI8                  MEM8(DMA_BASE + 0x10B)
#define DMA_DCHPRI15                 MEM8(DMA_BASE + 0x10C)
#define DMA_DCHPRI14                 MEM8(DMA_BASE + 0x10D)
#define DMA_DCHPRI13                 MEM8(DMA_BASE + 0x10E)
#define DMA_DCHPRI12                 MEM8(DMA_BASE + 0x10F)

#define DMA_TCD_SADDR(X)             MEM32(DMA_BASE + 0x1000 + 32 * (X))
#define DMA_TCD_SOFF(X)              MEM16(DMA_BASE + 0x1004 + 32 * (X))
#define DMA_TCD_ATTR(X)              MEM16(DMA_BASE + 0x1006 + 32 * (X))
#define DMA_TCD_NBYTES_MLNO(X)       MEM32(DMA_BASE + 0x1008 + 32 * (X))
#define DMA_TCD_NBYTES_MLOFFNO(X)    MEM32(DMA_BASE + 0x1008 + 32 * (X))
#define DMA_TCD_NBYTES_MLOFFYES(X)   MEM32(DMA_BASE + 0x1008 + 32 * (X))
#define DMA_TCD_SLAST(X)             MEM32(DMA_BASE + 0x100C + 32 * (X))
#define DMA_TCD_DADDR(X)             MEM32(DMA_BASE + 0x1010 + 32 * (X))
#define DMA_TCD_DOFF(X)              MEM16(DMA_BASE + 0x1014 + 32 * (X))
#define DMA_TCD_CITER_ELINKNO(X)     MEM16(DMA_BASE + 0x1016 + 32 * (X))
#define DMA_TCD_CITER_ELINKYES(X)    MEM16(DMA_BASE + 0x1016 + 32 * (X))
#define DMA_TCD_DLASTSGA(X)          MEM32(DMA_BASE + 0x1018 + 32 * (X))
#define DMA_TCD_CSR(X)               MEM16(DMA_BASE + 0x101C + 32 * (X))
#define DMA_TCD_BITER_ELINKNO(X)     MEM16(DMA_BASE + 0x101E + 32 * (X))
#define DMA_TCD_BITER_ELINKYES(X)    MEM16(DMA_BASE + 0x101E + 32 * (X))

#define DMA_CR_EMLM                  (1u << 7)
#define DMA_CR_ERCA                  (1u << 2)
#define DMA_TCD_ATTR_DSIZE(X)        ((uint16_t)(X) << 0)
#define DMA_TCD_ATTR_SSIZE(X)        ((uint16_t)(X) << 8)
#define DMA_TCD_NBYTES_SMLOE         (1u << 31)
#define DMA_TCD_NBYTES_DMLOE         (1u << 30)
#define DMA_TCD_NBYTES_MLOFF(X)      (((uint32_t)(X) & 0xFFFFF) << 10)
#define DMA_TCD_NBYTES_NBYTES(X)     ((uint32_t)(X) << 0)


#define DMAMUX_BASE             0x40021000u
#define DMAMUX_CHCFG(X)         MEM8(DMAMUX_BASE + (X))

#define DMAMUX_CHCFG_ENBL       (1u << 7)
#define DMAMUX_CHCFG_TRIG       (1u << 6)
#define DMAMUX_CHCFG_SOURCE(X)  ((uint8_t)(X) << 0)

#define ADC0_BASE                0x4003B000u
#define ADC1_BASE                0x40027000u

#define ADC0_SC1(X)              MEM32(ADC0_BASE + 4 * (X))
#define ADC0_CFG1                MEM32(ADC0_BASE + 0x40)
#define ADC0_CFG2                MEM32(ADC0_BASE + 0x44)
#define ADC0_R(X)                MEM32(ADC0_BASE + 0x48 + 4 * (X))
#define ADC0_CV1                 MEM32(ADC0_BASE + 0x88)
#define ADC0_CV2                 MEM32(ADC0_BASE + 0x8C)
#define ADC0_SC2                 MEM32(ADC0_BASE + 0x90)
#define ADC0_SC3                 MEM32(ADC0_BASE + 0x94)
#define ADC0_BASE_OFS            MEM32(ADC0_BASE + 0x98)
#define ADC0_OFS                 MEM32(ADC0_BASE + 0x9C)
#define ADC0_USR_OFS             MEM32(ADC0_BASE + 0xA0)
#define ADC0_XOFS                MEM32(ADC0_BASE + 0xA4)
#define ADC0_YOFS                MEM32(ADC0_BASE + 0xA8)
#define ADC0_G                   MEM32(ADC0_BASE + 0xAC)
#define ADC0_UG                  MEM32(ADC0_BASE + 0xB0)
#define ADC0_CLPS                MEM32(ADC0_BASE + 0xB4)
#define ADC0_CLP3                MEM32(ADC0_BASE + 0xB8)
#define ADC0_CLP2                MEM32(ADC0_BASE + 0xBC)
#define ADC0_CLP1                MEM32(ADC0_BASE + 0xC0)
#define ADC0_CLP0                MEM32(ADC0_BASE + 0xC4)
#define ADC0_CLPX                MEM32(ADC0_BASE + 0xC8)
#define ADC0_CLP9                MEM32(ADC0_BASE + 0xCC)
#define ADC0_CLPS_OFS            MEM32(ADC0_BASE + 0xD0)
#define ADC0_CLP3_OFS            MEM32(ADC0_BASE + 0xD4)
#define ADC0_CLP2_OFS            MEM32(ADC0_BASE + 0xD8)
#define ADC0_CLP1_OFS            MEM32(ADC0_BASE + 0xDC)
#define ADC0_CLP0_OFS            MEM32(ADC0_BASE + 0xE0)
#define ADC0_CLPX_OFS            MEM32(ADC0_BASE + 0xE4)
#define ADC0_CLP9_OFS            MEM32(ADC0_BASE + 0xE8)

#define ADC_SC1_COCO             (1u << 7)
#define ADC_SC1_AIEN             (1u << 6)
#define ADC_SC1_ADCH(X)          ((uint32_t)(X) << 0)
#define ADC_CFG1_ADIV(X)         ((uint32_t)(X) << 5)
#define ADC_CFG1_MODE(X)         ((uint32_t)(X) << 2)
#define ADC_CFG1_ADICLK(X)       ((uint32_t)(X) << 0)
#define ADC_CFG2_SMPLTS(X)       ((uint32_t)(X) << 0)
#define ADC_SC2_ADTRG            (1u << 6)
#define ADC_SC3_CAL              (1u << 7)
#define ADC_SC3_AVGE             (1u << 2)
#define ADC_SC3_AVGS(X)          ((uint32_t)(X) << 0)

#define PDB0_BASE                0x40036000u
#define PDB0_SC                  MEM32(PDB0_BASE + 0x0)
#define PDB0_MOD                 MEM32(PDB0_BASE + 0x4)
#define PDB0_CNT                 MEM32(PDB0_BASE + 0x8)
#define PDB0_IDLY                MEM32(PDB0_BASE + 0xC)
#define PDB0_CH0C1               MEM32(PDB0_BASE + 0x10)
#define PDB0_CH0S                MEM32(PDB0_BASE + 0x14)
#define PDB0_CH0DLY(X)           MEM32(PDB0_BASE + 0x18 + 4 * (X))
#define PDB0_CH1C1               MEM32(PDB0_BASE + 0x38)
#define PDB0_CH1S                MEM32(PDB0_BASE + 0x3C)
#define PDB0_CH1DLY(X)           MEM32(PDB0_BASE + 0x40 + 4 * (X))

#define PDB_SC_SWTRIG            (1u << 16)
#define PDB_SC_TRGSEL(X)         ((uint32_t)(X) << 8)
#define PDB_SC_PDBEN             (1u << 7)
#define PDB_SC_PDBIE             (1u << 5)
#define PDB_SC_LDOK              (1u << 0)
#define PDB_CHnC1_BB(X)          ((uint32_t)(X) << 16)
#define PDB_CHnC1_TOS(X)         ((uint32_t)(X) << 8)
#define PDB_CHnC1_EN(X)          ((uint32_t)(X) << 0)


#define SIM_BASE                 0x40048000u
#define SIM_CHIPCTL              MEM32(SIM_BASE + 4)

#define SIM_CHIPCTL_ADC_INTERLEAVE_EN(X)  ((uint32_t)(X) << 0)

#define ENET_BASE                0x40079000u
#define ENET_EIR                 MEM32(ENET_BASE + 0x4)
#define ENET_EIMR                MEM32(ENET_BASE + 0x8)
#define ENET_RDAR                MEM32(ENET_BASE + 0x10)
#define ENET_TDAR                MEM32(ENET_BASE + 0x14)
#define ENET_ECR                 MEM32(ENET_BASE + 0x24)
#define ENET_MMFR                MEM32(ENET_BASE + 0x40)
#define ENET_MSCR                MEM32(ENET_BASE + 0x44)
#define ENET_MIBC                MEM32(ENET_BASE + 0x64)
#define ENET_RCR                 MEM32(ENET_BASE + 0x84)
#define ENET_TCR                 MEM32(ENET_BASE + 0xC4)
#define ENET_PALR                MEM32(ENET_BASE + 0xE4)
#define ENET_PAUR                MEM32(ENET_BASE + 0xE8)
#define ENET_OPD                 MEM32(ENET_BASE + 0xEC)

#define ENET_IAUR                MEM32(ENET_BASE + 0x118)
#define ENET_IALR                MEM32(ENET_BASE + 0x11C)
#define ENET_GAUR                MEM32(ENET_BASE + 0x120)
#define ENET_GALR                MEM32(ENET_BASE + 0x124)
#define ENET_TFWR                MEM32(ENET_BASE + 0x144)
#define ENET_RDSR                MEM32(ENET_BASE + 0x180)
#define ENET_TDSR                MEM32(ENET_BASE + 0x184)
#define ENET_MRBR                MEM32(ENET_BASE + 0x188)
#define ENET_RSFL                MEM32(ENET_BASE + 0x190)
#define ENET_RSEM                MEM32(ENET_BASE + 0x194)
#define ENET_RAEM                MEM32(ENET_BASE + 0x198)
#define ENET_RAFL                MEM32(ENET_BASE + 0x19C)
#define ENET_TSEM                MEM32(ENET_BASE + 0x1A0)
#define ENET_TAEM                MEM32(ENET_BASE + 0x1A4)
#define ENET_TAFL                MEM32(ENET_BASE + 0x1A8)
#define ENET_TIPG                MEM32(ENET_BASE + 0x1AC)
#define ENET_FTRL                MEM32(ENET_BASE + 0x1B0)
#define ENET_TACC                MEM32(ENET_BASE + 0x1C0)
#define ENET_RACC                MEM32(ENET_BASE + 0x1C4)

#endif

