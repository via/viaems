  .syntax unified
  .cpu cortex-m4
  .thumb


  .section  .text.startup,"ax",%progbits
  .global  Default_Handler
  .thumb_func
Default_Handler:
Infinite_Loop:
   b Infinite_Loop

  .global Reset_Handler
  .thumb_func
Reset_Handler:
    ldr r1, =_sram_l_start
    ldr r2, =_sram_u_end

    subs    r2, r1
    subs    r2, #1
    ble .zero_end

    movs    r0, 0
    movs    r3, #4

.zero_ram:
    str r0, [r1]
    add	r1, r1, r3
    subs r2, 4
    bge .zero_ram

.zero_end:
  bl SystemInit
  bl startup
  b Infinite_Loop

  .section  .isr_vector,"a",%progbits
cortex_m_interrupts:
  .word  _stack_end
  .word  Reset_Handler

  .word  NMI_Handler
  .word  HardFault_Handler
  .word  MemManage_Handler
  .word  BusFault_Handler
  .word  UsageFault_Handler
  .word  0
  .word  0
  .word  0
  .word  0
  .word  SVC_Handler
  .word  DebugMon_Handler
  .word  0
  .word  PendSV_Handler
  .word  SysTick_Handler

  .weak      NMI_Handler
  .thumb_set NMI_Handler,Default_Handler

  .weak      HardFault_Handler
  .thumb_set HardFault_Handler,Default_Handler

  .weak      MemManage_Handler
  .thumb_set MemManage_Handler,Default_Handler

  .weak      BusFault_Handler
  .thumb_set BusFault_Handler,Default_Handler

  .weak      UsageFault_Handler
  .thumb_set UsageFault_Handler,Default_Handler

  .weak      SVC_Handler
  .thumb_set SVC_Handler,Default_Handler

  .weak      DebugMon_Handler
  .thumb_set DebugMon_Handler,Default_Handler

  .weak      PendSV_Handler
  .thumb_set PendSV_Handler,Default_Handler

  .weak      SysTick_Handler
  .thumb_set SysTick_Handler,Default_Handler              

