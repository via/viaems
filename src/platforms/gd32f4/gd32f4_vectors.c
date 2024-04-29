#include "gd32f4xx.h"
#include <stdint.h>

/* Must be provided by linker script */
extern uint32_t _stack;

struct vector_table {
  uint32_t *stack_pointer;
  void (*reset)(void);
  void (*nmi)(void);
  void (*hard_fault)(void);
  void (*memory_manage_fault)(void);
  void (*bus_fault)(void);
  void (*usage_fault)(void);
  void (*reserved_x001c[4])(void);
  void (*sv_call)(void);
  void (*debug_monitor)(void);
  void (*reserved_x0034)(void);
  void (*pend_sv)(void);
  void (*systick)(void);
  void (*irqs[91])(void);
};

void blocking_handler(void) {
  while (1)
    ;
}

void null_handler(void) {}

void reset_handler(void) __attribute__((weak, alias("blocking_handler")));
void nmi_handler(void) __attribute__((weak, alias("null_handler")));
void hard_fault_handler(void) __attribute__((weak, alias("null_handler")));
void memory_manage_fault_handler(void)
  __attribute__((weak, alias("null_handler")));
void bus_fault_handler(void) __attribute__((weak, alias("null_handler")));
void usage_fault_handler(void) __attribute__((weak, alias("null_handler")));
void debug_monitor_handler(void) __attribute__((weak, alias("null_handler")));
void sv_call_handler(void) __attribute__((weak, alias("null_handler")));
void pend_sv_handler(void) __attribute__((weak, alias("null_handler")));
void systick_handler(void) __attribute__((weak, alias("null_handler")));

void SVC_Handler(void);
void PendSV_Handler(void);

void WWDGT_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void LVD_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void TAMPER_STAMP_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void RTC_WKUP_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void FMC_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void RCU_CTC_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void EXTI0_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void EXTI1_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void EXTI2_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void EXTI3_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void EXTI4_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void DMA0_Channel0_IRQHandler(void)
  __attribute__((weak, alias("null_handler")));
void DMA0_Channel1_IRQHandler(void)
  __attribute__((weak, alias("null_handler")));
void DMA0_Channel2_IRQHandler(void)
  __attribute__((weak, alias("null_handler")));
void DMA0_Channel3_IRQHandler(void)
  __attribute__((weak, alias("null_handler")));
void DMA0_Channel4_IRQHandler(void)
  __attribute__((weak, alias("null_handler")));
void DMA0_Channel5_IRQHandler(void)
  __attribute__((weak, alias("null_handler")));
void DMA0_Channel6_IRQHandler(void)
  __attribute__((weak, alias("null_handler")));
void ADC_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void CAN0_TX_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void CAN0_RX0_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void CAN0_RX1_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void CAN0_EWMC_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void EXTI5_9_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void TIMER0_BRK_TIMER8_IRQHandler(void)
  __attribute__((weak, alias("null_handler")));
void TIMER0_UP_TIMER9_IRQHandler(void)
  __attribute__((weak, alias("null_handler")));
void TIMER0_TRG_CMT_TIMER10_IRQHandler(void)
  __attribute__((weak, alias("null_handler")));
void TIMER0_Channel_IRQHandler(void)
  __attribute__((weak, alias("null_handler")));
void TIMER1_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void TIMER2_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void TIMER3_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void I2C0_EV_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void I2C0_ER_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void I2C1_EV_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void I2C1_ER_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void SPI0_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void SPI1_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void USART0_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void USART1_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void USART2_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void EXTI10_15_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void RTC_Alarm_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void USBFS_WKUP_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void TIMER7_BRK_TIMER11_IRQHandler(void)
  __attribute__((weak, alias("null_handler")));
void TIMER7_UP_TIMER12_IRQHandler(void)
  __attribute__((weak, alias("null_handler")));
void TIMER7_TRG_CMT_TIMER13_IRQHandler(void)
  __attribute__((weak, alias("null_handler")));
void TIMER7_Channel_IRQHandler(void)
  __attribute__((weak, alias("null_handler")));
void DMA0_Channel7_IRQHandler(void)
  __attribute__((weak, alias("null_handler")));
void EXMC_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void SDIO_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void TIMER4_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void SPI2_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void UART3_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void UART4_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void TIMER5_DAC_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void TIMER6_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void DMA1_Channel0_IRQHandler(void)
  __attribute__((weak, alias("null_handler")));
void DMA1_Channel1_IRQHandler(void)
  __attribute__((weak, alias("null_handler")));
