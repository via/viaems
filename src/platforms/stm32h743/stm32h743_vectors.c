#include <stdint.h>
#include "device.h"


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
	void (*irqs[IRQ_NUMBER_MAX + 1])(void);
};

void __attribute__((weak)) 
reset_handler(void) {
	while (1);
}

void 
blocking_handler(void) {
	while (1);
}

void null_handler(void) {
}
void wwdg1_isr(void) __attribute__((weak, alias("null_handler")));
void pvd_pvm_isr(void) __attribute__((weak, alias("null_handler")));
void rtc_tamp_stamp_css_lse_isr(void) __attribute__((weak, alias("null_handler")));
void rtc_wkup_isr(void) __attribute__((weak, alias("null_handler")));
void flash_isr(void) __attribute__((weak, alias("null_handler")));
void rcc_isr(void) __attribute__((weak, alias("null_handler")));
void exti0_isr(void) __attribute__((weak, alias("null_handler")));
void exti1_isr(void) __attribute__((weak, alias("null_handler")));
void exti2_isr(void) __attribute__((weak, alias("null_handler")));
void exti3_isr(void) __attribute__((weak, alias("null_handler")));
void exti4_isr(void) __attribute__((weak, alias("null_handler")));
void dma_str0_isr(void) __attribute__((weak, alias("null_handler")));
void dma_str1_isr(void) __attribute__((weak, alias("null_handler")));
void dma_str2_isr(void) __attribute__((weak, alias("null_handler")));
void dma_str3_isr(void) __attribute__((weak, alias("null_handler")));
void dma_str4_isr(void) __attribute__((weak, alias("null_handler")));
void dma_str5_isr(void) __attribute__((weak, alias("null_handler")));
void dma_str6_isr(void) __attribute__((weak, alias("null_handler")));
void adc1_2_isr(void) __attribute__((weak, alias("null_handler")));
void fdcan1_it0_isr(void) __attribute__((weak, alias("null_handler")));
void fdcan2_it0_isr(void) __attribute__((weak, alias("null_handler")));
void fdcan1_it1_isr(void) __attribute__((weak, alias("null_handler")));
void fdcan2_it1_isr(void) __attribute__((weak, alias("null_handler")));
void exti9_5_isr(void) __attribute__((weak, alias("null_handler")));
void tim1_brk_isr(void) __attribute__((weak, alias("null_handler")));
void tim1_up_isr(void) __attribute__((weak, alias("null_handler")));
void tim1_trg_com_isr(void) __attribute__((weak, alias("null_handler")));
void tim_cc_isr(void) __attribute__((weak, alias("null_handler")));
void tim2_isr(void) __attribute__((weak, alias("null_handler")));
void tim3_isr(void) __attribute__((weak, alias("null_handler")));
void tim4_isr(void) __attribute__((weak, alias("null_handler")));
void i2c1_ev_isr(void) __attribute__((weak, alias("null_handler")));
void i2c1_er_isr(void) __attribute__((weak, alias("null_handler")));
void i2c2_ev_isr(void) __attribute__((weak, alias("null_handler")));
void i2c2_er_isr(void) __attribute__((weak, alias("null_handler")));
void spi1_isr(void) __attribute__((weak, alias("null_handler")));
void spi2_isr(void) __attribute__((weak, alias("null_handler")));
void usart1_isr(void) __attribute__((weak, alias("null_handler")));
void usart2_isr(void) __attribute__((weak, alias("null_handler")));
void usart3_isr(void) __attribute__((weak, alias("null_handler")));
void exti15_10_isr(void) __attribute__((weak, alias("null_handler")));
void rtc_alarm_isr(void) __attribute__((weak, alias("null_handler")));
void tim8_brk_tim12_isr(void) __attribute__((weak, alias("null_handler")));
void tim8_up_tim13_isr(void) __attribute__((weak, alias("null_handler")));
void tim8_trg_com_tim14_isr(void) __attribute__((weak, alias("null_handler")));
void tim8_cc_isr(void) __attribute__((weak, alias("null_handler")));
void dma1_str7_isr(void) __attribute__((weak, alias("null_handler")));
void fmc_isr(void) __attribute__((weak, alias("null_handler")));
void sdmmc1_isr(void) __attribute__((weak, alias("null_handler")));
void tim5_isr(void) __attribute__((weak, alias("null_handler")));
void spi3_isr(void) __attribute__((weak, alias("null_handler")));
void uart4_isr(void) __attribute__((weak, alias("null_handler")));
void uart5_isr(void) __attribute__((weak, alias("null_handler")));
void tim6_dac_isr(void) __attribute__((weak, alias("null_handler")));
void tim7_isr(void) __attribute__((weak, alias("null_handler")));
void dma2_str0_isr(void) __attribute__((weak, alias("null_handler")));
void dma2_str1_isr(void) __attribute__((weak, alias("null_handler")));
void dma2_str2_isr(void) __attribute__((weak, alias("null_handler")));
void dma2_str3_isr(void) __attribute__((weak, alias("null_handler")));
void dma2_str4_isr(void) __attribute__((weak, alias("null_handler")));
void eth_isr(void) __attribute__((weak, alias("null_handler")));
void eth_wkup_isr(void) __attribute__((weak, alias("null_handler")));
void fdcan_cal_isr(void) __attribute__((weak, alias("null_handler")));
void dma2_str5_isr(void) __attribute__((weak, alias("null_handler")));
void dma2_str6_isr(void) __attribute__((weak, alias("null_handler")));
void dma2_str7_isr(void) __attribute__((weak, alias("null_handler")));
void usart6_isr(void) __attribute__((weak, alias("null_handler")));
void i2c3_ev_isr(void) __attribute__((weak, alias("null_handler")));
void i2c3_er_isr(void) __attribute__((weak, alias("null_handler")));
void otg_hs_ep1_out_isr(void) __attribute__((weak, alias("null_handler")));
void otg_hs_ep1_in_isr(void) __attribute__((weak, alias("null_handler")));
void otg_hs_wkup_isr(void) __attribute__((weak, alias("null_handler")));
void otg_hs_isr(void) __attribute__((weak, alias("null_handler")));
void dcmi_isr(void) __attribute__((weak, alias("null_handler")));
void fpu_isr(void) __attribute__((weak, alias("null_handler")));
void uart7_isr(void) __attribute__((weak, alias("null_handler")));
void uart8_isr(void) __attribute__((weak, alias("null_handler")));
void spi4_isr(void) __attribute__((weak, alias("null_handler")));
void spi5_isr(void) __attribute__((weak, alias("null_handler")));
void spi6_isr(void) __attribute__((weak, alias("null_handler")));
void sai1_isr(void) __attribute__((weak, alias("null_handler")));
void ltdc_isr(void) __attribute__((weak, alias("null_handler")));
void ltdc_er_isr(void) __attribute__((weak, alias("null_handler")));
void dma2d_isr(void) __attribute__((weak, alias("null_handler")));
void sai2_isr(void) __attribute__((weak, alias("null_handler")));
void quadspi_isr(void) __attribute__((weak, alias("null_handler")));
void lptim1_isr(void) __attribute__((weak, alias("null_handler")));
void cec_isr(void) __attribute__((weak, alias("null_handler")));
void i2c4_ev_isr(void) __attribute__((weak, alias("null_handler")));
void i2c4_er_isr(void) __attribute__((weak, alias("null_handler")));
void spdif_isr(void) __attribute__((weak, alias("null_handler")));
void otg_fs_ep1_out_isr(void) __attribute__((weak, alias("null_handler")));
void otg_fs_ep1_in_isr(void) __attribute__((weak, alias("null_handler")));
void otg_fs_wkup_isr(void) __attribute__((weak, alias("null_handler")));
void otg_fs_isr(void) __attribute__((weak, alias("null_handler")));
void dmamux1_ov_isr(void) __attribute__((weak, alias("null_handler")));
void hrtim1_mst_isr(void) __attribute__((weak, alias("null_handler")));
void hrtim1_tima_isr(void) __attribute__((weak, alias("null_handler")));
void hrtim_timb_isr(void) __attribute__((weak, alias("null_handler")));
void hrtim1_timc_isr(void) __attribute__((weak, alias("null_handler")));
void hrtim1_timd_isr(void) __attribute__((weak, alias("null_handler")));
void hrtim_time_isr(void) __attribute__((weak, alias("null_handler")));
void hrtim1_flt_isr(void) __attribute__((weak, alias("null_handler")));
void dfsdm1_flt0_isr(void) __attribute__((weak, alias("null_handler")));
void dfsdm1_flt1_isr(void) __attribute__((weak, alias("null_handler")));
void dfsdm1_flt2_isr(void) __attribute__((weak, alias("null_handler")));
void dfsdm1_flt3_isr(void) __attribute__((weak, alias("null_handler")));
void sai3_isr(void) __attribute__((weak, alias("null_handler")));
void swpmi1_isr(void) __attribute__((weak, alias("null_handler")));
void tim15_isr(void) __attribute__((weak, alias("null_handler")));
void tim16_isr(void) __attribute__((weak, alias("null_handler")));
void tim17_isr(void) __attribute__((weak, alias("null_handler")));
void mdios_wkup_isr(void) __attribute__((weak, alias("null_handler")));
void mdios_isr(void) __attribute__((weak, alias("null_handler")));
void jpeg_isr(void) __attribute__((weak, alias("null_handler")));
void mdma_isr(void) __attribute__((weak, alias("null_handler")));
void sdmmc_isr(void) __attribute__((weak, alias("null_handler")));
void hsem0_isr(void) __attribute__((weak, alias("null_handler")));
void adc3_isr(void) __attribute__((weak, alias("null_handler")));
void dmamux2_ovr_isr(void) __attribute__((weak, alias("null_handler")));
void bdma_ch1_isr(void) __attribute__((weak, alias("null_handler")));
void bdma_ch2_isr(void) __attribute__((weak, alias("null_handler")));
void bdma_ch3_isr(void) __attribute__((weak, alias("null_handler")));
void bdma_ch4_isr(void) __attribute__((weak, alias("null_handler")));
void bdma_ch5_isr(void) __attribute__((weak, alias("null_handler")));
void bdma_ch6_isr(void) __attribute__((weak, alias("null_handler")));
void bdma_ch7_isr(void) __attribute__((weak, alias("null_handler")));
void bdma_ch8_isr(void) __attribute__((weak, alias("null_handler")));
void comp_isr(void) __attribute__((weak, alias("null_handler")));
void lptim2_isr(void) __attribute__((weak, alias("null_handler")));
void lptim3_isr(void) __attribute__((weak, alias("null_handler")));
void lptim4_isr(void) __attribute__((weak, alias("null_handler")));
void lptim5_isr(void) __attribute__((weak, alias("null_handler")));
void lpuart_isr(void) __attribute__((weak, alias("null_handler")));
void wwdg1_rst_isr(void) __attribute__((weak, alias("null_handler")));
void crs_isr(void) __attribute__((weak, alias("null_handler")));
void sai4_isr(void) __attribute__((weak, alias("null_handler")));
void wkup_isr(void) __attribute__((weak, alias("null_handler")));

