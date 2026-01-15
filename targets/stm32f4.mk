CC=arm-none-eabi-gcc
AS=arm-none-eabi-as
AR=arm-none-eabi-ar
LD=arm-none-eabi-ld
OBJCOPY=arm-none-eabi-objcopy
SPI_ADC?=TLV2553

CMSIS=contrib/CMSIS_5/
CMSISDEV=contrib/cmsis_device_f4/

VPATH=src/platforms/${PLATFORM} \
      contrib/tinyusb/src/class/cdc \
      contrib/tinyusb/src/common \
      contrib/tinyusb/src/device \
      contrib/tinyusb/src/portable/synopsys/dwc2 \
      contrib/tinyusb/src


TINYUSB_OBJS= cdc_device.o \
              tusb_fifo.o \
              usbd.o \
              usbd_control.o \
              dcd_dwc2.o \
              dwc2_common.o \
              tusb.o


OBJS+= stm32f4.o \
       stm32f4_sched.o \
       stm32f4_vectors.o \
       stm32f4_sensors.o \
       stm32f4_pwm.o \
       stm32f4_usb.o \
       stm32_sched_buffers.o \
       ${TINYUSB_OBJS}

OBJS+= libssp.a libssp_nonshared.a

CFLAGS= -DNDEBUG -ffunction-sections -fdata-sections -O2
CFLAGS+= -mfloat-abi=hard -mfpu=fpv4-sp-d16 -mthumb -mcpu=cortex-m4
CFLAGS+= -DSTM32F4 -DSTM32F4xx -DSTM32F427xx

CFLAGS+= -I${CMSIS}/CMSIS/Core/Include
CFLAGS+= -I${CMSISDEV}/Include
CFLAGS+= -Icontrib/tinyusb/src/
CFLAGS+= -Isrc/platforms/stm32f4
CFLAGS+= -DTICKRATE=4000000 -DSPI_${SPI_ADC}

LDFLAGS+= -lc -lnosys -L ${OBJDIR} -nostartfiles -Wl,--gc-sections
LDFLAGS+= -T src/platforms/stm32f4/stm32f4.ld

${OBJDIR}/libssp.a:
	${AR} rcs ${OBJDIR}/libssp.a

${OBJDIR}/libssp_nonshared.a:
	${AR} rcs ${OBJDIR}/libssp_nonshared.a

${OBJDIR}/viaems.dfu: ${OBJDIR}/viaems
	${OBJCOPY} -O ihex --remove-section=.configdata ${OBJDIR}/viaems ${OBJDIR}/viaems.hex
	${OBJCOPY} -O ihex --only-section=.configdata ${OBJDIR}/viaems ${OBJDIR}/viaems-config.hex
	python py/scripts/dfuse-pack.py -i ${OBJDIR}/viaems.hex -i ${OBJDIR}/viaems-config.hex ${OBJDIR}/viaems.dfu

program: ${OBJDIR}/viaems.dfu
	dfu-util -a 0 -s :mass-erase:force
	dfu-util -a 0 -D ${OBJDIR}/viaems.dfu -s :leave
