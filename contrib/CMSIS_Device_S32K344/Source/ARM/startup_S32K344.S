/*---------------------------------------------------------------------------
 * Name:    startup_S32K344.S
 * Purpose: S32K344 CMSIS Core Device Startup File
 * Rev.:    1.0.0
 *---------------------------------------------------------------------------*/
/*
 * Copyright (c) 2021 Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

       .syntax  unified
       .arch    armv7e-m

       #ifndef __INITIAL_SP
       #define __INITIAL_SP     Image$$ARM_LIB_STACK$$ZI$$Limit
       #endif

       .section RESET
       .align   2
       .globl   __Vectors
       .globl   __Vectors_End
       .globl   __Vectors_Size
__Vectors:
       .long    __INITIAL_SP                                   /*     Top of Stack */
       .long    Reset_Handler                                  /*     Reset Handler */
       .long    NMI_Handler                                    /* -14 NMI Handler */
       .long    HardFault_Handler                              /* -13 Hard Fault Handler */
       .long    MemManage_Handler                              /* -12 MPU Fault Handler */
       .long    BusFault_Handler                               /* -11 Bus Fault Handler */
       .long    UsageFault_Handler                             /* -10 Usage Fault Handler */
       .long    RESERVED_Handler                               /*     Reserved */
       .long    RESERVED_Handler                               /*     Reserved */
       .long    RESERVED_Handler                               /*     Reserved */
       .long    RESERVED_Handler                               /*     Reserved */
       .long    SVC_Handler                                    /*  -5 SVCall Handler */
       .long    DebugMon_Handler                               /*  -4 Debug Monitor Handler */
       .long    RESERVED_Handler                               /*     Reserved */
       .long    PendSV_Handler                                 /*  -2 PendSV Handler */
       .long    SysTick_Handler                                /*  -1 SysTick Handler */

       /* Interrupts */
       .long    INT0_Handler                                   /*   0 CPU to CPU int0 */
       .long    INT1_Handler                                   /*   1 CPU to CPU int1 */
       .long    INT2_Handler                                   /*   2 CPU to CPU int2 */
       .long    INT3_Handler                                   /*   3 CPU to CPU int3 */
       .long    DMATCD0_Handler                                /*   4 DMA transfer complete and error CH0 */
       .long    DMATCD1_Handler                                /*   5 DMA transfer complete and error CH1 */
       .long    DMATCD2_Handler                                /*   6 DMA transfer complete and error CH2 */
       .long    DMATCD3_Handler                                /*   7 DMA transfer complete and error CH3 */
       .long    DMATCD4_Handler                                /*   8 DMA transfer complete and error CH4 */
       .long    DMATCD5_Handler                                /*   9 DMA transfer complete and error CH5 */
       .long    DMATCD6_Handler                                /*  10 DMA transfer complete and error CH6 */
       .long    DMATCD7_Handler                                /*  11 DMA transfer complete and error CH7 */
       .long    DMATCD8_Handler                                /*  12 DMA transfer complete and error CH8 */
       .long    DMATCD9_Handler                                /*  13 DMA transfer complete and error CH9 */
       .long    DMATCD10_Handler                               /*  14 DMA transfer complete and error CH10 */
       .long    DMATCD11_Handler                               /*  15 DMA transfer complete and error CH11 */
       .long    DMATCD12_Handler                               /*  16 DMA transfer complete and error CH12 */
       .long    DMATCD13_Handler                               /*  17 DMA transfer complete and error CH13 */
       .long    DMATCD14_Handler                               /*  18 DMA transfer complete and error CH14 */
       .long    DMATCD15_Handler                               /*  19 DMA transfer complete and error CH15 */
       .long    DMATCD16_Handler                               /*  20 DMA transfer complete and error CH16 */
       .long    DMATCD17_Handler                               /*  21 DMA transfer complete and error CH17 */
       .long    DMATCD18_Handler                               /*  22 DMA transfer complete and error CH18 */
       .long    DMATCD19_Handler                               /*  23 DMA transfer complete and error CH19 */
       .long    DMATCD20_Handler                               /*  24 DMA transfer complete and error CH20 */
       .long    DMATCD21_Handler                               /*  25 DMA transfer complete and error CH21 */
       .long    DMATCD22_Handler                               /*  26 DMA transfer complete and error CH22 */
       .long    DMATCD23_Handler                               /*  27 DMA transfer complete and error CH23 */
       .long    DMATCD24_Handler                               /*  28 DMA transfer complete and error CH24 */
       .long    DMATCD25_Handler                               /*  29 DMA transfer complete and error CH25 */
       .long    DMATCD26_Handler                               /*  30 DMA transfer complete and error CH26 */
       .long    DMATCD27_Handler                               /*  31 DMA transfer complete and error CH27 */
       .long    DMATCD28_Handler                               /*  32 DMA transfer complete and error CH28 */
       .long    DMATCD29_Handler                               /*  33 DMA transfer complete and error CH29 */
       .long    DMATCD30_Handler                               /*  34 DMA transfer complete and error CH30 */
       .long    DMATCD31_Handler                               /*  35 DMA transfer complete and error CH31 */
       .long    ERM_0_Handler                                  /*  36 Single bit ECC error */
       .long    ERM_1_Handler                                  /*  37 Multi bit ECC error */
       .long    MCM_Handler                                    /*  38 Multi bit ECC error */
       .long    STM0_Handler                                   /*  39 Single interrupt vector for all four channels */
       .long    STM1_Handler                                   /*  40 Single interrupt vector for all four channels */
       .long    RESERVED_Handler                               /*  41 Reserved */
       .long    SWT0_Handler                                   /*  42 Platform watchdog initial time-out */
       .long    RESERVED_Handler                               /*  43 Reserved */
       .long    RESERVED_Handler                               /*  44 Reserved */
       .long    CTI0_Handler                                   /*  45 CTI Interrupt 0 */
       .long    RESERVED_Handler                               /*  46 Reserved */
       .long    RESERVED_Handler                               /*  47 Reserved */
       .long    FLASH_0_Handler                                /*  48 Program or erase operation is completed */
       .long    FLASH_1_Handler                                /*  49 Main watchdog timeout interrupt */
       .long    FLASH_2_Handler                                /*  50 Alternate watchdog timeout interrupt */
       .long    RGM_Handler                                    /*  51 Interrupt request to the system */
       .long    PMC_Handler                                    /*  52 One interrupt for all LVDs, One interrupt for all HVDs */
       .long    SIUL_0_Handler                                 /*  53 External Interrupt Vector 0 */
       .long    SIUL_1_Handler                                 /*  54 External Interrupt Vector 1 */
       .long    SIUL_2_Handler                                 /*  55 External Interrupt Vector 2 */
       .long    SIUL_3_Handler                                 /*  56 External Interrupt Vector 3 */
       .long    RESERVED_Handler                               /*  57 Reserved */
       .long    RESERVED_Handler                               /*  58 Reserved */
       .long    RESERVED_Handler                               /*  59 Reserved */
       .long    RESERVED_Handler                               /*  60 Reserved */
       .long    EMIOS0_0_Handler                               /*  61 Interrupt request 23,22,21,20 */
       .long    EMIOS0_1_Handler                               /*  62 Interrupt request 19,18,17,16 */
       .long    EMIOS0_2_Handler                               /*  63 Interrupt request 15,14,13,12 */
       .long    EMIOS0_3_Handler                               /*  64 Interrupt request 11,10,9,8 */
       .long    EMIOS0_4_Handler                               /*  65 Interrupt request 7,6,5,4 */
       .long    EMIOS0_5_Handler                               /*  66 Interrupt request 3,2,1,0 */
       .long    RESERVED_Handler                               /*  67 Reserved */
       .long    RESERVED_Handler                               /*  68 Reserved */
       .long    EMIOS1_0_Handler                               /*  69 Interrupt request 23,22,21,20 */
       .long    EMIOS1_1_Handler                               /*  70 Interrupt request 19,18,17,16 */
       .long    EMIOS1_2_Handler                               /*  71 Interrupt request 15,14,13,12 */
       .long    EMIOS1_3_Handler                               /*  72 Interrupt request 11,10,9,8 */
       .long    EMIOS1_4_Handler                               /*  73 Interrupt request 7,6,5,4 */
       .long    EMIOS1_5_Handler                               /*  74 Interrupt request 3,2,1,0 */
       .long    RESERVED_Handler                               /*  75 Reserved */
       .long    RESERVED_Handler                               /*  76 Reserved */
       .long    EMIOS2_0_Handler                               /*  77 Interrupt request 23,22,21,20 */
       .long    EMIOS2_1_Handler                               /*  78 Interrupt request 19,18,17,16 */
       .long    EMIOS2_2_Handler                               /*  79 Interrupt request 15,14,13,12 */
       .long    EMIOS2_3_Handler                               /*  80 Interrupt request 11,10,9,8 */
       .long    EMIOS2_4_Handler                               /*  81 Interrupt request 7,6,5,4 */
       .long    EMIOS2_5_Handler                               /*  82 Interrupt request 3,2,1,0 */
       .long    WKPU_Handler                                   /*  83 Interrupts from pad group 0,1,2,3, Interrupts from pad group 0_64, Interrupts from pad group 1_64, Interrupts from pad group 2_64, Interrupts from pad group 3_64 */
       .long    CMU0_Handler                                   /*  84 CMU0 interrupt */
       .long    CMU1_Handler                                   /*  85 CMU1 interrupt */
       .long    CMU2_Handler                                   /*  86 CMU2 interrupt */
       .long    BCTU_Handler                                   /*  87 An interrupt is requested when a conversion is issued to the ADC, An interrupt is requested when new data is available from ADC0 conversion, An interrupt is requested when new data is available from ADC1 conversion, An interrupt is requested when new data is available from ADC2 conversion, An interrupt is requested when the last command of a list is issued to the ADC,An Interrupt output for FIFO1,An Interrupt output for FIFO2 */
       .long    RESERVED_Handler                               /*  88 Reserved */
       .long    RESERVED_Handler                               /*  89 Reserved */
       .long    RESERVED_Handler                               /*  90 Reserved */
       .long    RESERVED_Handler                               /*  91 Reserved */
       .long    LCU0_Handler                                   /*  92 Interrupt 0, Interrupt 1 Interrupt 2 */
       .long    LCU1_Handler                                   /*  93 Interrupt 0, Interrupt 1 Interrupt 2 */
       .long    RESERVED_Handler                               /*  94 Reserved */
       .long    RESERVED_Handler                               /*  95 Reserved */
       .long    PIT0_Handler                                   /*  96 Interrupt for Channel0,Interrupt for Channel1,Interrupt for Channel2,Interrupt for Channel3,Interrupt for Channel4 */
       .long    PIT1_Handler                                   /*  97 Interrupt for Channel0,Interrupt for Channel1,Interrupt for Channel2,Interrupt for Channel3 */
       .long    PIT2_Handler                                   /*  98 Interrupt for Channel0,Interrupt for Channel1,Interrupt for Channel2,Interrupt for Channel3 */
       .long    RESERVED_Handler                               /*  99 Reserved */
       .long    RESERVED_Handler                               /* 100 Reserved */
       .long    RESERVED_Handler                               /* 101 Reserved */
       .long    RTC_Handler                                    /* 102 RTCF or ROVRF interrupt to be serviced by the system controller, APIF interrupt to be serviced by the system controller */
       .long    RESERVED_Handler                               /* 103 Reserved */
       .long    RESERVED_Handler                               /* 104 Reserved */
       .long    EMAC_0_Handler                                 /* 105 Common interrupt */
       .long    EMAC_1_Handler                                 /* 106 Tx interrupt 0, Tx interrupt 1 */
       .long    EMAC_2_Handler                                 /* 107 Rx interrupt 0, Rx interrupt 1 */
       .long    EMAC_3_Handler                                 /* 108 Safety interrupt correctable, Safety interrupt un-correctable */
       .long    FlexCAN0_0_Handler                             /* 109 Interrupt indicating that the CAN bus went to Bus Off state */
       .long    FlexCAN0_1_Handler                             /* 110 Message Buffer Interrupt line 0-31,ORed Interrupt for Message Buffers */
       .long    FlexCAN0_2_Handler                             /* 111 Message Buffer Interrupt line 32-63 */
       .long    FlexCAN0_3_Handler                             /* 112 Message Buffer Interrupt line 64-95 */
       .long    FlexCAN1_0_Handler                             /* 113 Interrupt indicating that the CAN bus went to Bus Off state */
       .long    FlexCAN1_1_Handler                             /* 114 Message Buffer Interrupt line 0-31 */
       .long    FlexCAN1_2_Handler                             /* 115 Message Buffer Interrupt line 32-63 */
       .long    FlexCAN2_0_Handler                             /* 116 Interrupt indicating that the CAN bus went to Bus Off state */
       .long    FlexCAN2_1_Handler                             /* 117 Message Buffer Interrupt line 0-31 */
       .long    FlexCAN2_2_Handler                             /* 118 Message Buffer Interrupt line 32-63 */
       .long    FlexCAN3_0_Handler                             /* 119 Interrupt indicating that the CAN bus went to Bus Off state */
       .long    FlexCAN3_1_Handler                             /* 120 Message Buffer Interrupt line 0-31 */
       .long    FlexCAN4_0_Handler                             /* 121 Interrupt indicating that the CAN bus went to Bus Off state */
       .long    FlexCAN4_1_Handler                             /* 122 Message Buffer Interrupt line 0-31 */
       .long    FlexCAN5_0_Handler                             /* 123 Interrupt indicating that the CAN bus went to Bus Off state */
       .long    FlexCAN5_1_Handler                             /* 124 Message Buffer Interrupt line 0-31 */
       .long    RESERVED_Handler                               /* 125 Reserved */
       .long    RESERVED_Handler                               /* 126 Reserved */
       .long    RESERVED_Handler                               /* 127 Reserved */
       .long    RESERVED_Handler                               /* 128 Reserved */
       .long    RESERVED_Handler                               /* 129 Reserved */
       .long    RESERVED_Handler                               /* 130 Reserved */
       .long    RESERVED_Handler                               /* 131 Reserved */
       .long    RESERVED_Handler                               /* 132 Reserved */
       .long    RESERVED_Handler                               /* 133 Reserved */
       .long    RESERVED_Handler                               /* 134 Reserved */
       .long    RESERVED_Handler                               /* 135 Reserved */
       .long    RESERVED_Handler                               /* 136 Reserved */
       .long    RESERVED_Handler                               /* 137 Reserved */
       .long    RESERVED_Handler                               /* 138 Reserved */
       .long    FLEXIO_Handler                                 /* 139 FlexIO Interrupt */
       .long    RESERVED_Handler                               /* 140 Reserved */
       .long    LPUART0_Handler                                /* 141 TX and RX interrupt */
       .long    LPUART1_Handler                                /* 142 TX and RX interrupt */
       .long    LPUART2_Handler                                /* 143 TX and RX interrupt */
       .long    LPUART3_Handler                                /* 144 TX and RX interrupt */
       .long    LPUART4_Handler                                /* 145 TX and RX interrupt */
       .long    LPUART5_Handler                                /* 146 TX and RX interrupt */
       .long    LPUART6_Handler                                /* 147 TX and RX interrupt */
       .long    LPUART7_Handler                                /* 148 TX and RX interrupt */
       .long    LPUART8_Handler                                /* 149 TX and RX interrupt */
       .long    LPUART9_Handler                                /* 150 TX and RX interrupt */
       .long    LPUART10_Handler                               /* 151 TX and RX interrupt */
       .long    LPUART11_Handler                               /* 152 TX and RX interrupt */
       .long    LPUART12_Handler                               /* 153 TX and RX interrupt */
       .long    LPUART13_Handler                               /* 154 TX and RX interrupt */
       .long    LPUART14_Handler                               /* 155 TX and RX interrupt */
       .long    LPUART15_Handler                               /* 156 TX and RX interrupt */
       .long    RESERVED_Handler                               /* 157 Reserved */
       .long    RESERVED_Handler                               /* 158 Reserved */
       .long    RESERVED_Handler                               /* 159 Reserved */
       .long    RESERVED_Handler                               /* 160 Reserved */
       .long    LPI2C0_Handler                                 /* 161 LPI2C Master Interrupt, LPI2C Interrupt */
       .long    LPI2C1_Handler                                 /* 162 LPI2C Master Interrupt, LPI2C Interrupt */
       .long    RESERVED_Handler                               /* 163 Reserved */
       .long    RESERVED_Handler                               /* 164 Reserved */
       .long    LPSPI0_Handler                                 /* 165 LPSPI Interrupt */
       .long    LPSPI1_Handler                                 /* 166 LPSPI Interrupt */
       .long    LPSPI2_Handler                                 /* 167 LPSPI Interrupt */
       .long    LPSPI3_Handler                                 /* 168 LPSPI Interrupt */
       .long    LPSPI4_Handler                                 /* 169 LPSPI Interrupt */
       .long    LPSPI5_Handler                                 /* 170 LPSPI Interrupt */
       .long    RESERVED_Handler                               /* 171 Reserved */
       .long    RESERVED_Handler                               /* 172 Reserved */
       .long    QSPI_Handler                                   /* 173 TX Buffer Fill interrupt, Transfer Complete / Transaction Finished, RX Buffer Drain interrupt, Buffer Overflow / Underrun interrupt, Serial Flash Communication Error interrupt, All interrupts ORed output */
       .long    SAI0_Handler                                   /* 174 RX interrupt,TX interrupt */
       .long    SAI1_Handler                                   /* 175 RX interrupt,TX interrupt */
       .long    RESERVED_Handler                               /* 176 Reserved */
       .long    RESERVED_Handler                               /* 177 Reserved */
       .long    JDC_Handler                                    /* 178 Indicates new data to be read from JIN_IPS register or can be written to JOUT_IPS register */
       .long    RESERVED_Handler                               /* 179 Reserved */
       .long    ADC0_Handler                                   /* 180 End of conversion, Error interrupt, Watchdog interrupt */
       .long    ADC1_Handler                                   /* 181 End of conversion, Error interrupt, Watchdog interrupt */
       .long    ADC2_Handler                                   /* 182 End of conversion, Error interrupt, Watchdog interrupt */
       .long    LPCMP0_Handler                                 /* 183 Async interrupt */
       .long    LPCMP1_Handler                                 /* 184 Async interrupt */
       .long    LPCMP2_Handler                                 /* 185 Async interrupt */
       .long    RESERVED_Handler                               /* 186 Reserved */
       .long    RESERVED_Handler                               /* 187 Reserved */
       .long    RESERVED_Handler                               /* 188 Reserved */
       .long    FCCU_0_Handler                                 /* 189 Interrupt request(ALARM state) */
       .long    FCCU_1_Handler                                 /* 190 Interrupt request(miscellaneous conditions) */
       .long    STCU_LBIST_MBIST_Handler                       /* 191 Interrupt request(miscellaneous conditions) */
       .long    HSE_MU0_TX_Handler                             /* 192 ORed TX interrupt to MU-0 */
       .long    HSE_MU0_RX_Handler                             /* 193 ORed RX interrupt to MU-0 */
       .long    HSE_MU0_ORED_Handler                           /* 194 ORed general purpose interrupt request to MU-0 */
       .long    HSE_MU1_TX_Handler                             /* 195 ORed TX interrupt to MU-1 */
       .long    HSE_MU1_RX_Handler                             /* 196 ORed RX interrupt to MU-1 */
       .long    HSE_MU1_ORED_Handler                           /* 197 ORed general purpose interrupt request to MU-1 */
       .long    RESERVED_Handler                               /* 198 Reserved */
       .long    RESERVED_Handler                               /* 199 Reserved */
       .long    RESERVED_Handler                               /* 200 Reserved */
       .long    RESERVED_Handler                               /* 201 Reserved */
       .long    RESERVED_Handler                               /* 202 Reserved */
       .long    RESERVED_Handler                               /* 203 Reserved */
       .long    RESERVED_Handler                               /* 204 Reserved */
       .long    RESERVED_Handler                               /* 205 Reserved */
       .long    RESERVED_Handler                               /* 206 Reserved */
       .long    RESERVED_Handler                               /* 207 Reserved */
       .long    RESERVED_Handler                               /* 208 Reserved */
       .long    RESERVED_Handler                               /* 209 Reserved */
       .long    RESERVED_Handler                               /* 210 Reserved */
       .long    RESERVED_Handler                               /* 211 Reserved */
       .long    SoC_PLL_Handler                                /* 212 PLL LOL interrupt */