s32_interrupts:
  .word   DMA0_IRQHandler                       /* DMA channel 0 transfer complete*/
  .word   DMA1_IRQHandler                       /* DMA channel 1 transfer complete*/
  .word   DMA2_IRQHandler                       /* DMA channel 2 transfer complete*/
  .word   DMA3_IRQHandler                       /* DMA channel 3 transfer complete*/
  .word   DMA4_IRQHandler                       /* DMA channel 4 transfer complete*/
  .word   DMA5_IRQHandler                       /* DMA channel 5 transfer complete*/
  .word   DMA6_IRQHandler                       /* DMA channel 6 transfer complete*/
  .word   DMA7_IRQHandler                       /* DMA channel 7 transfer complete*/
  .word   DMA8_IRQHandler                       /* DMA channel 8 transfer complete*/
  .word   DMA9_IRQHandler                       /* DMA channel 9 transfer complete*/
  .word   DMA10_IRQHandler                      /* DMA channel 10 transfer complete*/
  .word   DMA11_IRQHandler                      /* DMA channel 11 transfer complete*/
  .word   DMA12_IRQHandler                      /* DMA channel 12 transfer complete*/
  .word   DMA13_IRQHandler                      /* DMA channel 13 transfer complete*/
  .word   DMA14_IRQHandler                      /* DMA channel 14 transfer complete*/
  .word   DMA15_IRQHandler                      /* DMA channel 15 transfer complete*/
  .word   DMA_Error_IRQHandler                  /* DMA error interrupt channels 0-15*/
  .word   MCM_IRQHandler                        /* FPU sources*/
  .word   FTFC_IRQHandler                       /* FTFC Command complete*/
  .word   Read_Collision_IRQHandler             /* FTFC Read collision*/
  .word   LVD_LVW_IRQHandler                    /* PMC Low voltage detect interrupt*/
  .word   FTFC_Fault_IRQHandler                 /* FTFC Double bit fault detect*/
  .word   WDOG_EWM_IRQHandler                   /* Single interrupt vector for WDOG and EWM*/
  .word   RCM_IRQHandler                        /* RCM Asynchronous Interrupt*/
  .word   LPI2C0_Master_IRQHandler              /* LPI2C0 Master Interrupt*/
  .word   LPI2C0_Slave_IRQHandler               /* LPI2C0 Slave Interrupt*/
  .word   LPSPI0_IRQHandler                     /* LPSPI0 Interrupt*/
  .word   LPSPI1_IRQHandler                     /* LPSPI1 Interrupt*/
  .word   LPSPI2_IRQHandler                     /* LPSPI2 Interrupt*/
  .word   Reserved45_IRQHandler                 /* Reserved Interrupt 45*/
  .word   Reserved46_IRQHandler                 /* Reserved Interrupt 46*/
  .word   LPUART0_RxTx_IRQHandler               /* LPUART0 Transmit / Receive Interrupt*/
  .word   Reserved48_IRQHandler                 /* Reserved Interrupt 48*/
  .word   LPUART1_RxTx_IRQHandler               /* LPUART1 Transmit / Receive  Interrupt*/
  .word   Reserved50_IRQHandler                 /* Reserved Interrupt 50*/
  .word   LPUART2_RxTx_IRQHandler               /* LPUART2 Transmit / Receive  Interrupt*/
  .word   Reserved52_IRQHandler                 /* Reserved Interrupt 52*/
  .word   Reserved53_IRQHandler                 /* Reserved Interrupt 53*/
  .word   Reserved54_IRQHandler                 /* Reserved Interrupt 54*/
  .word   ADC0_IRQHandler                       /* ADC0 interrupt request.*/
  .word   ADC1_IRQHandler                       /* ADC1 interrupt request.*/
  .word   CMP0_IRQHandler                       /* CMP0 interrupt request*/
  .word   Reserved58_IRQHandler                 /* Reserved Interrupt 58*/
  .word   Reserved59_IRQHandler                 /* Reserved Interrupt 59*/
  .word   ERM_single_fault_IRQHandler           /* ERM single bit error correction*/
  .word   ERM_double_fault_IRQHandler           /* ERM double bit error non-correctable*/
  .word   RTC_IRQHandler                        /* RTC alarm interrupt*/
  .word   RTC_Seconds_IRQHandler                /* RTC seconds interrupt*/
  .word   LPIT0_Ch0_IRQHandler                  /* LPIT0 channel 0 overflow interrupt*/
  .word   LPIT0_Ch1_IRQHandler                  /* LPIT0 channel 1 overflow interrupt*/
  .word   LPIT0_Ch2_IRQHandler                  /* LPIT0 channel 2 overflow interrupt*/
  .word   LPIT0_Ch3_IRQHandler                  /* LPIT0 channel 3 overflow interrupt*/
  .word   PDB0_IRQHandler                       /* PDB0 interrupt*/
  .word   Reserved69_IRQHandler                 /* Reserved Interrupt 69*/
  .word   Reserved70_IRQHandler                 /* Reserved Interrupt 70*/
  .word   Reserved71_IRQHandler                 /* Reserved Interrupt 71*/
  .word   Reserved72_IRQHandler                 /* Reserved Interrupt 72*/
  .word   SCG_IRQHandler                        /* SCG bus interrupt request*/
  .word   LPTMR0_IRQHandler                     /* LPTIMER interrupt request*/
  .word   PORTA_IRQHandler                      /* Port A pin detect interrupt*/
  .word   PORTB_IRQHandler                      /* Port B pin detect interrupt*/
  .word   PORTC_IRQHandler                      /* Port C pin detect interrupt*/
  .word   PORTD_IRQHandler                      /* Port D pin detect interrupt*/
  .word   PORTE_IRQHandler                      /* Port E pin detect interrupt*/
  .word   SWI_IRQHandler                        /* Software interrupt*/
  .word   Reserved81_IRQHandler                 /* Reserved Interrupt 81*/
  .word   Reserved82_IRQHandler                 /* Reserved Interrupt 82*/
  .word   Reserved83_IRQHandler                 /* Reserved Interrupt 83*/
  .word   PDB1_IRQHandler                       /* PDB1 interrupt*/
  .word   FLEXIO_IRQHandler                     /* FlexIO Interrupt*/
  .word   Reserved86_IRQHandler                 /* Reserved Interrupt 86*/
  .word   Reserved87_IRQHandler                 /* Reserved Interrupt 87*/
  .word   Reserved88_IRQHandler                 /* Reserved Interrupt 88*/
  .word   Reserved89_IRQHandler                 /* Reserved Interrupt 89*/
  .word   Reserved90_IRQHandler                 /* Reserved Interrupt 90*/
  .word   Reserved91_IRQHandler                 /* Reserved Interrupt 91*/
  .word   Reserved92_IRQHandler                 /* Reserved Interrupt 92*/
  .word   Reserved93_IRQHandler                 /* Reserved Interrupt 93*/
  .word   CAN0_ORed_IRQHandler                  /* CAN0 OR'ed [Bus Off OR Transmit Warning OR Receive Warning]*/
  .word   CAN0_Error_IRQHandler                 /* CAN0 Interrupt indicating that errors were detected on the CAN bus*/
  .word   CAN0_Wake_Up_IRQHandler               /* CAN0 Interrupt asserted when Pretended Networking operation is enabled, and a valid message matches the selected filter criteria during Low Power mode*/
  .word   CAN0_ORed_0_15_MB_IRQHandler          /* CAN0 OR'ed Message buffer (0-15)*/
  .word   CAN0_ORed_16_31_MB_IRQHandler         /* CAN0 OR'ed Message buffer (16-31)*/
  .word   Reserved99_IRQHandler                 /* Reserved Interrupt 99*/
  .word   Reserved100_IRQHandler                /* Reserved Interrupt 100*/
  .word   CAN1_ORed_IRQHandler                  /* CAN1 OR'ed [Bus Off OR Transmit Warning OR Receive Warning]*/
  .word   CAN1_Error_IRQHandler                 /* CAN1 Interrupt indicating that errors were detected on the CAN bus*/
  .word   Reserved103_IRQHandler                /* Reserved Interrupt 103*/
  .word   CAN1_ORed_0_15_MB_IRQHandler          /* CAN1 OR'ed Interrupt for Message buffer (0-15)*/
  .word   Reserved105_IRQHandler                /* Reserved Interrupt 105*/
  .word   Reserved106_IRQHandler                /* Reserved Interrupt 106*/
  .word   Reserved107_IRQHandler                /* Reserved Interrupt 107*/
  .word   CAN2_ORed_IRQHandler                  /* CAN2 OR'ed [Bus Off OR Transmit Warning OR Receive Warning]*/
  .word   CAN2_Error_IRQHandler                 /* CAN2 Interrupt indicating that errors were detected on the CAN bus*/
  .word   Reserved110_IRQHandler                /* Reserved Interrupt 110*/
  .word   CAN2_ORed_0_15_MB_IRQHandler          /* CAN2 OR'ed Message buffer (0-15)*/
  .word   Reserved112_IRQHandler                /* Reserved Interrupt 112*/
  .word   Reserved113_IRQHandler                /* Reserved Interrupt 113*/
  .word   Reserved114_IRQHandler                /* Reserved Interrupt 114*/
  .word   FTM0_Ch0_Ch1_IRQHandler               /* FTM0 Channel 0 and 1 interrupt*/
  .word   FTM0_Ch2_Ch3_IRQHandler               /* FTM0 Channel 2 and 3 interrupt*/
  .word   FTM0_Ch4_Ch5_IRQHandler               /* FTM0 Channel 4 and 5 interrupt*/
  .word   FTM0_Ch6_Ch7_IRQHandler               /* FTM0 Channel 6 and 7 interrupt*/
  .word   FTM0_Fault_IRQHandler                 /* FTM0 Fault interrupt*/
  .word   FTM0_Ovf_Reload_IRQHandler            /* FTM0 Counter overflow and Reload interrupt*/
  .word   FTM1_Ch0_Ch1_IRQHandler               /* FTM1 Channel 0 and 1 interrupt*/
  .word   FTM1_Ch2_Ch3_IRQHandler               /* FTM1 Channel 2 and 3 interrupt*/
  .word   FTM1_Ch4_Ch5_IRQHandler               /* FTM1 Channel 4 and 5 interrupt*/
  .word   FTM1_Ch6_Ch7_IRQHandler               /* FTM1 Channel 6 and 7 interrupt*/
  .word   FTM1_Fault_IRQHandler                 /* FTM1 Fault interrupt*/
  .word   FTM1_Ovf_Reload_IRQHandler            /* FTM1 Counter overflow and Reload interrupt*/
  .word   FTM2_Ch0_Ch1_IRQHandler               /* FTM2 Channel 0 and 1 interrupt*/
  .word   FTM2_Ch2_Ch3_IRQHandler               /* FTM2 Channel 2 and 3 interrupt*/
  .word   FTM2_Ch4_Ch5_IRQHandler               /* FTM2 Channel 4 and 5 interrupt*/
  .word   FTM2_Ch6_Ch7_IRQHandler               /* FTM2 Channel 6 and 7 interrupt*/
  .word   FTM2_Fault_IRQHandler                 /* FTM2 Fault interrupt*/
  .word   FTM2_Ovf_Reload_IRQHandler            /* FTM2 Counter overflow and Reload interrupt*/
  .word   FTM3_Ch0_Ch1_IRQHandler               /* FTM3 Channel 0 and 1 interrupt*/
  .word   FTM3_Ch2_Ch3_IRQHandler               /* FTM3 Channel 2 and 3 interrupt*/
  .word   FTM3_Ch4_Ch5_IRQHandler               /* FTM3 Channel 4 and 5 interrupt*/
  .word   FTM3_Ch6_Ch7_IRQHandler               /* FTM3 Channel 6 and 7 interrupt*/
  .word   FTM3_Fault_IRQHandler                 /* FTM3 Fault interrupt*/
  .word   FTM3_Ovf_Reload_IRQHandler            /* FTM3 Counter overflow and Reload interrupt*/

  .weak      DMA0_IRQHandler
  .thumb_set DMA0_IRQHandler,Default_Handler

  .weak      DMA1_IRQHandler
  .thumb_set DMA1_IRQHandler,Default_Handler

  .weak      DMA2_IRQHandler
  .thumb_set DMA2_IRQHandler,Default_Handler

  .weak      DMA3_IRQHandler
  .thumb_set DMA3_IRQHandler,Default_Handler

  .weak      DMA4_IRQHandler
  .thumb_set DMA4_IRQHandler,Default_Handler

  .weak      DMA5_IRQHandler
  .thumb_set DMA5_IRQHandler,Default_Handler

  .weak      DMA6_IRQHandler
  .thumb_set DMA6_IRQHandler,Default_Handler

  .weak      DMA7_IRQHandler
  .thumb_set DMA7_IRQHandler,Default_Handler

  .weak      DMA8_IRQHandler
  .thumb_set DMA8_IRQHandler,Default_Handler

  .weak      DMA9_IRQHandler
  .thumb_set DMA9_IRQHandler,Default_Handler

  .weak      DMA10_IRQHandler
  .thumb_set DMA10_IRQHandler,Default_Handler

  .weak      DMA11_IRQHandler
  .thumb_set DMA11_IRQHandler,Default_Handler

  .weak      DMA12_IRQHandler
  .thumb_set DMA12_IRQHandler,Default_Handler

  .weak      DMA13_IRQHandler
  .thumb_set DMA13_IRQHandler,Default_Handler

  .weak      DMA14_IRQHandler
  .thumb_set DMA14_IRQHandler,Default_Handler

  .weak      DMA15_IRQHandler
  .thumb_set DMA15_IRQHandler,Default_Handler

  .weak      DMA_Error_IRQHandler
  .thumb_set DMA_Error_IRQHandler,Default_Handler

  .weak      MCM_IRQHandler
  .thumb_set MCM_IRQHandler,Default_Handler

  .weak      FTFC_IRQHandler
  .thumb_set FTFC_IRQHandler,Default_Handler

  .weak      Read_Collision_IRQHandler
  .thumb_set Read_Collision_IRQHandler,Default_Handler

  .weak      LVD_LVW_IRQHandler
  .thumb_set LVD_LVW_IRQHandler,Default_Handler

  .weak      FTFC_Fault_IRQHandler
  .thumb_set FTFC_Fault_IRQHandler,Default_Handler

  .weak      WDOG_EWM_IRQHandler
  .thumb_set WDOG_EWM_IRQHandler,Default_Handler

  .weak      RCM_IRQHandler
  .thumb_set RCM_IRQHandler,Default_Handler

  .weak      LPI2C0_Master_IRQHandler
  .thumb_set LPI2C0_Master_IRQHandler,Default_Handler

  .weak      LPI2C0_Slave_IRQHandler
  .thumb_set LPI2C0_Slave_IRQHandler,Default_Handler

  .weak      LPSPI0_IRQHandler
  .thumb_set LPSPI0_IRQHandler,Default_Handler

  .weak      LPSPI1_IRQHandler
  .thumb_set LPSPI1_IRQHandler,Default_Handler

  .weak      LPSPI2_IRQHandler
  .thumb_set LPSPI2_IRQHandler,Default_Handler

  .weak      Reserved45_IRQHandler
  .thumb_set Reserved45_IRQHandler,Default_Handler

  .weak      Reserved46_IRQHandler
  .thumb_set Reserved46_IRQHandler,Default_Handler

  .weak      LPUART0_RxTx_IRQHandler
  .thumb_set LPUART0_RxTx_IRQHandler,Default_Handler

  .weak      Reserved48_IRQHandler
  .thumb_set Reserved48_IRQHandler,Default_Handler

  .weak      LPUART1_RxTx_IRQHandler
  .thumb_set LPUART1_RxTx_IRQHandler,Default_Handler

  .weak      Reserved50_IRQHandler
  .thumb_set Reserved50_IRQHandler,Default_Handler

  .weak      LPUART2_RxTx_IRQHandler
  .thumb_set LPUART2_RxTx_IRQHandler,Default_Handler

  .weak      Reserved52_IRQHandler
  .thumb_set Reserved52_IRQHandler,Default_Handler

  .weak      Reserved53_IRQHandler
  .thumb_set Reserved53_IRQHandler,Default_Handler

  .weak      Reserved54_IRQHandler
  .thumb_set Reserved54_IRQHandler,Default_Handler

  .weak      ADC0_IRQHandler
  .thumb_set ADC0_IRQHandler,Default_Handler

  .weak      ADC1_IRQHandler
  .thumb_set ADC1_IRQHandler,Default_Handler

  .weak      CMP0_IRQHandler
  .thumb_set CMP0_IRQHandler,Default_Handler

  .weak      Reserved58_IRQHandler
  .thumb_set Reserved58_IRQHandler,Default_Handler

  .weak      Reserved59_IRQHandler
  .thumb_set Reserved59_IRQHandler,Default_Handler

  .weak      ERM_single_fault_IRQHandler
  .thumb_set ERM_single_fault_IRQHandler,Default_Handler

  .weak      ERM_double_fault_IRQHandler
  .thumb_set ERM_double_fault_IRQHandler,Default_Handler

  .weak      RTC_IRQHandler
  .thumb_set RTC_IRQHandler,Default_Handler

  .weak      RTC_Seconds_IRQHandler
  .thumb_set RTC_Seconds_IRQHandler,Default_Handler

  .weak      LPIT0_Ch0_IRQHandler
  .thumb_set LPIT0_Ch0_IRQHandler,Default_Handler

  .weak      LPIT0_Ch1_IRQHandler
  .thumb_set LPIT0_Ch1_IRQHandler,Default_Handler

  .weak      LPIT0_Ch2_IRQHandler
  .thumb_set LPIT0_Ch2_IRQHandler,Default_Handler

  .weak      LPIT0_Ch3_IRQHandler
  .thumb_set LPIT0_Ch3_IRQHandler,Default_Handler

  .weak      PDB0_IRQHandler
  .thumb_set PDB0_IRQHandler,Default_Handler

  .weak      Reserved69_IRQHandler
  .thumb_set Reserved69_IRQHandler,Default_Handler

  .weak      Reserved70_IRQHandler
  .thumb_set Reserved70_IRQHandler,Default_Handler

  .weak      Reserved71_IRQHandler
  .thumb_set Reserved71_IRQHandler,Default_Handler

  .weak      Reserved72_IRQHandler
  .thumb_set Reserved72_IRQHandler,Default_Handler

  .weak      SCG_IRQHandler
  .thumb_set SCG_IRQHandler,Default_Handler

  .weak      LPTMR0_IRQHandler
  .thumb_set LPTMR0_IRQHandler,Default_Handler

  .weak      PORTA_IRQHandler
  .thumb_set PORTA_IRQHandler,Default_Handler

  .weak      PORTB_IRQHandler
  .thumb_set PORTB_IRQHandler,Default_Handler

  .weak      PORTC_IRQHandler
  .thumb_set PORTC_IRQHandler,Default_Handler

  .weak      PORTD_IRQHandler
  .thumb_set PORTD_IRQHandler,Default_Handler

  .weak      PORTE_IRQHandler
  .thumb_set PORTE_IRQHandler,Default_Handler

  .weak      SWI_IRQHandler
  .thumb_set SWI_IRQHandler,Default_Handler

  .weak      Reserved81_IRQHandler
  .thumb_set Reserved81_IRQHandler,Default_Handler

  .weak      Reserved82_IRQHandler
  .thumb_set Reserved82_IRQHandler,Default_Handler

  .weak      Reserved83_IRQHandler
  .thumb_set Reserved83_IRQHandler,Default_Handler

  .weak      PDB1_IRQHandler
  .thumb_set PDB1_IRQHandler,Default_Handler

  .weak      FLEXIO_IRQHandler
  .thumb_set FLEXIO_IRQHandler,Default_Handler

  .weak      Reserved86_IRQHandler
  .thumb_set Reserved86_IRQHandler,Default_Handler

  .weak      Reserved87_IRQHandler
  .thumb_set Reserved87_IRQHandler,Default_Handler

  .weak      Reserved88_IRQHandler
  .thumb_set Reserved88_IRQHandler,Default_Handler

  .weak      Reserved89_IRQHandler
  .thumb_set Reserved89_IRQHandler,Default_Handler

  .weak      Reserved90_IRQHandler
  .thumb_set Reserved90_IRQHandler,Default_Handler

  .weak      Reserved91_IRQHandler
  .thumb_set Reserved91_IRQHandler,Default_Handler

  .weak      Reserved92_IRQHandler
  .thumb_set Reserved92_IRQHandler,Default_Handler

  .weak      Reserved93_IRQHandler
  .thumb_set Reserved93_IRQHandler,Default_Handler

  .weak      CAN0_ORed_IRQHandler
  .thumb_set CAN0_ORed_IRQHandler,Default_Handler

  .weak      CAN0_Error_IRQHandler
  .thumb_set CAN0_Error_IRQHandler,Default_Handler

  .weak      CAN0_Wake_Up_IRQHandler
  .thumb_set CAN0_Wake_Up_IRQHandler,Default_Handler

  .weak      CAN0_ORed_0_15_MB_IRQHandler
  .thumb_set CAN0_ORed_0_15_MB_IRQHandler,Default_Handler

  .weak      CAN0_ORed_16_31_MB_IRQHandler
  .thumb_set CAN0_ORed_16_31_MB_IRQHandler,Default_Handler

  .weak      Reserved99_IRQHandler
  .thumb_set Reserved99_IRQHandler,Default_Handler

  .weak      Reserved100_IRQHandler
  .thumb_set Reserved100_IRQHandler,Default_Handler

  .weak      CAN1_ORed_IRQHandler
  .thumb_set CAN1_ORed_IRQHandler,Default_Handler

  .weak      CAN1_Error_IRQHandler
  .thumb_set CAN1_Error_IRQHandler,Default_Handler

  .weak      Reserved103_IRQHandler
  .thumb_set Reserved103_IRQHandler,Default_Handler

  .weak      CAN1_ORed_0_15_MB_IRQHandler
  .thumb_set CAN1_ORed_0_15_MB_IRQHandler,Default_Handler

  .weak      Reserved105_IRQHandler
  .thumb_set Reserved105_IRQHandler,Default_Handler

  .weak      Reserved106_IRQHandler
  .thumb_set Reserved106_IRQHandler,Default_Handler

  .weak      Reserved107_IRQHandler
  .thumb_set Reserved107_IRQHandler,Default_Handler

  .weak      CAN2_ORed_IRQHandler
  .thumb_set CAN2_ORed_IRQHandler,Default_Handler

  .weak      CAN2_Error_IRQHandler
  .thumb_set CAN2_Error_IRQHandler,Default_Handler

  .weak      Reserved110_IRQHandler
  .thumb_set Reserved110_IRQHandler,Default_Handler

  .weak      CAN2_ORed_0_15_MB_IRQHandler
  .thumb_set CAN2_ORed_0_15_MB_IRQHandler,Default_Handler

  .weak      Reserved112_IRQHandler
  .thumb_set Reserved112_IRQHandler,Default_Handler

  .weak      Reserved113_IRQHandler
  .thumb_set Reserved113_IRQHandler,Default_Handler

  .weak      Reserved114_IRQHandler
  .thumb_set Reserved114_IRQHandler,Default_Handler

  .weak      FTM0_Ch0_Ch1_IRQHandler
  .thumb_set FTM0_Ch0_Ch1_IRQHandler,Default_Handler

  .weak      FTM0_Ch2_Ch3_IRQHandler
  .thumb_set FTM0_Ch2_Ch3_IRQHandler,Default_Handler

  .weak      FTM0_Ch4_Ch5_IRQHandler
  .thumb_set FTM0_Ch4_Ch5_IRQHandler,Default_Handler

  .weak      FTM0_Ch6_Ch7_IRQHandler
  .thumb_set FTM0_Ch6_Ch7_IRQHandler,Default_Handler

  .weak      FTM0_Fault_IRQHandler
  .thumb_set FTM0_Fault_IRQHandler,Default_Handler

  .weak      FTM0_Ovf_Reload_IRQHandler
  .thumb_set FTM0_Ovf_Reload_IRQHandler,Default_Handler

  .weak      FTM1_Ch0_Ch1_IRQHandler
  .thumb_set FTM1_Ch0_Ch1_IRQHandler,Default_Handler

  .weak      FTM1_Ch2_Ch3_IRQHandler
  .thumb_set FTM1_Ch2_Ch3_IRQHandler,Default_Handler

  .weak      FTM1_Ch4_Ch5_IRQHandler
  .thumb_set FTM1_Ch4_Ch5_IRQHandler,Default_Handler

  .weak      FTM1_Ch6_Ch7_IRQHandler
  .thumb_set FTM1_Ch6_Ch7_IRQHandler,Default_Handler

  .weak      FTM1_Fault_IRQHandler
  .thumb_set FTM1_Fault_IRQHandler,Default_Handler

  .weak      FTM1_Ovf_Reload_IRQHandler
  .thumb_set FTM1_Ovf_Reload_IRQHandler,Default_Handler

  .weak      FTM2_Ch0_Ch1_IRQHandler
  .thumb_set FTM2_Ch0_Ch1_IRQHandler,Default_Handler

  .weak      FTM2_Ch2_Ch3_IRQHandler
  .thumb_set FTM2_Ch2_Ch3_IRQHandler,Default_Handler

  .weak      FTM2_Ch4_Ch5_IRQHandler
  .thumb_set FTM2_Ch4_Ch5_IRQHandler,Default_Handler

  .weak      FTM2_Ch6_Ch7_IRQHandler
  .thumb_set FTM2_Ch6_Ch7_IRQHandler,Default_Handler

  .weak      FTM2_Fault_IRQHandler
  .thumb_set FTM2_Fault_IRQHandler,Default_Handler

  .weak      FTM2_Ovf_Reload_IRQHandler
  .thumb_set FTM2_Ovf_Reload_IRQHandler,Default_Handler

  .weak      FTM3_Ch0_Ch1_IRQHandler
  .thumb_set FTM3_Ch0_Ch1_IRQHandler,Default_Handler

  .weak      FTM3_Ch2_Ch3_IRQHandler
  .thumb_set FTM3_Ch2_Ch3_IRQHandler,Default_Handler

  .weak      FTM3_Ch4_Ch5_IRQHandler
  .thumb_set FTM3_Ch4_Ch5_IRQHandler,Default_Handler

  .weak      FTM3_Ch6_Ch7_IRQHandler
  .thumb_set FTM3_Ch6_Ch7_IRQHandler,Default_Handler

  .weak      FTM3_Fault_IRQHandler
  .thumb_set FTM3_Fault_IRQHandler,Default_Handler

  .weak      FTM3_Ovf_Reload_IRQHandler
  .thumb_set FTM3_Ovf_Reload_IRQHandler,Default_Handler


/* Flash (Security Byte) Configuration */
  .section .flashconfig, "a"
  .word 0xFFFFFFFF     /* 8 bytes backdoor comparison key           */
  .word 0xFFFFFFFF     /*                                           */
  .word 0xFFFFFFFF     /* 4 bytes program flash protection bytes    */
  .word 0xFFFF7FFE     /* FDPROT:FEPROT:FOPT:FSEC(0xFE = unsecured) */