void DMA1_Channel2_IRQHandler(void)
  __attribute__((weak, alias("null_handler")));
void DMA1_Channel3_IRQHandler(void)
  __attribute__((weak, alias("null_handler")));
void DMA1_Channel4_IRQHandler(void)
  __attribute__((weak, alias("null_handler")));
void ENET_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void ENET_WKUP_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void CAN1_TX_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void CAN1_RX0_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void CAN1_RX1_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void CAN1_EWMC_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void USBFS_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void DMA1_Channel5_IRQHandler(void)
  __attribute__((weak, alias("null_handler")));
void DMA1_Channel6_IRQHandler(void)
  __attribute__((weak, alias("null_handler")));
void DMA1_Channel7_IRQHandler(void)
  __attribute__((weak, alias("null_handler")));
void USART5_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void I2C2_EV_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void I2C2_ER_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void USBHS_EP1_Out_IRQHandler(void)
  __attribute__((weak, alias("null_handler")));
void USBHS_EP1_In_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void USBHS_WKUP_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void USBHS_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void DCI_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void TRNG_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void FPU_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void UART6_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void UART7_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void SPI3_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void SPI4_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void SPI5_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void TLI_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void TLI_ER_IRQHandler(void) __attribute__((weak, alias("null_handler")));
void IPA_IRQHandler(void) __attribute__((weak, alias("null_handler")));