__Vectors_End:
       .equ     __Vectors_Size, __Vectors_End - __Vectors
       .size    __Vectors, . - __Vectors


       .thumb
       .section ".text"
       .align   2

       .thumb_func
       .type    Reset_Handler, %function
       .globl   Reset_Handler
       .fnstart
       .cantunwind
Reset_Handler:

       MOV     R0,#0
       MSR     control, R0

#ifndef START_FROM_RAM
       CPSID   I               /* Mask interrupts */
       BL      DisableSWT0
       BL      InitECC
       CPSIE   I               /* Unmask interrupts */
#endif

       BL      SystemInit

       BL      __main

       .fnend
       .size    Reset_Handler, . - Reset_Handler


/* The default macro is not used for HardFault_Handler
 * because this results in a poor debug illusion.
 */
       .thumb_func
       .type   HardFault_Handler, %function
       .weak   HardFault_Handler
       .fnstart
       .cantunwind
HardFault_Handler:

       B       .

       .fnend
       .size   HardFault_Handler, . - HardFault_Handler


       .thumb_func
       .type   Default_Handler, %function
       .weak   Default_Handler
       .fnstart
       .cantunwind
Default_Handler:

       B       .

       .fnend
       .size   Default_Handler, . - Default_Handler


/* Macro to define default exception/interrupt handlers.
 * Default handler are weak symbols with an endless loop.
 * They can be overwritten by real handlers.
 */
       .macro  Set_Default_Handler  Handler_Name
       .weak   \Handler_Name
       .set    \Handler_Name, Default_Handler
       .endm


