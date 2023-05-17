AS=arm-none-eabi-as
CC=arm-none-eabi-gcc
AR=arm-none-eabi-ar
LD=arm-none-eabi-ld
OBJCOPY=arm-none-eabi-objcopy
SPI_ADC?=TLV2553
CRYSTAL_FREQ?=8

CMSIS=contrib/CMSIS_5
GDLIB=contrib/GD32F4xx_Firmware_Library

OBJS+= gd32f4_init.o \
       gd32f4_vectors.o \
       gd32f4_pwm.o \
       gd32f4_usb.o \
       gd32f4_sched.o \
       gd32f4_sensors.o \
       gd32f4_spiflash.o \
       gd32f4_sdcard.o \
       stm32_sched_buffers.o

GD32PERIPH= \
      gd32f4xx_adc.o \
      gd32f4xx_can.o \
      gd32f4xx_crc.o \
      gd32f4xx_ctc.o \
      gd32f4xx_dac.o \
      gd32f4xx_dbg.o \
      gd32f4xx_dci.o \
      gd32f4xx_dma.o \
      gd32f4xx_enet.o \
      gd32f4xx_exmc.o \
      gd32f4xx_exti.o \
      gd32f4xx_fmc.o \
      gd32f4xx_fwdgt.o \
      gd32f4xx_gpio.o \
      gd32f4xx_i2c.o \
      gd32f4xx_ipa.o \
      gd32f4xx_iref.o \
      gd32f4xx_misc.o \
      gd32f4xx_pmu.o \
      gd32f4xx_rcu.o \
      gd32f4xx_rtc.o \
      gd32f4xx_sdio.o \
      gd32f4xx_spi.o \
      gd32f4xx_syscfg.o \
      gd32f4xx_timer.o \
      gd32f4xx_tli.o \
      gd32f4xx_trng.o \
      gd32f4xx_usart.o \
      gd32f4xx_wwdgt.o

GD32USB=\
      drv_usb_core.o \
      drv_usb_dev.o \
      drv_usbd_int.o \
      usbd_core.o \
      usbd_enum.o \
      usbd_transc.o

OBJS+= ${GD32USB} ${GD32PERIPH}

VPATH=src/platforms/${PLATFORM} \
      ${GDLIB}/GD32F4xx_usb_library/device/core/Source \
      ${GDLIB}/GD32F4xx_usb_library/driver/Source \
      ${GDLIB}/GD32F4xx_standard_peripheral/Source

OBJS+= libssp.a libssp_nonshared.a

CFLAGS= -DNDEBUG -ffunction-sections -fdata-sections -O3 -g3 -gdwarf-2 \
            -mfloat-abi=hard -mfpu=fpv4-sp-d16 -mthumb -mcpu=cortex-m4

CFLAGS+= -Isrc/platforms/gd32f4
CFLAGS+= -I${CMSIS}/CMSIS/Core/Include
CFLAGS+= -I${GDLIB}/GD32F4xx/Include
CFLAGS+= -I${GDLIB}/GD32F4xx_standard_peripheral/Include
CFLAGS+= -I${GDLIB}/GD32F4xx_usb_library/driver/Include
CFLAGS+= -I${GDLIB}/GD32F4xx_usb_library/device/core/Include
CFLAGS+= -I${GDLIB}/GD32F4xx_usb_library/device/class/cdc/Include
CFLAGS+= -I${GDLIB}/GD32F4xx_usb_library/ustd/common
CFLAGS+= -I${GDLIB}/GD32F4xx_usb_library/ustd/class/cdc
CFLAGS+= -DGD32F470 -D TICKRATE=4000000
CFLAGS+= -DSPI_${SPI_ADC}
CFLAGS+= -DCRYSTAL_FREQ=${CRYSTAL_FREQ}

LDFLAGS+= -lc -lnosys -nostartfiles -L ${OBJDIR} -Wl,--gc-sections
LDFLAGS+= -T src/platforms/gd32f4/gd32f4.ld

${OBJDIR}/libssp.a:
	${AR} rcs ${OBJDIR}/libssp.a

${OBJDIR}/libssp_nonshared.a:
	${AR} rcs ${OBJDIR}/libssp_nonshared.a

${OBJDIR}/viaems.dfu: ${OBJDIR}/viaems
	${OBJCOPY} -O ihex --remove-section=.configdata ${OBJDIR}/viaems ${OBJDIR}/viaems.hex
	${OBJCOPY} -O ihex --only-section=.configdata ${OBJDIR}/viaems ${OBJDIR}/viaems-config.hex
	scripts/dfuse-pack.py -i ${OBJDIR}/viaems.hex -i ${OBJDIR}/viaems-config.hex ${OBJDIR}/viaems.dfu

program: ${OBJDIR}/viaems.dfu
	dfu-util -D ${OBJDIR}/viaems.dfu -s :leave