__attribute__ ((section(".vectors")))
struct vector_table vector_table = {
	.stack_pointer = &_stack,
	.reset = reset_handler,
	.nmi = nmi_handler,
	.hard_fault = hard_fault_handler,
	.memory_manage_fault = memory_manage_fault_handler,
	.bus_fault = bus_fault_handler,
	.usage_fault = usage_fault_handler,
	.debug_monitor = debug_monitor_handler,
	.sv_call = SVC_Handler,
	.pend_sv = PendSV_Handler,
	.systick = systick_handler,
  .irqs = {
    [WWDGT_IRQn] = WWDGT_IRQHandler,
    [LVD_IRQn] = LVD_IRQHandler,
    [TAMPER_STAMP_IRQn] = TAMPER_STAMP_IRQHandler,
    [RTC_WKUP_IRQn] = RTC_WKUP_IRQHandler,
    [FMC_IRQn] = FMC_IRQHandler,
    [RCU_CTC_IRQn] = RCU_CTC_IRQHandler,
    [EXTI0_IRQn] = EXTI0_IRQHandler,
    [EXTI1_IRQn] = EXTI1_IRQHandler,
    [EXTI2_IRQn] = EXTI2_IRQHandler,
    [EXTI3_IRQn] = EXTI3_IRQHandler,
    [EXTI4_IRQn] = EXTI4_IRQHandler,
    [DMA0_Channel0_IRQn] = DMA0_Channel0_IRQHandler,
    [DMA0_Channel1_IRQn] = DMA0_Channel1_IRQHandler,
    [DMA0_Channel2_IRQn] = DMA0_Channel2_IRQHandler,
    [DMA0_Channel3_IRQn] = DMA0_Channel3_IRQHandler,
    [DMA0_Channel4_IRQn] = DMA0_Channel4_IRQHandler,
    [DMA0_Channel5_IRQn] = DMA0_Channel5_IRQHandler,
    [DMA0_Channel6_IRQn] = DMA0_Channel6_IRQHandler,
    [ADC_IRQn] = ADC_IRQHandler,
    [CAN0_TX_IRQn] = CAN0_TX_IRQHandler,
    [CAN0_RX0_IRQn] = CAN0_RX0_IRQHandler,
    [CAN0_RX1_IRQn] = CAN0_RX1_IRQHandler,
    [CAN0_EWMC_IRQn] = CAN0_EWMC_IRQHandler,
    [EXTI5_9_IRQn] = EXTI5_9_IRQHandler,
    [TIMER0_BRK_TIMER8_IRQn] = TIMER0_BRK_TIMER8_IRQHandler,
    [TIMER0_UP_TIMER9_IRQn] = TIMER0_UP_TIMER9_IRQHandler,
    [TIMER0_TRG_CMT_TIMER10_IRQn] = TIMER0_TRG_CMT_TIMER10_IRQHandler,
    [TIMER0_Channel_IRQn] = TIMER0_Channel_IRQHandler,
    [TIMER1_IRQn] = TIMER1_IRQHandler,
    [TIMER2_IRQn] = TIMER2_IRQHandler,
    [TIMER3_IRQn] = TIMER3_IRQHandler,
    [I2C0_EV_IRQn] = I2C0_EV_IRQHandler,
    [I2C0_ER_IRQn] = I2C0_ER_IRQHandler,
    [I2C1_EV_IRQn] = I2C1_EV_IRQHandler,
    [I2C1_ER_IRQn] = I2C1_ER_IRQHandler,
    [SPI0_IRQn] = SPI0_IRQHandler,
    [SPI1_IRQn] = SPI1_IRQHandler,
    [USART0_IRQn] = USART0_IRQHandler,
    [USART1_IRQn] = USART1_IRQHandler,
    [USART2_IRQn] = USART2_IRQHandler,
    [EXTI10_15_IRQn] = EXTI10_15_IRQHandler,
    [RTC_Alarm_IRQn] = RTC_Alarm_IRQHandler,
    [USBFS_WKUP_IRQn] = USBFS_WKUP_IRQHandler,
    [TIMER7_BRK_TIMER11_IRQn] = TIMER7_BRK_TIMER11_IRQHandler,
    [TIMER7_UP_TIMER12_IRQn] = TIMER7_UP_TIMER12_IRQHandler,
    [TIMER7_TRG_CMT_TIMER13_IRQn] = TIMER7_TRG_CMT_TIMER13_IRQHandler,
    [TIMER7_Channel_IRQn] = TIMER7_Channel_IRQHandler,
    [DMA0_Channel7_IRQn] = DMA0_Channel7_IRQHandler,
    [EXMC_IRQn] = EXMC_IRQHandler,
    [SDIO_IRQn] = SDIO_IRQHandler,
    [TIMER4_IRQn] = TIMER4_IRQHandler,
    [SPI2_IRQn] = SPI2_IRQHandler,
    [UART3_IRQn] = UART3_IRQHandler,
    [UART4_IRQn] = UART4_IRQHandler,
    [TIMER5_DAC_IRQn] = TIMER5_DAC_IRQHandler,
    [TIMER6_IRQn] = TIMER6_IRQHandler,
    [DMA1_Channel0_IRQn] = DMA1_Channel0_IRQHandler,
    [DMA1_Channel1_IRQn] = DMA1_Channel1_IRQHandler,
    [DMA1_Channel2_IRQn] = DMA1_Channel2_IRQHandler,
    [DMA1_Channel3_IRQn] = DMA1_Channel3_IRQHandler,
    [DMA1_Channel4_IRQn] = DMA1_Channel4_IRQHandler,
    [ENET_IRQn] = ENET_IRQHandler,
    [ENET_WKUP_IRQn] = ENET_WKUP_IRQHandler,
    [CAN1_TX_IRQn] = CAN1_TX_IRQHandler,
    [CAN1_RX0_IRQn] = CAN1_RX0_IRQHandler,
    [CAN1_RX1_IRQn] = CAN1_RX1_IRQHandler,
    [CAN1_EWMC_IRQn] = CAN1_EWMC_IRQHandler,
    [USBFS_IRQn] = USBFS_IRQHandler,
    [DMA1_Channel5_IRQn] = DMA1_Channel5_IRQHandler,
    [DMA1_Channel6_IRQn] = DMA1_Channel6_IRQHandler,
    [DMA1_Channel7_IRQn] = DMA1_Channel7_IRQHandler,
    [USART5_IRQn] = USART5_IRQHandler,
    [I2C2_EV_IRQn] = I2C2_EV_IRQHandler,
    [I2C2_ER_IRQn] = I2C2_ER_IRQHandler,
    [USBHS_EP1_Out_IRQn] = USBHS_EP1_Out_IRQHandler,
    [USBHS_EP1_In_IRQn] = USBHS_EP1_In_IRQHandler,
    [USBHS_WKUP_IRQn] = USBHS_WKUP_IRQHandler,
    [USBHS_IRQn] = USBHS_IRQHandler,
    [DCI_IRQn] = DCI_IRQHandler,
    [TRNG_IRQn] = TRNG_IRQHandler,
    [FPU_IRQn] = FPU_IRQHandler,
    [UART6_IRQn] = UART6_IRQHandler,
    [UART7_IRQn] = UART7_IRQHandler,
    [SPI3_IRQn] = SPI3_IRQHandler,
    [SPI4_IRQn] = SPI4_IRQHandler,
    [SPI5_IRQn] = SPI5_IRQHandler,
    [TLI_IRQn] = TLI_IRQHandler,
    [TLI_ER_IRQn] = TLI_ER_IRQHandler,
    [IPA_IRQn] = IPA_IRQHandler,
  },
};