/* Default exception/interrupt handler */
       Set_Default_Handler  RESERVED_Handler

       Set_Default_Handler  NMI_Handler
       Set_Default_Handler  MemManage_Handler
       Set_Default_Handler  BusFault_Handler
       Set_Default_Handler  UsageFault_Handler
       Set_Default_Handler  SVC_Handler
       Set_Default_Handler  DebugMon_Handler
       Set_Default_Handler  PendSV_Handler
       Set_Default_Handler  SysTick_Handler

       Set_Default_Handler  INT0_Handler
       Set_Default_Handler  INT1_Handler
       Set_Default_Handler  INT2_Handler
       Set_Default_Handler  INT3_Handler
       Set_Default_Handler  DMATCD0_Handler
       Set_Default_Handler  DMATCD1_Handler
       Set_Default_Handler  DMATCD2_Handler
       Set_Default_Handler  DMATCD3_Handler
       Set_Default_Handler  DMATCD4_Handler
       Set_Default_Handler  DMATCD5_Handler
       Set_Default_Handler  DMATCD6_Handler
       Set_Default_Handler  DMATCD7_Handler
       Set_Default_Handler  DMATCD8_Handler
       Set_Default_Handler  DMATCD9_Handler
       Set_Default_Handler  DMATCD10_Handler
       Set_Default_Handler  DMATCD11_Handler
       Set_Default_Handler  DMATCD12_Handler
       Set_Default_Handler  DMATCD13_Handler
       Set_Default_Handler  DMATCD14_Handler
       Set_Default_Handler  DMATCD15_Handler
       Set_Default_Handler  DMATCD16_Handler
       Set_Default_Handler  DMATCD17_Handler
       Set_Default_Handler  DMATCD18_Handler
       Set_Default_Handler  DMATCD19_Handler
       Set_Default_Handler  DMATCD20_Handler
       Set_Default_Handler  DMATCD21_Handler
       Set_Default_Handler  DMATCD22_Handler
       Set_Default_Handler  DMATCD23_Handler
       Set_Default_Handler  DMATCD24_Handler
       Set_Default_Handler  DMATCD25_Handler
       Set_Default_Handler  DMATCD26_Handler
       Set_Default_Handler  DMATCD27_Handler
       Set_Default_Handler  DMATCD28_Handler
       Set_Default_Handler  DMATCD29_Handler
       Set_Default_Handler  DMATCD30_Handler
       Set_Default_Handler  DMATCD31_Handler
       Set_Default_Handler  ERM_0_Handler
       Set_Default_Handler  ERM_1_Handler
       Set_Default_Handler  MCM_Handler
       Set_Default_Handler  STM0_Handler
       Set_Default_Handler  STM1_Handler
       Set_Default_Handler  SWT0_Handler
       Set_Default_Handler  CTI0_Handler
       Set_Default_Handler  FLASH_0_Handler
       Set_Default_Handler  FLASH_1_Handler
       Set_Default_Handler  FLASH_2_Handler
       Set_Default_Handler  RGM_Handler
       Set_Default_Handler  PMC_Handler
       Set_Default_Handler  SIUL_0_Handler
       Set_Default_Handler  SIUL_1_Handler
       Set_Default_Handler  SIUL_2_Handler
       Set_Default_Handler  SIUL_3_Handler
       Set_Default_Handler  EMIOS0_0_Handler
       Set_Default_Handler  EMIOS0_1_Handler
       Set_Default_Handler  EMIOS0_2_Handler
       Set_Default_Handler  EMIOS0_3_Handler
       Set_Default_Handler  EMIOS0_4_Handler
       Set_Default_Handler  EMIOS0_5_Handler
       Set_Default_Handler  EMIOS1_0_Handler
       Set_Default_Handler  EMIOS1_1_Handler
       Set_Default_Handler  EMIOS1_2_Handler
       Set_Default_Handler  EMIOS1_3_Handler
       Set_Default_Handler  EMIOS1_4_Handler
       Set_Default_Handler  EMIOS1_5_Handler
       Set_Default_Handler  EMIOS2_0_Handler
       Set_Default_Handler  EMIOS2_1_Handler
       Set_Default_Handler  EMIOS2_2_Handler
       Set_Default_Handler  EMIOS2_3_Handler
       Set_Default_Handler  EMIOS2_4_Handler
       Set_Default_Handler  EMIOS2_5_Handler
       Set_Default_Handler  WKPU_Handler
       Set_Default_Handler  CMU0_Handler
       Set_Default_Handler  CMU1_Handler
       Set_Default_Handler  CMU2_Handler
       Set_Default_Handler  BCTU_Handler
       Set_Default_Handler  LCU0_Handler
       Set_Default_Handler  LCU1_Handler
       Set_Default_Handler  PIT0_Handler
       Set_Default_Handler  PIT1_Handler
       Set_Default_Handler  PIT2_Handler
       Set_Default_Handler  RTC_Handler
       Set_Default_Handler  EMAC_0_Handler
       Set_Default_Handler  EMAC_1_Handler
       Set_Default_Handler  EMAC_2_Handler
       Set_Default_Handler  EMAC_3_Handler
       Set_Default_Handler  FlexCAN0_0_Handler
       Set_Default_Handler  FlexCAN0_1_Handler
       Set_Default_Handler  FlexCAN0_2_Handler
       Set_Default_Handler  FlexCAN0_3_Handler
       Set_Default_Handler  FlexCAN1_0_Handler
       Set_Default_Handler  FlexCAN1_1_Handler
       Set_Default_Handler  FlexCAN1_2_Handler
       Set_Default_Handler  FlexCAN2_0_Handler
       Set_Default_Handler  FlexCAN2_1_Handler
       Set_Default_Handler  FlexCAN2_2_Handler
       Set_Default_Handler  FlexCAN3_0_Handler
       Set_Default_Handler  FlexCAN3_1_Handler
       Set_Default_Handler  FlexCAN4_0_Handler
       Set_Default_Handler  FlexCAN4_1_Handler
       Set_Default_Handler  FlexCAN5_0_Handler
       Set_Default_Handler  FlexCAN5_1_Handler
       Set_Default_Handler  FLEXIO_Handler
       Set_Default_Handler  LPUART0_Handler
       Set_Default_Handler  LPUART1_Handler
       Set_Default_Handler  LPUART2_Handler
       Set_Default_Handler  LPUART3_Handler
       Set_Default_Handler  LPUART4_Handler
       Set_Default_Handler  LPUART5_Handler
       Set_Default_Handler  LPUART6_Handler
       Set_Default_Handler  LPUART7_Handler
       Set_Default_Handler  LPUART8_Handler
       Set_Default_Handler  LPUART9_Handler
       Set_Default_Handler  LPUART10_Handler
       Set_Default_Handler  LPUART11_Handler
       Set_Default_Handler  LPUART12_Handler
       Set_Default_Handler  LPUART13_Handler
       Set_Default_Handler  LPUART14_Handler
       Set_Default_Handler  LPUART15_Handler
       Set_Default_Handler  LPI2C0_Handler
       Set_Default_Handler  LPI2C1_Handler
       Set_Default_Handler  LPSPI0_Handler
       Set_Default_Handler  LPSPI1_Handler
       Set_Default_Handler  LPSPI2_Handler
       Set_Default_Handler  LPSPI3_Handler
       Set_Default_Handler  LPSPI4_Handler
       Set_Default_Handler  LPSPI5_Handler
       Set_Default_Handler  QSPI_Handler
       Set_Default_Handler  SAI0_Handler
       Set_Default_Handler  SAI1_Handler
       Set_Default_Handler  JDC_Handler
       Set_Default_Handler  ADC0_Handler
       Set_Default_Handler  ADC1_Handler
       Set_Default_Handler  ADC2_Handler
       Set_Default_Handler  LPCMP0_Handler
       Set_Default_Handler  LPCMP1_Handler
       Set_Default_Handler  LPCMP2_Handler
       Set_Default_Handler  FCCU_0_Handler
       Set_Default_Handler  FCCU_1_Handler
       Set_Default_Handler  STCU_LBIST_MBIST_Handler
       Set_Default_Handler  HSE_MU0_TX_Handler
       Set_Default_Handler  HSE_MU0_RX_Handler
       Set_Default_Handler  HSE_MU0_ORED_Handler
       Set_Default_Handler  HSE_MU1_TX_Handler
       Set_Default_Handler  HSE_MU1_RX_Handler
       Set_Default_Handler  HSE_MU1_ORED_Handler
       Set_Default_Handler  SoC_PLL_Handler

       .end