__attribute__ ((section(".vectors")))
struct vector_table vector_table = {
	.stack_pointer = &_stack,
	.reset = reset_handler,
	.nmi = blocking_handler,
	.hard_fault = blocking_handler,
	.memory_manage_fault = blocking_handler,
	.bus_fault = blocking_handler,
	.usage_fault = blocking_handler,
	.debug_monitor = blocking_handler,
	.sv_call = blocking_handler,
	.pend_sv = blocking_handler,
	.systick = blocking_handler,
  .irqs = {
    [WWDG1_IRQ] = wwdg1_isr,
    [PVD_PVM_IRQ] = pvd_pvm_isr,
    [RTC_TAMP_STAMP_CSS_LSE_IRQ] = rtc_tamp_stamp_css_lse_isr,
    [RTC_WKUP_IRQ] = rtc_wkup_isr,
    [FLASH_IRQ] = flash_isr,
    [RCC_IRQ] = rcc_isr,
    [EXTI0_IRQ] = exti0_isr,
    [EXTI1_IRQ] = exti1_isr,
    [EXTI2_IRQ] = exti2_isr,
    [EXTI3_IRQ] = exti3_isr,
    [EXTI4_IRQ] = exti4_isr,
    [DMA_STR0_IRQ] = dma_str0_isr,
    [DMA_STR1_IRQ] = dma_str1_isr,
    [DMA_STR2_IRQ] = dma_str2_isr,
    [DMA_STR3_IRQ] = dma_str3_isr,
    [DMA_STR4_IRQ] = dma_str4_isr,
    [DMA_STR5_IRQ] = dma_str5_isr,
    [DMA_STR6_IRQ] = dma_str6_isr,
    [ADC1_2_IRQ] = adc1_2_isr,
    [FDCAN1_IT0_IRQ] = fdcan1_it0_isr,
    [FDCAN2_IT0_IRQ] = fdcan2_it0_isr,
    [FDCAN1_IT1_IRQ] = fdcan1_it1_isr,
    [FDCAN2_IT1_IRQ] = fdcan2_it1_isr,
    [EXTI9_5_IRQ] = exti9_5_isr,
    [TIM1_BRK_IRQ] = tim1_brk_isr,
    [TIM1_UP_IRQ] = tim1_up_isr,
    [TIM1_TRG_COM_IRQ] = tim1_trg_com_isr,
    [TIM_CC_IRQ] = tim_cc_isr,
    [TIM2_IRQ] = tim2_isr,
    [TIM3_IRQ] = tim3_isr,
    [TIM4_IRQ] = tim4_isr,
    [I2C1_EV_IRQ] = i2c1_ev_isr,
    [I2C1_ER_IRQ] = i2c1_er_isr,
    [I2C2_EV_IRQ] = i2c2_ev_isr,
    [I2C2_ER_IRQ] = i2c2_er_isr,
    [SPI1_IRQ] = spi1_isr,
    [SPI2_IRQ] = spi2_isr,
    [USART1_IRQ] = usart1_isr,
    [USART2_IRQ] = usart2_isr,
    [USART3_IRQ] = usart3_isr,
    [EXTI15_10_IRQ] = exti15_10_isr,
    [RTC_ALARM_IRQ] = rtc_alarm_isr,
    [TIM8_BRK_TIM12_IRQ] = tim8_brk_tim12_isr,
    [TIM8_UP_TIM13_IRQ] = tim8_up_tim13_isr,
    [TIM8_TRG_COM_TIM14_IRQ] = tim8_trg_com_tim14_isr,
    [TIM8_CC_IRQ] = tim8_cc_isr,
    [DMA1_STR7_IRQ] = dma1_str7_isr,
    [FMC_IRQ] = fmc_isr,
    [SDMMC1_IRQ] = sdmmc1_isr,
    [TIM5_IRQ] = tim5_isr,
    [SPI3_IRQ] = spi3_isr,
    [UART4_IRQ] = uart4_isr,
    [UART5_IRQ] = uart5_isr,
    [TIM6_DAC_IRQ] = tim6_dac_isr,
    [TIM7_IRQ] = tim7_isr,
    [DMA2_STR0_IRQ] = dma2_str0_isr,
    [DMA2_STR1_IRQ] = dma2_str1_isr,
    [DMA2_STR2_IRQ] = dma2_str2_isr,
    [DMA2_STR3_IRQ] = dma2_str3_isr,
    [DMA2_STR4_IRQ] = dma2_str4_isr,
    [ETH_IRQ] = eth_isr,
    [ETH_WKUP_IRQ] = eth_wkup_isr,
    [FDCAN_CAL_IRQ] = fdcan_cal_isr,
    [DMA2_STR5_IRQ] = dma2_str5_isr,
    [DMA2_STR6_IRQ] = dma2_str6_isr,
    [DMA2_STR7_IRQ] = dma2_str7_isr,
    [USART6_IRQ] = usart6_isr,
    [I2C3_EV_IRQ] = i2c3_ev_isr,
    [I2C3_ER_IRQ] = i2c3_er_isr,
    [OTG_HS_EP1_OUT_IRQ] = otg_hs_ep1_out_isr,
    [OTG_HS_EP1_IN_IRQ] = otg_hs_ep1_in_isr,
    [OTG_HS_WKUP_IRQ] = otg_hs_wkup_isr,
    [OTG_HS_IRQ] = otg_hs_isr,
    [DCMI_IRQ] = dcmi_isr,
    [FPU_IRQ] = fpu_isr,
    [UART7_IRQ] = uart7_isr,
    [UART8_IRQ] = uart8_isr,
    [SPI4_IRQ] = spi4_isr,
    [SPI5_IRQ] = spi5_isr,
    [SPI6_IRQ] = spi6_isr,
    [SAI1_IRQ] = sai1_isr,
    [LTDC_IRQ] = ltdc_isr,
    [LTDC_ER_IRQ] = ltdc_er_isr,
    [DMA2D_IRQ] = dma2d_isr,
    [SAI2_IRQ] = sai2_isr,
    [QUADSPI_IRQ] = quadspi_isr,
    [LPTIM1_IRQ] = lptim1_isr,
    [CEC_IRQ] = cec_isr,
    [I2C4_EV_IRQ] = i2c4_ev_isr,
    [I2C4_ER_IRQ] = i2c4_er_isr,
    [SPDIF_IRQ] = spdif_isr,
    [OTG_FS_EP1_OUT_IRQ] = otg_fs_ep1_out_isr,
    [OTG_FS_EP1_IN_IRQ] = otg_fs_ep1_in_isr,
    [OTG_FS_WKUP_IRQ] = otg_fs_wkup_isr,
    [OTG_FS_IRQ] = otg_fs_isr,
    [DMAMUX1_OV_IRQ] = dmamux1_ov_isr,
    [HRTIM1_MST_IRQ] = hrtim1_mst_isr,
    [HRTIM1_TIMA_IRQ] = hrtim1_tima_isr,
    [HRTIM_TIMB_IRQ] = hrtim_timb_isr,
    [HRTIM1_TIMC_IRQ] = hrtim1_timc_isr,
    [HRTIM1_TIMD_IRQ] = hrtim1_timd_isr,
    [HRTIM_TIME_IRQ] = hrtim_time_isr,
    [HRTIM1_FLT_IRQ] = hrtim1_flt_isr,
    [DFSDM1_FLT0_IRQ] = dfsdm1_flt0_isr,
    [DFSDM1_FLT1_IRQ] = dfsdm1_flt1_isr,
    [DFSDM1_FLT2_IRQ] = dfsdm1_flt2_isr,
    [DFSDM1_FLT3_IRQ] = dfsdm1_flt3_isr,
    [SAI3_IRQ] = sai3_isr,
    [SWPMI1_IRQ] = swpmi1_isr,
    [TIM15_IRQ] = tim15_isr,
    [TIM16_IRQ] = tim16_isr,
    [TIM17_IRQ] = tim17_isr,
    [MDIOS_WKUP_IRQ] = mdios_wkup_isr,
    [MDIOS_IRQ] = mdios_isr,
    [JPEG_IRQ] = jpeg_isr,
    [MDMA_IRQ] = mdma_isr,
    [SDMMC_IRQ] = sdmmc_isr,
    [HSEM0_IRQ] = hsem0_isr,
    [ADC3_IRQ] = adc3_isr,
    [DMAMUX2_OVR_IRQ] = dmamux2_ovr_isr,
    [BDMA_CH1_IRQ] = bdma_ch1_isr,
    [BDMA_CH2_IRQ] = bdma_ch2_isr,
    [BDMA_CH3_IRQ] = bdma_ch3_isr,
    [BDMA_CH4_IRQ] = bdma_ch4_isr,
    [BDMA_CH5_IRQ] = bdma_ch5_isr,
    [BDMA_CH6_IRQ] = bdma_ch6_isr,
    [BDMA_CH7_IRQ] = bdma_ch7_isr,
    [BDMA_CH8_IRQ] = bdma_ch8_isr,
    [COMP_IRQ] = comp_isr,
    [LPTIM2_IRQ] = lptim2_isr,
    [LPTIM3_IRQ] = lptim3_isr,
    [LPTIM4_IRQ] = lptim4_isr,
    [LPTIM5_IRQ] = lptim5_isr,
    [LPUART_IRQ] = lpuart_isr,
    [WWDG1_RST_IRQ] = wwdg1_rst_isr,
    [CRS_IRQ] = crs_isr,
    [SAI4_IRQ] = sai4_isr,
    [WKUP_IRQ] = wkup_isr,
  },
};

